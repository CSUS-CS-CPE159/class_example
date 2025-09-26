// Physical memory allocator, for user processes, kernel stacks, page-table pages
#include <spede/string.h>
#include <spede/machine/proc_reg.h>
#include <spede/stdio.h>
#include <kalloc.h>


uint32_t end = PAGE_STOP;

struct run {
    struct run *next;
};

struct {
    struct run *freelist;
}kmem;

void kinit(void *vstart, void *vend){
    freerange(vstart, vend);
}

void freerange(void *vstart, void *vend){
    uint32_t p;
    p = PAGE_ALIGN((uint32_t)vstart);
    
    for(; p+PAGE_SIZE <= (uint32_t)vend; p+=PAGE_SIZE)
        kfree((void *)p);
}

// Free the page of physical memory pointed at by v

void kfree(void *va){
    struct run *r;

    if (((uint32_t)va % PAGE_SIZE != 0) || (uint32_t)va >= end){
        printf("kfree error!");
        return;
    }
    memset(va, 1, PAGE_SIZE);

    r = (struct run *)va;
    r->next = kmem.freelist;
    kmem.freelist = r;
}

void* kalloc(void){
    struct run *r;

    r = kmem.freelist;
    if (r) {
        kmem.freelist = r->next;
        memset((char*)r, 5, PAGE_SIZE);
    } else{
        printf("Error, can't allocate page\n");
        return (void*)r;
    }
    printf("Kalloc a new page: 0x%x, kmem is %x\n", r, kmem.freelist, kmem.freelist->next);
    return (void*)r;
}