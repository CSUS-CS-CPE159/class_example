// events.h of events.S

#ifndef __EVENTS_H__
#define __EVENTS_H__

// events for time sharing and context switching (phase1)
#define K_CODE 		0x08    // kernel code segment # (register)
#define K_DATA 		0x10    // kernel data segment # (register)
#define K_STACK_SIZE 	16384  	// kernel runtime stack byte size
#define TIMER_EVENT  	0x20    // IDT entry #32, aka IRQ0, for timer device
// events for kernel services (phase2)
#define GETPID_EVENT 	0x64   	// IDT entry #100, for get pid event
#define SLEEP_EVENT     0x65    // IDT entry #101, for sleep event
// events for semaphores (phase3)
#define SEMALLOC_EVENT 	0x66 	// IDT entry #102, for sem alloc event
#define SEMWAIT_EVENT 	0x67  	// IDT entry #103, for sem wait event
#define SEMPOST_EVENT 	0x68  	// IDT entry #104, for sem post event
// event for device driver: busy poll (phase4)
#define SYSPRINT_EVENT 	0x69 	// IDT entry #105, for sys print event
// events for device driver: interrupt driven (phase5)
#define PORT_EVENT 	0x23    // IDT entry #35,  aka IRQ3; (35+1)=36 is IRQ4
#define PORTALLOC_EVENT 0x6A	// IDT entry #106, for port alloc event
#define PORTWRITE_EVENT 0x6B
#define PORTREAD_EVENT 	0x6C 	// IDT entry #108, for port read event
                            
// since below is not in assembler syntax
#ifndef ASSEMBLER           // skip below if ASSEMBLER defined (from an assembly code)
//__BEGIN_DECLS

#include "types.h"          // for 'TF_t' below

extern void TimerEvent(void);// coded in events.S, assembler won't like this syntax
extern void Loader(TF_t *);  // coded in events.S
extern void GetPidEvent();	 // coded in events.S, code for Kernel Services
extern void SleepEvent();		 // coded in events.S, code for Kernel Services
extern void SemAllocEvent(); // coded in events.S, code for Semaphores 
extern void SemWaitEvent();  // coded in events.S, code for Semaphores
extern void SemPostEvent();  // coded in events.S, code for Semaphores
extern void PortEvent();
extern void PortEvent1();
extern void PortAllocEvent();
extern void PortWriteEvent();
extern void PortReadEvent();

//__END_DECLS

#endif // ifndef ASSEMBLER

#endif // ifndef __EVENTS_H__
