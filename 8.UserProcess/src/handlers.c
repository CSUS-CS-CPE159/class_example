#include "spede.h"
#include "types.h"
#include "data.h"
#include "page.h"

size_t uniq_id;

void NewUserProcHandler(void *function, void *function_end) {
    size_t pid = uniq_id;
    uniq_id++;
    proc_t *proc = &p[pid];    
    proc->proc_type = PROC_TYPE_USER;
    proc->pid = pid;
    proc->trapframe = (trapframe_t *)((unsigned)&proc_kernel_stack[pid][STACK_SIZE] - sizeof(trapframe_t));

    trapframe_t *trapframe = proc->trapframe;
    /*  A selector is 16 bits. 
    15               3  2   1 0
    +-----------------+---+---+
    |   Index (13b)   |TI |RPL|
    +-----------------+---+---+
    for user mode (ring 3), RPL must be 11. 
    */
    trapframe->cs = 0x33;  // Entry 6 (0x30) as index + 11 = 0x33
    trapframe->ds = 0x3B;
    trapframe->gs = 0x3B;
    trapframe->fs = 0x3B;
    trapframe->es = 0x3B;
    trapframe->user_ss =  0x43;  /*User-mode stacksegment*/

    /* User-mode stack segment points to virtual address 2 GB -sizeof (TF) */
    trapframe->user_esp = 0x80000000 - sizeof(trapframe_t);
    trapframe->eflags = get_eflags() | EF_INTR; /*enable interrupts*/
    trapframe->eip = 0x40000000; /*1 GB virtual address for text section*/
    
    trapframe->esp = (unsigned)proc_kernel_stack[pid];
    trapframe->ebp = trapframe->esp;
    trapframe->eax = trapframe->esp;
    // Initialize page table
    proc->pagetable = setup_pagetable(trapframe, function, (size_t)function_end - (size_t)function);
}
void NewKernelProcHandler(void *func) {
    size_t pid = uniq_id;
    uniq_id++;
    proc_t *proc = &p[pid];    
    proc->proc_type = PROC_TYPE_KERNEL;
    proc->pid = pid;
    proc->trapframe = (trapframe_t *)((unsigned)&proc_kernel_stack[pid][STACK_SIZE] - sizeof(trapframe_t));
    proc->trapframe->ds = get_ds();
    proc->trapframe->es = get_es();
    proc->trapframe->fs = get_fs();
    proc->trapframe->gs = get_gs();
    proc->trapframe->eflags = get_eflags() | EF_INTR;
    proc->trapframe->cs = get_cs();
    proc->trapframe->eip = (unsigned)func; 
    proc->pagetable = (size_t *)kernel_cr3;
}


int ksyscall_printf(char *str){
    cons_printf(str);
    printf(str);
    return 0;
}

void SyscallHandler(void){
    printf("*********************This is the syscall function\n");
    /* Default return value */
    int rc = -1;
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

    syscall = active_process->trapframe->eax;
    arg1 = active_process->trapframe->ebx;

    switch(syscall) {
        case SYSCALL_PRINT:
            rc = ksyscall_printf((char *)arg1);
            break;
        default:
            cons_printf("Invalid system call %d\n", syscall);
    }
    if (active_process) {
        active_process->trapframe->eax = (unsigned int)rc;
    }
}
