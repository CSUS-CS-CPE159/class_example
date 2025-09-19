/* 
 * main.c
 */
#include "spede.h"
#include "events.h"
#include "process.h"
#include "handlers.h"
#include "types.h"

struct i386_gate *IDT_p; 

proc_t p[10];
char proc_kernel_stack[10][STACK_SIZE];
// Current running process
proc_t *active_process = NULL; 


int main(){
    // Set up interrupt for Page Fault
    IDT_p = get_idt_base(); // get IDT location
    /* Register PiC Timer to interrupt table: 0x20 */
    fill_gate(&IDT_p[32], (int)TimerEntry, get_cs(), ACC_INTR_GATE, 0);
    /* Register System call to interrupt table: 0x80 */
    fill_gate(&IDT_p[128], (int)SyscallEntry, get_cs(), ACC_INTR_GATE, 0);
    
    NewKernelProcHandler(SystemProc, 0);
    NewKernelProcHandler(SystemProc1, (size_t)SystemProc1_code_end - (size_t)SystemProc1);
    
    active_process = &p[0];
    kernel_context_exit(active_process->trapframe);    
    return 0;
}


void context_switch(trapframe_t *current) {
   	/// dismiss timer event (IRQ 0), otherwise, new event from timer won't be recognized by CPU
    // because hardware uses edge-trigger flipflop
    outportb(0x20, 0x60);
    
    active_process->trapframe = current;
 
    if (current->event_type == 0x80){
        SyscallHandler(); 
    }

    if (active_process == &p[0]){
        // kernel is running
        active_process = &p[1];
    } else {
        // Iteratively switch process 1 and process 0
        active_process = &p[0];
    }
    kernel_context_exit(active_process->trapframe);
}
