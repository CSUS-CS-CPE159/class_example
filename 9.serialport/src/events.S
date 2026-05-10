/**
 * events.S — Low-level interrupt entry and exit for the Serial Port Driver example
 *
 * CPE/CSC 159 - Operating System Pragmatics
 * California State University, Sacramento
 *
 * WHAT THIS FILE DEMONSTRATES:
 *   - How every interrupt (hardware or software) enters the kernel through a
 *     common path that saves the full CPU state into a trapframe.
 *   - A key technique: after saving the CPU state, the kernel switches to a
 *     SEPARATE kernel stack (k_stack) before calling the C Kernel() function.
 *     This keeps the kernel's own call stack clean and separate from the
 *     per-process stacks.
 *   - KernelExit: the reverse path that loads a trapframe back into the CPU
 *     registers and executes IRET to resume a (possibly different) process.
 *
 * CONCEPTS TO LEARN:
 *   - Each ENTRY() label is an IDT entry point.  When the corresponding
 *     interrupt fires, the CPU jumps here (after pushing EIP/CS/EFLAGS).
 *   - event_num pushed before jumping to kernel_enter allows the C Kernel()
 *     function to know which interrupt fired (via TF_p->event_num).
 *   - The kernel stack (k_stack) is a globally allocated buffer declared with
 *     the .comm directive.  All processes share ONE kernel stack because only
 *     one process runs at a time (no preemption while already in kernel mode).
 *   - SET_K_SEGS macro sets DS and ES to the kernel data segment (0x10) so
 *     the C kernel can safely access kernel-space variables after an interrupt
 *     that may have arrived from user mode with different segment registers.
 *
 * HOW IT FITS:
 *   The IDT entries are registered in main.c's IDTEntrySet() calls.  When an
 *   interrupt fires, hardware jumps to one of the ENTRY() labels, which push
 *   the event number and jump to kernel_enter.  kernel_enter builds the full
 *   trapframe, switches to k_stack, and calls Kernel() in main.c.  When
 *   Kernel() is done it calls KernelExit() (from here) which restores the
 *   chosen process's register state and executes IRET.
 */

#include <spede/machine/asmacros.h> // do not use/include "spede.h"
#include "events.h"                 // for K_DATA, K_STACK_SIZE below

/* Macro: set DS and ES to the kernel data segment.
 * Needed because an interrupt from user mode may arrive with user-mode
 * segment registers loaded; the kernel C code requires kernel segments. */
#define SET_K_SEGS movw $(K_DATA), %ax; mov %ax, %ds; mov %ax, %es

/* Declare the kernel stack in the BSS section (zero-initialized at boot).
 * Using a single shared kernel stack is safe here because the kernel is
 * non-preemptible: once Kernel() starts running, no new interrupt can
 * push a new frame onto this stack before the current one is done. */
.comm k_stack, K_STACK_SIZE, 1
.text

/*
 * Each ENTRY() below is a separate IDT gate entry point.
 * All they do is push their event_num (so Kernel() can tell them apart)
 * and jump to the shared kernel_enter path.
 */

/* Hardware timer (IRQ 0 → IDT 32) */
ENTRY(TimerEvent)
    pushl $TIMER_EVENT       /* push event number for Kernel() to read */
    jmp kernel_enter

/* Software interrupt: allocate a semaphore (INT 0x66) */
ENTRY(SemAllocEvent)
    pushl $SEMALLOC_EVENT
    jmp kernel_enter

/* Software interrupt: SemWait — decrement / block (INT 0x67) */
ENTRY(SemWaitEvent)
    pushl $SEMWAIT_EVENT
    jmp kernel_enter

/* Software interrupt: SemPost — increment / unblock (INT 0x68) */
ENTRY(SemPostEvent)
    pushl $SEMPOST_EVENT
    jmp kernel_enter

/* Hardware serial port interrupt (IRQ 3 / IRQ 4 → IDT 35 / 36) */
ENTRY(PortEvent)
    pushl $PORT_EVENT
    jmp kernel_enter

/* Software interrupt: allocate/initialize a serial port (INT 0x6A) */
ENTRY(PortAllocEvent)
    pushl $PORTALLOC_EVENT
    jmp kernel_enter

/* Software interrupt: write one character to a port (INT 0x6B) */
ENTRY(PortWriteEvent)
    pushl $PORTWRITE_EVENT
    jmp kernel_enter

/* Software interrupt: read one character from a port (INT 0x6C) */
ENTRY(PortReadEvent)
    pushl $PORTREAD_EVENT
    jmp kernel_enter

/**
 * kernel_enter — common kernel entry path; builds the trapframe
 *
 * Stack layout on arrival (top = lowest address):
 *   [ESP+0]  event_num    ← we just pushed this
 *   [ESP+4]  EIP          ← pushed by CPU
 *   [ESP+8]  CS           ← pushed by CPU
 *   [ESP+12] EFLAGS       ← pushed by CPU
 *   (for hardware IRQs that arrived from Ring 3, CPU also pushed ESP+SS below)
 *
 * After kernel_enter builds the frame, the layout on the PROCESS stack is:
 *   [gs][fs][es][ds]            ← we push these
 *   [edi..eax]                  ← PUSHA
 *   [event_num]                 ← pushed above
 *   [eip][cs][eflags]           ← pushed by CPU
 *
 * ECX is used to remember the trapframe address across the stack switch.
 */
kernel_enter:
    pusha           /* save all 8 general-purpose registers */
    pushl %ds       /* save the 4 data segment registers */
    pushl %es
    pushl %fs
    pushl %gs
    movl %esp, %ecx /* ECX = trapframe address (top of what we just pushed) */
    cld             /* clear direction flag (string ops go forward) */
    SET_K_SEGS      /* switch DS/ES to kernel data segment */

    /* Switch to the dedicated kernel stack.
     * leal computes: address of k_stack + K_STACK_SIZE (i.e. the stack TOP).
     * Using a separate stack means Kernel()'s local variables and call chain
     * don't touch the current process's stack. */
    leal k_stack + K_STACK_SIZE, %esp

    /* Push the trapframe address as the argument to Kernel(TF_t *TF_p). */
    pushl %ecx
    call CNAME(Kernel)   /* Kernel() selects next process and calls KernelExit() */
    /* If Kernel() somehow returns (it shouldn't), we fall through to KernelExit. */

/**
 * KernelExit — restore a process's register state and resume it
 *
 * Called from Kernel() (in C) with one argument:
 *   TF_p — pointer to the trapframe of the process to resume
 *
 * Loads ESP from TF_p, pops all saved registers in reverse order, skips the
 * event_num field, and executes IRET to jump back to the process.
 *
 * This is the ONLY way a process ever gets CPU time: every context switch
 * goes through KernelExit.
 */
ENTRY(KernelExit)
    /* TF_p is the first (and only) C argument, located at 4(%esp). */
    movl 4(%esp), %eax  /* EAX = TF_p */
    movl %eax, %esp     /* ESP = TF_p → now pointing at the saved trapframe */

    /* Restore registers in reverse push order. */
    popl %gs
    popl %fs
    popl %es
    popl %ds
    popa                /* restore edi, esi, ebp, (skip esp), ebx, edx, ecx, eax */

    add $4, %esp        /* skip the event_num field (not a CPU register) */

    /* IRET: pops EIP, CS, EFLAGS (and ESP, SS if returning to Ring 3). */
    iret
