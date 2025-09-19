#ifndef _HANDLERS_H_
#define _HANDLERS_H_


#include "types.h"

void PageFaultHandler(void);
void GeneralProtectionFaultHandler(void);
void NewKernelProcHandler(void *);

#endif
