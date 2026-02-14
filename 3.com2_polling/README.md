## COM2 Example:

#### How to Run Code

##### install minicom
```
apt install minicom
```

#### open a new terminal before you run "spede-run -d"
```
spede-term com2
```

#### Example: initialization
```
	// Program UART to 9600 7-E-1, enable RX/TX interrupts
  	int baud = 9600;
  	int divisor = 115200 / baud;
    	
	outportb(port + IER, 0x0);  // Disable all interrupts
  	outportb(port + CFCR, CFCR_DLAB);		// Enable DLAB (set baud rate divisor)
  	outportb(port + BAUDLO, (unsigned char)(divisor & 0xFF)); // set divisor
  	outportb(port + BAUDHI, (unsigned char)((divisor >> 8) & 0xFF));
	outportb(port + CFCR, CFCR_PEVEN | CFCR_PENAB | CFCR_7BITS); // 7 bits, Even Parity, and 1 stop bit
  	outportb(port + MCR, MCR_DTR | MCR_RTS );  // IRQs enabled, RTS/DSR set
  	// small delay for hardware settle	
	for (int i = 0; i<= 0x63; ++i) asm volatile("inb $0x80");
    // Re-enable interrupts (Recieved data & transmitter holding register empty)
  	outportb(port + IER, 0x03);
```

### Receiving data from host
```
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

```

### Sending data to host
```
unsigned int uart_isWriteByteReady() { 
    // On x86, we check the Line Status Register (LSR) bit 5 
    // (Transmitter Holding Register Empty)
    unsigned char iir = inportb(port + LSR);
    return iir & 0x20;
}

void uart_writeByteBlockingActual(unsigned char ch) {
    while (!uart_isWriteByteReady()){
    }
  	outportb(port + DATA, (unsigned char)ch);
}

void uart_writeText(char *buffer) {
    while (*buffer) {
       if (*buffer == '\n') uart_writeByteBlockingActual('\r');
       uart_writeByteBlockingActual(*buffer++);
    }
}
```
