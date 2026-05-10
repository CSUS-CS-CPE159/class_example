/*
 * main.c — Kernel entry point and dispatcher for the Serial Port Driver example
 *
 * WHAT THIS FILE DEMONSTRATES:
 *   - How a simple OS kernel is structured as an infinite loop driven by
 *     hardware interrupts.  Every interrupt causes the kernel to run briefly,
 *     dispatch to a handler, then select and resume the appropriate process.
 *   - A minimal round-robin scheduler (Scheduler) based on a ready queue.
 *   - How to wire up multiple interrupt handlers in the IDT, including both
 *     hardware IRQs (timer, serial port) and software interrupts (system calls).
 *   - How the kernel maintains the split between "which process is running"
 *     (current_pid) and "which process SHOULD be running next" (Scheduler).
 *
 * CONCEPTS TO LEARN:
 *   - current_pid = 0 means no process is selected.  The kernel uses 0 as a
 *     sentinel "nobody is running" value; real process PIDs start at 1.
 *   - Kernel() is the central dispatch function called on EVERY interrupt.
 *     It reads the event number from the trapframe to decide which handler
 *     to invoke, then calls Scheduler() to pick the next process to run.
 *   - PIC mask: writing to port 0x21 (master PIC) controls which hardware
 *     IRQs are delivered to the CPU.  A 0-bit enables the IRQ; a 1-bit masks it.
 *       ~0x19 = 0b11100110: enables IRQ0 (timer), IRQ3 (COM2), IRQ4 (COM1).
 *
 * HOW IT FITS:
 *   main() runs once at boot (as a pseudo-process 0 that is never put on
 *   the ready queue).  It initializes data structures, registers handlers,
 *   creates real processes, calls Scheduler() to pick the first one, and
 *   calls KernelExit() to start it.  After that, every interrupt returns
 *   to Kernel() via events.S, which calls Kernel() to dispatch and continue.
 */
// Device Driver: "Serials Port with Interrupt Driven"

#include "spede.h"    // given SPEDE stuff
#include "types.h"    // data types
#include "handlers.h" // handler code
#include "proc.h"     // processes such as Init()
#include "events.h"   // events for kernel to serve
#include "queue.h"    // queue function

/* ---- Kernel global variables -------------------------------------------- */
int current_pid, current_time;  /* current running PID (0 = none); system timer tick */
q_t ready_q, free_q;           /* ready-to-run PIDs; unallocated PIDs */
pcb_t pcb[PROC_NUM];           /* one PCB per possible process */
char proc_stack[PROC_NUM][PROC_STACK_SIZE]; /* one runtime stack per process */
struct i386_gate *IDT_p;        /* pointer into the CPU's IDT */
sem_t sem[Q_SIZE];              /* semaphore table */
port_t port[PORT_NUM];          /* serial port descriptor table */

/*
 * IDTEntrySet — register one handler function in the IDT
 *
 * Parameters:
 *   event_num  — IDT index (e.g. 32 for timer, 0x66 for SemAlloc)
 *   event_addr — address of the assembly entry-point function (from events.S)
 *
 * Uses ACC_INTR_GATE so the CPU automatically disables hardware interrupts
 * when this gate is triggered, preventing nested kernel entry.
 */
void IDTEntrySet(int event_num, func_ptr_t event_addr){
    fill_gate(&IDT_p[event_num], (int)event_addr, get_cs(), ACC_INTR_GATE, 0);
}

/*
 * Scheduler — choose the next process to run
 *
 * If current_pid != 0, a process is already running (or will continue after
 * the current interrupt), so nothing needs to be done.
 *
 * If current_pid == 0, no process is selected.  The scheduler dequeues the
 * next PID from ready_q, marks it RUN, and sets current_pid.
 *
 * This is a pure FIFO (first-in, first-out) scheduler.  A process stays on
 * the CPU until it either blocks (e.g. SemWait) or exhausts its time slice
 * (TIME_LIMIT timer ticks).
 */
void Scheduler(){
    if(current_pid != 0) return;  /* someone is still scheduled; do nothing */

    if (queue_is_empty(&ready_q)){
        cons_printf("Kernel Panic: no process to run!\n");
        breakpoint();
    }
    queue_out(&ready_q, &current_pid);  /* pick the front process */
    pcb[current_pid].state = RUN;
    pcb[current_pid].cpu_time = 0;     /* reset its CPU time budget */
}

/*
 * main — kernel bootstrap (runs as implicit process 0, never re-entered)
 *
 * Initializes all kernel data structures, installs interrupt handlers,
 * creates the Init and TermProc processes, and launches the first one.
 */
