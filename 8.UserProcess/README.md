# Example 8 — User Process

This example extends the kernel with two new capabilities:
**user-mode processes** (Ring 3) and **virtual memory** (per-process page tables).
A kernel-mode process and a user-mode process alternate on the CPU, communicating
through a software interrupt system call (INT 0x80).

---

## Learning Objectives

After studying this example you should be able to:

1. Explain the purpose of the GDT, IDT, and TSS and how they interact at boot.
2. Decode a segment selector (index, TI, RPL) and connect it to a GDT entry.
3. Describe how a trapframe is built on the kernel stack during an interrupt.
4. Trace a context switch from timer interrupt → `kernel_enter` → `context_switch()` → `ProcLoader()` → IRET.
5. Explain how a user process issues a system call (INT 0x80) and how the kernel dispatches it.
6. Describe the two-level x86 page table and how `setup_pagetable()` maps a user process's code and stack.
7. Explain the free-list physical page allocator (`kalloc` / `kfree`).

---

## File Structure

```
8.UserProcess/
├── src/
│   ├── main.c          Kernel entry: GDT/TSS/IDT setup, process creation, context_switch()
│   ├── events.S        Assembly: interrupt entry (TimerEntry, SyscallEntry), ProcLoader, KernelTssFlush
│   ├── handlers.c      Process creation (NewKernelProcHandler, NewUserProcHandler), SyscallHandler
│   ├── process.c       SystemProc — the kernel-mode process (PID 0)
│   ├── kalloc.c        Physical page allocator (free-list, 4 KB pages)
│   └── page.c          Page table construction for user processes (setup_pagetable)
├── user/
│   └── src/
│       └── process.c   User-space code: sys_printf() via INT 0x80, Process() loop
├── include/
│   ├── types.h         trapframe_t, proc_t, syscall_t, page flags
│   ├── kalloc.h        Page allocator constants and prototypes
│   ├── page.h          Page table prototypes and PF error bit definitions
│   ├── handlers.h      Handler prototypes
│   ├── events.h        Event entry-point prototypes
│   ├── data.h          extern declarations for kernel globals
│   └── process.h       SystemProc prototype
└── Makefile
```

---

## How It Works

### Boot sequence (`main.c`)

1. **Locate the GDT** — `get_gdt()` finds the table already set up by SPEDE.
2. **Configure the TSS** — fill in the kernel segment selectors so the CPU knows which stack to use when an interrupt promotes Ring 3 → Ring 0.
3. **Initialize `kalloc`** — build a free list from 50 pre-allocated 4 KB pages (`page_address[]`).
4. **Register interrupt handlers** — `TimerEntry` at IDT[32] (IRQ 0), `SyscallEntry` at IDT[128] (INT 0x80, user-accessible).
5. **Save `kernel_cr3`** — the kernel's page-directory base, so user processes can inherit kernel mappings.
6. **Create processes** — `NewKernelProcHandler(SystemProc)` (PID 0) and `NewUserProcHandler(...)` (PID 1).
7. **Jump to PID 0** — `ProcLoader()` restores the first trapframe and executes IRET.

### Context switch (`main.c` + `events.S`)

Every interrupt follows the same path:

```
Timer fires (or user executes INT 0x80)
  │
  ▼
TimerEntry / SyscallEntry  (events.S)
  push error_code, event_type
  │
  ▼
kernel_enter  (events.S)
  pusha; push ds/es/fs/gs       ← build trapframe on process's kernel stack
  switch DS/ES to kernel segment
  call context_switch(trapframe *)
  │
  ▼
context_switch()  (main.c)
  if syscall  → SyscallHandler() → return (same process resumes)
  if timer    → send EOI to PIC
                save trapframe in PCB
                alternate active_process between p[0] and p[1]
                update TSS.esp0 and CR3
  │
  ▼
ProcLoader(trapframe *)  (events.S)
  ESP ← trapframe address
  pop gs/fs/es/ds; popa
  skip event_type + error_code
  IRET  ← restores EIP, CS, EFLAGS (and ESP, SS for Ring-3 return)
```

### System call (`user/src/process.c` + `handlers.c`)

