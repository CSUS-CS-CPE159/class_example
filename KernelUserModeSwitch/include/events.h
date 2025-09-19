/* 
 * events.h, Assignment -- Timer Event
 */

#ifndef __EVENTS_H__
#define __EVENTS_H__

#include "types.h"

#ifndef ASSEMBLER  // skip if ASSEMBLER defined (in assembly code)

void TimerEntry(); // defined in events.S
void ProcLoader(trapframe_t *);
//void setup_gdt();
void SyscallEntry();
void KernelTssFlush();

#endif

#endif