int main() {
    int i;

    /* Zero all kernel data so there are no stale pointers or garbage. */
    memset((char *)&ready_q, 0, sizeof(q_t));
    memset((char *)&free_q,  0, sizeof(q_t));
    memset((char *)&sem,  0, sizeof(sem_t) * Q_SIZE);
    memset((char *)&port, 0, sizeof(port_t) * PORT_NUM);

    current_time = 0;

    /* Fill free_q with PID 1 through PROC_NUM-1.
     * PID 0 is reserved for main() itself; real processes start at PID 1. */
    for(i = 1; i < PROC_NUM; i++){
        queue_in(&free_q, i);
    }

    /* Mark all ports as unowned (owner = 0 means available). */
    for(i = 0; i < PORT_NUM; i++){
        port[i].owner = 0;
    }

    /* ---- Set up the IDT ------------------------------------------------- */
    IDT_p = get_idt_base();
    cons_printf("IDT located @ DRAM addr %x (%d).\n", IDT_p, IDT_p);

    /* Hardware timer (IRQ 0 → IDT entry 32): preempts processes every ~10 ms */
    IDTEntrySet(TIMER_EVENT, TimerEvent);

    /* Software interrupts for semaphore operations (system calls from user code) */
    IDTEntrySet(SEMALLOC_EVENT, SemAllocEvent);  /* 0x66 — allocate a semaphore */
    IDTEntrySet(SEMWAIT_EVENT,  SemWaitEvent);   /* 0x67 — wait/decrement */
    IDTEntrySet(SEMPOST_EVENT,  SemPostEvent);   /* 0x68 — signal/increment */

    /* Hardware serial port interrupts (IRQ 3 → IDT 35, IRQ 4 → IDT 36) */
    IDTEntrySet(PORT_EVENT,     PortEvent);      /* 0x23 — COM2 (IRQ 3) */
    IDTEntrySet(PORT_EVENT + 1, PortEvent);      /* 0x24 — COM1 (IRQ 4); same handler */

    /* Software interrupts for port operations */
    IDTEntrySet(PORTALLOC_EVENT, PortAllocEvent);/* 0x6A — allocate/init a serial port */
    IDTEntrySet(PORTWRITE_EVENT, PortWriteEvent);/* 0x6B — send one character */
    IDTEntrySet(PORTREAD_EVENT,  PortReadEvent); /* 0x6C — receive one character */

    /* ---- Enable selected hardware IRQs ---------------------------------- */
    /* Port 0x21 = master PIC interrupt mask register.
     * Bit = 0 → IRQ enabled.  ~0x19 enables bits 0 (IRQ0), 3 (IRQ3), 4 (IRQ4).
     *   IRQ0 = timer, IRQ3 = COM2, IRQ4 = COM1 (serial ports for the terminal). */
    outportb(0x21, ~0x19);

    /* ---- Create processes ------------------------------------------------ */
    NewProcHandler(Init);      /* PID 1: keyboard/console handler */
    NewProcHandler(TermProc);  /* PID 2: serial-port terminal I/O */

    /* ---- Start the first process ---------------------------------------- */
    Scheduler();               /* select current_pid from ready_q */
    KernelExit(pcb[current_pid].TF_p); /* restore its registers and run it */
    return 0;                  /* never reached; satisfies the compiler */
}

/*
 * Kernel — the central interrupt dispatch function
 *
 * Called from events.S on EVERY interrupt (timer, system call, serial I/O).
 * TF_p points to the trapframe built on the kernel stack.
 *
 * Flow:
 *   1. Save TF_p into the current process's PCB so we can resume it later.
 *   2. Dispatch to the appropriate handler based on event_num.
 *   3. Call Scheduler() to (re)select current_pid.
 *   4. Call KernelExit() to restore the selected process's registers and resume it.
 *
 * Note: Kernel() never returns in the normal sense; KernelExit() ends with IRET.
 */
void Kernel(TF_t *TF_p) {
    /* Save the current trapframe into the running process's PCB. */
    pcb[current_pid].TF_p = TF_p;

    /* Dispatch based on which interrupt fired. */
    switch (TF_p->event_num){
        case TIMER_EVENT:
            TimerHandler();    /* count time, preempt if time-slice expired */
            break;

        /* Semaphore system calls */
        case SEMALLOC_EVENT:
            SemAllocHandler(TF_p->eax);   /* EAX = initial pass count */
            break;
        case SEMWAIT_EVENT:
            SemWaitHandler(TF_p->eax);    /* EAX = semaphore ID */
            break;
        case SEMPOST_EVENT:
            SemPostHandler(TF_p->eax);    /* EAX = semaphore ID */
            break;

        /* Serial port system calls and hardware IRQs */
        case PORT_EVENT:
            PortHandler();      /* hardware IRQ: read/write a byte from the UART */
            break;
        case PORTALLOC_EVENT:
            PortAllocHandler((int *)&TF_p->eax);  /* allocate & init a port; return port# in EAX */
            break;
        case PORTWRITE_EVENT:
            PortWriteHandler((char)TF_p->eax, TF_p->ebx); /* EAX=char, EBX=port# */
            break;
        case PORTREAD_EVENT:
            PortReadHandler((char *)TF_p->eax, TF_p->ebx); /* EAX=buf ptr, EBX=port# */
            break;
        default:
            cons_printf("Kernel Panic: unknown event_num %d!\n");
            breakpoint();
    }

    /* Re-select the process to run (may be the same one, or a newly unblocked one). */
    Scheduler();
    KernelExit(pcb[current_pid].TF_p);  /* resume selected process */
}
