#include <spede/machine/io.h>
#include <spede/machine/pic.h>
#include <spede/machine/proc_reg.h>
#include <spede/machine/seg.h>
#include <spede/string.h>

#include "process.h"
#include "events.h"

uint32_t whoIsRunning = 0;
unsigned char proc_stack[2][PROC_STACK_SIZE];


trapframe_t *p1_stack;
trapframe_t *p2_stack;

/* Process One */
void Process1(void){
    cons_printf("This is the first process\n");
    //uint32_t *x = (uint32_t *)0x10000000;
    //*x = 20;
    while (1){
        cons_printf("This is the first process\n");
        for(uint32_t i = 0; i< 16660000; i++)
            asm("inb $0x80");
    }
}

/* Process Two */
void Process2(void){
    while (1){
        cons_printf("This is the second process\n");
        for(uint32_t i = 0; i< 16660000*2; i++)
            asm("inb $0x80");
    }
}

/* Initialize process stack, setup stack frame information */
void init_stack(trapframe_t *trapframe, void *function, uint32_t esp) {
    // trapframe store all CPU register information
    trapframe->eip = (uint32_t)function;
    trapframe->esp = esp;
    trapframe->eflags = EF_DEFAULT_VALUE | EF_INTR;
    trapframe->ebp = trapframe->esp;
    trapframe->cs = get_cs();
    trapframe->eax = trapframe->esp;
    trapframe->ds = get_ds();
    trapframe->gs = get_gs();
    trapframe->fs = get_fs();
    trapframe->es = get_es();
    trapframe->ss = get_ss();
}


void create_processes(){
    p1_stack = (trapframe_t *)&proc_stack[0][PROC_STACK_SIZE - sizeof(trapframe_t)];
    p2_stack = (trapframe_t *)&proc_stack[1][PROC_STACK_SIZE - sizeof(trapframe_t)];
    init_stack(p1_stack, &Process1, (uint32_t)proc_stack[0]);
    init_stack(p2_stack, &Process2, (uint32_t)proc_stack[1]);
}

void context_switch(trapframe_t *current) {
    // dismiss timer event (IRQ 0), otherwise, new event from timer won't be recognized by CPU
	// because hardware uses edge-trigger flipflop
    outportb(0x20, 0x60);
    if (whoIsRunning == 0){
        // kernel is running
        whoIsRunning = 1;
        // Exit the kernel states
        kernel_context_exit(p1_stack);
    } else {
        // Iteratively switch process 1 and process 2
        if (whoIsRunning == 1) {
            whoIsRunning = 2;
            p1_stack = current;
            kernel_context_exit(p2_stack);
        } else{
            whoIsRunning = 1;
            p2_stack = current;
            kernel_context_exit(p1_stack);
        }
    }
}
