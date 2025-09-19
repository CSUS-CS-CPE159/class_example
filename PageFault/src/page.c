#include <spede/machine/proc_reg.h>
#include <spede/stdlib.h>
#include <spede/stdio.h>

#define PAGE_SIZE       4096        // Each page is 4KB
#define NUM_ENTRIES     1024        // Each Page directory and table has 1024 entries

#define PAGE_PRESENT    0x1
#define PAGE_WRITE      0x2
#define PAGE_USER       0x4

size_t pg_table[NUM_ENTRIES] __attribute__((aligned(PAGE_SIZE)));
size_t page[NUM_ENTRIES] __attribute__((aligned(PAGE_SIZE)));

void PageFaultHandler(void){
    size_t va; /* virtual address*/
    asm volatile ("mov %%cr2, %0" : "=r"(va));	
    printf("Page Fault: access address[0x%x]\n", va);

    size_t pd_index = va >> 22; 
    size_t pt_index = va >> 12 & 0x03FF;

    size_t *pg_directory = (size_t*)get_cr3();

    printf("pd_index %d, pt_index %d", pd_index, pt_index);    
    // 100 0000  
    // 64 M = 64 * 1024 * 1024 
    pg_table[pt_index] = (size_t)page | PAGE_PRESENT | PAGE_WRITE;

    pg_directory[pd_index] = ((size_t)pg_table) | PAGE_PRESENT | PAGE_WRITE;

}
