/**
 * events.S -- Interrupt Service Routine (ISR) for CPU Exceptions
 *
 * Assembly-language code to handle CPU exceptions.
 * ISRs are responsible for:
 * 1. Saving the CPU state (registers)
 * 2. Calling the C-language exception handler
 * 3. Restoring the CPU state
 * 4. Returning from the interrupt
 */

#include <spede/machine/asmacros.h> 
#include "events.h"

/**
 * ISR for Exception 0: Divide-by-Zero
 * 
 * When the CPU detects integer division by zero, it automatically:
 * 1. Pushes EFLAGS (flags register)
 * 2. Pushes CS (code segment selector)
 * 3. Pushes EIP (instruction pointer - address of faulting instruction)
 * 4. Jumps to this handler's address (from IDT entry 0)
 * 
 * Our handler then:
 * 1. Saves all general-purpose registers with PUSHA
 * 2. Calls the C-language handler _interrupt_0()
 * 3. Restores all general-purpose registers with POPA
 * 4. Returns from interrupt with IRET
 */
ENTRY(isr0)           /* Entry point for ISR 0 */
  pusha               /* Push all GP registers: EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI */
                      /* This saves the processor state so C code can work with it */
  
  cld                 /* Clear Direction Flag - reset string operations direction */
  call CNAME(_interrupt_0)  /* Call C function _interrupt_0() */
  
  popa                /* Pop all GP registers - restore processor state */
  
  iret                /* Return from interrupt: pops EIP, CS, EFLAGS and continues */
                      /* IRET is different from RET - it handles the saved flags state */


/**
 * Alternative ISR that demonstrates skipping the faulting instruction
 * 
 * This is an optional approach to handle divide-by-zero by:
 * 1. Advancing the Instruction Pointer (EIP) past the faulting instruction
 * 2. Allowing the program to continue without handling the error
 * 
 * This is a workaround - normally you'd want to handle the error properly.
 * In a real OS, this would likely terminate the process.
 */
ENTRY(isr0_solution)
  push %eax              /* Save EAX since we'll use it */
  
  /* On the stack now:
   * 4(%esp) = EIP (address of the faulting instruction)
   * 0(%esp) = EAX (saved)
   */
  
  mov 4(%esp), %eax      /* Load EIP into EAX */
  add $3, %eax           /* Add 3 bytes (typical length of divide instruction) */
  mov %eax, 4(%esp)      /* Write modified EIP back to stack */

  pop %eax               /* Restore EAX */
  iret                   /* Return - EIP will skip past the faulting instruction */

