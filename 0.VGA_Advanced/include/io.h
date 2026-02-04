#ifndef KERNEL_IO_H
#define KERNEL_IO_H

/*
 * Write one byte to an output port
 *
 * @param port  Port address
 * @param val   Value
 */
static inline void outportb(unsigned short port, unsigned char data) {
    asm volatile ("outb %1, %0" : : "dN" (port), "a" (data));
}

/*
 * Read one byte from an input port
 *
 * @param port  Port address
 * @return      Read value
 */
static inline unsigned char inportb(unsigned short port) {
    unsigned char val;
    asm volatile("inb %1, %0" : "=a"(val) : "dN"(port));
    return val;
}

/*
 * Write a 16-bits word to an output port
 *
 * @param port  Port address
 * @param val   Value
 */
static inline void outportw(unsigned short port, unsigned short val) {
    asm volatile("outw %1, %0" : : "a"(val), "Nd"(port));
}

/*
 * Read a 16-bits word from an input port
 *
 * @param port  Port address
 * @return      Read value
 */
static inline unsigned short inportw(unsigned short port) {
    unsigned short val;

    asm volatile("inw %0, %1" : "=a"(val) : "Nd"(port));
    return val;
}

/*
 * Write a 32-bits long word to an output port
 *
 * @param port  Port address
 * @param val   Value
 */
static inline void outportl(unsigned short port, unsigned int val) {
    asm volatile("outl %1, %0" : : "a"(val), "Nd"(port));
}

/*
 * Read a 32-bits long word from an input port
 *
 * @param port  Port address
 * @return      Read value
 */
static inline unsigned int inportl(unsigned short port) {
    unsigned int val;

    asm volatile("inl %0, %1" : "=a"(val) : "Nd"(port));
    return val;
}

#endif
