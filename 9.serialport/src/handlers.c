/**
 * handlers.c — All kernel event handlers for the Serial Port Driver example
 *
 * WHAT THIS FILE DEMONSTRATES:
 *   - Process creation: how a kernel allocates a PID, initializes a PCB and
 *     stack, and places the process on the ready queue.
 *   - Preemptive scheduling via the timer: the timer ISR increments a counter
 *     and forcibly moves a process off the CPU when it has run long enough.
 *   - Semaphores: a classic synchronization primitive.  SemWait blocks the
 *     caller if no resource is available; SemPost wakes a blocked caller.
 *   - Interrupt-driven (split) device driver:
 *       Upper half  = PortWriteHandler / PortReadHandler (called by user code
 *                     via system call; buffers data and returns quickly).
 *       Lower half  = PortWriteOne / PortReadOne (called from the hardware
 *                     interrupt handler; does the actual UART I/O one byte at
 *                     a time and signals semaphores to unblock upper-half callers).
 *
 * CONCEPTS TO LEARN:
 *   - Split driver design: the upper half never touches hardware directly.
 *     It puts/gets data in queues, then blocks on a semaphore.  The lower
 *     half runs asynchronously when the UART fires an interrupt, does the
 *     byte transfer, then posts the semaphore to wake the upper half.
 *   - Semaphore for flow control:
 *       write_sid starts with passes = QUEUE_SIZE (the TX queue can absorb
 *         QUEUE_SIZE characters before the writer must block).
 *       read_sid  starts with passes = 0 (the reader must block until the
 *         UART delivers at least one character).
 *   - Loopback queue: when a character is received from the serial port, it
 *     is also placed in a loopback queue so the UART echoes it back to the
 *     terminal (what the user typed appears on the screen).
 *   - IIR register (Interrupt Identification Register): after a serial-port
 *     IRQ fires, the driver reads IIR to discover whether the UART needs a
 *     byte written (TXRDY) or has a byte ready to be read (RXRDY).
 *
 * HOW IT FITS:
 *   main.c calls NewProcHandler() to create processes, and Kernel() calls all
 *   other handlers in this file depending on the event_num in the trapframe.
 */

#include "spede.h"
#include "types.h"
#include "handlers.h"
#include "data.h"
#include "proc.h"
#include "queue.h"

/*
 * NewProcHandler — create a new process and add it to the ready queue
 *
 * Parameter:
 *   p — function pointer to the process's entry point
 *
 * Steps:
 *   1. Dequeue a free PID from free_q.
 *   2. Zero the PCB and stack so there is no stale data.
 *   3. Place a trapframe at the TOP of the stack (TF_p points there).
 *      The trapframe is pre-filled so that KernelExit()'s IRET will jump
 *      to 'p' with interrupts enabled and current segment registers.
 *   4. Set the process state to READY and enqueue it in ready_q.
 *
 * The trapframe-at-top-of-stack trick is the same technique used in the
 * 8.UserProcess example: we fake a previous interrupt so the restore path
 * works for a brand-new process just like it does for a preempted one.
 */
void NewProcHandler(func_ptr_t p){
    int pid;

    if(queue_is_empty(&free_q)){
        cons_printf("Kernel Panic: no more PID left!\n");
        breakpoint();
        return;
    }
    queue_out(&free_q, &pid);
    printf("pid is %d\n", pid);

    memset((char *)&pcb[pid],        0, sizeof(pcb_t));
    memset((char *)&proc_stack[pid], 0, PROC_STACK_SIZE);

    /* Place the trapframe at the highest address in the stack.
     * proc_stack[pid][PROC_STACK_SIZE - 1] is the last byte; the TF
     * occupies the last sizeof(TF_t) bytes. */
    pcb[pid].TF_p = (TF_t *)&proc_stack[pid][PROC_STACK_SIZE - sizeof(TF_t)];

    /* Pre-fill the trapframe so IRET lands at the process entry point. */
    pcb[pid].TF_p->eip    = (unsigned int)p;              /* start executing here */
    pcb[pid].TF_p->eflags = EF_DEFAULT_VALUE | EF_INTR;  /* enable interrupts */
    pcb[pid].TF_p->cs     = get_cs();   /* inherit segment registers */
    pcb[pid].TF_p->ds     = get_ds();
    pcb[pid].TF_p->es     = get_es();
    pcb[pid].TF_p->fs     = get_fs();
    pcb[pid].TF_p->gs     = get_gs();

    pcb[pid].state = READY;
    queue_in(&ready_q, pid);
}

