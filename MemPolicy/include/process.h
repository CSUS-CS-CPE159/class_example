/* 
 */

#ifndef __PROCESS_H__
#define __PROCESS_H__

#define PROC_STACK_SIZE 16384

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

    seg_type_t ss;
    seg_type_t _notss;

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
    unsigned int interrupt;
    // CPU state, enter interrupt
    unsigned int eip;
    unsigned int cs;

    unsigned int eflags;
} trapframe_t;

// Process types
typedef enum proc_type_t {
    PROC_TYPE_NONE,     // Undefined/none
    PROC_TYPE_KERNEL,   // Kernel process
    PROC_TYPE_USER      // User process
} proc_type_t;


typedef struct proc_t {
    int pid;                  // Process id
    proc_type_t type;         // Process type (kernel or user)

    size_t *pagetable;
    unsigned char *stack;     // Pointer to the process stack
    trapframe_t *trapframe;   // Pointer to the trapframe
} proc_t;


void Process1(void);
void Process2(void);
void create_processes();

#endif
