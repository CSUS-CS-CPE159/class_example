# Compile into <i>MyOS.dli</i>
```
make
```
Start the target window, and you can see the file spede.sock in current folder:
```
spede-target
```
Run it under GDB.
```
spede-run -d
```

type any key to test the code.

## tracing stack register (esp) content
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
