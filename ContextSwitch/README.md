
### Context Switch
* This document helps student understand the context switch.
* CS register can't manual update. You need to use interrupt or other way. 
* reference:
    - https://www.felixcloutier.com/x86/pusha:pushad
### thinking and question?
* In this case, after context switch, each process will start to run the code from scratch, do you know the reason?

### How to check kstack address
gdb: p (int)&kstack + 0x4000

### Memory layout
```
 PHYS_BASE +----------------------------------+
           |          unused space            |
           |                |                 |
           |                |                 |
           |                V                 |
           |                                  |
           |                ^                 |
           |                |                 |
           |                |                 |
           +----------------------------------+--------------
0x1111b4   |------------kstack + 0x4000-------|         ^
           |                |                 |         |
           |                |                 |         |
           |                V                 |         |
           |        grows  downward           |         |
0x10d1b4   |-------------stack----------------|         |
           |                                  |         |
           |------thread2 stack end-----------|         |
           |                |                 |
           |                V                 |
           |           grows downward         |
           |                |                 |
0x109180   |----------thread 2 stack----------|         |
           |                |                 |
           |----------thread1 stack end-------|         |
           |                |                 |         |
           |                V                 |
           |           grows downward         |
           |                |                 |
0x105180   |----------thread1 stack-----------|
           |                                  |
           |                                  |    data segment (DS)
           | uninitialized data segment (BSS) |
           +----------------------------------+         |
           |     initialized data segment     |         |
           |             whoIsRunning         |         V
           +----------------------------------+-------------
           |           Process2               |         ^ 
           |           Process1               |         |
           |                                  |     code segment (CS)
eip--->    |           main()                 |         |
           |                                  |         V 
0x101000   +----------------------------------+--------------
           |                                  |
           |                                  |
0x011000   |----------------------------------|--------------
esp--->    |           spede stack (main)     |    spede stack
           |                                  |
         0 +----------------------------------+--------------
```

