#include "spede.h"
#include "types.h"
#include "data.h"

size_t uniq_id;

void NewKernelProcHandler(void *func, size_t func_size) {
    size_t pid = uniq_id;
    uniq_id++;
    proc_t *proc = &p[pid];    
    proc->pid = pid;
    proc->trapframe = (trapframe_t *)((unsigned)&proc_kernel_stack[pid][STACK_SIZE] - sizeof(trapframe_t));
    proc->trapframe->ds = get_ds();
    proc->trapframe->es = get_es();
    proc->trapframe->fs = get_fs();
    proc->trapframe->gs = get_gs();
    proc->trapframe->eflags = get_eflags() | EF_INTR;
    proc->trapframe->cs = get_cs();
    if (pid != 0){
        proc->trapframe->eip = 0x200000; 
        printf("eip is %x\n", 0x200000);
        memcpy((void*)0x200000, (void *)func, func_size);
    } else {
        proc->trapframe->eip = (unsigned)func;
    }
}


int ksyscall_printf(char *str){
    cons_printf(str);
    printf(str);
    return 0;
}

void SyscallHandler(void){
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
    
    cons_printf("system call %d\n", syscall);
    printf("system call %d\n", syscall);
     
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
