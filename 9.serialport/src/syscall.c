/**
 * syscall.c — User-library system-call wrappers
 *
 * CPE/CSC 159 - Operating System Pragmatics
 * California State University, Sacramento
 *
 * WHAT THIS FILE DEMONSTRATES:
 *   - How user-space (or kernel-space) code invokes kernel services by
 *     triggering a software interrupt via "int <event_num>".
 *   - The register-based calling convention for this OS: arguments go in
 *     EAX (and EBX, ECX for additional arguments); return value comes back
 *     in EAX after the interrupt handler stores it in the trapframe.
 *   - How semaphores are used as flow-control mechanisms for port I/O:
 *       PortWrite() waits on write_sid before each byte to prevent overflowing
 *         the TX queue (QUEUE_SIZE slots available).
 *       PortRead()  waits on read_sid before each byte to block until the
 *         hardware has received a character (starts with 0 passes, so the
 *         first call always blocks until the UART fires an RX interrupt).
 *
 * HOW IT FITS:
 *   These functions are the "user-space API" for the kernel's services.
 *   Processes in proc.c call SemAlloc/SemWait/SemPost and PortAlloc/
 *   PortWrite/PortRead.  Each function uses inline assembly to load
 *   arguments into registers and trigger the corresponding software interrupt,
 *   which causes the CPU to jump to the matching ENTRY() in events.S, build
 *   a trapframe, and call the handler in handlers.c.
 *
 * INLINE ASSEMBLY QUICK REFERENCE:
 *   asm volatile("instruction"
 *       : output-operands       (e.g. "=a"(var) → store EAX in var)
 *       : input-operands        (e.g. "a"(val)  → load val into EAX before instruction)
 *       : clobbers              (registers this code modifies that GCC must not assume)
 *   );
 *   "i"(CONST)  — immediate: compiler embeds the constant directly.
 *   "a"(x)      — use EAX register for x.
 *   "b"(x)      — use EBX register for x.
 *   "cc"        — flags register is modified.
 *   "memory"    — tell GCC this code may read/write arbitrary memory.
 */
#include "spede.h"
#include "events.h"
#include "data.h"

/*
 * SemAlloc — allocate a new semaphore with 'passes' initial tokens
 *
 * Parameter:
 *   passes — number of times SemWait can succeed without blocking.
 *            Use 0 to create a "start blocked" semaphore (readers waiting
 *            for data).  Use QUEUE_SIZE for a flow-control semaphore that
 *            limits how many items can be buffered.
 *
 * Returns the semaphore ID (an index into the kernel's sem[] array).
 * The kernel writes the ID to EAX in the trapframe; "=a"(sid) reads it back.
 */
int SemAlloc(int passes){
    int sid;
    asm volatile(
        "int %1"
        : "=a"(sid)                        /* output: EAX → sid */
        : "i"(SEMALLOC_EVENT), "a"(passes) /* input:  EAX = passes; interrupt # = 0x66 */
        : "cc","memory"
    );
    return sid;
}

/*
 * SemWait — wait (P operation) on semaphore 'sid'
 *
 * If the semaphore has passes > 0, decrement it and return immediately.
 * If passes == 0, block the calling process until SemPost is called.
 *
 * Parameter:
 *   sid — semaphore ID from SemAlloc
 */
void SemWait(int sid) {
    asm volatile(
        "int %0"
        :
        : "i"(SEMWAIT_EVENT), "a"(sid)  /* EAX = sid; interrupt # = 0x67 */
        : "cc","memory"
    );
}

/*
 * SemPost — signal (V operation) on semaphore 'sid'
 *
 * Increments passes, or directly wakes the first blocked process.
 *
 * Parameter:
 *   sid — semaphore ID from SemAlloc
 */
void SemPost(int sid) {
    asm volatile(
        "int %0"
        :
        : "i"(SEMPOST_EVENT), "a"(sid)  /* EAX = sid; interrupt # = 0x68 */
        : "cc","memory"
    );
}

