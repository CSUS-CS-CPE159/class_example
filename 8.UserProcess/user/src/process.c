typedef enum {
     SYSCALL_NONE,
     SYSCALL_PRINT,
}syscall_t;

char str[] = "This is the first process\n";

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

void Process(void){
    while (1){
        sys_printf(str);
        for(int i = 0; i < 16660000; i++)
            asm("inb $0x80");
    }
}

void main(){
    Process();
}

