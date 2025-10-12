#include "spede.h"
#include "data.h"
#include "types.h"
#include "kalloc.h"

pagetable_t kpagemake(void){
    pagetable_t kpgtbl = 0;

    kpgtbl = (pagetable_t)kalloc();
    memset((void *)kpgtbl, 0, PAGE_SIZE);
    return kpgtbl;
}

size_t* setup_pagetable(void *func, size_t func_size) {
    /* We need five page for each process
     * page 1: page directory
     * page 2: page table for stack 
     * page 3: stack page for process
     * page 4: page table for code 
     * page 5: code page for process
     * */
    size_t *page_directory;
    size_t *page_table;
    size_t *page;

    page_directory = (size_t *)kpagemake();

    /* Copy the first four entries in the kernel page table*/
    size_t *kernel_pagetable_directory = (size_t *)kernel_cr3;
    page_directory[0] = kernel_pagetable_directory[0];
    page_directory[1] = kernel_pagetable_directory[1];
    page_directory[2] = kernel_pagetable_directory[2];
    page_directory[3] = kernel_pagetable_directory[3];
    page_directory[4] = kernel_pagetable_directory[4];


    /* handle stack page */     
    page_table = (size_t *)kpagemake();
    page = (size_t *)kpagemake();
    page_table[1023] =  (size_t)page | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    // Allocate 0x8000 0000 to 0x8000 1000 for stack, 2GB
    page_directory[511] = ((size_t)page_table) | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;

    /* handle code page */ 
    page_table = (size_t *)kpagemake();
    page = (size_t *)kpagemake();
    
    page_table[0] =  (size_t)page | PAGE_PRESENT | PAGE_WRITE |PAGE_USER;
    // Allocate 0x4000 0000 to 0x4000 1000 for code section, 1GB
    page_directory[256] = ((size_t)page_table) | PAGE_PRESENT | PAGE_WRITE |PAGE_USER;
    
    // Copy code to physical page 
    memcpy(page, func, func_size);
    printf("Copy function code to a new allocated physical address: 0x%x, 0x%x\n", func, page); 
    return page_directory;
}
