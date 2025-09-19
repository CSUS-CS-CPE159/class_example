#include "spede.h"
#include "handlers.h"
#include "data.h"
#include "events.h"

void SystemProc(void){
    outportb(0x21, ~0x01); // 0x21 is PIC mask, ~1 is mask
    asm ("sti");    
    while (1){
        cons_printf("This is the system process\n");
        printf("This is the system process\n");
        for(uint32_t i = 0; i< 16660000; i++)
            asm("inb $0x80");
    }
}

void sys_printf(char *);

void Process1(void){
    while (1){
        sys_printf("This is the first process\n");
        for(uint32_t i = 0; i < 16660000; i++)
            asm("inb $0x80");
    }
}

/* We need to define a syscall function at user space and copy this code to user page.
 * We write this code to manually call a software interrupt. Therefore, we need to use 
 * a suitable interrupt number 0x80 (128) to trap into kernel states. We use a syscall number to specify to syscall function. 
 * */
void sys_printf(char *str){
    int rc = -1;
    asm("movl %1, %%eax;"       /* Data copied into registers to be sent to the kernel*/
        "movl %2, %%ebx;" 
        "int $0x80;"            /* Trigger a software interrupt to perform context switch*/
        "movl %%eax, %0;"       
        : "=g"(rc)              /* Operands indicate data received from the kernel*/
        : "g"(SYSCALL_PRINT), "g"(str)  /* Operands indicate data sent to the kernel*/
        /* Register used in the operation so the compiler can optimize/save/restore*/
        : "%eax", "%ebx");     
}
extern void Process1_code_end();
__asm__(".global Process1_code_end\n"
        "Process1_code_end:");
