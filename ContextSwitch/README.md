
### Context Switch
* This document helps people understand the context switch.
* CS register can't manual update. You need to use interrupt or other way. 
* Carefully understand the context switch.
* reference:
    - https://www.felixcloutier.com/x86/pusha:pushad

### How to check kstack address
gdb: p (int)&kstack + 0x4000

### Case study
* process A is running.

Memory content:  |process A                                                  <span style="color: red;">:Stack</span>|                          kernel stack|
* timer device triger a intetrrupt: TimerEntry
```
ENTRY(TimerEntry)  // push eflag, cs, eip (by hardware)
    // Indicate which interrupt occured
    pushl $0x20
    // Enter into the kernel context for processing
    jmp kernel_enter
```
Memory:  |process A                                               <span style="color: red;">:eip:cs:eflag:Stack</span>|                          kernel stack|
* save all register information into user stack 
```
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
```
Memory:  |process A                 <span style="color: red;">gs:fs:es:ds:ss:edx:ecx:ebx:eax:eip:cs:eflag:Stack</span>|                          kernel stack|
* Switch to kernel stack (update ds, es register)
```
    // Save user stack address
    movl %esp, %edx
    // Load the kernel stack
    movw $(0x10), %ax
    mov %ax, %ds
    mov %ax, %es
    leal kstack + 16384, %esp
```
Memory:  |process A                 gs:fs:es:ds:ss:edx:ecx:ebx:eax:eip:cs:eflag:Stack|                 <span style="color: red;">user esp:kernel stack</span>|
```
    // save esp address to process frame
    pushl %edx
```
* Call Context Switch (user kernel stack to complete task)
```
    // Trigger entry into the kernel
    call CNAME(context_switch)
```
* return from Context Switch
```
/**
 * Exit the kernel context
 *   - Load the process stack
 *   - Restore register state
 *   - Return from the previous interrupt
 */
ENTRY(kernel_context_exit)
```
Memory:  |process A                 gs:fs:es:ds:ss:edx:ecx:ebx:eax:eip:cs:eflag:Stack|                 <span style="color: red;">user esp:kernel stack</span>|
* Load the stack pointer
```
    movl 4(%esp), %eax
    movl %eax, %esp
```
Memory:  |process A                 <span style="color: red;">gs:fs:es:ds:ss:edx:ecx:ebx:eax:eip:cs:eflag:Stack</span>|                 :kernel stack|
* restore the register status
```
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
```
Memory:  |process A                 <span style="color: red;">:Stack</span>|                 :kernel stack|
