.global gdt_flush
gdt_flush:
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    mov $0x08, %ax
    jmp $0x08, $next
next:
    ret
