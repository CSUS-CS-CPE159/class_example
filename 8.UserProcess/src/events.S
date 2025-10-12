/**
 * CPE/CSC 159 - Operating System Pragmatics
 * California State University, Sacramento
 *
 * Context Switch Functions
 */
#include <spede/machine/asmacros.h>

// define kernel stack space
.text

ENTRY(TimerEntry)  // push user_ss, user_esp, eflag, cs, eip (by hardware)
    // Indicate which interrupt occured
    pushl $0x0
    pushl $0x20
    // Enter into the kernel context for processing
    jmp kernel_enter

//General protection
ENTRY(GeneralProtectionFaultEntry)  // push eflag, cs, eip (by hardware)
    // Push error code (by hardware)
    // Save register state, interrupt number 0xC = 13
    pushl $0x0C         
    // Enter into the kernel context for processing
    jmp kernel_enter

// Page Fault
ENTRY(PageFaultEntry)  // push eflag, cs, eip (by hardware)
    // Push error code (by hardware)
    // Save register state, interrupt number 0xD = 14
    pushl $0x0D
    // Enter into the kernel context for processing
    jmp kernel_enter

// System Call 
ENTRY(SyscallEntry)  // push eflag, cs, eip (by hardware)
    //push error code (by hardware)
    // Save register state
    pushl $0x0
    pushl $0x80 
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
    //pushl %ss
    pushl %ds
    pushl %es
    pushl %fs
    pushl %gs
    // Load the kernel stack
    mov %esp, %ebx
    movw $(0x10), %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    // save esp address to process frame
    pushl %ebx
    cld
    // Trigger entry into the kernel
    call CNAME(context_switch)

/**
 * Exit the kernel context
 *   - Load the process stack
 *   - Restore register state
 *   - Return from the previous interrupt
 */
 
ENTRY(ProcLoader)
    // Load the stack pointer
    movl 4(%esp), %ebx
    movl %ebx, %esp
    // Restore register state
    popl %gs
    popl %fs
    popl %es
    popl %ds

    popa
    // When kernel context was entered, the interrupt number
    // was pushed to the stack, so adjust the stack pointer
    // before returning
    addl $8, %esp
    iret //pop eip, cs, eflags, user_esp, user_ss

// LoadKernelTSS
ENTRY(KernelTssFlush)
    movw $0x48, %ax  /*loads entry 9 from GDT (Kernel TSS Descriptor, 9*8 + 3 = 0x4B*/
    ltr %ax
    ret