### Case study
* process A is running.
* timer device triger a intetrrupt: TimerEntry
```
ENTRY(TimerEntry)  // push eflag, cs, eip (by hardware)
    // Indicate which interrupt occured
    pushl $0x20
    // Enter into the kernel context for processing
    jmp kernel_enter
```
Memory content:
```
 PHYS_BASE +----------------------------------+
           |          unused space            |
           |                |                 |
           |                |                 |
           |                V                 |
           |                                  |
           |                ^                 |
           |                |                 |
           |                |                 |
           +----------------------------------+--------------
0x1111b4   |------------kstack + 0x4000-------|         ^
           |                |                 |         |
           |                |                 |         |
           |                V                 |         |
           |        grows  downward           |         |
0x10d1b4   |-------------stack----------------|         |
           |                                  |         |
           |------thread2 stack end-----------|         |
           |             eflag                |
           |               cs                 |
esp-->     |               eip                |
           |                |                 |
0x109180   |----------thread 2 stack----------|         |
           |                |                 |
           |----------thread1 stack end-------|         |
           |                |                 |         |
           |                V                 |
0x105180   |----------thread1 stack-----------|
           |                                  |
           |                                  |    data segment (DS)
           +----------------------------------+         |
           |             whoIsRunning         |         V
           +----------------------------------+-------------
           |           Process2               |         ^ 
           |           Process1               |         |
           |                                  |     code segment (CS)
           |           main()                 |         |
           |                                  |         V 
0x101000   +----------------------------------+--------------
           |                                  |
           |                                  |
0x011000   |----------------------------------|--------------
           |           spede stack (main)     |    spede stack
           |                                  |
         0 +----------------------------------+--------------
```
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
Memory content:
```
 PHYS_BASE +----------------------------------+
           |          unused space            |
           |                |                 |
           |                |                 |
           |                V                 |
           |                                  |
           |                ^                 |
           |                |                 |
           |                |                 |
           +----------------------------------+--------------
0x1111b4   |------------kstack + 0x4000-------|         ^
           |                |                 |         |
           |                |                 |         |
           |                V                 |         |
           |        grows  downward           |         |
0x10d1b4   |-------------stack----------------|         |
           |                                  |         |
           |------thread2 stack end-----------|         |
           |             eflag                |
           |               cs                 |
           |               eip                |
           |               eax                |
           |               ecx                |
           |               edx                |
           |               ebx                |
           |               esp                |
           |               ebp                |
           |               esi                |
           |               edi                |
           |               ss                 |
           |               ds                 |
           |               es                 |
           |               fs                 |
esp----->  |               gs                 |
           |                |                 |
0x109180   |----------thread 2 stack----------|         |
           |                |                 |
           |----------thread1 stack end-------|         |
           |                |                 |         |
           |                V                 |
0x105180   |----------thread1 stack-----------|
           |                                  |
           |                                  |    data segment (DS)
           +----------------------------------+         |
           |             whoIsRunning         |         V
           +----------------------------------+-------------
           |           Process2               |         ^ 
           |           Process1               |         |
           |                                  |     code segment (CS)
           |           main()                 |         |
           |                                  |         V 
0x101000   +----------------------------------+--------------
           |                                  |
           |                                  |
0x011000   |----------------------------------|--------------
           |           spede stack (main)     |    spede stack
           |                                  |
         0 +----------------------------------+--------------
```
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
Memory content:
```
 PHYS_BASE +----------------------------------+
           |          unused space            |
           +----------------------------------+--------------
0x1111b4   |------------kstack + 0x4000-------|         ^
esp -->    |                |                 |         |
           |                |                 |         |
           |                V                 |         |
           |        grows  downward           |         |
0x10d1b4   |-------------stack----------------|         |
           |                                  |         |
           |------thread2 stack end-----------|         |
           |             eflag                |
           |               cs                 |
           |               eip                |
           |               eax                |
           |               ecx                |
           |               edx                |
           |               ebx                |
           |               esp                |
           |               ebp                |
           |               esi                |
           |               edi                |
           |               ss                 |
           |               ds                 |
           |               es                 |
           |               fs                 |
           |               gs                 |
           |                |                 |
0x109180   |----------thread 2 stack----------|         |
           |                |                 |
           |----------thread1 stack end-------|         |
           |                |                 |         |
           |                V                 |
0x105180   |----------thread1 stack-----------|
           |                                  |
           |                                  |    data segment (DS)
           +----------------------------------+         |
           |             whoIsRunning         |         V
           +----------------------------------+-------------
           |           Process2               |         ^ 
           |           Process1               |         |
           |                                  |     code segment (CS)
           |           main()                 |         |
           |                                  |         V 
0x101000   +----------------------------------+--------------
           |                                  |
           |                                  |
0x011000   |----------------------------------|--------------
           |           spede stack (main)     |    spede stack
           |                                  |
         0 +----------------------------------+--------------
```
```
    // save esp address to process frame
    pushl %edx
```
Memory content:
```
 PHYS_BASE +----------------------------------+
           |          unused space            |
           +----------------------------------+--------------
0x1111b4   |------------kstack + 0x4000-------|         ^
esp -->    |          old esp (thread2 esp)   |         |
           |                |                 |         |
           |                V                 |         |
           |        grows  downward           |         |
0x10d1b4   |-------------stack----------------|         |
           |                                  |         |
           |------thread2 stack end-----------|         |
           |             eflag                |
           |               cs                 |
           |               eip                |
           |               eax                |
           |               ecx                |
           |               edx                |
           |               ebx                |
           |               esp                |
           |               ebp                |
           |               esi                |
           |               edi                |
           |               ss                 |
           |               ds                 |
           |               es                 |
           |               fs                 |
           |               gs                 |
           |                |                 |
0x109180   |----------thread 2 stack----------|         |
           |                |                 |
           |----------thread1 stack end-------|         |
           |                |                 |         |
           |                V                 |
0x105180   |----------thread1 stack-----------|
           |                                  |
           |                                  |    data segment (DS)
           +----------------------------------+         |
           |             whoIsRunning         |         V
           +----------------------------------+-------------
           |           Process2               |         ^ 
           |           Process1               |         |
           |                                  |     code segment (CS)
           |           main()                 |         |
           |                                  |         V 
0x101000   +----------------------------------+--------------
           |                                  |
0x011000   |----------------------------------|--------------
           |           spede stack (main)     |    spede stack
         0 +----------------------------------+--------------
```

