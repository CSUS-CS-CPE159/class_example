// syscall.h 

#ifndef __SYSCALL_H__
#define __SYSCALL_H__

int GetPid(void);
void Sleep(int);

int SemAlloc(int);
void SemWait(int);
void SemPost(int);

void SysPrint(char *);

int PortAlloc(void);
void PortWrite(char *, int);
void PortRead(char *, int);

#endif
