/*
 * main.c — Kernel entry point for the User Process example
 *
 * WHAT THIS FILE DEMONSTRATES:
 *   - How the kernel sets up hardware tables (GDT, TSS, IDT) at boot
 *   - How to create both a kernel-mode process and a user-mode process
 *   - How context switching works at a high level: each interrupt saves
 *     the current process state, then the kernel picks the next process
 *     and restores its state before returning.
 *
 * CONCEPTS TO LEARN:
 *   - Global Descriptor Table (GDT): defines code/data/stack segments and
 *     their privilege levels (Ring 0 = kernel, Ring 3 = user).
 *   - Task State Segment (TSS): tells the CPU where to find the kernel
 *     stack when an interrupt transitions from user mode to kernel mode.
 *   - Interrupt Descriptor Table (IDT): maps interrupt numbers to handler
 *     functions.  Timer = IRQ 0 → IDT[32]; system call = INT 0x80 → IDT[128].
 *   - Page tables: each process has its own virtual address space.
 *     kernel_cr3 holds the kernel's page-table base so user processes can
 *     inherit kernel mappings.
 *
 * HOW IT FITS:
 *   main() runs once at boot to initialize all hardware structures, allocate
 *   physical pages, create two processes, and then hand control to the first
 *   process via ProcLoader().  After that, context_switch() is called on
 *   every timer tick or system call.
 */
#include "spede.h"
#include "events.h"
#include "page.h"
#include "process.h"
#include "handlers.h"
#include "types.h"
#include "kalloc.h"

struct i386_gate *IDT_p;               /* pointer into the CPU's IDT */
struct i386_tss kernel_tss;            /* kernel Task State Segment */
struct i386_descriptor *GDT_p;         /* pointer into the CPU's GDT */

proc_t p[10];                          /* process table (up to 10 processes) */
char proc_kernel_stack[10][STACK_SIZE];/* one kernel-mode stack per process */
char page_address[50][PAGE_SIZE];      /* pool of 50 physical pages for kalloc */
proc_t *active_process = NULL;         /* which process is currently running */
unsigned int kernel_cr3;               /* kernel's CR3 (page-directory base addr) */

/* These symbols are injected by the linker and point to the embedded user
 * binary (user/src/user.bin).  The kernel copies this binary into a freshly
 * allocated user page so it runs at a fixed virtual address. */
extern unsigned char _binary_src_user_bin_start[];
extern unsigned char _binary_src_user_bin_end[];

