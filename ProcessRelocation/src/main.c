/* 
 * main.c -- Timer Event
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
    /* Register General Protection to interrupt table*/
    fill_gate(&IDT_p[13], (int)GeneralProtectionFaultEntry, get_cs(), ACC_INTR_GATE, 0);
    /* Register Page Fault to interrupt table*/
    fill_gate(&IDT_p[14], (int)PageFaultEntry, get_cs(), ACC_INTR_GATE, 0);
    /* Register PiC Timer to interrupt table: 0x20 */
    fill_gate(&IDT_p[32], (int)TimerEntry, get_cs(), ACC_INTR_GATE, 0);
    
    NewKernelProcHandler(SystemProc);
    NewKernelProcHandler(SystemProc1);
    active_process = &p[0];
    kernel_context_exit(active_process->trapframe);    
    return 0;
}


void context_switch(trapframe_t *current) {

    if (current->event_type == 0xD){
        PageFaultHandler();
    }
    else if (current->event_type == 0xC) {
        GeneralProtectionFaultHandler();
    }
    // dismiss timer event (IRQ 0), otherwise, new event from timer won't be recognized by CPU
	// because hardware uses edge-trigger flipflop
    outportb(0x20, 0x60);
    //printf("context switch from process %d\n", active_process->pid);    
    active_process->trapframe = current;
    if (active_process == &p[0]){
        // kernel is running
        active_process = &p[1];
        printf("switch process 1\n");
    } else {
        // Iteratively switch process 1 
        active_process = &p[0];
        printf("switch process 0\n");
    }
    kernel_context_exit(active_process->trapframe);
}
