/* 
 * main.c -- Context Switch 
 */

#include <spede/machine/io.h>
#include <spede/machine/proc_reg.h>
#include <spede/machine/seg.h>


#include "process.h"
#include "events.h"

#define PIC_DATA 0x21

struct i386_gate *IDT_p; 

int main(){
    // create two simplified process for testing context switch
    create_processes();

    /* Set up interrupt */
    IDT_p = get_idt_base(); // get interrup descriptor table location
    // Display IDT addr by VGA (Video Graph Array)
    cons_printf("IDT located @ DRAM addr %x (%d).\n", IDT_p, IDT_p); // show IDT addr 
    // Customize the Time Interrup Entry 
    fill_gate(&IDT_p[32], (int)TimerEntry, get_cs(), ACC_INTR_GATE, 0);
    // 0x21 is prime PIC data register address. ~0x01 is 1111 1110 , enable the PIC interrupt 0
    outportb(PIC_DATA, ~0x01); 
    // Enable CPU interrupt 
    asm("sti");	
  
    // infinite loop
    while(1);

    return 0;
}
