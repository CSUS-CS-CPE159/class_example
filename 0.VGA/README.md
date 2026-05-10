# VGA Text Output Example

## Overview
This example demonstrates how to directly write text to the VGA display by accessing memory-mapped I/O at address 0xB8000. You'll see "hello, world" appear on the screen in two different ways.

## Learning Objectives
- Understand memory-mapped I/O (MMIO) - how hardware devices appear as memory
- Learn VGA text mode memory layout and addressing
- Practice direct hardware manipulation from C code
- See how to encode character and color attributes into VGA memory

## Build and Run

Compile into MyOS.dli:
```bash
make
```

Start the target simulator (run in a separate terminal):
```bash
spede-target
```

Load and debug the code:
```bash
spede-run -d
```

## How It Works

### VGA Text Memory Layout
- **Address**: 0xB8000 (VGA text buffer in memory)
- **Size**: 80 columns × 25 rows
- **Entry format**: Each cell uses 2 bytes
  - Byte 0: ASCII character code
  - Byte 1: Attribute (color and formatting)

### Attribute Byte Format
The attribute byte (0xXX00 pattern in the code) controls display:
- **Bits 7-4**: Background color (0=black, F=bright white)
- **Bits 3-0**: Foreground color (7=light gray, F=bright white, etc.)

Example: `0x0700` = white text on black background

### Screen Positioning
- Row 1, Column 0: `vga_base[0]`
- Row 2, Column 0: `vga_base[80]` (80 = 80 characters per row)
- Any position: `vga_base[row*80 + column]`

## What You'll See

Two lines of "hello, world" on the screen with different colors:
1. **First line**: White text on black background (0x0700 attribute)
2. **Second line**: White text on red background (0xB400 attribute)

## Key Concepts

### Memory-Mapped I/O (MMIO)
Unlike traditional I/O ports (using `inb`/`outb` instructions), VGA memory is accessed like regular RAM. The hardware automatically updates the display whenever memory is written.

### Direct Hardware Access
Modern operating systems abstract hardware behind device drivers and system calls, but at the kernel level (and in this educational environment), you can directly manipulate hardware like this.

### VGA Color Palette
Standard VGA has 16 colors:
- 0 = Black, 1 = Blue, 2 = Green, 3 = Cyan
- 4 = Red, 5 = Magenta, 6 = Yellow, 7 = White
- 8-F = Bright versions of the above

## Progression
- **Next**: `0.VGA_Advanced` - Custom font loading and more advanced VGA features
- **Before**: `0.calling_convention` - Understanding function calls and stack

## Related Concepts
- Protected mode memory addressing
- Hardware addressing and memory mapping
- Bare metal programming
- Device drivers