/*
 * PortAlloc — allocate and initialize a serial port
 *
 * Returns the port number (index into the kernel's port[] array).
 *
 * After the kernel assigns the hardware port (PortAllocHandler), we set up
 * two semaphores to manage the data flow for this port:
 *   write_sid: starts with QUEUE_SIZE passes so the writer can fill the TX
 *              queue before blocking.  Each PortWrite() call consumes one
 *              pass; each completed TX (in PortWriteOne) posts one pass back.
 *   read_sid:  starts with 0 passes so PortRead() always blocks until the
 *              UART delivers a character (PortReadOne posts one pass per byte).
 *
 * Also resets read_q.size = 0 to ensure the local queue starts empty
 * (the kernel zeroed the port_t struct, but this makes the intent explicit).
 */
int PortAlloc(void){
    int port_num;

    /* Ask the kernel to allocate and program the UART hardware. */
    asm volatile("int %1"
        : "=a"(port_num)
        : "i"(PORTALLOC_EVENT)
        : "cc", "memory"
    );

    /* Set up TX flow-control: QUEUE_SIZE slots available in write_q. */
    port[port_num].write_sid = SemAlloc(QUEUE_SIZE);
    /* Set up RX synchronization: block until data arrives. */
    port[port_num].read_sid  = SemAlloc(0);
    port[port_num].read_q.size = 0;

    return port_num;
}

/*
 * PortWrite — send a null-terminated string to a serial port
 *
 * Parameters:
 *   p        — pointer to the null-terminated string to send
 *   port_num — port index from PortAlloc
 *
 * For each character, this function:
 *   1. Calls SemWait(write_sid) to ensure there is a free slot in the TX queue.
 *      (Blocks if the queue is full, i.e. the UART is slower than the caller.)
 *   2. Calls the PORTWRITE_EVENT system call to hand the character to the kernel.
 *      The kernel's PortWriteHandler() enqueues it, and PortWriteOne() sends
 *      it to the UART when a TXRDY interrupt fires (or immediately if idle).
 *
 * EAX = the character (*p cast to int), EBX = port_num.
 */
void PortWrite(char *p, int port_num){
    while (*p != '\0') {
        /* Block until there is space in the TX queue. */
        SemWait(port[port_num].write_sid);

        asm volatile(
            "int %0"
            :
            : "i"(PORTWRITE_EVENT), "a"(*p), "b"(port_num) /* EAX=char, EBX=port# */
            : "cc","memory"
        );
        ++p;
    }
}

/*
 * PortRead — read one line from a serial port into a buffer
 *
 * Parameters:
 *   p        — pointer to the buffer where received characters will be stored
 *   port_num — port index from PortAlloc
 *
 * Reads characters one at a time until a carriage return ('\r') is received
 * or the buffer is full (BUFF_SIZE - 1 characters).  Appends a NUL terminator.
 *
 * For each character:
 *   1. Calls SemWait(read_sid) to block until a character is available.
 *      (The UART's RX interrupt → PortReadOne() posts read_sid for each byte.)
 *   2. Calls PORTREAD_EVENT to dequeue one character from the kernel's read_q.
 *
 * EAX = pointer to the destination char (p), EBX = port_num.
 * The kernel's PortReadHandler() stores the character at *p.
 */
void PortRead(char *p, int port_num) {
    int size = 0;
    for (;;) {
        /* Block until at least one character is in the receive buffer. */
        SemWait(port[port_num].read_sid);

        asm volatile(
            "int %0"
            :
            : "i"(PORTREAD_EVENT), "a"(p), "b"(port_num)  /* EAX=buf ptr, EBX=port# */
            : "cc","memory"
        );

        if (*p == '\r') break;              /* stop reading at carriage return */
        ++p;
        if (++size == BUFF_SIZE - 1) break; /* guard against buffer overflow */
    }
    *p = '\0';  /* null-terminate the string (overwrites the '\r' or the last char) */
}
