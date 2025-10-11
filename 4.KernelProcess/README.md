### This code is used for practicing static process relocation.

We create two kernel tasks and relocate one of them to a new memory address (segAddr). To complete the relocation, we update the task’s EIP to point to this new address. This example demonstrates that, during static process relocation, the entire process must remain contiguous in memory. Otherwise, certain instructions (such as jumps or calls) may reference invalid locations, since compiled code often computes absolute addresses based on the current EIP and an offset.

Unlike the previous example (context switching), this version also incorporates a PCB (Process Control Block) structure to represent each task.

### Suggested Reading
- Computer Systems: A Programmer's Perspective: Chapter 7 Linker 
- Computer Systems: A Programmer's Perspective: Chapter 7.7 Relocation (quick reference)

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
0x11c000   |------------kstack + 0x4000-------|         ^
           |                |                 |         |
           |             size 16kb            |         |
           |                |                 |         |
0x118000   |-------------kstack---------------|         |
           |------SysProc1 stack end----------|         |
           |                |                 |
           |             size 4kb             |          
           |                |                 |
0x10e000   |----------SysProc1 stack----------|          
           |----------SysProc stack end-------|          
           |                |                 |
           |             size 4kb             |          
           |                |                 |
0x10d000   |----------SysProc stack-----------|
           |                                  |
           |                                  |
           |                                  |
           |                                  |
           |                                  |
           |                                  |
           |----------------------------------|
           |      new location for SysProc1   |
0x107000   |----------------------------------|
           |                                  |
           |                                  |
           |                                  |
           |                                  |    data segment (DS)
           |           active_process         |         V
           +----------------------------------+-------------
           |              SysProc1            |         ^ 
           |              SysProc             |         |
           |                                  |     code segment (CS)
           |              main()              |         |
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
* We have two process: SysProc and SysProc1. We relocate SysProc1 to a new memory location.

### Question
* What happens if we use printf function in SysProc1 function?

### Check Assembly code if we use printf function.

* use printf function in SysProc1 function
```
```
Running result:
```
SPEDE GDB$ c
Continuing.
eip is 107000
This is the system thread

Program received signal SIGSEGV, Segmentation fault.
0x0010afff in code_addr ()
```
check memory content
```
SPEDE GDB$ x/10i &code_addr[1]
   0x107000 <code_addr+4096>:	push   %ebp
   0x107001 <code_addr+4097>:	mov    %esp,%ebp
   0x107003 <code_addr+4099>:	sub    $0x18,%esp
   0x107006 <code_addr+4102>:	movl   $0x200000,-0x10(%ebp)
   0x10700d <code_addr+4109>:	mov    -0x10(%ebp),%eax
   0x107010 <code_addr+4112>:	movl   $0x1,(%eax)
   0x107016 <code_addr+4118>:	sub    $0xc,%esp
   0x107019 <code_addr+4121>:	push   $0x104801
   0x10701e <code_addr+4126>:	call   0x109683 <code_addr+13955>
   0x107023 <code_addr+4131>:	add    $0x10,%esp
SPEDE GDB$ x/10i SystemProc1
   0x1014dd <SystemProc1>:	push   %ebp
   0x1014de <SystemProc1+1>:	mov    %esp,%ebp
   0x1014e0 <SystemProc1+3>:	sub    $0x18,%esp
   0x1014e3 <SystemProc1+6>:	movl   $0x200000,-0x10(%ebp)
   0x1014ea <SystemProc1+13>:	mov    -0x10(%ebp),%eax
   0x1014ed <SystemProc1+16>:	movl   $0x1,(%eax)
   0x1014f3 <SystemProc1+22>:	sub    $0xc,%esp
   0x1014f6 <SystemProc1+25>:	push   $0x104801
   0x1014fb <SystemProc1+30>:	call   0x103b60 <printf>
   0x101500 <SystemProc1+35>:	add    $0x10,%esp
SPEDE GDB$ x/10x SystemProc1
0x1014dd <SystemProc1>:	0x83e58955	0x45c718ec	0x200000f0	0xf0458b00
0x1014ed <SystemProc1+16>:	0x000100c7	0xec830000	0x4801680c	0x60e80010
0x1014fd <SystemProc1+32>:	0x83000026	0x45c710c4
SPEDE GDB$ x/10x &code_addr[1]
0x107000 <code_addr+4096>:	0x83e58955	0x45c718ec	0x200000f0	0xf0458b00
0x107010 <code_addr+4112>:	0x000100c7	0xec830000	0x4801680c	0x60e80010
0x107020 <code_addr+4128>:	0x83000026	0x45c710c4
```
Translate the data into machine instruction
```
SPEDE GDB$ disassemble /r SystemProc1,SystemProc1+32
Dump of assembler code from 0x1014dd to 0x10150f:
   0x001014dd <SystemProc1+0>:	55	push   %ebp
   0x001014de <SystemProc1+1>:	89 e5	mov    %esp,%ebp
   0x001014e0 <SystemProc1+3>:	83 ec 18	sub    $0x18,%esp
   0x001014e3 <SystemProc1+6>:	c7 45 f0 00 00 20 00	movl   $0x200000,-0x10(%ebp)
   0x001014ea <SystemProc1+13>:	8b 45 f0	mov    -0x10(%ebp),%eax
   0x001014ed <SystemProc1+16>:	c7 00 01 00 00 00	movl   $0x1,(%eax)
   0x001014f3 <SystemProc1+22>:	83 ec 0c	sub    $0xc,%esp
   0x001014f6 <SystemProc1+25>:	68 01 48 10 00	push   $0x104801
   0x001014fb <SystemProc1+30>:	e8 60 26 00 00	call   0x103b60 <printf>
   0x00101500 <SystemProc1+35>:	83 c4 10	add    $0x10,%esp
   0x00101503 <SystemProc1+38>:	c7 45 f4 00 00 00 00	movl   $0x0,-0xc(%ebp)
SPEDE GDB$ disassemble /r &code_addr[1], 0x107032
Dump of assembler code from 0x107000 to 0x107032:
   0x00107000 <code_addr+4096>:	55	push   %ebp
   0x00107001 <code_addr+4097>:	89 e5	mov    %esp,%ebp
   0x00107003 <code_addr+4099>:	83 ec 18	sub    $0x18,%esp
   0x00107006 <code_addr+4102>:	c7 45 f0 00 00 20 00	movl   $0x200000,-0x10(%ebp)
   0x0010700d <code_addr+4109>:	8b 45 f0	mov    -0x10(%ebp),%eax
   0x00107010 <code_addr+4112>:	c7 00 01 00 00 00	movl   $0x1,(%eax)
   0x00107016 <code_addr+4118>:	83 ec 0c	sub    $0xc,%esp
   0x00107019 <code_addr+4121>:	68 01 48 10 00	push   $0x104801
   0x0010701e <code_addr+4126>:	e8 60 26 00 00	call   0x109683 <code_addr+13955>
   0x00107023 <code_addr+4131>:	83 c4 10	add    $0x10,%esp
   0x00107026 <code_addr+4134>:	c7 45 f4 00 00 00 00	movl   $0x0,-0xc(%ebp)
```

Call instruction explain
```
E8 <rel32>

E8 → opcode for CALL rel32 (near, relative)
<rel32> → 32-bit signed displacement = target – next_instruction

target = (EIP of next instruction) + rel32
0x107023+ 0x26 60 = 0x109683
```