int main(){
    struct pseudo_descriptor pseudo_desc;  /* SPEDE helper to locate the GDT */

    /* ------------------------------------------------------------------ */
    /* Step 1: Locate and validate the GDT                                 */
    /* ------------------------------------------------------------------ */
    GDT_p = NULL;
    get_gdt(&pseudo_desc);
    GDT_p = (struct i386_descriptor *)pseudo_desc.linear_base;

    if (GDT_p == NULL)
    {
        cons_printf("Kernel Panic: GDT table is null!\n");
        return 0;
    }

    /* ------------------------------------------------------------------ */
    /* Step 2: Configure the kernel TSS                                     */
    /* ------------------------------------------------------------------ */
    /*
     * The TSS tells the CPU which stack to switch to when an interrupt
     * elevates privilege from Ring 3 (user) to Ring 0 (kernel).
     * The segment selector values come from the GDT layout shown below:
     *
     *   GDT index | selector | purpose
     *   ----------+----------+--------------------
     *      0      |  0x00    | null (required)
     *      1      |  0x08    | kernel code  (Ring 0)
     *      2      |  0x10    | kernel data  (Ring 0)
     *      3      |  0x18    | kernel stack (Ring 0)
     *      4      |  0x20    | (reserved by SPEDE)
     *      5      |  0x28    | (reserved by SPEDE)
     *      6      |  0x30    | user code    (Ring 3)
     *      7      |  0x38    | user data    (Ring 3)
     *      8      |  0x40    | user stack   (Ring 3)
     *      9      |  0x48    | kernel TSS   (filled below)
     */
    kernel_tss.tss_ss0 = 0x18;   /* kernel stack segment for Ring 0 */
    kernel_tss.tss_cs  = 0x8;    /* kernel code segment */
    kernel_tss.tss_ds  = 0x10;   /* kernel data segment */
    kernel_tss.tss_es  = 0x10;
    kernel_tss.tss_fs  = 0x10;
    kernel_tss.tss_ss  = 0x18;
    /* tss_ioopt set to sizeof(tss) disables the I/O permission bitmap,
     * meaning user code cannot access I/O ports directly. */
    kernel_tss.tss_ioopt = sizeof(kernel_tss);

    /* Register the TSS descriptor in GDT entry 9, then load it into TR. */
    fill_descriptor(&GDT_p[9], (unsigned)&kernel_tss, sizeof(kernel_tss)-1, ACC_TSS, 0x0);
    KernelTssFlush(); /* executes LTR to activate the TSS */

    /* ------------------------------------------------------------------ */
    /* Step 3: Initialize the physical page allocator                       */
    /* ------------------------------------------------------------------ */
    printf("initialized kalloc!\n");
    /* kinit() builds a free list from page_address[0..49].
     * The second argument is the address one byte past the last page. */
    kinit(page_address, (unsigned int)page_address + 50 * PAGE_SIZE);

    /* ------------------------------------------------------------------ */
    /* Step 4: Register interrupt handlers in the IDT                       */
    /* ------------------------------------------------------------------ */
    IDT_p = get_idt_base();
    cons_printf("IDT located @ DRAM addr %x (%d).\n", IDT_p, IDT_p);

    /* IDT entry 128 (0x80): system-call gate.
     * ACC_PL_U allows Ring-3 (user) code to invoke INT 0x80 without
     * triggering a general protection fault. */
    fill_gate(&IDT_p[128], (int)SyscallEntry, get_cs(), ACC_INTR_GATE | ACC_PL_U, 0);

    /* IDT entry 32 (0x20): timer interrupt.
     * IRQ 0 from the PIC is remapped to IDT entry 32 by the BIOS. */
    fill_gate(&IDT_p[32], (int)TimerEntry, get_cs(), ACC_INTR_GATE, 0);

    /* ------------------------------------------------------------------ */
    /* Step 5: Save the kernel's page-directory base (CR3)                  */
    /* ------------------------------------------------------------------ */
    /* SPEDE runs with paging already enabled.  We save the kernel's CR3
     * so that user-process page tables can copy the kernel mappings
     * (allowing the kernel to still be accessible when handling interrupts
     *  from user space). */
    kernel_cr3 = get_cr3();

    /* ------------------------------------------------------------------ */
    /* Step 6: Create processes and start the first one                     */
    /* ------------------------------------------------------------------ */
    NewKernelProcHandler(SystemProc);  /* p[0]: kernel-mode process */
    /* p[1]: user-mode process; the binary is embedded via objcopy */
    NewUserProcHandler(_binary_src_user_bin_start, _binary_src_user_bin_end);

    /* Begin execution with p[0] (the kernel process). */
    active_process = &p[0];
    /* Point the TSS kernel-stack to p[0]'s stack top so that when a
     * hardware interrupt fires, the CPU can find the right stack. */
    kernel_tss.tss_esp0 = (unsigned)&proc_kernel_stack[0][STACK_SIZE];
    ProcLoader(active_process->trapframe);    /* jump into the first process */
    return 0;
}


/*
 * context_switch — called from events.S on every timer tick or system call
 *
 * On entry, 'current' points to the trapframe that events.S built on the
 * kernel stack.  The trapframe captures the full CPU state of the process
 * that was interrupted, so we can resume it later.
 *
 * This simple scheduler alternates between p[0] (kernel) and p[1] (user).
 * A real OS would have a proper ready queue here.
 */
void context_switch(trapframe_t *current) {
    if (current->event_type == 0x80){
        /* System call: dispatch to SyscallHandler, then return to the
         * same process (no process switch needed). */
        SyscallHandler();
        return;
    } else{
        /* Timer interrupt: acknowledge it to the PIC (Programmable
         * Interrupt Controller) so future timer interrupts are delivered.
         * Writing 0x60 to port 0x20 sends an EOI (End-Of-Interrupt). */
        outportb(0x20, 0x60);

        /* Save the current trapframe pointer into the process's PCB so
         * we can restore this process later. */
        active_process->trapframe = current;

        /* Alternate between the kernel process (p[0]) and the user
         * process (p[1]) in round-robin fashion. */
        if (active_process == &p[0]){
            active_process = &p[1];   /* switch to user process */
        } else {
            active_process = &p[0];   /* switch back to kernel process */
        }

        /* Update the TSS so the CPU knows which kernel stack to use if
         * the newly scheduled process is preempted by an interrupt. */
        kernel_tss.tss_esp0 = (unsigned)&proc_kernel_stack[active_process->pid][STACK_SIZE];

        /* Switch the virtual address space to the new process's page table. */
        set_cr3((unsigned int)active_process->pagetable);
    }
    /* Restore the selected process's register state and resume it. */
    ProcLoader(active_process->trapframe);
}
