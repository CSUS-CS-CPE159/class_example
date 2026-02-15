// Device Driver: "Interrupt Driven"
#include <spede/flames.h> // IO_DELAY(), breakpoint(), cons_putchar(), cons_getchar(), cons_kbhit(),
#include <spede/machine/proc_reg.h> // get_idt_base(), get_cs(), set_cr3(), etc
#include <spede/machine/seg.h> // struct i386_gate, fill_gate(), etc.
#include <spede/machine/io.h> // inportb(), outportb(), inportw(), etc.

#include "uart.h"
#define PORT_EVENT 0x23 // PIC IRQ 3 is com2, 0x23 = 32 + 3
#define PIC_DATA 0x21   // PIC data register

int main() {
    uart_init();
    uart_writeText("Hello World!\n"); 

    // Get IDT table address
    struct i386_gate *IDT_p;
    IDT_p = get_idt_base();   // locate IDT location
    fill_gate(&IDT_p[PORT_EVENT], (int)PortEntry, get_cs(), ACC_INTR_GATE, 0);
   
    // Enable PIC com2 interrupt 
    outportb(PIC_DATA, ~0x8);    // PIC mask for IRQ3: com2
    // Enable CPU interrupt
    asm volatile ("sti"); 

    while(1){
        // do nothing; waiting interrupt;
    }
    return 0;                
} 
 
