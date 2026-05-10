# Keyboard Input - Polling Method

## Overview
This example demonstrates polling-based keyboard input. The CPU continuously checks the keyboard status port, and when a key is available, it reads the scan code and translates it to an ASCII character.

## Learning Objectives
- Understand polling as an input mechanism
- Learn about keyboard hardware and scan codes
- Practice I/O port programming (reading status and data ports)
- See how to translate hardware codes to ASCII characters
- Compare polling vs. interrupts (see 2.Keyboard_Interrupt for interrupt version)

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

### Keyboard Hardware
The keyboard controller uses two I/O ports:
- **Port 0x60**: Keyboard data port (read scan codes)
- **Port 0x64**: Keyboard status port (read status flags)

### Polling Loop
The program continuously:
1. **Check status**: Read bit 0 of port 0x64
   - If bit 0 = 1: Data is available
   - If bit 0 = 0: No data yet (loop and check again)
2. **Read data**: When data is available, read from port 0x60
3. **Translate**: Convert scan code to ASCII character
4. **Repeat**: Loop forever

### Scan Codes
- Each key produces a unique scan code (0x00 - 0x7F)
- Scan codes are NOT the same as ASCII characters
- Extended keys (arrows, Home, End) produce escape sequences:
  - First byte: 0xE0 (escape marker)
  - Second byte: Extended key scan code
- Key release: Bit 7 set (e.g., 0x80 for key 0 release)

### Scan Code Translation
Two translation tables:
1. **keyboard_map_primary**: Maps standard keys to ASCII
   - Index = scan code
   - Value = ASCII character
   - Example: scan code 0x1C = Enter key = '\n'
2. **keyboard_map_ext**: Maps extended keys
   - Used after 0xE0 escape sequence
   - Maps arrow keys, function keys, special keys

## What You'll See

When you type:
- Console output showing: `keyboard scancode: XX, 0xXX, ascii code: C`
- VGA display shows the scan code and ASCII equivalent
- Works with all printable characters and special keys (arrows, F1-F10, etc.)

## Key Concepts

### Polling vs. Interrupts

| Feature | Polling | Interrupts |
|---------|---------|-----------|
| CPU Activity | Continuously checks status | Only notified when key pressed |
| CPU Waste | High (spins in loop) | Low (CPU does other work) |
| Latency | Variable (depends on loop frequency) | Low and consistent |
| Complexity | Simple | More complex (handlers needed) |
| Real-world | Rarely used for keyboards | Standard approach |

### The Problem with Polling
In this simple example, the CPU loops checking for keyboard input. This is fine for testing but problematic in real systems because:
- CPU wastes cycles that could do useful work
- Response time depends on loop frequency
- Scales poorly with multiple input devices

### Why Interrupts Are Better
When a key is pressed, the keyboard hardware signals an interrupt:
- CPU stops current work
- Runs interrupt handler immediately
- CPU efficiency improves dramatically

## Hardware Details

### Status Register (Port 0x64)
```
Bit 0: Output Buffer Full (OBF)
  - 1 = Data available in port 0x60
  - 0 = No data available
Bit 1: Input Buffer Full (IBF)
  - For keyboard commands (not used in this example)
Bit 5: Keyboard Enabled
Bit 6: Timeout error
Bit 7: Parity error
```

### Scan Code Examples
| Key | Scan Code | ASCII |
|-----|-----------|-------|
| 'A' | 0x1E | 'a' |
| '1' | 0x02 | '1' |
| Enter | 0x1C | '\n' |
| Backspace | 0x0E | '\b' |
| Space | 0x39 | ' ' |
| Left Arrow | 0xE0 0x4B | KEY_LEFT |
| F1 | 0x3B | KEY_F1 |

## Keyboard Layouts
This code uses a US 102-key layout. Extended keys (like arrows and numpad) have special handling through the escape mechanism.

## Progression
- **Before**: `1.Divide-Zero-Interrupt` - Exception handling
- **Next**: `2.Keyboard_Interrupt` - Interrupt-driven keyboard input (more efficient)
- **After**: `3.com2_polling` - Serial port polling

## Advanced Topics

### State Machine for Extended Keys
The code uses a state variable `ext` to track extended keys:
- Normal state: `ext = 0`
- Receive 0xE0: Set `ext = 1`
- Next byte: Use extended map, reset `ext = 0`

### Debouncing
Real keyboards need debouncing (wait for signal to settle). This example doesn't implement it - the hardware does it for us in the SPEDE environment.

### Scan Code Sets
PS/2 keyboards support multiple scan code sets. This example assumes Scan Code Set 1 (most common).

## Related Concepts
- I/O port programming (inportb/outportb)
- Character encoding and ASCII
- Hardware polling and busy-waiting
- Input device drivers
- State machines for protocol handling

---

Compare this polling approach with the interrupt-driven version in `2.Keyboard_Interrupt` to see the efficiency improvement.

type any key to test the code.
