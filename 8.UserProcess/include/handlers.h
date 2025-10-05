#ifndef _HANDLERS_H_
#define _HANDLERS_H_


#include "types.h"

void SyscallHandler(void);
void NewUserProcHandler(void *, void *);
void NewKernelProcHandler(void *);

#endif
