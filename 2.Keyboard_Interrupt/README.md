# Keyboard Input - Interrupt-Driven Method

## Overview
This example demonstrates interrupt-driven keyboard input. Rather than polling the keyboard continuously (wasting CPU time), the keyboard hardware signals an interrupt when a key is pressed. The CPU then runs an interrupt handler to process the key.

## Learning Objectives
- Understand hardware interrupts for I/O devices
- Learn about interrupt masks and PIC (Programmable Interrupt Controller)
- See how to write interrupt handlers for hardware devices
- Compare interrupt-driven vs. polling-based input
- Understand interrupt priorities and masking
- Practice mixing assembly and C code for interrupt handling

## Build and Run

Compile:
```bash
make
```

Start the target simulator (separate terminal):
```bash
spede-target
```

Run with debugging:
```bash
spede-run -d
```

Type any key to test the code.

## How It Works

### Hardware Interrupt Flow
When you press a key:
1. Keyboard controller generates an interrupt signal
2. Interrupt signal sent to PIC (Programmable Interrupt Controller)
3. PIC signals CPU (if interrupts enabled)
4. CPU finishes current instruction
5. CPU jumps to interrupt handler (from IDT)
6. Handler processes the keystroke
7. CPU continues previous work

### Interrupt Descriptor Table (IDT)
Like exceptions, keyboard interrupts use the IDT:
- **Keyboard interrupt**: IRQ 1 (Interrupt 33 in IDT, since IRQ 1 maps to vector 32+1)
- **IDT[KEYBOARD_EVENT]**: Points to our keyboard handler

### Programmable Interrupt Controller (PIC)
The 8259 PIC is a chip that:
- Accepts interrupt signals from multiple devices
- Prioritizes them
- Signals the CPU

**Key registers** (addressed as I/O ports):
- **Port 0x21**: PIC data port (mask register)
  - Bit 0: Timer interrupt
  - Bit 1: Keyboard interrupt
  - Bit i: Device i (1 = masked/disabled, 0 = enabled)

### Interrupt Masking
To enable the keyboard interrupt:
```c
outportb(0x21, ~0x02);  // Mask = 11111101 (enable bit 1 for keyboard)
```

The `~0x02` pattern:
- 0x02 = 00000010 (keyboard IRQ 1)
- ~0x02 = 11111101 (all bits set except bit 1)
- Writing to PIC: 1 = masked (disabled), 0 = unmasked (enabled)

### Enabling CPU Interrupts
After registering handlers and configuring PIC:
```asm
sti  ; Set Interrupt Flag (enable CPU interrupts)
```

This allows the CPU to respond to interrupts.

## Stack Frame During Interrupt

Here's a trace of the stack register (esp) content during interrupt handling:

## Tracing stack register (esp) content
```
spede@spedevm:~/Desktop/class_example/2.Keyboard_Interrupt$ spede-run -d
The SPEDE Target will be reset, are you sure? (y/n) y
Resetting the SPEDE Target...
Downloading image 'build/MyOS.dli' to SPEDE Target via /dev/pts/1...
File type is 'ELF'
Total blocks to download:  0x83  (128 bytes each)

Load Successful ; Code loaded at 0x0x101000 (8416 bytes/sec)
Executing the image 'build/MyOS.dli' on the SPEDE Target with GDB Enabled
Launching GDB...
Reading symbols from build/MyOS.dli...
Expanding full symbols from build/MyOS.dli...
Remote debugging using /dev/pts/1
0x00102695 in breakpoint ()
Temporary breakpoint 1 at 0x101299: file src/main.c, line 18.

Temporary breakpoint 1, main () at src/main.c:18
18	    struct i386_gate *IDT_p = get_idt_base();
SPEDE GDB$ b KeyboardEntry
Breakpoint 2 at 0x1012e0: file src/events.S, line 9.
SPEDE GDB$ info b
Num     Type           Disp Enb Address    What
2       breakpoint     keep y   0x001012e0 src/events.S:9
SPEDE GDB$ c
Continuing.

Breakpoint 2, KeyboardEntry () at src/events.S:9
9	  pusha  // push all: eax, ebx, edx, ecx
SPEDE GDB$ print $esp
$1 = (void *) 0x10e64
SPEDE GDB$ n
KeyboardEntry () at src/events.S:11
11	  cld  // clear direction flag
SPEDE GDB$ n
12	  call CNAME(keyboard_interrupt_handler)  // call C function keyboard_interrupt_handler()
SPEDE GDB$ si
keyboard_interrupt_handler () at src/keyboard.c:16
16	void keyboard_interrupt_handler(void) {
SPEDE GDB$ print $esp
$2 = (void *) 0x10e40
SPEDE GDB$ n
17	    int ext = 0;
SPEDE GDB$ n
19	    unsigned char b = inportb(KBD_PORT_DATA);
SPEDE GDB$ n
20		if (b == 0xE0) {
SPEDE GDB$ n
25		unsigned char c = b & 0x7F;
SPEDE GDB$ n
28		unsigned char token = KEY_NULL;
SPEDE GDB$ n
29		if (ext) {
SPEDE GDB$ n
32		    token = (c < sizeof(keyboard_map_primary))
SPEDE GDB$ n
33		        	? keyboard_map_primary[c] : KEY_NULL;
SPEDE GDB$ n
35		printf("keyboard scancode: %d, 0x%x, ascii code: %c\n", c, c, token);
SPEDE GDB$ n
keyboard scancode: 28, 0x1c, ascii code:

37		*(unsigned short*)0xB8000 = 0x0700 + (unsigned int)c;
SPEDE GDB$ n
38		*(unsigned short*)0xB8002 = 0x0700 + (unsigned int)token;
SPEDE GDB$ n
43	    outportb(PIC_CONTROL, PIC_EOI);
SPEDE GDB$ n
44	}
SPEDE GDB$ n
KeyboardEntry () at src/events.S:13
13	  popa  // pop all: eax, ebx, edx, ecx
SPEDE GDB$ print $esp
$3 = (void *) 0x10e44
SPEDE GDB$ n
KeyboardEntry () at src/events.S:15
15	  iret  // pop eip, cs, eflags
SPEDE GDB$ print $esp
$4 = (void *) 0x10e64
SPEDE GDB$ n
main () at src/main.c:33
33	    while (1);
SPEDE GDB$ print $esp
$5 = (void *) 0x10e70
```
