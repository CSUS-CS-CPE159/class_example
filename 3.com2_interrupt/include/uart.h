// uart.h, 159
#ifndef __UART_H__
#define __UART_H__

void PortEntry();

void uart_init();
void uart_writeByte(unsigned char ch);
void uart_writeText(char *pos);
unsigned char uart_readByte();
#endif
