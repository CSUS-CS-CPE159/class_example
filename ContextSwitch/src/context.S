/**
 * CPE/CSC 159 - Operating System Pragmatics
 * California State University, Sacramento
 *
 * Context Switch Functions
 */
#include <spede/machine/asmacros.h>

// define kernel stack space
.comm kstack, 16384, 1
.text

ENTRY(TimerEntry)  // push eflag, cs, eip (by hardware)
    // Indicate which interrupt occured
    pushl $0x20
    // Enter into the kernel context for processing
    jmp kernel_enter

/**
 * Enter the kernel context
 *  - Save register state
 *  - Load the kernel stack
 *  - Trigger entry into the kernel
 */
kernel_enter:
    // Save register state
    pusha
    pushl %ss
    pushl %ds
    pushl %es
    pushl %fs
    pushl %gs
    // Load the kernel stack
    movl %esp, %edx
    cld
    movw $(0x10), %ax
    mov %ax, %ds
    mov %ax, %es
    leal kstack + 16384, %esp
    // save esp address to process frame
    pushl %edx
    // Trigger entry into the kernel
    call CNAME(context_switch)

/**
 * Exit the kernel context
 *   - Load the process stack
 *   - Restore register state
 *   - Return from the previous interrupt
 */
 
ENTRY(kernel_context_exit)
    // Load the stack pointer
    movl 4(%esp), %eax
    movl %eax, %esp
    // Restore register state
    popl %gs
    popl %fs
    popl %es
    popl %ds
    popl %ss
    popa
    // When kernel context was entered, the interrupt number
    // was pushed to the stack, so adjust the stack pointer
    // before returning
    add $4, %esp
    iret // automatically pop eip, cs, eflag by hardware

