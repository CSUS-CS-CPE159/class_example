/* 
 * events.h, Assignment -- Timer Event
 */

#ifndef __EVENTS_H__
#define __EVENTS_H__

#define TIMER_EVENT 32 // 32-bit long

#ifndef ASSEMBLER  // skip if ASSEMBLER defined (in assembly code)
void TimerEntry(); // defined in events.S
void kernel_context_exit(trapframe_t *);
void setup_gdt();
void PageFaultEntry();
extern void page_fault_handler(size_t);
#endif

#endif
