/* 
 * events.h, Assignment -- Timer Event
 */

#ifndef __EVENTS_H__
#define __EVENTS_H__

#include "types.h"

#ifndef ASSEMBLER  // skip if ASSEMBLER defined (in assembly code)

void TimerEntry(); // defined in events.S
void kernel_context_exit(trapframe_t *);
//void setup_gdt();
void GeneralProtectionFaultEntry();
void PageFaultEntry();

#endif

#endif
