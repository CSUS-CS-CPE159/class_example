/* 
 * events.S -- Port Event: com2
 */

#include <spede/machine/asmacros.h> 

ENTRY(PortEntry)  // push eflag, cs, eip (by hardware)
  pusha  // push all: eax, ebx, edx, ecx
         //           esp, ebp, esi, edi
  call CNAME(uart_interrupt_handler)  // call C function port_interrupt_handler()
  popa  // pop all: eax, ebx, edx, ecx
        //           esp, ebp, esi, edi
  iret  // pop eip, cs, eflags
