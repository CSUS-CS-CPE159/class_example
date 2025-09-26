<h3>Class practice - ide block </h3>

The code is adapted from xv6. 
The tool code is from xv6. 

<p>
add the following content to /opt/spede/bin/spede-target

```
-drive file=fs.img,index=1,media=disk,format=raw\
 
```
<\p>

<p>
The objective of this assigment is to practice how to
program to use the timer interrupt events.
</p>

Write your own code following class lectures and this pseudo code:
```
test
```
Compile into <i>MyOS.dli</i>
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
###

```
2:01:12 $ spede
The SPEDE Target has quit. Exiting...
Launching SPEDE target to run in the background
The SPEDE Target will be reset, are you sure? (y/n) y
Resetting the SPEDE Target...
Downloading image 'build/MyOS.dli' to SPEDE Target via /dev/pts/1...
File type is 'ELF'
Total blocks to download:  0xc8  (128 bytes each)

Load Successful ; Code loaded at 0x0x101000 (12832 bytes/sec)
Executing the image 'build/MyOS.dli' on the SPEDE Target with GDB Enabled
Launching GDB...
Reading symbols from build/MyOS.dli...
Expanding full symbols from build/MyOS.dli...
Remote debugging using /dev/pts/1
0x0010417d in breakpoint ()
Temporary breakpoint 1 at 0x1019de: file src/main.c, line 107.

Temporary breakpoint 1, main () at src/main.c:107
107	    memset(buf, 0, sizeof(buf));
SPEDE GDB$ n
119	    IDT_p = get_idt_base(); // get IDT location
SPEDE GDB$ n
121	    fill_gate(&IDT_p[32+14], (int)IdeEntry, get_cs(), ACC_INTR_GATE, 0);
SPEDE GDB$ n
122	    picinit();
SPEDE GDB$ n
123	    binit();
SPEDE GDB$ n
124	    ideinit();
SPEDE GDB$ n
127	    outb(0x20, 0x20); // Master PIC
SPEDE GDB$ n
130	    asm("sti");
SPEDE GDB$ n
133	    struct buf * b = bread(1, 1);
SPEDE GDB$ n
134	    printf("%d\n", b->dev);
SPEDE GDB$ n
135	    b = bread(1, 2);
SPEDE GDB$ n
137	    b = bread(1, 29);
SPEDE GDB$ print *b
$6 = {
  flags = 0x3,
  dev = 0x1,
  blockno = 0x2,
  prev = 0x1081d0 <bcache+1072>,
  next = 0x107da0 <bcache>,
  qnext = 0x0,
  data = '\000' <repeats 64 times>, "\001\000\000\000\000\000\001\000c\000\001\000\000\a\000\000\000\002\000\000\035", '\000' <repeats 43 times>, "\002\000\000\000\000\000\001\000c\000\001\000\000\a\000\000>\006\000\000\036\000\000\000\037\000\000\000 \000\000\000!", '\000' <repeats 31 times>, "\002\000\000\000\000\000\001\000"...
}
SPEDE GDB$ print &b->data
$7 = (uchar (*)[512]) 0x107fd0 <bcache+560>
SPEDE GDB$ printf "root directory\n"
root directory
SPEDE GDB$ print *(struct dinode*)(0x107fd0 + 0x40)
$12 = {
  type = 0x1,
  major = 0x0,
  minor = 0x0,
  nlink = 0x1,
  ownerid = 0x63,
  groupid = 0x1,
  mode = 0x700,
  size = 0x200,
  addrs = {0x1d, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}
}
SPEDE GDB$ printf "root directory\n"
root directory
SPEDE GDB$ print *(struct dinode*)(0x107fd0 + 0x40*2)
$14 = {
  type = 0x2,
  major = 0x0,
  minor = 0x0,
  nlink = 0x1,
  ownerid = 0x63,
  groupid = 0x1,
  mode = 0x700,
  size = 0x63e,
  addrs = {0x1e, 0x1f, 0x20, 0x21, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}
}
SPEDE GDB$ print *(struct dinode*)(0x107fd0 + 0x40*3)
$16 = {
  type = 0x2,
  major = 0x0,
  minor = 0x0,
  nlink = 0x1,
  ownerid = 0x63,
  groupid = 0x1,
  mode = 0x700,
  size = 0x5050,
  addrs = {0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c}
}
SPEDE GDB$ printf "cat command, size is 0x5050, the data block is 0x5050/512 = 41\n"

SPEDE GDB$ n
139	    b = bread(1, 0x2c);
SPEDE GDB$ n
142	    readsb(1, &sb);
SPEDE GDB$ printf "check indirect data block, the last block 0x2c is used for indirect block\n"
check indirect data block, the last block 0x2c is used for indirect block

SPEDE GDB$ print &b->data
$17 = (uchar (*)[512]) 0x108400 <bcache+1632>
SPEDE GDB$ print *(struct dinode*)(0x107fd0 + 0x40*3)
$18 = {
  type = 0x2,
  major = 0x0,
  minor = 0x0,
  nlink = 0x1,
  ownerid = 0x63,
  groupid = 0x1,
  mode = 0x700,
  size = 0x5050,
  addrs = {0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c}
}
SPEDE GDB$ x/64 0x108400
0x108400 <bcache+1632>:	0x2d	0x2e	0x2f	0x30
0x108410 <bcache+1648>:	0x31	0x32	0x33	0x34
0x108420 <bcache+1664>:	0x35	0x36	0x37	0x38
0x108430 <bcache+1680>:	0x39	0x3a	0x3b	0x3c
0x108440 <bcache+1696>:	0x3d	0x3e	0x3f	0x40
0x108450 <bcache+1712>:	0x41	0x42	0x43	0x44
0x108460 <bcache+1728>:	0x45	0x46	0x47	0x48
0x108470 <bcache+1744>:	0x49	0x4a	0x4b	0x0
0x108480 <bcache+1760>:	0x0	0x0	0x0	0x0
0x108490 <bcache+1776>:	0x0	0x0	0x0	0x0
0x1084a0 <bcache+1792>:	0x0	0x0	0x0	0x0
0x1084b0 <bcache+1808>:	0x0	0x0	0x0	0x0
0x1084c0 <bcache+1824>:	0x0	0x0	0x0	0x0
0x1084d0 <bcache+1840>:	0x0	0x0	0x0	0x0
0x1084e0 <bcache+1856>:	0x0	0x0	0x0	0x0
0x1084f0 <bcache+1872>:	0x0	0x0	0x0	0x0
SPEDE GDB$ q
```
