#ifndef __PAGE_H_
#define __PAGE_H_

#include "types.h"
#define PF_ERR_USER_SUP_BIT     4   /* User/Supervisor bit position in PF error code*/
#define PF_ERR_PRES_BIT         1   /* Page present bit position in PF error code*/

size_t* setup_pagetable(trapframe_t *trapframe, void *function, size_t size);
pagetable_t kpagemake(void);

#endif
