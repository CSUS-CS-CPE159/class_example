/* 
 * GNU Assembly Code: AT & T
 */

#include <spede/machine/asmacros.h> 
#include "events.h"

ENTRY(PageFaultEntry)  // push eflag, cs, eip (by hardware)
  // Hardware also push error code to stack
  pusha               // push all: eax, ebx, edx, ecx, esp, ebp, esi, edi
  pushl %ds          // save data segment
  pushl %es          // extra segment
  pushl %fs          // Save FS register
  pushl %gs          // Save GS register 
  call CNAME(PageFaultHandler)  // call C function PageFaultHandler()
  popl %gs
  popl %fs
  popl %es
  popl %ds 
  popa              // pop all: eax, ebx, edx, ecx, esp, ebp, esi, edi
  addl $4, %esp     //pop error code 
  iret              // pop eip, cs, eflags
