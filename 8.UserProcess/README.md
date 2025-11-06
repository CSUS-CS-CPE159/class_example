
### User Process
* This document helps people understand user process and how to configure user mode and kernel mode.
* reference:
    - https://wiki.osdev.org/Global_Descriptor_Table
### Single kernel stack vs Kernel Stack for each process
https://docs.google.com/drawings/d/1qZzdBx5CEMplXS7eRqygyUI197gAqi66XVGQlpDGb-U/edit?usp=sharing


### what is Global Descriptor Table (GDT)
The Global Descriptor Table (GDT) defines memory segments for code and data. The GDT is used for memory segmentation and protection, specifying properties like base address, limit, and privilege for memory segments. 

### GDT vs IDT (Interrupt Descriptor Table)


| Feature	        | GDT (Global Descriptor Table)                             | IDT (Interrupt Descriptor Table)                   |
|:---|:---:|---:|
| Primary Function	| Memory segmentation and protection.	                    | Interrupt and exception handling.                  |
| Contents	        | Segment descriptors, TSS descriptors, call gates.	        | Gate descriptors (interrupt, trap, task).          |
| Indexing Method	| Indexed by segment selectors.	                            | Indexed by interrupt/exception vectors (0-255).    |
| CPU Instruction	| LGDT (Load GDT Register).	                                | LIDT (Load IDT Register).                          |
| Purpose           | Manages system structures (TSS, TLS); required for boot.  | Crucial for responding to hardware events and errors. |



### 32 bits GDT table entry
In SPEDE, the entries in the GDT are 8 bytes long and form a table like this:

Global Descriptor Table
----------------------
|Address             |   Entry       |   Content
|:---|:---:|---:|
|GDTR offset + 0     |   NULL        |   NULL
|GDTR offset + 0x8   |   Entry 1     |   kernel Code 
|GDTR offset + 0x10  |   Entry 2     |   kernel Data
|GDTR offset + 0x18  |   Entry 3     |   kernel Stack
|GDTR offset + 0x20  |   Entry 4     |   Unknown
|GDTR offset + 0x28  |   Entry 5     |   Unknown
|GDTR offset + 0x30  |   Entry 6     |   User Code
|GDTR offset + 0x38  |   Entry 7     |   User Data
|GDTR offset + 0x40  |   Entry 8     |   user Stack
|GDTR offset + 0x48  |   Entry 9     |   kernel TSS

How to check GDTR content
```
GDB$ x/64b GDT_p
0xfb1e:	0xff	0xff	0x1e	0xfb	0x0	0x0	    0x0	    0x0
0xfb26:	0xff	0xff	0x0	    0x0	    0x0	0x9a	0xcf	0x0
0xfb2e:	0xff	0xff	0x0	    0x0	    0x0	0x93	0xcf	0x0
0xfb36:	0xff	0xff	0x0	    0x0	    0x0	0x93	0xcf	0x0
0xfb3e:	0xff	0xff	0x0	    0x0	    0x0	0x9a	0x8f	0x0
0xfb46:	0xff	0xff	0x0	    0x0	    0x0	0x92	0x8f	0x0
0xfb4e:	0xff	0xff	0x0	    0x0	    0x0	0xfa	0xcf	0x0
0xfb56:	0xff	0xff	0x0	    0x0	    0x0	0xf2	0xcf	0x0
```
x86 processors use little-endian byte ordering, meaning the least significant byte of an integer is stored at the lowest memory address.

#### Segment Descriptor


|Byte 7|Byte 6| Byte 5|Byte 4|Byte 3 - 2|Byte 1-0|
|:---|:---:|---:|---:|:---:|---:|
|63   -  56|55  -  48|47     -    40|39  -   32|31   -    16|15    -   0|
|Base[31:24]      |Granularity/Flags (bits) + Limit[19:16]   |Limit[15:0]  |Access Byte  |Base[23:16]     |Base[15:0]       |Limit[15:0]     |

Access Byte

