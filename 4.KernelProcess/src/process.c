#include "spede.h"
#include "handlers.h"
#include "data.h"
#include "events.h"

void SystemProc(void){
 
    outportb(0x21, ~0x01); // 0x21 is PIC mask, ~1 is mask
    asm ("sti"); 

    while (1){
        cons_printf("This is the system thread\n");
        printf("This is the system thread\n");
        for(uint32_t i = 0; i< 16660000; i++)
            asm("inb $0x80");
    }
}

void SystemProc1(void){
    size_t *x = (size_t*)0x200000;
    *x = 1; 
    while (1){
        // printf("This is the system thread 1\n");
        for(uint32_t i = 0; i< 16660000; i++)
            asm("inb $0x80");
        *x = *x + 1;
    }
}
