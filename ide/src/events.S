/* 
 * events.S -- Timer Event
 */

#include <spede/machine/asmacros.h> 
#include "events.h"

ENTRY(isr_entry_ide)  // push eflag, cs, eip (by hardware)
  pusha  // push all: eax, ebx, edx, ecx
         //           esp, ebp, esi, edi
  cld  // clear direction flag
  call CNAME(ide_irq_handler)  // call C function ide_irq_handler()
  popa  // pop all: eax, ebx, edx, ecx
        //           esp, ebp, esi, edi
  iret  // pop eip, cs, eflags
