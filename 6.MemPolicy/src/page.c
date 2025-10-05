#include <spede/stdlib.h>
#include <spede/stdio.h>
#include <spede/string.h>
#include <spede/machine/io.h>
#include <spede/machine/pic.h>
#include <spede/machine/proc_reg.h>
#include "kalloc.h"
#include "process.h"

#define NUM_ENTRIES     1024        // Each Page directory and table has 1024 entries

#define PAGE_PRESENT    0x1
#define PAGE_WRITE      0x2
#define PAGE_USER       0x4

typedef uint32_t pagetable_t;
extern proc_t *active_process;

pagetable_t kpagemake(void){
    pagetable_t kpgtbl = 0;

    kpgtbl = (pagetable_t)kalloc();
    memset((void *)kpgtbl, 0, PAGE_SIZE);
    return kpgtbl;
}

size_t* setup_pagetable() {

    size_t *page_directory;
    size_t *page_table;
    page_directory = (size_t *)kpagemake();
    
    page_table = (size_t *)kpagemake();
    for (size_t i = 0; i < NUM_ENTRIES; i++) {
        // Identity-map the first 4MB of memory 
        page_table[i] = (i*PAGE_SIZE) | PAGE_PRESENT | PAGE_WRITE ;
    }
    // Point the first Entry in the page directory to the first page table
    // First entry: 0x4000 (one page size, byte) times 1024 entries = 4MB, 0x400000
    page_directory[0] = ((size_t)page_table) | PAGE_PRESENT | PAGE_WRITE ;

    page_table = (size_t *)kpagemake();
    for (size_t i = 0; i < NUM_ENTRIES; i++) {
        page_table[i] = (i*PAGE_SIZE + PAGE_SIZE*NUM_ENTRIES) | PAGE_PRESENT | PAGE_WRITE;
    }
    // Point the second Entry in the page directory to the second page table
    //Second entry: 4MB to 8MB 
    page_directory[1] = ((size_t)page_table) | PAGE_PRESENT | PAGE_WRITE ;

    return page_directory;
}

void enable_page_table(size_t *page_directory){
    // Load the page directory base into the CR3 register 
    asm volatile ("mov %0, %%cr3" :: "r"(page_directory));

    size_t cr4;
    asm volatile ("mov %%cr4, %0" : "=r"(cr4));	
    cr4 |=  0x00000010; // Set PSE
    asm volatile ("mov %0, %%cr4" :: "r"(cr4));

    // Enable paging by setting the PG bit (bit 31) in the CRO register 
    size_t cr0; 
    asm volatile ("mov %%cr0, %0" : "=r"(cr0));	
    cr0 |=  0x80000000; // Set PG bit
    asm volatile ("mov %0, %%cr0" :: "r"(cr0));
}

void page_fault_handler(void){

    size_t va; /* virtual address*/
    asm volatile ("mov %%cr2, %0" : "=r"(va));	
   
    cons_printf("page fault error 0x%x, from process 0x%x\n", va, active_process);
    printf("page fault error 0x%x, from process 0x%x\n", va, active_process);

    size_t pd_index = va >> 22; 
    size_t pt_index = va >> 12 & 0x03FF;

    printf("pd_index %d, pt_index %d\n", pd_index, pt_index); 
    size_t *page_directory = active_process->pagetable;
    size_t *page_table = 0;

    if (page_directory[pd_index] == 0) {
        // Check page directory entry is empty or not
        page_table = (size_t *)kpagemake();
        printf("Allocate a new page for page_table entry, address is 0x%x\n", page_table);
        page_directory[pd_index] = (size_t)page_table | PAGE_PRESENT | PAGE_WRITE|PAGE_USER;
    }else {
        page_table = (size_t *)page_directory[pd_index];
    }
    
    size_t new_page = (size_t)kpagemake();
    printf("New page for data, address is 0x%x\n", (uint32_t)new_page);
    page_table[pt_index] = new_page | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    //asm volatile ("invlpg (%0)" :: "r"(virtual_address) : "memory");
    // Update page table for process 
    enable_page_table(active_process->pagetable);
}
