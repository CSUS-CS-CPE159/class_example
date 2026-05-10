/**
 * events.S — Low-level interrupt entry and exit for the User Process example
 *
 * WHAT THIS FILE DEMONSTRATES:
 *   - How the CPU and the kernel cooperate to save/restore process state
 *     across an interrupt or system call.
 *   - The difference between what the CPU pushes automatically (EIP, CS,
 *     EFLAGS, and for Ring-3 transitions: ESP, SS) and what the kernel
 *     pushes manually (segment registers, general-purpose registers).
 *   - How to switch segment registers so that kernel C code can safely
 *     access memory after an interrupt arrives from user mode.
 *
 * CONCEPTS TO LEARN:
 *   - Trapframe: the block of saved registers on the kernel stack that
 *     represents a frozen snapshot of the interrupted process.  Its layout
 *     must exactly match the trapframe_t struct in types.h.
 *   - IRET: the special x86 instruction that pops EIP/CS/EFLAGS (and
 *     optionally ESP/SS when returning to a lower-privilege ring) to resume
 *     a previously interrupted process.
 *   - ProcLoader: an assembly routine that takes a trapframe pointer and
 *     jumps directly into the saved process state, bypassing the normal
 *     function-call mechanism.
 *
 * HOW IT FITS:
 *   When an interrupt fires, the CPU transfers control to TimerEntry or
 *   SyscallEntry.  Those routines build the trapframe and call context_switch()
 *   in main.c.  context_switch() chooses the next process and calls
 *   ProcLoader(), which restores that process's registers and executes IRET
 *   to resume it.
 */
#include <spede/machine/asmacros.h>

.text

/*
 * TimerEntry — IDT entry point for the periodic timer interrupt (IRQ 0)
 *
 * When the timer fires, the CPU automatically pushes onto the current stack:
 *   [ESP+12] EFLAGS
 *   [ESP+ 8] CS
 *   [ESP+ 4] EIP        ← address where the interrupted code will resume
 *   (Because this is a Ring-3 → Ring-0 transition, the CPU also pushes
 *    the user-mode SS and ESP before EFLAGS.)
 *
 * We then push two values manually so that kernel_enter can build a
 * uniform trapframe regardless of which interrupt fired:
 *   push $0x0   — placeholder error code (timer has no error code)
 *   push $0x20  — event type (0x20 = IDT entry 32 = timer)
 *
 * The event_type field lets context_switch() know why it was called.
 */
ENTRY(TimerEntry)
    pushl $0x0          /* error code placeholder */
    pushl $0x20         /* event type: timer (IDT entry 32) */
    jmp kernel_enter

/*
 * SyscallEntry — IDT entry point for software interrupts (INT 0x80)
 *
 * User code calls "int $0x80" to request a kernel service.  Because
 * this is a software interrupt (not an exception), there is no hardware
 * error code, so we push a placeholder.  The event_type 0x80 tells
 * context_switch() to dispatch to SyscallHandler() instead of switching
 * processes.
 */
ENTRY(SyscallEntry)
    pushl $0x0          /* error code placeholder */
    pushl $0x80         /* event type: system call (INT 0x80) */
    jmp kernel_enter

/**
 * kernel_enter — common path that finishes building the trapframe
 *
 * Stack on arrival (bottom = lower address, top = higher address):
 *   ... [user SS] [user ESP] [EFLAGS] [CS] [EIP]  ← pushed by CPU (Ring-3 transition)
 *       [event_type] [error_code]                  ← pushed above
 *
 * After kernel_enter completes the frame, the stack looks like:
 *   [gs][fs][es][ds][edi][esi][ebp][esp][ebx][edx][ecx][eax]
 *   [event_type][error_code][eip][cs][eflags][user_esp][user_ss]
 *
 * This matches the trapframe_t layout in types.h, read bottom-to-top.
 */
kernel_enter:
    /* Save all eight general-purpose registers in one instruction. */
    pusha

    /* Save the four data segment registers (each is 16-bit, padded to 32). */
    pushl %ds
    pushl %es
    pushl %fs
    pushl %gs

    /* EBX now holds the address of the trapframe (top of the frame we
     * just finished building). */
    mov %esp, %ebx

    /* Switch DS/ES/FS/GS to the kernel data segment (0x10).
     * Without this switch, the kernel's C code could not reliably
     * address kernel variables after an interrupt from user mode. */
    movw $(0x10), %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    /* Pass the trapframe address as the argument to context_switch().
     * The calling convention expects the argument on the stack. */
    pushl %ebx
    cld                             /* string ops go forward (defensive) */
    call CNAME(context_switch)
    addl $4, %esp                   /* pop the argument we pushed */

    /* context_switch() returns here only if it did NOT call ProcLoader.
     * In practice that happens only when the syscall path returns to
     * the same process.  Restore registers and return from interrupt. */
    popl %gs
    popl %fs
    popl %es
    popl %ds
    popa

    /* Skip the two values we pushed manually (error_code + event_type). */
    addl $8, %esp
    iret    /* pop EIP, CS, EFLAGS, and (for Ring-3 return) ESP, SS */


/**
 * ProcLoader — resume a process from its saved trapframe
 *
 * Called from context_switch() when the kernel wants to run (or resume)
 * a particular process.  'trapframe' is passed as a C function argument,
 * so it sits 4 bytes above the return address on the stack.
 *
 * This function does NOT use a normal return.  It ends with IRET, which
 * pops the saved EIP/CS/EFLAGS (and user ESP/SS for Ring-3 processes)
 * and transfers control directly to wherever the process was interrupted.
 *
 * Parameter:
 *   trapframe (trapframe_t *) — pointer to the trapframe to restore
 */
.global ProcLoader
ProcLoader:
    /* Load the trapframe pointer (4(%esp) = first argument after ret addr). */
    movl 4(%esp), %ebx
    /* Replace ESP with the trapframe address so the pops below restore
     * the saved register values in the correct order. */
    movl %ebx, %esp

    /* Restore segment registers (pushed last, so restored first). */
    popl %gs
    popl %fs
    popl %es
    popl %ds

    /* Restore the eight general-purpose registers. */
    popa

    /* Skip event_type and error_code fields (not restored to any register). */
    addl $8, %esp

    /* IRET restores EIP, CS, EFLAGS (and ESP/SS for Ring-3). */
    iret


/**
 * KernelTssFlush — load the kernel TSS into the CPU's Task Register (TR)
 *
 * The LTR instruction takes a GDT selector.  GDT entry 9 holds the TSS
 * descriptor filled in by main().  The selector is:
 *   index 9 × 8 bytes/entry = 0x48, plus flag bits → 0x48 (TI=0, RPL=0).
 *
 * This must be called once after the TSS descriptor is written into the GDT
 * so that the CPU knows where to find the kernel stack on future interrupts.
 */
ENTRY(KernelTssFlush)
    movw $0x48, %ax   /* GDT entry 9: 9 * 8 = 0x48 */
    ltr %ax           /* Load Task Register */
    ret
