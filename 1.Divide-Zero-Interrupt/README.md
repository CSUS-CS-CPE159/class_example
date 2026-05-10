
# Divide-by-Zero Exception Handling

## Overview
This example demonstrates how to handle CPU exceptions (specifically division by zero) using the Interrupt Descriptor Table (IDT). The program intentionally triggers divide-by-zero exceptions repeatedly while showing that they can be caught and handled by custom exception handlers.

## Learning Objectives
- Understand CPU exceptions vs. external interrupts
- Learn how the IDT (Interrupt Descriptor Table) works
- See how to register and write exception handlers
- Understand the flow from hardware exception → IDT → handler
- Learn the assembly-language code needed for exception handling
- Practice mixing assembly and C code in exception contexts

## Build and Run

Compile into MyOS.dli:
```bash
make
```

Start the target simulator (run in a separate terminal):
```bash
spede-target
```

Run with GDB debugging:
```bash
spede-run -d
```

For detailed stack trace and debugging, see: 
https://docs.google.com/document/d/1QIQNUnc6QRnIbUtByykH35ToVw9EbhXTAR7IrmEu7ec/edit?usp=sharing

## How It Works

### CPU Exceptions
When the CPU encounters certain error conditions (divide by zero, page fault, segment violation, etc.), it:
1. Automatically stops executing user code
2. Saves the current instruction pointer and flags
3. Looks up the exception handler in the IDT
4. Jumps to the handler code

Unlike external interrupts (keyboard, timer), exceptions are synchronous - they occur at a specific point in the instruction stream.

### Interrupt Descriptor Table (IDT)
The IDT is a table of **256 entries**, each pointing to an interrupt/exception handler:
- **Entries 0-31**: CPU exceptions (divide, page fault, general protection, etc.)
- **Entries 32-47**: Hardware interrupts (timer, keyboard, etc.)
- **Entries 48+**: Software interrupts and reserved

### Exception 0: Divide Error
When the CPU detects integer division by zero:
1. It throws exception 0 (divide error)
2. Hardware automatically pushes: EFLAGS, CS, EIP
3. CPU looks up `IDT[0]` to find the handler
4. Jumps to the handler address

### Handler Registration
In main.c, we register our exception handler:
```c
fill_gate(&IDT_p[0], (int)isr0, get_cs(), ACC_INTR_GATE, 0);
```

This tells the CPU: "When exception 0 occurs, jump to function `isr0`"

### Handler Structure
The handler has two parts:

#### Assembly Part (events.S)
```asm
ENTRY(isr0)
  pusha                          /* Save all registers */
  call CNAME(_interrupt_0)       /* Call C handler */
  popa                           /* Restore all registers */
  iret                           /* Return from interrupt */
```

#### C Part (handlers.c)
```c
void _interrupt_0() {
    cons_printf("\n exception: Divide Error \n");
    /* Could do error recovery here */
}
```

### What Happens When You Run It
1. Program starts and registers the divide-by-zero handler
2. Process() runs in a loop printing 'z' characters
3. Every iteration, it divides 5 by 0
4. This triggers exception 0
5. Handler prints "exception: Divide Error"
6. Program continues (prints another 'z')
7. Repeat until you press a key

If you see multiple 'z' characters with exception messages between them, the handler is working!

## Key Concepts

### Stack Frame During Exception
When an exception occurs:
```
[old] ESP
      |
      v  After hardware pushes:
     [EFLAGS]  <- ESP points here
     [CS]
     [EIP]
     
     After PUSHA:
     [EFLAGS]
     [CS]
     [EIP]
     [EAX]
     [ECX]
     [EDX]
     [EBX]
     [ESP] <- (value before pusha)
     [EBP]
     [ESI]
     [EDI]  <- ESP points here now
```

### Privilege Levels
- Exception handlers run in **kernel mode** (privilege level 0)
- Even if the exception was triggered from user mode
- This allows the kernel to handle errors safely

### Return from Exception
The `IRET` instruction:
- Pops EIP, CS, and EFLAGS (in that order)
- Restores the execution state as it was before the exception
- Automatically handles privilege level switching if needed
- Is NOT the same as RET

## Related Concepts

### Other CPU Exceptions (IDT entries 0-31)
- **0**: Divide Error (this example)
- **1**: Debug
- **3**: Breakpoint
- **4**: Overflow
- **5**: Bounds Check
- **6**: Invalid Opcode
- **7**: Coprocessor Not Available
- **8**: Double Fault
- **10**: Invalid TSS
- **11**: Segment Not Present
- **12**: Stack Fault
- **13**: General Protection
- **14**: Page Fault (explored in module 5)

### Error Recovery
In a real OS:
- User processes that cause exceptions would be terminated
- The OS might generate a signal that the process could catch
- Critical exceptions (page fault, GPF) require careful handling
- Some exceptions (breakpoint, overflow) might be recoverable

## Progression
- **Before**: `0.VGA_Advanced` - Hardware I/O and VGA
- **Next**: `2.Keyboard_Polling` or `2.Keyboard_Interrupt` - Hardware interrupts

## Advanced Topics

### Alternative: Skip the Faulting Instruction
The code includes `isr0_solution` which skips over the problematic instruction instead of handling the error. This works by:
1. Modifying the saved EIP
2. Adding 3 bytes (approximate instruction length)
3. Returning to skip past the divide instruction

While educational, this is poor practice in real code!

### Double Fault
If the exception handler itself causes an exception (e.g., divide by zero in the handler), the CPU generates exception 8 (Double Fault). This is why exception handlers must be carefully written.

## GDB Debugging Tips
- `b _interrupt_0` - Breakpoint at C exception handler
- `b isr0` - Breakpoint at assembly handler
- `info locals` - Show local variables in handler
- `print $esp` - Check stack pointer during exception
- `layout asm` - Show assembly code window

---

For more details on exception handling and debugging, see the GDB documentation link above.

