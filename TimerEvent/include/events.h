/* 
 * events.h, Assignment -- Timer Event
 */

#ifndef __EVENTS_H__
#define __EVENTS_H__

#define TIMER_EVENT 32 // 32-bit long

#ifndef ASSEMBLER  // skip if ASSEMBLER defined (in assembly code)
void TimerEntry(); // defined in events.S
#endif

#endif
