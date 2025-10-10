/* 
 * events.S -- Keyboard Event
 */

#include <spede/machine/asmacros.h> 
#include "events.h"

ENTRY(KeyboardEntry)  // push eflag, cs, eip (by hardware)
  pusha  // push all: eax, ebx, edx, ecx
         //           esp, ebp, esi, edi
  cld  // clear direction flag
  call CNAME(keyboard_interrupt_handler)  // call C function TimerHandler()
  popa  // pop all: eax, ebx, edx, ecx
        //           esp, ebp, esi, edi
  iret  // pop eip, cs, eflags
