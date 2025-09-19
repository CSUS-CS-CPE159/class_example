#include "spede.h"
#include "types.h"
#include "data.h"

size_t seg_addr[5][1024] __attribute__((aligned(4096)));

size_t uniq_id;

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
    if (pid != 0){
        proc->trapframe->eip = (unsigned)seg_addr[pid]; 
        printf("eip is %x\n", seg_addr[pid]);
        memcpy((void*)seg_addr[pid], (void *)func, 512);
    } else {
        proc->trapframe->eip = (unsigned)func;
    }
}

void GeneralProtectionFaultHandler(void){
    size_t error_code = active_process->trapframe->error_code;
    
    /* General Protection Fault was caused by a descriptor privilige violation */ 
    if (error_code != 0) {
        printf("Process[%d]: General Protection: Error code[0x%x]\n",active_process->pid, error_code);
        cons_printf("Process[%d]: Kernel Panic in General Protection Handler: General Protection Fault, Error Code[0x%x]\n", active_process->pid, error_code); 
        return; 
    } else {
        cons_printf("Process[%d]: Kernel Panic in General Protection Handler: protection violation\n", active_process->pid); 
    }
}

void PageFaultHandler(void){
    size_t error_code = active_process->trapframe->error_code; 
    printf("Page Fault: Error Code[0x%x], Process %d\n", error_code, active_process->pid);
}

