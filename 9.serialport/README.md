# Example 9 — Serial Port Driver (Interrupt-Driven)

This example adds an **interrupt-driven serial port device driver** to the OS.
A terminal process (`TermProc`) communicates with a remote terminal over a
serial (COM) port, using **semaphores** for flow control and **circular queues**
to buffer data between the process (upper half) and the hardware interrupt
handler (lower half).

---

## Learning Objectives

After studying this example you should be able to:

1. Explain the **upper-half / lower-half** split in an interrupt-driven device driver.
2. Describe how **semaphores** are used to synchronize a process with hardware events
   (blocking the writer when the TX buffer is full; blocking the reader until data arrives).
3. Trace a complete **TX path**: `PortWrite()` → `SemWait` → `PortWriteHandler` → UART data register.
4. Trace a complete **RX path**: UART IRQ → `PortReadOne` → `SemPost` → `PortRead()` unblocks.
5. Explain the **loopback queue** and why it takes TX priority over application data.
6. Program a UART (set baud rate, parity, enable RX/TX interrupts) using I/O port writes.
7. Explain how `Scheduler()` and `current_pid` cooperate to implement preemptive round-robin scheduling.

---

## File Structure

```
9.serialport/
├── src/
│   ├── main.c        Kernel entry: IDT setup, Scheduler(), Kernel() dispatcher
│   ├── events.S      Assembly: interrupt entry points, kernel stack switch, KernelExit
│   ├── handlers.c    All handlers: NewProc, Timer, Semaphores, Port upper/lower halves
│   ├── queue.c       Circular queue implementation (queue_in, queue_out, …)
│   ├── syscall.c     User-library wrappers: SemAlloc/Wait/Post, PortAlloc/Write/Read
│   └── proc.c        User processes: Init (keyboard) and TermProc (serial terminal)
├── include/
│   ├── types.h       TF_t, pcb_t, sem_t, port_t, state_t, constants
│   ├── queue.h       q_t struct and function prototypes
│   ├── events.h      IDT event numbers, entry-point prototypes
│   ├── handlers.h    Handler function prototypes
│   ├── syscall.h     Syscall wrapper prototypes
│   ├── data.h        extern declarations for kernel globals
│   └── proc.h        Process function prototypes
└── Makefile
```

---

## Architecture Overview

```
 ┌────────────────────────────────────────────────────────┐
 │              User Processes                            │
 │  TermProc: PortWrite() ─────┐   PortRead()             │
 │                             │   (blocks on read_sid)   │
 │  Init:  cons_kbhit loop     │     ▲                    │
 └─────────────────────────────┼─────┼────────────────────┘
                 system calls  │     │
 ┌─────────────────────────────┼─────┼──────────────────────┐              ┌────────────────┐            
 │              Kernel         │     │                      │              │                │
 │                             │     │                      │              │   spede-term   │
 │  Semaphores                 ▼     │                      │              │   (minicom)    │
 │   write_sid ────► port[].write_q  ──► PortWriteOne()  ───┼─── UART TX ──┼                │
 │   (starts=N)      (TX buffer)     │    UART TX IRQ       │              │                │
 │                                   │                      │              │                │
 │   read_sid  ◄────────  port[].read_q ◄─── PortReadOne()◄─┼─── UART RX ──┼                │
 │   (starts=0)     (RX buffer)           UART RX IRQ       │              │                │
 │                                                          │              │                │
 │  port[].loopback_q ──────────────► PortWriteOne()        │              │                │
 │  (echo received chars back to terminal)                  │              │                │
 └──────────────────────────────────────────────────────────┘              └────────────────┘
```