```c
// User side (Ring 3)
asm("movl %1, %%eax;"   // EAX = SYSCALL_PRINT
    "movl %2, %%ebx;"   // EBX = string pointer
    "int $0x80;");      // trap into kernel

// Kernel side — SyscallHandler() reads from active_process->trapframe:
syscall = trapframe->eax;   // which service
arg1    = trapframe->ebx;   // argument
// ... dispatch, then write return value back:
trapframe->eax = rc;
```

### Virtual memory (`page.c` + `kalloc.c`)

Each user process has its own page directory.  `setup_pagetable()` allocates
five physical pages:

| Page | Purpose | Virtual address mapped |
|------|---------|----------------------|
| 1 | Page directory | — (the CR3 value itself) |
| 2 | Page table for stack | dir[511] |
| 3 | Stack page | 0x7FFFF000 (ESP starts at 0x80000000 − 16) |
| 4 | Page table for code | dir[512] |
| 5 | Code page | 0x80000000 (EIP = 0x80000080) |

The first five kernel directory entries are copied into every user page directory
so kernel code remains reachable after a Ring-3 → Ring-0 transition.

---

## Global Descriptor Table (GDT)

### What is the GDT?

The GDT defines **memory segments** — contiguous regions of the address space
with an associated privilege level (Ring 0 = kernel, Ring 3 = user).
Before the CPU executes any instruction it checks the segment registers (CS, DS, …)
against the GDT to enforce protection.

### GDT layout in SPEDE

Each entry is 8 bytes.  The selector value for entry `n` is `n × 8`.

| Index | Selector | Privilege | Purpose |
|------:|:--------:|:---------:|---------|
| 0 | 0x00 | — | Null (required by x86) |
| 1 | 0x08 | Ring 0 | Kernel code |
| 2 | 0x10 | Ring 0 | Kernel data |
| 3 | 0x18 | Ring 0 | Kernel stack |
| 4 | 0x20 | — | (SPEDE reserved) |
| 5 | 0x28 | — | (SPEDE reserved) |
| 6 | 0x30 | Ring 3 | User code |
| 7 | 0x38 | Ring 3 | User data |
| 8 | 0x40 | Ring 3 | User stack |
| 9 | 0x48 | Ring 0 | Kernel TSS (added by our kernel) |

### Segment selectors

A segment register holds a 16-bit **selector**:

```
15               3   2   1 0
┌─────────────────┬───┬─────┐
│   GDT index     │TI │ RPL │
└─────────────────┴───┴─────┘
  bits [15:3]      bit 2  bits [1:0]
```

- **TI** = 0 → use GDT (1 → LDT, not used here)
- **RPL** = Requested Privilege Level: `00` = Ring 0, `11` = Ring 3

Examples used in `handlers.c`:
```
User CS  = 0x30 | 0x3 = 0x33   (GDT[6], Ring 3)
User DS  = 0x38 | 0x3 = 0x3B   (GDT[7], Ring 3)
User SS  = 0x40 | 0x3 = 0x43   (GDT[8], Ring 3)
```

### GDT entry format (8 bytes)

```
Byte:  7        6         5         4     3–2       1–0
       Base     Flags +   Access    Base  Base      Limit
       [31:24]  Limit     Byte      [23:  [15:0]    [15:0]
                [19:16]             16]
```

**Access Byte** bit fields:

| Bit 7 | Bits 6–5 | Bit 4 | Bit 3 | Bit 2 | Bit 1 | Bit 0 |
|:-----:|:--------:|:-----:|:-----:|:-----:|:-----:|:-----:|
| P (Present) | DPL (privilege) | S (1=code/data) | E (1=code) | DC | RW | A |

**Flags byte** (upper nibble of byte 6):

| Bit 7 | Bit 6 | Bit 5 | Bit 4 |
|:-----:|:-----:|:-----:|:-----:|
| G (granularity: 1=4KB) | D (1=32-bit) | L (1=64-bit) | AVL |

### Worked example: kernel code segment (entry 1)

Raw bytes at GDT+0x08: `ff ff 00 00 00 9a cf 00`

