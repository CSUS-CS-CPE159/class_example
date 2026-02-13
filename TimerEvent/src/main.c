/* 
 * main.c, Assignment 3 -- Timer Event
 */

#define LOOP 16666666 // loop to time 6. us
#include <spede/flames.h>
#include <spede/machine/asmacros.h>
#include <spede/machine/io.h>
#include <spede/machine/pic.h>
#include <spede/machine/proc_reg.h>
#include <spede/machine/seg.h>
#include <spede/stdio.h>
#include <spede/string.h>
#include "events.h" // needs addr of TimerEvent

typedef void (* func_ptr_t)();
struct i386_gate *IDT_p; 

void Process(void){
	int i;
	while(1){
		if(cons_kbhit()){
			char ch = cons_getchar();                 // read in pressed key
			cons_printf("\nTimer Event Process is stopping!\n");
         	if (ch) 
            	break; // break while loop
		}else{
			// delay 1 second
			// inb command is used for slow delay. 
			// Read data from port 80 to AL register
			for(i=0; i<LOOP; i++) asm("inb $0x80"); 
			
			cons_putchar('z'); // prints char z on screen
		}
	}
}

int main(){
	IDT_p = get_idt_base(); // get IDT location
	cons_printf("IDT located @ DRAM addr %x (%d).\n", IDT_p, IDT_p); // show IDT addr

	fill_gate(&IDT_p[TIMER_EVENT], (int)TimerEntry, get_cs(), ACC_INTR_GATE,0);
	outportb(0x21, ~0x01); // 0x21 is PIC mask, ~1 is mask
	asm("sti"); // set/enable intr in CPU EFLAGS reg

	Process();
	return 0; // main() ends
}
