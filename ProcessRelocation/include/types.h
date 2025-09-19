#ifndef _TYPES_H_
#define _TYPES_H_

#define STACK_SIZE      16384
#define TIMER_EVENT     32 // 32-bit long

typedef unsigned short seg_type_t;  // 16-bit segment value


typedef struct {
    // Saved segment registers
    seg_type_t gs;      // unsigned short, 2 bytes
    seg_type_t _notgs;  // filler, 2 bytes

    seg_type_t fs;
    seg_type_t _notfs;

    seg_type_t es;
    seg_type_t _notes;

    seg_type_t ds;
    seg_type_t _notds;

    // register states
    unsigned int edi;
    unsigned int esi;
    unsigned int ebp;
    unsigned int esp;  // Push: before PUSHA, Pop: skipped
    unsigned int ebx;
    unsigned int edx;
    unsigned int ecx;
    unsigned int eax;
 
    // Indicate the type of interrupt that has happened
    unsigned int event_type;
    unsigned int error_code; /*exception error code*/
    
    // CPU state, enter interrupt
    unsigned int eip;
    unsigned int cs;
    unsigned int eflags;
} trapframe_t;

// Process types
typedef enum {
    PROC_TYPE_NONE,     // Undefined/none
    PROC_TYPE_KERNEL,   // Kernel process
    PROC_TYPE_USER      // User process
} proc_type_t;


typedef struct {
    int pid;                  // Process id
    proc_type_t proc_type;         // Process type (kernel or user)
    trapframe_t *trapframe;
}proc_t;
#endif
