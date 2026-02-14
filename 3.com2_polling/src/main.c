// Device Driver: "Interrupt Driven"

#include "spede.h"    // given SPEDE stuff
#include "uart.h"

int main() {
    uart_init();
    // send data to host
    uart_writeText("Hello world!\n"); 
    while(1){
        unsigned char incoming = uart_readByte();
        // show data 
        uart_writeByteBlockingActual(incoming);
        // print data at Target VGA  
        cons_printf("%c\n", incoming);
    }; 
    return 0;                
} 
 
