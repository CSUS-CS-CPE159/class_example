/*
 * types.h — Core data type definitions for the User Process example
 *
 * WHAT THIS FILE DEMONSTRATES:
 *   - The trapframe_t structure, which must exactly mirror the sequence of
 *     push instructions in events.S.  Getting this layout wrong causes the
 *     kernel to restore incorrect register values when resuming a process.
 *   - How the process control block (proc_t) stores the minimum state needed
 *     to freeze and resume a process: its type, a PID, a page table base,
 *     and a pointer back to its saved trapframe.
 *   - A tiny syscall_t enum that defines the system-call numbers shared
 *     between user space and the kernel.
 *
 * HOW IT FITS:
 *   Every kernel source file includes this header.  The user-space binary
 *   also redefines syscall_t locally (see user/src/process.c) because the
 *   user and kernel are compiled separately.
 */
#ifndef _TYPES_H_
#define _TYPES_H_

/* Bit position of the "present" flag in a TSS descriptor's access byte. */
#define TSS_PRESENT     8

/* Size of each process's dedicated kernel stack in bytes (16 KB). */
#define STACK_SIZE      16384

/* IDT entry number for the timer interrupt (IRQ 0 is remapped to entry 32). */
#define TIMER_EVENT     32

/* Number of entries in both a page directory and a page table (2^10 = 1024). */
#define NUM_ENTRIES     1024

/*
 * Page-table entry flag bits (stored in the low 12 bits of each entry).
 * The CPU checks these bits before translating a virtual address.
 */
#define PAGE_PRESENT    0x1         /* entry is valid; CPU will use it */
#define PAGE_WRITE      0x2         /* page is writable (0 = read-only) */
#define PAGE_USER       0x4         /* Ring-3 code may access this page */
#define PAGE_SIZE       0x00001000  /* 4096 bytes per page */


typedef unsigned short seg_type_t;  /* 16-bit x86 segment register value */


/*
 * trapframe_t — snapshot of all CPU registers at the moment an interrupt fires
 *
 * The layout here must exactly match the order of push/pop instructions in
 * events.S.  The structure is built from the BOTTOM of the frame upward:
 *   1. CPU automatically pushes: (user_ss, user_esp,) eflags, cs, eip
 *   2. events.S pushes:          error_code, event_type
 *   3. PUSHA pushes:             edi, esi, ebp, esp, ebx, edx, ecx, eax
 *   4. events.S pushes manually: ds, es, fs, gs   (in that order)
 *
 * When reading this struct, remember the stack grows DOWNWARD, so the
 * first member (gs) is at the LOWEST address (the most-recently-pushed item).
 *
 * The filler fields after each 16-bit segment register exist because PUSHL
 * always pushes 32 bits; the upper 16 bits hold undefined garbage, so we
 * name them to make the struct size unambiguous.
 */
typedef struct {
    /* Segment registers — pushed last (LIFO), restored first */
    seg_type_t gs;       /* data segment (extra) */
    seg_type_t _notgs;   /* padding: pushl pushes 32 bits for a 16-bit reg */

    seg_type_t fs;
    seg_type_t _notfs;

    seg_type_t es;
    seg_type_t _notes;

    seg_type_t ds;       /* main data segment */
    seg_type_t _notds;

    /* General-purpose registers — saved/restored by PUSHA/POPA */
    unsigned int edi;
    unsigned int esi;
    unsigned int ebp;
    unsigned int esp;    /* value ESP had before PUSHA; POPA skips restoring it */
    unsigned int ebx;    /* also used for syscall argument 1 (by convention) */
    unsigned int edx;
    unsigned int ecx;
    unsigned int eax;    /* syscall number (in), return value (out) */

    /* Set by events.S before calling context_switch() */
    unsigned int event_type;   /* 0x20 = timer, 0x80 = syscall */
    unsigned int error_code;   /* CPU exception error code (0 for software ints) */

    /* Pushed automatically by the CPU when the interrupt fires */
    unsigned int eip;          /* saved instruction pointer (where to resume) */
    unsigned int cs;           /* saved code segment */
    unsigned int eflags;       /* saved CPU flags (interrupt enable, etc.) */

    /* These two are only pushed by the CPU when crossing privilege rings
     * (e.g., Ring-3 user code interrupted into Ring-0 kernel). */
    unsigned int user_esp;     /* user-mode stack pointer */
    unsigned int user_ss;      /* user-mode stack segment */
} trapframe_t;

/* Process privilege levels. */
typedef enum {
    PROC_TYPE_NONE,     /* slot not in use */
    PROC_TYPE_KERNEL,   /* kernel-mode process (Ring 0, shares kernel page table) */
    PROC_TYPE_USER      /* user-mode process  (Ring 3, has its own page table) */
} proc_type_t;


/*
 * proc_t — process control block (PCB)
 *
 * Stores everything the kernel needs to freeze and resume a process.
 * trapframe is a pointer into proc_kernel_stack[] (not a separate allocation).
 * pagetable is the physical address of the process's page directory (CR3 value).
 */
typedef struct {
    int pid;                /* unique process identifier */
    proc_type_t proc_type;  /* kernel or user */
    size_t *pagetable;      /* page directory base address (loaded into CR3) */
    trapframe_t *trapframe; /* pointer to saved CPU state on the kernel stack */
} proc_t;

typedef uint32_t pagetable_t;  /* physical address of a page directory or table */

/*
 * syscall_t — system-call numbers
 *
 * The user process puts one of these values in EAX before executing
 * INT 0x80.  SyscallHandler() in handlers.c reads EAX to dispatch.
 * Both user and kernel must agree on the same numbering.
 */
typedef enum {
     SYSCALL_NONE,   /* 0 — no-op / unused */
     SYSCALL_PRINT,  /* 1 — print a string; EBX = pointer to the string */
} syscall_t;

#endif
