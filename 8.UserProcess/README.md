
### Context Switch
* This document helps people understand the context switch.
* CS register can't manual update. You need to use interrupt or other way. 
* Carefully understand the context switch.
* reference:
    - https://www.felixcloutier.com/x86/pusha:pushad

### Stupid mistakes I made
* typed struct error
```
typed struct proc_t {...} proc_t;
```
* Memcpy error. 
```
memcpy();
```
* protected mode to user mode or user mode to protected mode
    - the hardware will put different value into stack
    - No mode change: push eflags, cs, and eip
    - mode change: push user\_ss, user\_esp, eflags, cs, and eip

* Page Fault
    - Hardware will push "error code" to stack
    - https://wiki.osdev.org/Exceptions
    - CR2 register is used to store the virtual address which caused the Page Faults.

* General Protection
    - Haredware will push "error code" to stack
    
### Attention
* We can't use printf or cons_printf function which will cause General Protection Error.
* Stack page: we should copy content to the location: stack + page_size.


