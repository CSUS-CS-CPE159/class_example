/**
 * events.h -- Exception Handler Declarations
 * 
 * Declares the assembly-language interrupt service routines (ISRs)
 * that handle CPU exceptions and hardware interrupts.
 */

#ifndef __EVENTS_H__
#define __EVENTS_H__

/* Skip these declarations in assembly files */
#ifndef ASSEMBLER

/**
 * Interrupt Service Routine for exception 0 (divide-by-zero)
 * 
 * This is an assembly-language function that:
 * 1. Saves the processor state (all registers)
 * 2. Calls the C-language handler _interrupt_0()
 * 3. Restores the processor state
 * 4. Returns from the interrupt with iret
 * 
 * Located in events.S
 */
void isr0();

#endif /* ASSEMBLER */

#endif /* __EVENTS_H__ */
