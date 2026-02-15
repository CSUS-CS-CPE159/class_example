// Device Driver: "COM2 Polling"
#include <spede/flames.h> // IO_DELAY(), breakpoint(), cons_putchar(), cons_getchar(), cons_kbhit(),

#include "uart.h"

int main() {
    uart_init();
    // send data to host
    uart_writeText("Hello world!\r\n"); 
    while(1){
        unsigned char ch = uart_readByte();
        
        // print data on Minicom 
        // Handle newline for Minicom
        if (ch == '\r' || ch == '\n' )
            uart_writeText("\r\n");
        else
            uart_writeByte(ch);

        // print data at Target VGA  
        cons_printf("%c\n", ch);
    }; 
    return 0;                
} 
 
