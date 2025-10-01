
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

Check the following values:
```
SPEDE GDB$ b kernel_context_exit
Breakpoint 2 at 0x1013ec: file src/context.S, line 55.
SPEDE GDB$ c
Continuing.

Breakpoint 2, kernel_context_exit () at src/context.S:55
55	    movl 4(%esp), %eax
SPEDE GDB$ x $esp
0x11118c <kstack+16344>:	0x0010136c
SPEDE GDB$ x/10i 0x10136c
   0x10136c <context_switch+54>:	add    $0x10,%esp
   0x10136f <context_switch+57>:	jmp    0x1013c3 <context_switch+141>
   0x101371 <context_switch+59>:	mov    0x105160,%eax
   0x101376 <context_switch+64>:	cmp    $0x1,%eax
   0x101379 <context_switch+67>:	jne    0x1013a0 <context_switch+106>
   0x10137b <context_switch+69>:	movl   $0x2,0x105160
   0x101385 <context_switch+79>:	mov    0x8(%ebp),%eax
   0x101388 <context_switch+82>:	mov    %eax,0x10d180
   0x10138d <context_switch+87>:	mov    0x10d184,%eax
   0x101392 <context_switch+92>:	sub    $0xc,%esp
SPEDE GDB$ x/x $esp + 4
0x111190 <kstack+16348>:	0x0010913c
SPEDE GDB$ print p1_stack
$1 = (trapframe_t *) 0x10913c <proc_stack+16316>
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
