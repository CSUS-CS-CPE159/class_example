/* 
 * events.S -- Timer Event
 */

#include <spede/machine/asmacros.h> 
#include "events.h"

ENTRY(TimerEntry)  // push eflag, cs, eip (by hardware)
  pusha  // push all: eax, ebx, edx, ecx
         //           esp, ebp, esi, edi
  cld  // clear direction flag
  call CNAME(TimerHandler)  // call C function TimerHandler()
  popa  // pop all: eax, ebx, edx, ecx
        //           esp, ebp, esi, edi
  iret  // pop eip, cs, eflags
