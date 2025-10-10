/**
 * CPE/CSC 159 Operating System Pragmatics
 * California State University, Sacramento
 *
 * Operating system entry point
 */
#include <spede/machine/proc_reg.h>
#include <spede/machine/seg.h>

#include "spede/stdio.h"
#include "io.h"
#include "keyboard.h"
#include "events.h"
 
void main(void) {
    /* get IDT location */
    struct i386_gate *IDT_p = get_idt_base(); 
    
    /* Fill a trap/interrupt/task/call gate with particular values.  */
    /* You can find this function at /opt/spede/include/spede/machine/seg.h */
	fill_gate(&IDT_p[KEYBOARD_EVENT], (int)KeyboardEntry, get_cs(), ACC_INTR_GATE, 0);
    
    /* Enable PIC to accept keyboard interrupt */
    outportb(0x21, ~0x02); // 0x21 is PIC mask, ~1 is mask

    /* Enable CPU to handle interrupt*/
    asm volatile ("sti");

    // Loop in place forever
    while (1);
}
