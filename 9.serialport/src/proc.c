/**
 * proc.c — User processes for the Serial Port Driver example
 *
 * CPE/CSC 159 - Operating System Pragmatics
 * California State University, Sacramento
 *
 * WHAT THIS FILE DEMONSTRATES:
 *   - How processes are written from the user's perspective: they call system
 *     services (defined in syscall.h) and never touch kernel data directly.
 *   - Two complementary processes:
 *       Init     — monitors the host keyboard (via SPEDE console) and runs
 *                  forever, never blocked by I/O.
 *       TermProc — communicates with a remote terminal over a serial port,
 *                  demonstrating full-duplex I/O using semaphore-synchronized
 *                  port read/write calls.
 *   - The "processes do not use kernel space" discipline: processes interact
 *     with the kernel ONLY through system calls (SemAlloc, PortAlloc, etc.).
 *     This mirrors how real user processes on Linux/UNIX work.
 *
 * HOW IT FITS:
 *   Both processes are created by NewProcHandler() in main.c and queued in
 *   ready_q.  The scheduler picks them based on their time slice and blocking
 *   state.  TermProc blocks frequently (waiting on read/write semaphores),
 *   which allows Init to run in between without starving.
 *
 * PROCESSES DO NOT DIRECTLY ACCESS:
 *   kernel data (data.h), handler functions (handlers.h), or hardware ports.
 *   All kernel interaction must go through the system-call wrappers in syscall.h.
 */

#include "spede.h"      /* cons_xxx below needs this */
#include "syscall.h"    /* SemAlloc, SemWait, SemPost, PortAlloc, PortWrite, PortRead */
#include "data.h"       /* current_pid — read-only for display/debug purposes */
#include "proc.h"       /* prototypes of processes defined here */
#include "handlers.h"

/*
 * Init — the keyboard/console monitor process (PID 1)
 *
 * Init runs forever and is never preempted by a blocking system call.
 * It polls the SPEDE host keyboard (cons_kbhit / cons_getchar) and responds
 * to single-keystroke commands:
 *   'p' — print a greeting string to the SPEDE console
 *   'b' — enter GDB breakpoint (useful for debugging)
 *   'q' — quit the simulation
 *
 * The busy-wait delay (LOOP iterations of "inb $0x80") approximates a
 * 1-second pause per iteration.  This prevents Init from consuming 100% of
 * the CPU when no key is pressed, while still remaining very responsive to
 * key presses (the delay can be interrupted by any timer tick).
 *
 * Note: Init intentionally does NOT use semaphores or port I/O so that it
 * always remains in the READY state and can be scheduled as a fallback when
 * TermProc is blocked.
 */
void Init(void) {
    int i;
    char key;
    char str[] = " Hello, World! Team TestOS: Name1, Name2, Name3\n\r";

    while(1){
        if(cons_kbhit()){       /* check if a key was pressed on the host keyboard */
            key = cons_getchar();

            switch(key){
                case 'p':
                    cons_printf(str);   /* print to SPEDE console */
                    break;
                case 'b':
                    breakpoint();       /* drop into GDB for debugging */
                    break;
                case 'q':
                    exit(0);            /* cleanly terminate the simulation */
            }
        }

        /* Busy-wait ~1 second: each "inb $0x80" ≈ 600 ns; LOOP = 166666 iterations.
         * This yields the CPU on timer ticks during the loop (interrupts are enabled),
         * so Init does not starve other processes during its delay. */
        for(i = 0; i < LOOP; i++){
            asm("inb $0x80");
        }
    }
}

/*
 * TermProc — serial-port terminal I/O process
 *
 * This process demonstrates a complete request/response loop over a serial port:
 *   1. Allocates a serial port (PortAlloc).
 *   2. Enters an infinite loop where it:
 *      a. Sends a prompt string to the remote terminal (PortWrite).
 *      b. Reads a line from the remote terminal (PortRead — blocks until Enter).
 *      c. Logs what was received to the SPEDE console (cons_printf).
 *      d. Repeats with a second prompt (password), then replies.
 *
 * PortWrite and PortRead are blocking calls:
 *   - PortWrite blocks if the TX queue is full (UART is slow).
 *   - PortRead  blocks until the remote user presses Enter.
 *
 * Between these blocking points, the scheduler can run Init or other processes.
 *
 * The "\r\n" line endings in the write strings are required by most terminals:
 *   '\r' moves the cursor to column 0 (carriage return).
 *   '\n' moves the cursor down one line (line feed).
 *   Using only '\n' would leave the cursor at column 0 of the same line.
 */
void TermProc(void){
    char str_read[BUFF_SIZE];           /* buffer for one line of input */
    int my_port = PortAlloc();          /* request a serial port from the kernel */

    while(1){
        PortWrite("Now enter (username): \r\n", my_port);
        PortRead(str_read, my_port);    /* blocks until user presses Enter */
        cons_printf("Read from port #%d: %s\n", my_port, str_read);

        PortWrite("Now enter (password): \r\n", my_port);
        PortRead(str_read, my_port);
        cons_printf("Read from port #%d: %s\n", my_port, str_read);

        /* Send a greeting back to the remote terminal. */
        PortWrite("Hello, Team MyOS here! \r\n", my_port);
    }
}