| Field | Value | Meaning |
|-------|-------|---------|
| Limit[15:0] | 0xFFFF | |
| Base[15:0] | 0x0000 | |
| Base[23:16] | 0x00 | |
| Access = 0x9A | `1001 1010` | P=1, DPL=00 (Ring 0), S=1, E=1 (code), R=1 |
| Flags = 0xCF | `1100 1111` | G=1 (4 KB), D=1 (32-bit), Limit[19:16]=0xF |
| Base[31:24] | 0x00 | |

Computed: base = 0x00000000, limit = 4 GiB − 1.
**Ring-0, 32-bit, readable code segment, flat 4 GiB.**

### Worked example: user code segment (entry 6)

Raw bytes at GDT+0x30: `ff ff 00 00 00 fa cf 00`

Access = 0xFA = `1111 1010`:  P=1, DPL=**11** (Ring 3), S=1, E=1, R=1.
**Ring-3, 32-bit, readable code segment, flat 4 GiB.**

### GDT summary table

| Index | Purpose | Bytes (low → high) | Notes |
|------:|---------|---------------------|-------|
| 0 | Null | `00 00 00 00 00 00 00 00` | Required |
| 1 | Kernel code | `ff ff 00 00 00 9a cf 00` | Ring-0, 32-bit, 4 GiB |
| 2 | Kernel data | `ff ff 00 00 00 92 cf 00` | Ring-0 |
| 3 | Kernel stack | `ff ff 00 00 00 93 cf 00` | Ring-0 |
| 6 | User code | `ff ff 00 00 00 fa cf 00` | Ring-3, 32-bit, 4 GiB |
| 7 | User data | `ff ff 00 00 00 f2 cf 00` | Ring-3 |
| 9 | Kernel TSS | filled by `main()` | Loaded by `KernelTssFlush` |

### GDT vs IDT

| | GDT | IDT |
|--|-----|-----|
| **Purpose** | Memory segmentation and protection | Interrupt and exception dispatch |
| **Contents** | Segment descriptors, TSS descriptor | Gate descriptors (interrupt/trap gates) |
| **Indexed by** | Segment selector (CS, DS, …) | Interrupt / exception vector (0–255) |
| **Load instruction** | `LGDT` | `LIDT` |
| **Key role here** | Defines Ring-0 and Ring-3 segments; hosts TSS | Routes timer (32) and syscall (128) to our handlers |

---

## Task State Segment (TSS)

The TSS tells the CPU **which stack to switch to** when an interrupt elevates
privilege from Ring 3 to Ring 0.  Without a correctly configured TSS, the CPU
cannot safely enter kernel mode from user code.

Fields set in `main.c`:

| Field | Value | Meaning |
|-------|-------|---------|
| `tss_ss0` | 0x18 | Kernel stack segment (GDT[3]) |
| `tss_esp0` | updated each switch | Top of current process's kernel stack |
| `tss_cs` | 0x08 | Kernel code segment |
| `tss_ds/es/fs/ss` | 0x10/0x18 | Kernel data/stack segments |
| `tss_ioopt` | `sizeof(tss)` | Disables the I/O permission bitmap (user code cannot access I/O ports) |

`KernelTssFlush()` executes `LTR $0x48` (selector for GDT entry 9) to load the
TSS into the CPU's Task Register.

---

## Debugging Tips

```bash
# Inspect the GDT entries (64 bytes = 8 entries × 8 bytes)
(gdb) x/64bx GDT_p

# Verify the active process and its trapframe
(gdb) p *active_process
(gdb) p *active_process->trapframe

# Confirm user-process page directory (five non-zero entries expected)
(gdb) x/1024wx active_process->pagetable

# Check the current CR3 (page directory base)
(gdb) monitor info registers

# See the user binary entry point
$ objdump -D user/src/user.bin | grep "<main>"
```

---

## References

- [OSDev Wiki — Global Descriptor Table](https://wiki.osdev.org/Global_Descriptor_Table)
- [OSDev Wiki — Paging](https://wiki.osdev.org/Paging)
- [OSDev Wiki — Task State Segment](https://wiki.osdev.org/TSS)
- [Single kernel stack vs per-process kernel stack (diagram)](https://docs.google.com/drawings/d/1qZzdBx5CEMplXS7eRqygyUI197gAqi66XVGQlpDGb-U/edit?usp=sharing)
