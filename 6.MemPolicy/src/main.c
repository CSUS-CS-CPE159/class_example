/* 
 * main.c -- Timer Event
 */
#include <spede/stdio.h>
#include <spede/machine/io.h>
#include <spede/machine/pic.h>
#include <spede/machine/proc_reg.h>
#include <spede/machine/seg.h>


#include "process.h"
#include "page.h"
#include "events.h"
#include "kalloc.h"

struct i386_gate *IDT_p; 

int main(){
    setup_gdt();
    printf("initialized kalloc!\n");
    kinit(PAGE_START, PAGE_STOP);
    // Set up interrupt for Page Fault
    IDT_p = get_idt_base(); // get IDT location
    fill_gate(&IDT_p[14], (int)PageFaultEntry, get_cs(), ACC_INTR_GATE, 0);

    create_processes();

    // Set up interrupt
    cons_printf("IDT located @ DRAM addr %x (%d).\n", IDT_p, IDT_p); // show IDT addr 
    fill_gate(&IDT_p[32], (int)TimerEntry, get_cs(), ACC_INTR_GATE, 0);
    outportb(0x21, ~0x01); // 0x21 is PIC mask, ~1 is mask
    asm("sti");	
   
    while(1){}

    return 0;
}
