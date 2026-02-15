#include <spede/flames.h> // IO_DELAY(), breakpoint(), cons_putchar(), cons_getchar(), cons_kbhit(),
#include <spede/machine/io.h> // inportb(), outportb(), inportw(), etc.
#include "spede/machine/rs232.h"

// COM2, COM3, COM4 address: {0x2f8, 0x3e8, 0x2e8};
int port = 0x2f8;
	
void uart_init() {
	
	// Program UART to 9600 7-E-1, enable RX/TX interrupts
  	int baud = 9600;
  	int divisor = 115200 / baud;
  	outportb(port + CFCR, CFCR_DLAB);		// Enable DLAB (set baud rate divisor)
  	outportb(port + BAUDLO, (unsigned char)(divisor & 0xFF)); // set divisor
  	outportb(port + BAUDHI, (unsigned char)((divisor >> 8) & 0xFF));
	outportb(port + CFCR, CFCR_PEVEN | CFCR_PENAB | CFCR_7BITS); // 7 bits, Even Parity, and 1 stop bit
}

unsigned int uart_isWriteByteReady() { 
    // On x86, we check the Line Status Register (LSR) bit 5 
    // (Transmitter Holding Register Empty)
    unsigned char iir = inportb(port + LSR);
    return iir & 0x20;
}

void uart_writeByte(unsigned char ch) {
    while (!uart_isWriteByteReady()){
    }
  	outportb(port + DATA, (unsigned char)ch);
}

void uart_writeText(char *buffer) {
    while (*buffer) {
       if (*buffer == '\n') uart_writeByte('\r');
       uart_writeByte(*buffer++);
    }
}

// Check if data is waiting to be read
int uart_is_ReadByteReady() {
    // Bit 0 of LSR is "Data Ready" (DR)
    return inportb(port + LSR) & 0x01;
}

// Blocking read: waits until a byte arrives
unsigned char uart_readByte(){
    while (uart_is_ReadByteReady() == 0) {
        // Just sit here and wait for data
    }
    return inportb(port + DATA);
}