/*
 * TimerHandler — handle one tick of the periodic hardware timer
 *
 * Called every ~10 ms (IRQ 0).  Increments the running process's CPU time
 * budget and the global system clock.  If the process has used up its time
 * slice (TIME_LIMIT ticks), it is moved off the CPU:
 *   - state set to READY
 *   - PID re-enqueued in ready_q
 *   - current_pid set to 0 so Scheduler() will select a new process
 *
 * Side effect: sends EOI (End-Of-Interrupt, 0x20 to port 0x20) to the master
 * PIC so future timer interrupts will be delivered.
 */
void TimerHandler(void){
    pcb[current_pid].cpu_time++;
    current_time++;

    if(pcb[current_pid].cpu_time == TIME_LIMIT){
        pcb[current_pid].state = READY;
        queue_in(&ready_q, current_pid);
        current_pid = 0;    /* signal Scheduler() to pick a new process */
    }
    outportb(0x20, 0x20);   /* EOI to master PIC */
}

/*
 * SemAllocHandler — allocate a new semaphore
 *
 * Parameter:
 *   passes — initial pass count (number of times SemWait can succeed without
 *             blocking).  Use 0 for a "start blocked" semaphore (e.g. read_sid),
 *             or QUEUE_SIZE for a "N slots available" semaphore (e.g. write_sid).
 *
 * Scans sem[] for an unused slot (owner == 0), initializes it, and writes
 * the semaphore ID back to EAX in the caller's trapframe so the user process
 * receives the ID as the system-call return value.
 */
void SemAllocHandler(int passes){
    int sid;

    /* Find the first free semaphore slot. */
    for(sid = 0; sid < Q_SIZE; sid++){
        if(sem[sid].owner == 0) break;
    }
    if(sid == Q_SIZE){
        cons_printf("Kernel panic: no more semaphores left!\n");
        return;
    }

    memset((char *)&sem[sid], 0, sizeof(sem_t));
    sem[sid].passes = passes;
    sem[sid].owner  = current_pid;
    /* Return the semaphore ID to the caller via EAX. */
    pcb[current_pid].TF_p->eax = sid;
}

/*
 * SemWaitHandler — decrement (P operation) on a semaphore
 *
 * Parameter:
 *   sid — semaphore ID returned by SemAllocHandler
 *
 * If passes > 0, the process can proceed immediately (passes--).
 * If passes == 0, no resource is available: the process is blocked.
 *   - It is moved to sem[sid].wait_q (not ready_q).
 *   - Its state is set to WAIT.
 *   - current_pid = 0 forces Scheduler() to pick a different process.
 *
 * The blocked process is released by SemPostHandler().
 */
void SemWaitHandler(int sid){
    if(sem[sid].passes > 0){
        sem[sid].passes--;         /* resource available: consume one pass */
    } else {
        /* Block the caller until someone calls SemPost on this semaphore. */
        queue_in(&sem[sid].wait_q, current_pid);
        pcb[current_pid].state = WAIT;
        current_pid = 0;           /* force scheduler to find another process */
    }
}

/*
 * SemPostHandler — increment (V operation) on a semaphore
 *
 * Parameter:
 *   sid — semaphore ID
 *
 * If a process is waiting in sem[sid].wait_q, it is moved directly to
 * ready_q (no increment needed; the "pass" is transferred immediately).
 * If nobody is waiting, passes++ records the extra resource for a future caller.
 */
void SemPostHandler(int sid){
    int free_pid = 0;

    if(sem[sid].wait_q.size == 0){
        sem[sid].passes++;         /* no waiter; bank the pass for later */
    } else {
        /* Wake the first waiting process. */
        queue_out(&sem[sid].wait_q, &free_pid);
        queue_in(&ready_q, free_pid);
        pcb[free_pid].state = READY;
    }
}

/*
 * ---- Serial Port Lower Half ---------------------------------------------- *
 *                                                                              *
 * PortWriteOne and PortReadOne are the LOWER HALF of the serial driver.       *
 * They are called from PortHandler() when the UART fires a hardware interrupt. *
 * They do one byte of real I/O and signal a semaphore to wake the upper half. *
 * --------------------------------------------------------------------------- */

/*
 * PortWriteOne — transmit one byte to the UART (lower half TX)
 *
 * Parameter:
 *   port_num — index into port[] (0 = COM2, 1 = COM3, 2 = COM4)
 *
 * Priority: the loopback queue (echo of received characters) takes precedence
 * over the write queue (data the user process wants to send).  This ensures
 * that characters the user types appear on the terminal immediately.
 *
 * If both queues are empty, write_ok is set to 1 so that the next call to
 * PortWriteHandler() will trigger an immediate send without waiting for a
 * TXRDY interrupt.
 *
 * After writing a byte from write_q (not loopback_q), SemPost on write_sid
 * is called to free a slot and possibly unblock a waiting PortWrite() caller.
 */