|bit 7      |bit 6      bit 5|bit 4 |bit 3  |bit 2  |bit1  | bit 0 |
|:---|:---:|---:|---:|:---:|---:| ---:|
|P      |DPL     |S |E  |DC |RW |A |


We use 0xfb26 as example: 0xfb26 is entry 1 for kernel code.

Breakdown:

|Byte 7|Byte 6| Byte 5|Byte 4|Byte 3 - 2|Byte 1-0|
|:---|:---:|---:|---:|:---:|---:|
|Base[31:24]      |Granularity/Flags (bits) + Limit[19:16]   |Limit[15:0]  |Access Byte  |Base[23:16]     |Base[15:0]       |Limit[15:0]     |
|0x00 | 0xcf | 0x9a | 0x00 | 0x00 00 | 0xFFFF |

- Limit[15:0] = 0xffff

- Base[15:0] = 0x0000

- Base[23:16] = 0x00

- Access = 0x9a = 1001 1010b → P=1, DPL=00 (ring 0), S=1 (code/data), E=1 (code), C=0 (non-conforming), R=1 (readable), A=0

|bit 7      |bit 6      bit 5|bit 4 |bit 3  |bit 2  |bit1  | bit 0 |
|:---|:---:|---:|---:|:---:|---:| ---:|
|P      |DPL     |S |E  |DC |RW |A |
| 1 |  0 0 | 1 | 1 | 0| 1| 0 |  

- Gran/Flags = 0xcf = 1100 1111b → G=1 (4 KiB), D=1 (32-bit), L=0, AVL=0, Limit[19:16]=0xF

- Base[31:24] = 0x00

Computed:

- Base = 0x00000000

- Limit = {Limit[19:16]=0xF, Limit[15:0]=0xFFFF} with G=1 ⇒ effective limit = (0xFFFFF << 12) | 0xFFF = 4 GiB − 1

- Meaning: Ring-0, 32-bit, readable code segment, base 0, flat 4 GiB. 

#### we check oxfb4e content, which is user code section
Entry at 0xfb4e → ff ff 00 00 00 fa cf 00 (User code)

- Limit[15:0] = 0xffff

- Base[15:0] = 0x0000

- Base[23:16] = 0x00

- Access = 0xFA = 1111 1010b, P=1 (present), DPL=11b → 3 (user ring), S=1 (code/data descriptor, not system), E=1 (executable → code segment), C=0 (non-conforming), R=1 (readable code), A=0 (accessed bit; CPU may set)

- Flags = 0xCF = 1100 1111b, G=1 (4 KiB granularity), D=1 (default 32-bit), L=0 (not 64-bit), AVL=0, Limit[19:16]=0xF

- Base[31:24] = 0x00

Computed:

- Base = 0x00000000

- Limit (with G=1) = {0xF:0xFFFF} → 0x000F FFFF pages ⇒ effective byte limit = (0x000F_FFFF << 12) | 0xFFF = 0xFFFF FFFF (4 GiB−1)

- Meaning: Ring-3, 32-bit, readable code segment, flat 0..4 GiB.

### Summary

| Index | Purpose       | Bytes (low→high)          | Notes                      |
|:---|:---:|---:|---:|
|     0 | Null          | `00 00 00 00 00 00 00 00` | Must be all zeros          |
|     1 | Kernel code   | `ff ff 00 00 00 9a cf 00` | Ring-0 code, base 0, 4 GiB |
|     2 | Kernel data   | `ff ff 00 00 00 92 cf 00` | Ring-0 data, base 0, 4 GiB |
|     3 | Kernel stack* | (same as data)            | Typically identical to #2  |
|     ... |  ... | ...           | ...  |
|     6 | Data   code   | `ff ff 00 00 00 fa cf 00` | Ring-3 code, base 0, 4 GiB |
|     7 | Data   data   | `ff ff 00 00 00 f2 cf 00` | Ring-3 data, base 0, 4 GiB |
|     8 | Data   stack* | (same as data)            | Typically identical to #8  |