![alt text](https://github.com/CSUS-CS-CPE159/class_example/blob/main/9.serialport/SerialPort.jpg?raw=true)

---

## How It Works

### Kernel dispatch loop (`main.c`)

Every interrupt calls `Kernel(TF_t *TF_p)`:

```
Interrupt fires  →  events.S builds trapframe  →  Kernel() called
                                                        │
                    ┌───────────────────────────────────┤
                    │  switch(TF_p->event_num)          │
                    │    TIMER_EVENT  → TimerHandler    │
                    │    SEMALLOC    → SemAllocHandler  │
                    │    SEMWAIT     → SemWaitHandler   │
                    │    SEMPOST     → SemPostHandler   │
                    │    PORT_EVENT  → PortHandler      │
                    │    PORTALLOC   → PortAllocHandler │
                    │    PORTWRITE   → PortWriteHandler │
                    │    PORTREAD    → PortReadHandler  │
                    └───────────────────────────────────┘
                    Scheduler()  →  KernelExit()  →  IRET
```

### Scheduler and `current_pid`

`current_pid = 0` is a sentinel meaning "no process is selected."

- `TimerHandler()` sets `current_pid = 0` when a process exceeds `TIME_LIMIT` ticks.
- `SemWaitHandler()` sets `current_pid = 0` when a process blocks on a semaphore.
- After every handler, `Scheduler()` checks: if `current_pid == 0`, dequeue the next PID from `ready_q`.

### Upper half / Lower half split

The driver is split into two layers that communicate through queues and semaphores:

| Layer | When it runs | What it does |
|-------|-------------|--------------|
| **Upper half** | Inside a process (via system call) | Puts/gets data in queues; blocks on semaphores if full/empty |
| **Lower half** | Inside a hardware interrupt handler | Transfers one byte to/from the UART; posts semaphores to unblock the upper half |

This split means **a process is never blocked waiting for hardware** — it simply
waits on a semaphore, which keeps the CPU free for other processes.

### TX data flow (sending)

```
PortWrite("hello", port_num)        ← user process, one char at a time
  │
  ├─ SemWait(write_sid)             ← block if TX queue is full
  │
  └─ INT PORTWRITE_EVENT
       │
       └─ PortWriteHandler(ch, port_num)   ← upper half
            enqueue ch into port[].write_q
            if UART idle (write_ok): call PortWriteOne() immediately
            │
            └─ PortWriteOne(port_num)      ← lower half
                 dequeue ch from write_q (or loopback_q if non-empty)
                 outportb(IO + DATA, ch)   ← write to UART
                 SemPost(write_sid)        ← free one TX slot for the writer
```

### RX data flow (receiving)

```
User types a key on the terminal
  │
  └─ UART fires IRQ3/IRQ4  →  PortHandler()
       reads IIR register
       if IIR_RXRDY:
         PortReadOne(port_num)              ← lower half
           raw = inportb(IO + DATA)
           one = raw & 0x7F                ← strip parity bit
           queue_in(read_q, one)           ← for upper-half PortRead
           queue_in(loopback_q, one)       ← echo back to terminal
           SemPost(read_sid)               ← wake waiting PortRead()

... later, process calls PortRead() ...
  │
  ├─ SemWait(read_sid)                     ← block until data arrives
  │
  └─ INT PORTREAD_EVENT
       └─ PortReadHandler(*ch, port_num)   ← upper half
            queue_out(read_q, ch)          ← get one received byte
```

### Loopback (echo)

When a character is received, it is placed in `loopback_q` in addition to
`read_q`.  `PortWriteOne()` checks `loopback_q` first, so echo bytes are sent
back to the terminal before any application-data bytes.  This makes characters
appear on the screen immediately as the user types them.

---

## Semaphore Flow Control

Two semaphores manage each port:

| Semaphore | Initial passes | Blocks when | Unblocked by |
|-----------|:--------------:|-------------|--------------|
| `write_sid` | `QUEUE_SIZE` (e.g. 20) | TX queue is full | `PortWriteOne()` → `SemPost` after each byte sent |
| `read_sid` | 0 | No received data | `PortReadOne()` → `SemPost` after each byte received |

The initial pass count for `write_sid` equals the queue capacity so the writer
can buffer up to `QUEUE_SIZE` characters before blocking.  The 0 initial count
for `read_sid` forces the reader to always block until the hardware delivers data.

---

## Serial Port Hardware

### PIC mask

```c
outportb(0x21, ~0x19);
// ~0x19 = ~0b00011001 = 0b11100110
//  Bit 0 = IRQ0 (timer)    → enabled
//  Bit 3 = IRQ3 (COM2)     → enabled
//  Bit 4 = IRQ4 (COM1)     → enabled
//  All others               → masked
```

### COM port base I/O addresses

| Port | Base address | IRQ | IDT entry |
|------|:-----------:|:---:|:---------:|
| COM1 | 0x3F8 | 4 | 36 (0x24) |
| COM2 | 0x2F8 | 3 | 35 (0x23) |
| COM3 | 0x3E8 | 4 | 36 (0x24) |
| COM4 | 0x2E8 | 3 | 35 (0x23) |

This example uses COM2/COM3/COM4 (`port[0..2]`).

### UART register offsets from base I/O address

| Offset | Name | Purpose |
|:------:|------|---------|
| +0 | DATA | TX/RX data register (also BAUDLO when DLAB=1) |
| +1 | IER | Interrupt Enable Register (also BAUDHI when DLAB=1) |
| +2 | IIR | Interrupt Identification Register (read: which IRQ fired) |
| +3 | CFCR | Line Control Register (data bits, parity, stop bits; bit 7 = DLAB) |
| +4 | MCR | Modem Control Register (DTR, RTS, IRQ enable) |

### UART initialization (correct order)

```c
int baud    = 9600;
int divisor = 115200 / baud;   // = 12

// 1. Disable all UART interrupts while programming
outportb(IO + IER,  0x0);

// 2. Set DLAB=1 to access the baud-rate divisor registers
outportb(IO + CFCR, CFCR_DLAB);

// 3. Write 16-bit divisor (low byte first, then high byte)
outportb(IO + BAUDLO, (unsigned char)(divisor & 0xFF));
outportb(IO + BAUDHI, (unsigned char)((divisor >> 8) & 0xFF));

// 4. Clear DLAB; configure 7 data bits, even parity, 1 stop bit
outportb(IO + CFCR, CFCR_PEVEN | CFCR_PENAB | CFCR_7BITS);

// 5. Assert DTR/RTS; enable the UART's hardware IRQ output
outportb(IO + MCR, MCR_DTR | MCR_RTS | MCR_IENABLE);

// 6. Short I/O delay to let hardware settle (~60 µs)
for (int i = 0; i <= 0x63; i++) asm volatile("inb $0x80");

// 7. Enable RX-ready and TX-ready interrupt sources
outportb(IO + IER, IER_ERXRDY | IER_ETXRDY);
```

> **Note:** Steps must be in this order.  DLAB must be set (step 2) before the
> baud divisor is written (step 3), and DLAB must be cleared (step 4) before
> enabling interrupts (step 7), otherwise the IER write would hit the baud
> divisor register instead.

### Reading the IIR (Interrupt Identification Register)

```c
unsigned char iir = inportb(port[port_num].IO + IIR);
if (iir == IIR_RXRDY) PortReadOne(port_num);   // data arrived from terminal
if (iir == IIR_TXRDY) PortWriteOne(port_num);  // UART ready to send next byte
```

---

## How to Run

### 1. Install minicom

```bash
sudo apt install minicom
```

### 2. Open a terminal window connected to COM2 (before starting the emulator)

```bash
spede-term com2
```

### 3. Start the emulator

```bash
spede-run -d
```

The `spede-term` window becomes the serial terminal.  `TermProc` will prompt for
username/password and echo your input.  The SPEDE console shows `Init` responding
to key presses (`p` to print, `b` for GDB, `q` to quit).

---

## Debugging Tips

```bash
# Inspect the semaphore table
(gdb) p sem[0]
(gdb) p sem[1]

# Check a port descriptor (queues, semaphore IDs, write_ok flag)
(gdb) p port[0]

# See all PCB states
(gdb) p pcb[1].state
(gdb) p pcb[2].state

# Watch the ready queue
(gdb) p ready_q

# Break when PortReadOne is called (RX interrupt)
(gdb) break PortReadOne

# Confirm which process is currently scheduled
(gdb) p current_pid
```

---

## References

1. Cox, R., Kaashoek, F., & Morris, R. (2020). *xv6: a simple, Unix-like teaching operating system*. MIT PDOS.
2. [OSDev Wiki — Serial Ports](https://wiki.osdev.org/Serial_Ports)
3. [OSDev Wiki — PIC](https://wiki.osdev.org/8259_PIC)
