## Serial Port 
### Device drivers Background

Many device drives execute code in two contexts: a <i>top half</i> that runs in a process's kernel thread, and a <i>bottom half</i> that executes at interrupt time. The top half is called via system calls such as read and write that want the device to perform I/O. This code may ask the hardware to start an operation (e.g., ask the disk to read a block); then the code wait for the operation to complete. Eventually, the device completes the operation and raises an interrupt. The driver's interrupt handler, acting as the bottom half, figures out what operation has completed, wakes up a waiting process if appropriate, and tells the hardware to start work on any waiting next operation [1]. 

### Goal
The goal of this phase is to learn how a device driver that handles
two-way communication works and to be incorporated into an OS to serve
processes to interact with users at peripherals such as <tt>terminal</tt>
devices (TTY, teletype)</tt>.


The test will be done by a terminal process that is to send and receive
text data via a serial data port. This is usually called the <i>upper-half</i>
in driver software. The <i>lower half</i> of the software is the handler code
triggered by the electronics of the port. The PC <i>IRQ</i> numbers 3 and 4
are the designated interrupt wires that eventually evoke the OS event handler
<i>PortHandler()</i>.

Besides a hardware port, there is a software <i>port_t</i> data structure
that interfaces both halves of the device driver. It buffers input and
output with the use of semaphores to <i>flow control</i> the buffers.
It also contains other information about the port.

#### The Upper and Lower Halves
A device-driver upper-half is a process that requests a read or
write via I/O data buffers. The lower-half is a handler which
also read/write the I/O buffers but only does this asynchronously
from the upper-half (whence IRQ events occur).

The serial ports in a PC used to connect to terminals are <tt>COM2</tt>
and <tt>COM3</tt> ports. As a port event occurs, either IRQ 3 or 4 is
sent to the PIC. The event numbers are 35 (32 + 3) and 36 ( 32 + 4). The PIC mask must be
set to open up for them  (added to the original timer IRQ 0). The handler
code must in the end dismiss the events (with PIC).

The handler <i><b>PortHandler</i></b> reads an <i>Interrupt Indicator
Register</i> (IIR) from the port circuit. On event <tt>TXRDY</tt>
(Transmit Ready), a character from the port write data buffer is sent
to the port data register. On event <tt>RXRDY</tt> (Receive Ready), the
handler to read from the port data register and enqueue it to the read
data buffer. There are semaphores to flow-control the use of the buffers 
between a process (upper half) and the handler code (lower half).

A special condition must be understood and treated. As a port is reset,
it is ready to accept transmission (a TXRDY event is assumed). Also,
after sending the last character to the port, the subsequent TXRDY will
have no more character to send. So the handler code must save the information
of this port condition in a flag in order to start sending as new transmission
is requested by a process.

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
    // check this file for macro: /opt/spede/include/spede/machine/rs232.h
    port[p].IO = 0x3F8      // COM1
    // Program UART to 9600 7-E-1, enable RX/TX interrupts
    int baud = 9600;
    int divisor = 115200 / baud;

    outportb(port[p].IO + CFCR, CFCR_DLAB);
    outportb(port[p].IO + BAUDLO, (unsigned char)(divisor & 0xFF));
    outportb(port[p].IO + BAUDHI, (unsigned char)((divisor >> 8) & 0xFF));
    outportb(port[p].IO + CFCR, CFCR_PEVEN | CFCR_PENAB | CFCR_7BITS);
    //outportb(port[port_num].IO + CFCR, CFCR_8BITS);
    outportb(port[p].IO + IER, 0x0);
    outportb(port[p].IO + MCR, MCR_DTR | MCR_RTS | MCR_IENABLE);
```

### Receiving data
```
{
...
        unsigned char iir = inportb(port[port_num].IO+IIR);
        if(iir == IIR_RXRDY) PortReadOne(port_num);
...
}
// ---- One-byte RX path (lower half) ----
void PortReadOne(int port_num){
    unsigned char raw = inportb(port[port_num].IO+DATA);
    ...
}
```

### Sending data
```
{
...
        unsigned char iir = inportb(port[port_num].IO+IIR);
        if(iir == IIR_TXRDY) PortWriteOne(port_num);
...
}
// ---- One-byte TX path (lower half) ----
void PortWriteOne(int port_num){
    ...
    // DATA = 0
    outportb(port[port_num].IO + DATA, (unsigned char)one);
}
```


### Reference
[1]. Cox, R., Kaashoek, F., & Morris, R. (2020). xv6: a simple, Unix-like teaching operating system. MIT PDOS.
[2] https://wiki.osdev.org/Serial_Ports
