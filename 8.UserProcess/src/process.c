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

