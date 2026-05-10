/**
 * handlers.c -- CPU Exception Handlers
 * 
 * Contains the C-language exception handlers that are called from
 * assembly-language interrupt service routines (ISRs).
 * 
 * These handlers execute in the exception context after all CPU state
 * has been saved by the ISR.
 */
#include <spede/flames.h>
#include <spede/machine/asmacros.h>
#include <spede/machine/io.h>
#include <spede/machine/pic.h>
#include <spede/machine/proc_reg.h>
#include <spede/machine/seg.h>
#include <spede/stdio.h>
#include <spede/string.h>

/**
 * Handler for divide-by-zero exception (exception 0)
 * 
 * Called when the CPU detects a division by zero operation.
 * This handler runs at the kernel privilege level (ring 0).
 * 
 * In a real OS:
 * - The exception might kill the user process
 * - Or provide error recovery
 * - Or allow the process to handle it with a signal
 * 
 * In this example:
 * - We simply print an error message
 * - Program execution continues (problematic in real systems!)
 */
void _interrupt_0(){
    cons_printf("\n exception: Divide Error \n");
    
    /* In a real OS, we would:
     * 1. Save register state for debugging
     * 2. Identify which process caused the exception
     * 3. Terminate the process or send a signal
     * 4. Allow recovery if possible
     */
}
