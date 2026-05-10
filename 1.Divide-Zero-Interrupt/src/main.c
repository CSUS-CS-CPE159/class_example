/**
 * Divide-by-Zero Exception Handling Example
 * 
 * Demonstrates how to handle CPU exceptions (divide by zero) using the
 * Interrupt Descriptor Table (IDT).
 * 
 * Key Concepts:
 * - CPU exceptions (division by zero, page fault, etc.) are automatic
 * - Exceptions trigger interrupts through the IDT
 * - Exception handlers must be registered in the IDT before they occur
 * - The divide-by-zero exception is interrupt 0x00
 * 
 * How it works:
 * 1. Program intentionally divides by zero in a loop
 * 2. Hardware generates a divide exception
 * 3. CPU looks up exception handler in IDT[0]
 * 4. Handler prints an error message
 * 5. Program continues (if handler returns properly)
 * 
 * Learning Objectives:
 * - Understand CPU exception hierarchy and IDT structure
 * - Learn how to register exception handlers
 * - See interrupt handler assembly code
 * - Practice exception recovery
 */
#include <spede/flames.h>
#include <spede/machine/asmacros.h>
#include <spede/machine/io.h>
#include <spede/machine/pic.h>
#include <spede/machine/proc_reg.h>
#include <spede/machine/seg.h>
#include <spede/stdio.h>
#include <spede/string.h>

#include "events.h" /* Declares isr0 (exception handler entry point) */

#define LOOP 16660000 /* Busy-loop counter for timing delays */

typedef void (* func_ptr_t)();
struct i386_gate *IDT_p; /* Pointer to Interrupt Descriptor Table */

/**
 * Process function that deliberately triggers exceptions
 * 
 * This function:
 * - Displays 'z' characters (indicating it's running)
 * - Polls for keyboard input
 * - Intentionally divides by zero in each iteration
 * - Without proper exception handling, this would crash the system
 * - With our handler registered, the system recovers and continues
 */
void Process(void){
	size_t i;
    
    while(1){
        /* Poll keyboard - returns true if a key was pressed */
        if( cons_kbhit() ) {
            cons_printf("Stop the current process!");
            break; 
        }

        /* Delay loop - simulates some work being done */
        for (i = 0; i < LOOP; i++)
            asm("inb $0x80"); /* Dummy I/O instruction for timing */

        cons_putchar('z'); /* Print a character to show we're running */

        /* This intentionally causes a divide-by-zero exception */
        /* Without an exception handler, the system would crash here */
        i = 0;
        i = 5 / i;  /* EXCEPTION: Division by zero! */
        /* If you see multiple 'z' characters, the exception was handled */
    }
}

int main(){
    /* Get the address of the Interrupt Descriptor Table (IDT) */
    /* IDT is where the CPU looks up interrupt/exception handlers */
    IDT_p = get_idt_base();
    
    /* Display where the IDT is located in memory */
    cons_printf("IDT located @ DRAM addr %x (%d).\n", IDT_p, IDT_p);

    /**
     * Register the divide-by-zero exception handler
     * 
     * fill_gate parameters:
     * - &IDT_p[0]: IDT entry for exception 0 (divide-by-zero)
     * - (int)isr0: Address of our exception handler function
     * - get_cs(): Code segment selector (kernel code segment)
     * - ACC_INTR_GATE: Gate type = interrupt gate (disables interrupts)
     * - 0: Privilege level (0 = kernel only)
     */
    fill_gate(&IDT_p[0], (int)isr0, get_cs(), ACC_INTR_GATE, 0);
	
    /* Start the process that will trigger exceptions */
    Process();
    return 0;
}
