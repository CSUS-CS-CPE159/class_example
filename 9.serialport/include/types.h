/*
 * types.h — Core data type definitions for the Serial Port Driver example
 *
 * WHAT THIS FILE DEMONSTRATES:
 *   - How the OS represents its key abstractions as C structs:
 *       TF_t   — trapframe: the CPU register snapshot at interrupt time
 *       pcb_t  — process control block: all state needed to freeze/resume a process
 *       sem_t  — semaphore: a counter plus a wait queue for blocking processes
 *       port_t — serial port descriptor: hardware base address, semaphores, and
 *                data queues for the interrupt-driven driver
 *   - Why the layout of TF_t must exactly match the push sequence in events.S.
 *
 * HOW IT FITS:
 *   Included by every kernel source file.  The q_t (queue) type comes from
 *   queue.h, which this file includes so higher-level structs can embed queues.
 */

#ifndef __TYPES_H__
#define __TYPES_H__

#include "queue.h"

/* ---- System-wide constants ---------------------------------------------- */
#define LOOP            166666  /* busy-wait iterations for ~1 second delay */
#define TIME_LIMIT      10      /* timer ticks before a process is preempted */
#define PROC_NUM        20      /* maximum number of simultaneous processes */
#define Q_SIZE          20      /* semaphore table size; also max queue capacity */
#define PROC_STACK_SIZE 4096    /* each process gets a 4 KB runtime stack */
#define PORT_NUM        3       /* number of supported serial ports (COM2/3/4) */
#define BUFF_SIZE       101     /* max line length for PortRead (including NUL) */

/*
 * TF_t — Trapframe (CPU register snapshot)
 *
 * Built by events.S when an interrupt fires; restored by KernelExit.
 * The field order here must match the push/pop order in kernel_enter / KernelExit
 * in events.S.  Reading from bottom-of-frame to top-of-frame (as the CPU
 * pushes onto the stack, which grows downward):
 *
 *   Lowest address (most-recently-pushed item) → gs
 *   ...
 *   Highest address (first-pushed item)        → eflags
 *
 * Filler fields exist because PUSHL always writes 32 bits; the upper 16 bits
 * of a 16-bit segment register push are undefined, so we name the padding.
 */
typedef struct {
    /* Segment registers (pushed last in kernel_enter; therefore at the top of the frame) */
    unsigned short gs;       /* extra data segment */
    unsigned short filler1;
    unsigned short fs;
    unsigned short filler2;
    unsigned short es;
    unsigned short filler3;
    unsigned short ds;       /* main data segment */
    unsigned short filler4;

    /* General-purpose registers saved by PUSHA */
    unsigned int edi;
    unsigned int esi;
    unsigned int ebp;
    unsigned int esp;  /* value of ESP before PUSHA; POPA ignores this field */
    unsigned int ebx;  /* syscall argument (e.g. port number in PortWrite) */
    unsigned int edx;
    unsigned int ecx;
    unsigned int eax;  /* syscall number (in) / return value (out) */

    unsigned int event_num;  /* which interrupt fired (set in events.S ENTRY points) */

    /* Pushed automatically by the CPU when the interrupt fires */
    unsigned int eip;    /* saved instruction pointer: where to resume */
    unsigned int cs;     /* saved code segment */
    unsigned int eflags; /* saved CPU flags (interrupt enable bit, etc.) */
    /* Note: ESP and SS are only pushed by the CPU for Ring-3 → Ring-0 transitions.
     * Since all processes in this example run at Ring 0, they are NOT in TF_t. */
} TF_t;

typedef void (*func_ptr_t)();   /* generic void-return function pointer */

/*
 * state_t — process lifecycle states
 *
 *   FREE  — PID slot is unallocated (in free_q)
 *   RUN   — process is currently executing on the CPU
 *   READY — process is runnable but waiting for the CPU (in ready_q)
 *   SLEEP — process is waiting for a specific time to elapse (wake_time)
 *   WAIT  — process is blocked on a semaphore (in sem[].wait_q)
 *   ZOMBIE — process has exited but not yet been cleaned up
 */
typedef enum {FREE, RUN, READY, SLEEP, WAIT, ZOMBIE} state_t;

/*
 * pcb_t — Process Control Block
 *
 * Everything the kernel needs to pause one process and resume another.
 * TF_p always points into the process's proc_stack[], so the trapframe
 * and the stack share the same memory (the trapframe lives at the TOP of
 * the stack, growing downward from proc_stack[pid][PROC_STACK_SIZE]).
 */
typedef struct {
    state_t state;      /* current lifecycle state */
    int cpu_time;       /* timer ticks used in the current time slice */
    int wake_time;      /* absolute tick count when a SLEEPing process should wake */
    TF_t *TF_p;         /* pointer to the process's trapframe in proc_stack[] */
} pcb_t;

/*
 * sem_t — Semaphore
 *
 * A classic counting semaphore.  'passes' is the resource count:
 *   > 0 → SemWait() can proceed without blocking.
 *   = 0 → SemWait() blocks the caller in wait_q.
 * SemPost() either increments passes or wakes the first waiter.
 *
 * owner identifies which process created this semaphore (used to detect
 * unallocated slots: owner == 0 means the slot is free).
 */
typedef struct {
    int owner;      /* PID of the process that called SemAlloc; 0 = unowned */
    int passes;     /* remaining tokens (resources available without blocking) */
    q_t wait_q;    /* queue of PIDs blocked waiting for a pass */
} sem_t;

/*
 * port_t — Serial port descriptor
 *
 * Holds everything the kernel needs to manage one serial port (UART):
 *
 *   owner     — PID of the process that called PortAlloc; 0 = unowned.
 *   IO        — base I/O address of the UART (e.g. 0x2F8 for COM2).
 *   write_sid — semaphore controlling TX queue occupancy.  Initial passes =
 *               QUEUE_SIZE, so the writer can buffer up to QUEUE_SIZE chars
 *               before blocking.  Each transmitted byte posts one pass back.
 *   read_sid  — semaphore signaling RX data availability.  Initial passes = 0,
 *               so PortRead() blocks until at least one byte arrives.  Each
 *               received byte posts one pass.
 *   write_ok  — flag (1 = UART TX path is idle; 0 = a TX is in progress).
 *               Allows PortWriteHandler() to kick off an immediate write
 *               instead of waiting for the next TXRDY interrupt.
 *   write_q   — circular queue of characters waiting to be transmitted.
 *   read_q    — circular queue of characters received from the UART.
 *   loopback_q— circular queue of characters to be echoed back to the terminal
 *               (so the user sees what they typed; takes TX priority over write_q).
 */
typedef struct {
    int owner;
    int IO;
    int write_sid;
    int read_sid;
    int write_ok;
    q_t write_q;
    q_t read_q;
    q_t loopback_q;
} port_t;

#endif /* __TYPES_H__ */
