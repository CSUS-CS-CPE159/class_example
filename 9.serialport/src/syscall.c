// services.c, 159
#include "spede.h"
#include "events.h"
#include "data.h"

int GetPid(void){             // function receives no arguments, but return an integer
    int pid;
    asm volatile(
	"int %1"
	: "=a"(pid)
	: "i" (GETPID_EVENT)
	: "cc", "memory"
	);
    return pid;
}

void Sleep(int sleep_amount){ // function receives arguments, return an integer
    asm volatile(
	"int %0"
	:
	: "i"(SLEEP_EVENT), "a"(sleep_amount)
	: "cc", "memory"
	);
}

int SemAlloc(int passes){
    int sid;
    asm volatile(
        "int %1"
        : "=a"(sid)
        : "i"(SEMALLOC_EVENT), "a"(passes)     // or 0x66
        : "cc","memory"
    );
    return sid;
}

void SemWait(int sid) {
    asm volatile(
        "int %0"
        :
        : "i"(SEMWAIT_EVENT), "a"(sid)          // or 0x67
        : "cc","memory"
    );
}

void SemPost(int sid) {
    asm volatile(
        "int %0"
        :
        : "i"(SEMPOST_EVENT), "a"(sid)          // or 0x68
        : "cc","memory"
    );
}

int PortAlloc(void){
    int port_num;

    asm volatile("int %1"
    : "=a" (port_num)
    : "i"(PORTALLOC_EVENT)
    : "cc", "memory"
    );
    Sleep(1);
    port[port_num].write_sid = SemAlloc(QUEUE_SIZE);
    port[port_num].read_sid = SemAlloc(0);
    port[port_num].read_q.size = 0;
    return port_num;
}

void PortWrite(char *p, int port_num){
    while (*p != '\0') {
        SemWait(port[port_num].write_sid);

        asm volatile(
            "int %0"
            :
            : "i"(PORTWRITE_EVENT), "a"(*p), "b"(port_num)
            : "cc","memory"
        );
        ++p;
    }	
}

void PortRead(char *p, int port_num) {
    int size = 0;
    for (;;) {
        SemWait(port[port_num].read_sid);
        
	    asm volatile(
            "int %0"
            :
            : "i"(PORTREAD_EVENT), "a"(p), "b"(port_num)
            : "cc","memory"
        );

        if (*p == '\r') break;         // stop on CR (per spec)
        ++p;
        if (++size == BUFF_SIZE - 1) break;
    }
    *p = '\0';                          // overwrite CR with NUL
}
