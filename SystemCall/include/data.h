#ifndef _DATA_H_
#define _DATA_H_

#include "types.h"

extern char proc_kernel_stack[10][STACK_SIZE];
extern proc_t p[10];
extern proc_t *active_process;
extern unsigned int uniq_id;
#endif

