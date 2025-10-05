/* 
 */
#include "spede.h"
#include "events.h"
#include "process.h"
#include "handlers.h"
#include "types.h"

struct i386_gate *IDT_p; 

proc_t p[10];
char proc_kernel_stack[10][STACK_SIZE]  __attribute__((aligned(4096)));

// Current running process
proc_t *active_process = NULL; 

int main(){
    // Set up interrupt for Page Fault
    IDT_p = get_idt_base(); // get IDT location
    /* Register General Protection to interrupt table*/
    fill_gate(&IDT_p[0xC], (int)GeneralProtectionFaultEntry, get_cs(), ACC_INTR_GATE, 0);
    /* Register Page Fault to interrupt table*/
    fill_gate(&IDT_p[0xD], (int)PageFaultEntry, get_cs(), ACC_INTR_GATE, 0);
    /* Register PiC Timer to interrupt table: 0x20 */
    fill_gate(&IDT_p[0x20], (int)TimerEntry, get_cs(), ACC_INTR_GATE, 0);
    
    // create two process    
    NewKernelProcHandler(SystemProc);
    NewKernelProcHandler(SystemProc1);
    // make the first process to run
    active_process = &p[0];
    kernel_context_exit(active_process->trapframe);    
    return 0;
}

void kernel_event(trapframe_t *current) {

    if (current->event_type == 0xD){
	/* Catch Page Fault */
        PageFaultHandler();
    }
    else if (current->event_type == 0xC) {
	/* Catch General Protection Fault */
        GeneralProtectionFaultHandler();
    } else {
	/* handle timer event */
    	// dismiss timer event (IRQ 0), otherwise, new event from timer won't be recognized by CPU
	// because hardware uses edge-trigger flipflop
    	outportb(0x20, 0x60);
        active_process->trapframe = current;
	if (active_process == &p[0]){
        	active_process = &p[1];
    	} else {
        	active_process = &p[0];
    	}
    }
    kernel_context_exit(active_process->trapframe);
}
