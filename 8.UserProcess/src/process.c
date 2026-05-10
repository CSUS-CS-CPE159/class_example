/*
 * process.c (kernel side) — SystemProc, the kernel-mode process
 *
 * WHAT THIS FILE DEMONSTRATES:
 *   - A kernel process runs in Ring 0 with full hardware access.
 *   - Interrupt-driven preemption: once STI enables hardware interrupts,
 *     the timer can fire at any time and call context_switch() to give
 *     the CPU to another process.
 *   - A simple software delay using repeated port I/O reads, a common
 *     bare-metal technique before high-resolution sleep functions exist.
 *
 * CONCEPTS TO LEARN:
 *   - PIC (Programmable Interrupt Controller): the 8259 chip that routes
 *     hardware IRQ lines to CPU interrupts.  Writing to port 0x21 sets its
 *     mask register, where a 0-bit enables the corresponding IRQ.
 *     ~0x01 = 0xFE enables only IRQ0 (the timer) and masks everything else.
 *   - STI instruction: sets the Interrupt Enable flag in EFLAGS, allowing
 *     the CPU to respond to hardware interrupts.  Until STI is called,
 *     the timer cannot preempt this process.
 *   - I/O delay via "inb $0x80": reading from I/O port 0x80 (a POST
 *     diagnostic port) takes roughly 600 nanoseconds on PC hardware and is
 *     a standard busy-wait delay technique used in early kernels.
 *
 * HOW IT FITS:
 *   SystemProc is created by NewKernelProcHandler() in main.c and runs as
 *   the first process (PID 0).  It cooperates with the user process (PID 1)
 *   through timer-driven preemption: the timer fires every ~10 ms and
 *   context_switch() alternates between the two processes.
 */
#include "spede.h"
#include "handlers.h"
#include "data.h"
#include "events.h"

void SystemProc(void){
    /* Enable IRQ0 (timer) on the PIC; mask all other IRQs.
     * Port 0x21 is the master PIC interrupt mask register.
     * Writing ~0x01 = 0b11111110 enables only bit 0 (IRQ0 = timer). */
    outportb(0x21, ~0x01);

    /* Allow hardware interrupts.  The timer can now preempt this process. */
    asm ("sti");

    while (1){
        cons_printf("This is the system process\n");
        printf("This is the system process\n");

        /* Busy-wait delay: each "inb $0x80" takes ~600 ns, so 16,660,000
         * iterations ≈ 10 seconds of wall-clock delay on a 1 GHz machine.
         * This gives the user process visible time on screen between prints. */
        for(uint32_t i = 0; i< 16660000; i++)
            asm("inb $0x80");
    }
}
