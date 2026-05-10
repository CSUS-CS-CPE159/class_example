/*
 * handlers.c — Process creation and system-call handling
 *
 * WHAT THIS FILE DEMONSTRATES:
 *   - How to initialize a kernel-mode process (runs in Ring 0, shares the
 *     kernel page table, no separate virtual address space).
 *   - How to initialize a user-mode process (runs in Ring 3, gets its own
 *     page table, must use segment selectors with RPL=3).
 *   - How the kernel handles system calls: reading the syscall number and
 *     arguments from the process's saved registers (EAX, EBX), performing
 *     the requested service, and writing the return value back to EAX.
 *
 * CONCEPTS TO LEARN:
 *   - Segment selectors: a 16-bit value where bits [15:3] are the GDT index,
 *     bit 2 is the Table Indicator (0 = GDT), and bits [1:0] are the
 *     Requested Privilege Level (RPL).  For Ring-3 code, RPL must be 11b = 3,
 *     so user CS = 0x30 | 0x3 = 0x33, user DS = 0x38 | 0x3 = 0x3B, etc.
 *   - Trapframe initialization: to "create" a process, we pre-fill a
 *     trapframe so that when ProcLoader runs IRET it looks as if the process
 *     was interrupted at EIP with a clean register set.
 *   - System-call ABI: user code puts the syscall number in EAX, arguments
 *     in EBX (etc.), then executes INT 0x80.  The kernel reads these from
 *     the saved trapframe and writes the return value back to EAX.
 *
 * HOW IT FITS:
 *   main() calls NewKernelProcHandler() and NewUserProcHandler() once at
 *   boot.  SyscallHandler() is called by context_switch() every time the
 *   user process invokes INT 0x80.
 */
#include "spede.h"
#include "types.h"
#include "data.h"
#include "page.h"

/* Monotonically increasing process ID; also used as index into p[] and
 * proc_kernel_stack[].  Starts at 0 so the first process gets PID 0. */
size_t uniq_id;

/*
 * NewUserProcHandler — create and initialize a Ring-3 (user-mode) process
 *
 * Parameters:
 *   function     — pointer to the start of the user binary in kernel memory
 *   function_end — pointer one byte past the end of the user binary
 *
 * This routine:
 *   1. Allocates a PID and a kernel stack for the new process.
 *   2. Places a trapframe at the TOP of the kernel stack (so that when
 *      ProcLoader is called, IRET will land in the user process).
 *   3. Sets the segment registers in the trapframe to Ring-3 selectors,
 *      which restricts the process to user-mode privilege.
 *   4. Sets EIP to the fixed virtual address where the user binary lives
 *      and ESP to the virtual address of the user-mode stack top.
 *   5. Calls setup_pagetable() to allocate and populate a page table that
 *      maps both the user code and the user stack at the expected VAs.
 *
 * Segment selector values (see GDT layout in main.c):
 *   0x33 = GDT[6] (user code)  | RPL 3  →  CS for Ring-3 code
 *   0x3B = GDT[7] (user data)  | RPL 3  →  DS/ES/FS/GS for Ring-3 data
 *   0x43 = GDT[8] (user stack) | RPL 3  →  SS for Ring-3 stack
 */
void NewUserProcHandler(void *function, void *function_end) {
    size_t pid = uniq_id;
    uniq_id++;
    proc_t *proc = &p[pid];
    proc->proc_type = PROC_TYPE_USER;
    proc->pid = pid;

    /* Carve out the trapframe at the very top of this process's kernel stack.
     * When ProcLoader() loads this process, it will point ESP here and execute
     * the pops/IRET sequence to restore all registers and jump to EIP. */
    proc->trapframe = (trapframe_t *)((unsigned)&proc_kernel_stack[pid][STACK_SIZE] - sizeof(trapframe_t));

    trapframe_t *trapframe = proc->trapframe;

    /*
     * Selector format (16 bits):
     *   Bits 15–3: GDT index
     *   Bit  2:    Table Indicator (0 = GDT, 1 = LDT)
     *   Bits 1–0:  RPL (Requested Privilege Level)
     *
     * For user mode, RPL = 3 (binary 11).
     *   GDT index 6 → 6 << 3 = 0x30 → add RPL 3 → 0x33
     *   GDT index 7 → 7 << 3 = 0x38 → add RPL 3 → 0x3B
     *   GDT index 8 → 8 << 3 = 0x40 → add RPL 3 → 0x43
     */
    trapframe->cs     = 0x33;   /* user code segment, RPL 3 */
    trapframe->ds     = 0x3B;   /* user data segment, RPL 3 */
    trapframe->gs     = 0x3B;
    trapframe->fs     = 0x3B;
    trapframe->es     = 0x3B;
    trapframe->user_ss = 0x43;  /* user stack segment, RPL 3 */

    /* The user stack lives at virtual address 2 GB - 16 bytes.
     * setup_pagetable() maps one physical page just below 2 GB for this.
     * Subtracting 16 gives the initial stack pointer some alignment headroom. */
    trapframe->user_esp = 0x80000000 - 16;

    /* Enable hardware interrupts in the saved EFLAGS so the timer can
     * preempt the user process once it starts running. */
    trapframe->eflags = get_eflags() | EF_INTR;

    /* The user binary is copied to virtual address 0x80000080.
     * (0x80000000 is the start of the page; the 0x80 offset is where
     *  the ELF/flat binary's first instruction lands after the header —
     *  verify with: objdump -D user/src/user.bin) */
    trapframe->eip = 0x80000080;

    /* These kernel-side stack fields are used internally; for a user
     * process the CPU's hardware-push mechanism handles ESP/EBP on the
     * user stack automatically at the next interrupt. */
    trapframe->esp = (unsigned)proc_kernel_stack[pid];
    trapframe->ebp = trapframe->esp;
    trapframe->eax = trapframe->esp;

    /* Build a page table mapping the user binary (code) and stack. */
    proc->pagetable = setup_pagetable(function, (size_t)function_end - (size_t)function);
}

