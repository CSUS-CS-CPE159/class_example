
# CSC/CPE 159: Operating System Pragmatics - Class Examples

This repository contains progressive example code for **CSC/CPE 159 (Operating System Pragmatics)** at California State University, Sacramento. These examples demonstrate fundamental OS concepts from low-level hardware interaction to advanced kernel features.

## Overview

These examples build progressively from basic x86 assembly and hardware access to complete kernel features:

- **Hardware Basics**: VGA display, memory-mapped I/O, calling conventions
- **Interrupts**: Handling CPU exceptions and hardware interrupts (keyboard, timer, serial)
- **Processes**: Context switching, process management, memory protection
- **Memory Management**: Virtual memory, paging, memory allocation
- **System Features**: System calls, user processes, serial I/O, disk access

## Prerequisites

- SPEDE (Simulated Environment for Processor Design Education) environment
- x86 cross-compiler and development tools
- Basic understanding of x86 assembly and C programming
- Familiarity with operating systems concepts

## Building and Running

All examples follow a consistent build pattern:

```bash
cd <example_directory>
make              # Compile the code into MyOS.dli
spede-target      # Start the target simulator (in a separate terminal)
spede-run -d      # Load and debug on the SPEDE target
```

## Suggested Learning Path

Students should work through these examples in order to build understanding progressively:

### Phase 1: Hardware & Low-Level Basics
1. **0.calling_convention** - x86 calling conventions and stack layout
2. **0.VGA** - Basic VGA text display output
3. **0.VGA_Advanced** - Advanced VGA text manipulation

### Phase 2: Interrupts & I/O
4. **1.Divide-Zero-Interrupt** - Exception handling (divide by zero)
5. **2.Keyboard_Polling** - Polling-based keyboard input
6. **2.Keyboard_Interrupt** - Interrupt-driven keyboard input
7. **3.com2_polling** - Serial port communication (polling)
8. **3.com2_interrupt** - Serial port communication (interrupt-driven)
9. **TimerEvent** - Timer interrupt handling

### Phase 3: Process Management
10. **4.ContextSwitch** - Basic context switching between processes
11. **4.KernelProcess** - Kernel processes with Process Control Blocks (PCB)
12. **5.PageFault** - Page fault exception handling

### Phase 4: Advanced Kernel Features
13. **6.MemPolicy** - Memory management and allocation policies
14. **7.Syscall** - System call implementation and handling
15. **8.UserProcess** - User-mode processes and privilege levels
16. **9.serialport** - Serial port driver implementation
17. **A.ide** - IDE/ATA disk driver implementation

## Key Concepts Demonstrated

- **x86 Architecture**: Real-mode and protected-mode operation
- **Interrupt Handling**: IDT (Interrupt Descriptor Table), interrupt handlers
- **Process Management**: Context switching, PCB, process scheduling
- **Memory Management**: Virtual memory, paging, segmentation
- **Device I/O**: Keyboard, serial ports, disk controllers
- **Protected Mode**: Privilege levels, system calls, user vs. kernel mode

## Important Notes

- **SPEDE Environment Required**: These examples are specifically designed for the SPEDE environment and may not run on standard x86 systems
- **Hardware Simulation**: The SPEDE environment provides simulated x86 hardware for learning purposes
- **Educational Purpose**: These examples prioritize clarity and learning over production optimization

## Debugging Tips

- Use GDB through `spede-run -d` to debug with breakpoints and step execution
- Monitor the VGA display for output and status information
- Check serial terminals with `spede-term com2` for serial port output
- Review IDT locations and interrupt vectors with GDB
- Use `layout asm` and `layout prev` in GDB to view assembly code and registers

---

For more information about CSC/CPE 159 or SPEDE, consult your course materials and instructor.   
