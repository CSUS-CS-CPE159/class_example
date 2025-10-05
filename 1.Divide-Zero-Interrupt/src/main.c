/* 
 * main.c -- Timer Event
 */

#include <spede/flames.h>
#include <spede/machine/asmacros.h>
#include <spede/machine/io.h>
#include <spede/machine/pic.h>
#include <spede/machine/proc_reg.h>
#include <spede/machine/seg.h>
#include <spede/stdio.h>
#include <spede/string.h>

#include "events.h" // needs addr of TimerEvent

#define LOOP 16660000 // handy loop to time .6 microseconds

typedef void (* func_ptr_t)();
struct i386_gate *IDT_p; 

void Process(void){
	size_t i;
    
    while(1){
        if( cons_kbhit() ) {                    // poll keyboard, returns 1 if pressed
            cons_printf("Stop the current process!");
            break; 
        }
        for (i = 0; i<LOOP; i++)
            asm("inb $0x80");
        cons_putchar('z'); 

        i = 0;
        i = 5/i;
    }
}

int main(){
	IDT_p = get_idt_base(); // get IDT location

	cons_printf("IDT located @ DRAM addr %x (%d).\n", IDT_p, IDT_p); // show IDT addr

    /* handle the divide error  */
    fill_gate(&IDT_p[0], (int)isr0, get_cs(), ACC_INTR_GATE, 0);
	
    Process();
	return 0;
}
