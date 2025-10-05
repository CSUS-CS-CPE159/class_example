/* 
 */

#include <spede/machine/proc_reg.h>
#include <spede/machine/seg.h>
#include <spede/stdio.h>
#include <spede/string.h>

#include "events.h"

void Process(void){
	int i;
    size_t *x = (size_t *)0x40000000;

    while(1){
        for (i = 0; i < 16660000; i++)
            asm("inb $0x80");
        cons_printf("This message from Process\n");
        *x = 100;  
    }
}

int main(){
    struct i386_gate *IDT_p; 
    IDT_p = get_idt_base(); // get IDT location
    /* Register the interrupt 14: Page Fault*/
    fill_gate(&IDT_p[14], (int)PageFaultEntry, get_cs(), ACC_INTR_GATE, 0);
 
    Process();
    return 0;
}
