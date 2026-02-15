// uart.h, 159
#ifndef __UART_H__
#define __UART_H__

void uart_init();
void uart_writeText(char *buffer);
unsigned char uart_readByte();
void uart_writeByte(unsigned char);
#endif