* Call Context Switch (user kernel stack to complete task)
```
    // Trigger entry into the kernel
    call CNAME(context_switch) //push return_addr
    // when you use debug mode to check stack content, please use stepi command. 
```

Memory content:
```
 PHYS_BASE +----------------------------------+
           |          unused space            |
           +----------------------------------+--------------
0x1111b4   |------------kstack + 0x4000-------|         ^
           |          old esp (thread2 esp)   |         |
esp -->    |          return addr             |         |
           |                |                 |         |
           |                V                 |         |
           |        grows  downward           |         |
0x10d1b4   |-------------stack----------------|         |
           |                                  |         |
           |------thread2 stack end-----------|         |
           |             eflag                |
           |               cs                 |
           |               eip                |
           |               eax                |
           |               ecx                |
           |               edx                |
           |               ebx                |
           |               esp                |
           |               ebp                |
           |               esi                |
           |               edi                |
           |               ss                 |
           |               ds                 |
           |               es                 |
           |               fs                 |
           |               gs                 |
           |                |                 |
0x109180   |----------thread 2 stack----------|         |
           |                |                 |
           |----------thread1 stack end-------|         |
           |                |                 |         |
           |                V                 |
0x105180   |----------thread1 stack-----------|
           |                                  |
           |                                  |    data segment (DS)
           +----------------------------------+         |
           |             whoIsRunning         |         V
           +----------------------------------+-------------
           |           Process2               |         ^ 
           |           Process1               |         |
           |                                  |     code segment (CS)
           |           main()                 |         |
           |                                  |         V 
0x101000   +----------------------------------+--------------
           |                                  |
0x011000   |----------------------------------|--------------
           |           spede stack (main)     |    spede stack
         0 +----------------------------------+--------------
`
additional information: https://docs.google.com/document/d/1Of6DHIXRtkVtfyDj82JntVhWhxY4hZi8Y632JS780DM/edit?usp=sharing
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
Memory content:
```
 PHYS_BASE +----------------------------------+
           |          unused space            |
           +----------------------------------+--------------
0x1111b4   |------------kstack + 0x4000-------|         ^
           |        old esp (thread2's esp)   |         |
           |          return addr             |         |
           |                ...               |         |
           |          thread2's esp           |         |
esp -->    |          return addr             |         |
           |                                  |         |
           |                                  |         |
0x10d1b4   |-------------stack----------------|         |
           |                                  |         |
           |------thread2 stack end-----------|         |
           |             eflag                |
           |               cs                 |
           |               eip                |
           |               eax                |
           |               ecx                |
           |               edx                |
           |               ebx                |
           |               esp                |
           |               ebp                |
           |               esi                |
           |               edi                |
           |               ss                 |
           |               ds                 |
           |               es                 |
           |               fs                 |
           |               gs                 |
           |                |                 |
0x109180   |----------thread 2 stack----------|         |
           |                |                 |
           |----------thread1 stack end-------|         |
           |                |                 |         |
           |                V                 |
0x105180   |----------thread1 stack-----------|
           |                                  |
           |                                  |    data segment (DS)
           +----------------------------------+         |
           |             whoIsRunning         |         V
           +----------------------------------+-------------
           |           Process2               |         ^ 
           |           Process1               |         |
           |                                  |     code segment (CS)
           |           main()                 |         |
           |                                  |         V 
0x101000   +----------------------------------+--------------
           |                                  |
0x011000   |----------------------------------|--------------
           |           spede stack (main)     |    spede stack
         0 +----------------------------------+--------------
```
* Load the stack pointer
```
    movl 4(%esp), %eax
    movl %eax, %esp
```

