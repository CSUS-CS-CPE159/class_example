#include <spede/flames.h> // IO_DELAY(), breakpoint(), cons_putchar(), cons_getchar(), cons_kbhit(),
#include <spede/machine/io.h> // inportb(), outportb(), inportw(), etc.
#include "spede/machine/rs232.h"

#define PIC_CONTROL 0x20   // PIC control register
#define EOI 0x20            //EOI (End of Interrupt)
// COM2, COM3, COM4 address: {0x2f8, 0x3e8, 0x2e8};
int port = 0x2f8;

void uart_init() {
	// Program UART to 9600 7-E-1, enable RX/TX interrupts
  	int baud = 9600;
  	int divisor = 115200 / baud;
    	
	outportb(port + IER, 0x0);  // Disable all interrupts
  	outportb(port + CFCR, CFCR_DLAB);		// Enable DLAB (set baud rate divisor)
  	outportb(port + BAUDLO, (unsigned char)(divisor & 0xFF)); // set divisor
  	outportb(port + BAUDHI, (unsigned char)((divisor >> 8) & 0xFF));
	outportb(port + CFCR, CFCR_PEVEN | CFCR_PENAB | CFCR_7BITS); // 7 bits, Even Parity, and 1 stop bit
    outportb(port + MCR, MCR_IENABLE | MCR_DTR | MCR_RTS);  // IRQs enabled, RTS/DSR set
  	// small delay for hardware settle	
	for (int i = 0; i<= 0x63; ++i) asm volatile("inb $0x80");
    // Re-enable interrupts (Recieved data & transmitter holding register empty)
  	outportb(port + IER, IER_ERXRDY | IER_ETXRDY);
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

void uart_writeText(char *pos) {
    while (*pos) {
       if (*pos == '\n') uart_writeByte('\r');
       uart_writeByte(*pos++);
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

void uart_interrupt_handler(){
	unsigned char iir = inportb(port + IIR);
    // Check if bit 0 is 0 (Interrupt Pending)
    if (!(iir & 0x01)) {
        unsigned char cause = iir & 0x0E;

        if (cause == IIR_RXRDY || cause == IIR_RXTOUT) {
            // Read character and echo it back
            unsigned char ch = uart_readByte();
            
            // Optional: Handle Newline for Minicom
            if (ch == '\r' || ch == '\n') {
                uart_writeByte('\r');
                uart_writeByte('\n');
            } else {
                uart_writeByte(ch);
            }
            cons_printf("Echoed: %c\n", ch);
        }
        if (cause == IIR_TXRDY) {
            // we need to use a queue to send large text
        }
    }
    
    // Send EOI to PIC
    outportb(PIC_CONTROL, EOI); 
}