/*
 * NewKernelProcHandler — create and initialize a Ring-0 (kernel-mode) process
 *
 * Parameter:
 *   func — address of the C function that will serve as the process's entry point
 *
 * A kernel process:
 *   - Uses the kernel's own segment selectors (copied from current CPU registers).
 *   - Shares the kernel's page table (kernel_cr3), so it can access all kernel
 *     memory directly without a separate virtual address space.
 *   - Starts with interrupts enabled so the timer can preempt it.
 */
void NewKernelProcHandler(void *func) {
    size_t pid = uniq_id;
    uniq_id++;
    proc_t *proc = &p[pid];
    proc->proc_type = PROC_TYPE_KERNEL;
    proc->pid = pid;

    /* Place the trapframe at the top of the kernel stack, same as for
     * user processes.  When this process is first loaded via ProcLoader,
     * IRET will jump to 'func'. */
    proc->trapframe = (trapframe_t *)((unsigned)&proc_kernel_stack[pid][STACK_SIZE] - sizeof(trapframe_t));

    /* Inherit the current segment registers; kernel code and data segments
     * don't change between processes. */
    proc->trapframe->ds     = get_ds();
    proc->trapframe->es     = get_es();
    proc->trapframe->fs     = get_fs();
    proc->trapframe->gs     = get_gs();
    proc->trapframe->eflags = get_eflags() | EF_INTR; /* enable interrupts */
    proc->trapframe->cs     = get_cs();
    proc->trapframe->eip    = (unsigned)func; /* where to start executing */

    /* Reuse the kernel's existing page directory; no virtual address
     * translation is needed beyond what SPEDE already set up. */
    proc->pagetable = (size_t *)kernel_cr3;
}


/*
 * ksyscall_printf — kernel-side implementation of the SYSCALL_PRINT service
 *
 * Prints 'str' to both the SPEDE console and the simulator terminal.
 * Returns 0 on success.
 *
 * Note: the string pointer comes from user space.  In a production kernel
 * we would validate that the pointer is within the user's address space
 * before dereferencing it.
 */
int ksyscall_printf(char *str){
    cons_printf(str);
    printf(str);
    return 0;
}

/*
 * SyscallHandler — dispatch system calls from the active user process
 *
 * Called by context_switch() whenever active_process triggers INT 0x80.
 * The user process sets up the call like this (see user/src/process.c):
 *   EAX = syscall number  (e.g. SYSCALL_PRINT)
 *   EBX = first argument  (e.g. pointer to the string to print)
 *
 * After the handler finishes, the return value is written back to EAX
 * in the trapframe, so the user process sees it when INT returns.
 *
 * Side effects:
 *   - Modifies active_process->trapframe->eax (the syscall return value).
 */
void SyscallHandler(void){
    printf("*********************This is the syscall function\n");
    int rc = -1;    /* default return value: error */
    int syscall;
    unsigned int arg1;

    if (!active_process) {
        cons_printf("Kernel Panic: No active process");
        return;
    }
    if(!active_process->trapframe){
        cons_printf("Kernel Panic: invalid trapframe");
        return;
    }

    /* Read the syscall number and first argument from the saved registers. */
    syscall = active_process->trapframe->eax;
    arg1    = active_process->trapframe->ebx;

    switch(syscall) {
        case SYSCALL_PRINT:
            /* arg1 is the user-space address of a null-terminated string. */
            rc = ksyscall_printf((char *)arg1);
            break;
        default:
            cons_printf("Invalid system call %d\n", syscall);
    }

    /* Write the return code back to EAX in the saved trapframe.
     * When ProcLoader restores this process, EAX will hold 'rc'. */
    if (active_process) {
        active_process->trapframe->eax = (unsigned int)rc;
    }
}