void PortWriteOne(int port_num){
    int one;

    /* Nothing to send; record that the UART is idle (write_ok = 1). */
    if( queue_is_empty(&port[port_num].write_q) &&
        queue_is_empty(&port[port_num].loopback_q) ){
        port[port_num].write_ok = 1;
        return;
    }

    /* Loopback (echo) bytes take priority over application TX bytes. */
    if( !queue_is_empty(&port[port_num].loopback_q) ){
        queue_out(&port[port_num].loopback_q, &one);
    } else {
        queue_out(&port[port_num].write_q, &one);
        printf("[TX p=%d ch='%c']\n", port_num, (char)one);
        /* Freeing a slot in write_q: unblock any PortWrite() caller that
         * was waiting for space. */
        SemPostHandler(port[port_num].write_sid);
    }
    /* Write the byte to the UART Data Register. */
    outportb(port[port_num].IO + DATA, (unsigned char)one);
}

/*
 * PortReadOne — receive one byte from the UART (lower half RX)
 *
 * Parameter:
 *   port_num — index into port[]
 *
 * Reads a raw byte from the UART Data Register, strips the parity bit
 * (raw & 0x7F yields a 7-bit ASCII character), and places it in two queues:
 *   read_q    — for the upper-half PortRead() call to consume.
 *   loopback_q — so PortWriteOne() will echo it back to the terminal.
 *
 * For newline/carriage-return: an extra '\n' is appended to loopback_q so
 * the terminal moves to a new line after the echo.
 *
 * SemPost on read_sid unblocks a PortRead() caller that was waiting for input.
 */
void PortReadOne(int port_num){
    unsigned char raw = inportb(port[port_num].IO + DATA);

    /* Strip the parity bit: 8-bit raw → 7-bit ASCII. */
    char one = (char)(raw & 0x7F);

    if(queue_is_full(&port[port_num].read_q)){
        cons_printf("Kernel Panic: you are typing on terminal is super fast!\n");
        return;
    }
    printf("[RX p=%d ch='%c', '0x%x']\n", port_num, (char)one, one);

    queue_in(&port[port_num].read_q,     one);  /* for upper-half PortRead */
    queue_in(&port[port_num].loopback_q, one);  /* echo it back */

    /* When the user presses Enter, add '\n' to the echo so the terminal
     * moves down to a new line (CR alone just moves to column 0). */
    if(one == '\r' || one == '\n'){
        queue_in(&port[port_num].loopback_q, '\n');
    }
    /* Signal read_sid: a character is now available for the upper half. */
    SemPostHandler(port[port_num].read_sid);
}

/*
 * PortHandler — hardware IRQ handler for serial ports (lower half dispatcher)
 *
 * Called when the UART fires IRQ3 or IRQ4.  Loops over all configured ports
 * and checks each port's IIR (Interrupt Identification Register):
 *   IIR_RXRDY — UART received a byte; call PortReadOne().
 *   IIR_TXRDY — UART is ready to transmit; call PortWriteOne().
 *
 * Also checks write_ok: if a previous call to PortWriteHandler() found the
 * UART busy (write_ok was 0), it set write_ok = 1 and deferred the send.
 * We catch that here and retry the send now that we're in an interrupt context.
 *
 * Sends EOI (0x20) to port 0x20 to acknowledge the interrupt to the PIC.
 */
void PortHandler(){
    for(int port_num = 0; port_num < PORT_NUM; port_num++){
        if(port[port_num].owner == 0) continue;  /* skip unallocated ports */

        unsigned char iir = inportb(port[port_num].IO + IIR);
        if(iir == IIR_RXRDY) PortReadOne(port_num);
        if(iir == IIR_TXRDY) PortWriteOne(port_num);
        if(port[port_num].write_ok != 0) PortWriteOne(port_num);
    }
    outportb(0x20, 0x20);   /* EOI to master PIC */
}

