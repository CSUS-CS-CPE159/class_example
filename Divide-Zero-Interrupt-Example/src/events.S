/* 
 * events.S -- Timer Event
 */

#include <spede/machine/asmacros.h> 
#include "events.h"

ENTRY(isr0)  // push eflag, cs, eip (by hardware)
  pusha  // push all: eax, ebx, edx, ecx
         //           esp, ebp, esi, edi
  
  cld  // clear direction flag
  call CNAME(_interrupt_0)  // call C function TimerHandler()
  popa  // pop all: eax, ebx, edx, ecx
        //           esp, ebp, esi, edi
  iret  // pop eip, cs, eflags


ENTRY(isr0_solution)  // push eflag, cs, eip (by hardware)
  // jump the current instruction.
  push %eax  
  mov 4(%esp), %eax
  add $3, %eax
  mov %eax, 4(%esp)

  pop %eax
  iret 
