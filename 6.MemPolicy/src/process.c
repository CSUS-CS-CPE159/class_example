#include <spede/machine/io.h>
#include <spede/machine/pic.h>
#include <spede/machine/proc_reg.h>
#include <spede/machine/seg.h>
#include <spede/string.h>

#include "process.h"
#include "page.h"
#include "events.h"

proc_t p1;
proc_t p2;
// Current running process
proc_t *active_process = NULL;

void Process1(void){
    while (1){
        uint32_t *x = (uint32_t *)0x40000000;
        *x = 20;
        cons_printf("This is the first process\n");
        for(uint32_t i = 0; i< 16660000; i++)
            asm("inb $0x80");
    }
}
extern void Process1_code_end();
void Process2(void){
    while (1){
        uint32_t *x = (uint32_t *)0x40000000;
        *x = 30;
        cons_printf("This is the second process\n");
        for(uint32_t i = 0; i< 16660000*2; i++)
            asm("inb $0x80");
    }
}
extern void Process2_code_end();

__asm__(".global Process1_code_end\n"
        "Process1_code_end:");
__asm__(".global Process2_code_end\n"
        "Process2_code_end:");

void init_proc(proc_t *process, void *function, void *function_end, 
    uint32_t code_seg, uint32_t data_seg, uint32_t code_address) {

    uint32_t stack_address = code_address + 0x7FFFF;

    process->trapframe = (trapframe_t *)(stack_address - sizeof(trapframe_t));

    trapframe_t *trapframe = process->trapframe;
    trapframe->eip = (uint32_t)function;
    trapframe->esp = stack_address;
    trapframe->eflags = EF_DEFAULT_VALUE | EF_INTR;
    trapframe->ebp = trapframe->esp;
    trapframe->cs = code_seg;
    trapframe->eax = trapframe->esp;
    trapframe->ds = data_seg;
    trapframe->gs = data_seg;
    trapframe->fs = data_seg;
    trapframe->es = data_seg;
    trapframe->ss = data_seg;
    size_t function_size = (size_t)function_end - (size_t)function;
    memcpy((void*)trapframe->eip, (void *)function, function_size);

    // Initialize page table
    process->pagetable = setup_pagetable();
}


void create_processes(){
    uint32_t p1_base_addr = 0x200000;
    uint32_t p2_base_addr = 0x300000;
    init_proc(&p1, &Process1, &Process1_code_end, 0x18, 0x20, p1_base_addr);
    init_proc(&p2, &Process2, &Process2_code_end, 0x28, 0x30, p2_base_addr);
}

void context_switch(trapframe_t *current) {
    // dismiss timer event (IRQ 0), otherwise, new event from timer won't be recognized by CPU
	// because hardware uses edge-trigger flipflop
    outportb(0x20, 0x60);
    if (active_process == NULL){
        // kernel is running
        active_process = &p1;
    } else {
        active_process->trapframe = current;
        // Iteratively switch process 1 and process 2
        if (active_process == &p1) 
            active_process = &p2;
        else
            active_process = &p1;
    }
    enable_page_table(active_process->pagetable);
    kernel_context_exit(active_process->trapframe);
}