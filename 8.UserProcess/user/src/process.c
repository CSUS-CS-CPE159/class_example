/*
 * user/src/process.c — User-space process code
 *
 * WHAT THIS FILE DEMONSTRATES:
 *   - How user-mode code requests kernel services using a software interrupt
 *     (INT 0x80).  This is the classic Linux/UNIX system-call mechanism.
 *   - How to write an inline assembly system call in GCC, passing arguments
 *     in registers and receiving a return value.
 *   - Why user processes cannot call kernel functions directly: they run in
 *     Ring 3 and can only cross into Ring 0 through a designated gate.
 *
 * CONCEPTS TO LEARN:
 *   - Software interrupt (INT): a deliberate, programmer-triggered interrupt.
 *     Unlike hardware interrupts (from the timer or keyboard), INT is
 *     synchronous and occurs exactly where the programmer writes it.
 *     INT 0x80 is the conventional system-call gate on x86 Linux systems.
 *
 *   - GCC inline assembly (asm volatile):
 *       "int $0x80"        — the instruction to emit
 *       : "=g"(rc)         — output operand: store EAX result in 'rc'
 *       : "g"(SYSCALL_PRINT), "g"(str)  — input operands
 *       : "%eax", "%ebx"   — clobber list: tell GCC we modified these registers
 *
 *   - Register conventions for this OS's system-call ABI:
 *       EAX — syscall number (input); return value (output)
 *       EBX — first argument
 *
 * HOW IT FITS:
 *   This file is compiled into user/src/user.bin by the user/Makefile and
 *   then embedded into the kernel binary with objcopy.  At boot, main.c
 *   copies user.bin into a freshly allocated physical page mapped at virtual
 *   address 0x80000000 in the user process's page table.
 *   The kernel-side handler for INT 0x80 is SyscallHandler() in handlers.c.
 *
 * NOTE:
 *   The binary produced by compiling this file is flat (no OS loader), so
 *   the linker places main() at a fixed offset.  Verify the actual EIP entry
 *   point with: objdump -D user/src/user.bin
 *   The expected entry address is 0x80000080 (set in handlers.c).
 */

/* syscall_t must match the definition in include/types.h; it is redefined
 * here because the user binary is compiled independently without access to
 * the kernel's include directory. */
typedef enum {
     SYSCALL_NONE,   /* 0 */
     SYSCALL_PRINT,  /* 1 — print a string; EBX = address of the string */
} syscall_t;

void Process();
void sys_printf(char *str);

void main(){
    Process();
}

/*
 * sys_printf — user-space wrapper that asks the kernel to print a string
 *
 * Parameter:
 *   str — pointer to a null-terminated string (in user virtual memory)
 *
 * This function uses inline assembly to set up registers and trigger INT 0x80:
 *   EAX ← SYSCALL_PRINT   (which kernel service to call)
 *   EBX ← str             (the argument: pointer to the string)
 *   INT $0x80              (trap into the kernel)
 *   rc  ← EAX             (kernel writes the return value back to EAX)
 *
 * Inline assembly constraint syntax:
 *   "=g"(rc)              — '=' means output; 'g' lets GCC pick any register
 *                            or memory location; result stored in variable 'rc'
 *   "g"(SYSCALL_PRINT)    — input: compiler may place this in any location
 *   "g"(str)              — input: same flexibility
 *   : "%eax", "%ebx"      — clobber list: we explicitly write these registers,
 *                            so GCC must not assume they hold any prior value
 *
 * The "movl %1, %%eax" and "movl %2, %%ebx" lines copy the C operands into
 * the specific registers the kernel-side ABI requires.
 */
void sys_printf(char *str){
    int rc = -1;
    asm("movl %1, %%eax;"       /* EAX = syscall number (SYSCALL_PRINT) */
        "movl %2, %%ebx;"       /* EBX = first argument (string pointer) */
        "int $0x80;"            /* trap into the kernel */
        "movl %%eax, %0;"       /* rc = return value from EAX */
        : "=g"(rc)              /* output: EAX → rc */
        : "g"(SYSCALL_PRINT), "g"(str)  /* inputs: syscall number, string ptr */
        : "%eax", "%ebx");      /* these registers are modified */
}

/*
 * Process — the main loop of the user process
 *
 * Repeatedly calls sys_printf() to print a message, then burns CPU cycles
 * with a busy-wait delay so the output is visible before the next timer
 * interrupt switches back to the kernel process.
 */
void Process(void){
    char str[] = "This is the first process\n";
    while (1){
        sys_printf(str);
        /* Busy-wait delay: each "inb $0x80" ≈ 600 ns; ~16 million iterations
         * ≈ several seconds on a 1 GHz machine, giving the message time to
         * appear on screen before the process is preempted again. */
        for(int i = 0; i < 16660000; i++)
            asm("inb $0x80");
    }
}
