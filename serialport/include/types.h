// types.h, 159

#ifndef __TYPES_H__
#define __TYPES_H__

#include "queue.h"

#define LOOP 166666          // handly loop limit exec asm("inb $0x80");
#define TIME_LIMIT 10        // max timer count, then rotate process
#define PROC_NUM 20          // max number of processes
#define Q_SIZE 20            // queuing capacity
#define PROC_STACK_SIZE 4096 // process runtime stack in bytes
#define PORT_NUM 3
#define BUFF_SIZE 101

// Trapframe to save the state of CPU registers /before entering
// kernel code, and loaded back (in reverse) to resume process
typedef struct { 
	unsigned short	gs;      // 16-bit data seg registers below
	unsigned short	filler1; // filler, making 4-byte alignment
	unsigned short	fs; 
	unsigned short	filler2; 
	unsigned short	es; 
	unsigned short	filler3; 
	unsigned short	ds; 
	unsigned short	filler4; 
 
	unsigned int	edi; // PUSHA register state frame below
	unsigned int	esi; 
	unsigned int	ebp; 
	unsigned int	esp; // Push: before PUSHA, Pop: skipped 
	unsigned int	ebx; 
	unsigned int	edx; 
	unsigned int	ecx; 
	unsigned int	eax; 
 
	unsigned int	event_num; // indicate what event occurred
 
	unsigned int	eip; // processor state frame below
	unsigned int	cs; 
	unsigned int	eflags; 
} TF_t;           // trapframe type
 
typedef void (*func_ptr_t)(); // void-return function pointer type

// this is the same as constant defines: FREE=0, RUN=1, etc.
typedef enum {FREE, RUN, READY, SLEEP, WAIT, ZOMBIE} state_t;

typedef struct {  // PCB describes proc image
   state_t state; // state of process
   int cpu_time;	// CPU runtime
   int wake_time;	// used to note when a process is supposed to be waken up from SLEEP state
   TF_t *TF_p;    // points to trapframe of process
} pcb_t;

typedef struct {
  int owner;      // owner pid
  int passes;     // max no. of procs allowed to pass this sem
  q_t wait_q;     // blocked pids
} sem_t;


typedef struct{
  int owner,
      IO,
      write_sid,
      read_sid,
      write_ok;
      q_t write_q;
      q_t read_q;
      q_t loopback_q;
} port_t;

#endif // __TYPES_H__
