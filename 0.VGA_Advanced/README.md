### VGA Text Mode
VGA text mode is fundamentally an 8-bit system, which means it is limited by how many unique character "slots" the hardware can address at once. 

#### Standard Mode: 256 characters. By default, VGA uses an 8-bit character code for each screen position, allowing for 256 unique glyphs simultaneously. 

#### Internal Storage
VGA hardware has Plane 2 (font memory), which is dedicated to storing character bitmaps. 

Each character in the memory is allocated as 16 pixels high (standard for 80x25)

# Advanced VGA: Custom Font/Glyph Upload

## Overview
This example demonstrates advanced VGA programming by uploading custom character bitmaps (glyphs) to the VGA font ROM. This shows how VGA memory is organized into multiple planes and how to manipulate the sequencer and graphics controller registers.

## Learning Objectives
- Understand VGA's planar graphics memory architecture (planes 0-3)
- Learn how to access VGA registers via port I/O
- See how to modify font ROM at runtime
- Practice creating bitmap graphics for custom characters
- Understand the relationship between font ROM and text display

## Build and Run

Compile into MyOS.dli:
```bash
make
```

Start the target simulator (run in a separate terminal):
```bash
spede-target
```

Load and debug:
```bash
spede-run -d
```

## How It Works

### VGA Memory Organization
VGA graphics memory is divided into 4 **planes** (banks) of memory:
- **Planes 0-1**: Text character display data (80×25 text buffer)
- **Plane 2**: Font ROM (character bitmap definitions)
- **Plane 3**: Reserved/unused

By switching between planes, we can:
- Write to text buffer (normal display)
- Access font ROM to replace character bitmaps

### Font ROM Structure
- Each character occupies **32 bytes** in font ROM
- For 8×16 pixel fonts, we use 16 bytes (one per row)
- Remaining 16 bytes are padding/reserved
- Character position = `ascii_code * 32`

### Glyph Bitmap Format
Each byte in the glyph represents one row of an 8-pixel-wide character:
```
0x3C = 00111100 (pixels on/off from left to right)
0x42 = 01000010
...etc
```

### VGA Register Programming
VGA registers are accessed through I/O ports:

**Sequencer Registers** (Port 0x3C4/0x3C5):
- Register 0x02: Plane write mask (which planes to write to)
- Register 0x04: Memory mode (addressing scheme)

**Graphics Controller Registers** (Port 0x3CE/0x3CF):
- Register 0x04: Read plane selector
- Register 0x05: Mode register (addressing scheme)
- Register 0x06: Miscellaneous register (memory mapping)

### Font Loading Process
1. **Switch to Font-Load Mode**: Configure registers to access plane 2
2. **Write Bitmap Data**: Copy 16-byte glyph to font ROM
3. **Restore Text Mode**: Reconfigure to display text normally

## What You'll See
The program uploads three custom glyphs:
1. **'A'** → Omega symbol (Greek Ω)
2. **'B'** → Katakana 'A' (ア) character
3. **'C'** → Katakana 'HE' (ヘ) character

Then it displays these custom characters on the screen, appearing as different symbols than normal text characters.

## Advanced Concepts

### Protected vs. Text Mode
Modern systems run in protected mode (32-bit), but still support VGA for compatibility. This example works in protected mode while accessing legacy VGA hardware.

### Planar Graphics Memory
VGA's planar architecture allows:
- Multiple independent memory banks
- More flexible graphics modes
- But also complexity in programming

### Hardware Abstraction
In modern OSes, such direct hardware access is forbidden. This example shows why device drivers are needed to abstract complex hardware.

## Related Topics
- VGA BIOS interrupts (int 0x10)
- Video memory management
- Hardware registers and port I/O
- Character encoding and fonts
- Graphics mode programming

## Progression
- **Before**: `0.VGA` - Basic text output
- **Next**: `1.Divide-Zero-Interrupt` - Exception handling

## Resources
- VGA Font Editing: Study how the bitmap arrays define character shapes
- Port I/O: Understanding the pattern of index + data ports
- Plane Switching: Key to advanced VGA programming


#### Reference

https://wiki.osdev.org/VGA_Hardware