/*
 * PortAllocHandler — allocate and initialize a serial port (upper half setup)
 *
 * Parameter:
 *   eax — pointer to the caller's EAX register; the port number is written
 *          here so the user process receives it as the system-call return value.
 *
 * Assigns the next free port slot, programs the UART hardware, and writes
 * the port index back through *eax.
 *
 * UART initialization sequence for 9600 baud, 7 data bits, even parity, 1 stop bit:
 *   1. Disable all UART interrupts temporarily (IER = 0).
 *   2. Enable DLAB (Divisor Latch Access Bit) by setting bit 7 of CFCR.
 *      This redirects reads/writes of the data register to the baud divisor.
 *   3. Write the 16-bit divisor (115200 / baud_rate) to BAUDLO + BAUDHI.
 *   4. Clear DLAB and set the line parameters in CFCR.
 *   5. Configure MCR (Modem Control Register) to enable DTR, RTS, and IRQs.
 *   6. A short I/O-delay loop lets the hardware settle before enabling interrupts.
 *   7. Enable RX and TX interrupts via IER.
 *
 * Static IO array:
 *   COM2 = 0x2F8, COM3 = 0x3E8, COM4 = 0x2E8 (standard PC base addresses).
 */
void PortAllocHandler(int *eax){
    static int IO[PORT_NUM] = {0x2f8, 0x3e8, 0x2e8};  /* COM2, COM3, COM4 */

    int p;
    for(p = 0; p < PORT_NUM; p++){
        if(port[p].owner == 0) break;   /* find first unowned slot */
    }
    if(p == PORT_NUM){
        cons_printf("Kernel Panic: no port left!\n");
        return;
    }

    *eax = p;   /* return port index to caller */

    memset((char *)&port[p], 0, sizeof(port_t));
    port[p].owner = current_pid;
    port[p].IO    = IO[p];
    port[p].write_ok = 1;  /* UART starts idle; first write can go immediately */

    /* ---- Program the UART ------------------------------------------- */
    int baud    = 9600;
    int divisor = 115200 / baud;   /* 12 for 9600 baud */

    outportb(port[p].IO + IER,  0x0);         /* 1. disable all UART interrupts */
    outportb(port[p].IO + CFCR, CFCR_DLAB);  /* 2. enable DLAB to access divisor */
    outportb(port[p].IO + BAUDLO, (unsigned char)(divisor & 0xFF));        /* 3. low byte */
    outportb(port[p].IO + BAUDHI, (unsigned char)((divisor >> 8) & 0xFF)); /* 3. high byte */
    /* 4. Clear DLAB; set 7 data bits, even parity, 1 stop bit */
    outportb(port[p].IO + CFCR, CFCR_PEVEN | CFCR_PENAB | CFCR_7BITS);
    /* 5. Assert DTR and RTS; enable the UART's IRQ output pin */
    outportb(port[p].IO + MCR, MCR_DTR | MCR_RTS | MCR_IENABLE);
    /* 6. Short I/O delay: each inb $0x80 ≈ 600 ns; 100 iterations ≈ 60 µs */
    for(int i = 0; i <= 0x63; ++i) asm volatile("inb $0x80");
    /* 7. Enable RX-ready and TX-ready interrupt sources */
    outportb(port[p].IO + IER, IER_ERXRDY | IER_ETXRDY);
}

/*
 * PortWriteHandler — buffer one character for transmission (upper half TX)
 *
 * Parameters:
 *   one      — the character to send
 *   port_num — which port to write to
 *
 * Called via system call after PortWrite() in syscall.c has already called
 * SemWait(write_sid) to ensure there is space in write_q.
 *
 * If the UART TX path is currently idle (write_ok != 0), PortWriteOne() is
 * called immediately to start transmission rather than waiting for the next
 * TXRDY interrupt.
 */
void PortWriteHandler(char one, int port_num){
    if(queue_is_full(&port[port_num].write_q)){
        cons_printf("Kernel Panic: terminal is not prompting (fast enough)?\n");
        return;
    }
    queue_in(&port[port_num].write_q, one);

    /* If the UART was idle, kick off the first byte now. */
    if(port[port_num].write_ok != 0){
        PortWriteOne(port_num);
    }
}

/*
 * PortReadHandler — dequeue one received character (upper half RX)
 *
 * Parameters:
 *   one      — pointer to a char where the received byte will be stored
 *   port_num — which port to read from
 *
 * Called via system call after PortRead() in syscall.c has already called
 * SemWait(read_sid) to ensure at least one character is in read_q.
 * Pulls one character from read_q and stores it at *one.
 */
void PortReadHandler(char *one, int port_num){
    if(queue_is_empty(&port[port_num].read_q)){
        cons_printf("Kernel Panic: nothing in typing/read buffer?\n");
        return;
    }
    int ch;
    queue_out(&port[port_num].read_q, &ch);
    *one = (char)ch;
}