Notes: When we create a new process/thread, we will initialize the stack content for context switch, even we don't run this process/thread yet.  

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
Memory Content:
```
 PHYS_BASE +----------------------------------+
           |          unused space            |
           +----------------------------------+--------------
0x1111b4   |------------kstack + 0x4000-------|         ^
           |        old esp (thread2's esp)   |         |
           |          return addr             |         |
           |                ...               |         |
           |          thread2's esp           |         |
           |          return addr             |         |
           |                                  |         |
           |                                  |         |
0x10d1b4   |-------------stack----------------|         |
           |                                  |         |
           |------thread2 stack end-----------|         |
           |             eflag                |
           |               cs                 |
           |               eip                |
           |               eax                |
           |               ecx                |
           |               edx                |
           |               ebx                |
           |               esp                |
           |               ebp                |
           |               esi                |
           |               edi                |
           |               ss                 |
           |               ds                 |
           |               es                 |
           |               fs                 |
           |               gs                 |
           |                |                 |
0x109180   |----------thread 2 stack----------|         |
           |                |                 |
           |----------thread1 stack end-------|         |
           |             eflag                |
           |               cs                 |
           |               eip                |
           |               eax                |
           |               ecx                |
           |               edx                |
           |               ebx                |
           |               esp                |
           |               ebp                |
           |               esi                |
           |               edi                |
           |               ss                 |
           |               ds                 |
           |               es                 |
           |               fs                 |
esp --->   |               gs                 |
           |                |                 |         |
0x105180   |----------thread1 stack-----------|         |
           |                                  |    data segment (DS)
           +----------------------------------+         |
           |             whoIsRunning         |         V
           +----------------------------------+-------------
           |           Process2(thread2)      |         ^ 
           |           Process1(thread1)      |         |
           |                                  |     code segment (CS)
           |           main()                 |         |
           |                                  |         V 
0x101000   +----------------------------------+--------------
           |                                  |
0x011000   |----------------------------------|--------------
           |           spede stack (main)     |    spede stack
         0 +----------------------------------+--------------
```
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
Memory Content: 
```
 PHYS_BASE +----------------------------------+
           |          unused space            |
           +----------------------------------+--------------
0x1111b4   |------------kstack + 0x4000-------|         ^
           |        old esp (thread2's esp)   |         |
           |          return addr             |         |
           |                ...               |         |
           |          thread2's esp           |         |
           |          return addr             |         |
           |                                  |         |
           |                                  |         |
0x10d1b4   |-------------stack----------------|         |
           |                                  |         |
           |------thread2 stack end-----------|         |
           |             eflag                |
           |               cs                 |
           |               eip                |
           |               eax                |
           |               ecx                |
           |               edx                |
           |               ebx                |
           |               esp                |
           |               ebp                |
           |               esi                |
           |               edi                |
           |               ss                 |
           |               ds                 |
           |               es                 |
           |               fs                 |
           |               gs                 |
           |                |                 |
0x109180   |----------thread 2 stack----------|         |
           |                |                 |
           |----------thread1 stack end-------|         |
esp ---->  |                                  |
           |                                  |
           |                                  |         |
0x105180   |----------thread1 stack-----------|         |
           |                                  |    data segment (DS)
           +----------------------------------+         |
           |             whoIsRunning         |         V
           +----------------------------------+-------------
           |           Process2(thread2)      |         ^ 
           |           Process1(thread1)      |         |
           |                                  |     code segment (CS)
           |           main()                 |         |
           |                                  |         V 
0x101000   +----------------------------------+--------------
           |                                  |
0x011000   |----------------------------------|--------------
           |           spede stack (main)     |    spede stack
         0 +----------------------------------+--------------
```
When next context switch happens, the kernel stack content will be overwrited. 

