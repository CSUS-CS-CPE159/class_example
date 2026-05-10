/*
 * kalloc.c — Physical page allocator (free-list allocator)
 *
 * WHAT THIS FILE DEMONSTRATES:
 *   - How an operating system can manage physical memory using a simple
 *     linked free list, where each free page stores a pointer to the next.
 *   - The difference between physical addresses (real hardware RAM locations)
 *     and virtual addresses (what a process sees through the page table).
 *   - Page alignment: every allocation must start on a 4 KB boundary so the
 *     CPU's paging hardware can map it correctly.
 *
 * CONCEPTS TO LEARN:
 *   - Free list: the allocator keeps a singly-linked list of available pages.
 *     kfree() inserts a page at the head; kalloc() removes from the head.
 *     The first word of each free page is reused as the 'next' pointer, so
 *     there is zero extra metadata storage overhead.
 *   - PAGE_ALIGN: rounds an address UP to the nearest 4 KB boundary so the
 *     allocator only hands out properly-aligned pages.
 *   - Sentinels: kfree() writes 0x01 to the freed page (to catch use-after-
 *     free bugs), and kalloc() writes 0x05 to the allocated page (to catch
 *     uninitialized-memory bugs).  Real kernels use similar techniques.
 *
 * HOW IT FITS:
 *   main.c calls kinit() once at boot with the array of 50 pre-allocated pages
 *   (page_address[][]).  After that, setup_pagetable() in page.c calls kalloc()
 *   to obtain individual physical pages for page directories, page tables, and
 *   user code/stack pages.
 */
#include "spede.h"
#include "kalloc.h"

/* Physical address limit for safety checks in kfree(). */
uint32_t end = PAGE_STOP;

/*
 * struct run — overlaid on top of each free page
 *
 * Each free page's first 4 bytes are reused to store a pointer to the next
 * free page.  This avoids the need for a separate metadata array.
 */
struct run {
    struct run *next;
};

/* kmem — the allocator's global state (just the free list head). */
struct {
    struct run *freelist;
} kmem;

/*
 * kinit — initialize the allocator over a range of memory
 *
 * Parameters:
 *   vstart — address of the first byte available for allocation
 *   vend   — address one byte past the last byte available
 *
 * Delegates to freerange() which page-aligns vstart and iteratively
 * calls kfree() to add every page to the free list.
 */
void kinit(void *vstart, void *vend){
    freerange(vstart, vend);
}

/*
 * freerange — add all page-aligned pages in [vstart, vend) to the free list
 *
 * PAGE_ALIGN rounds vstart up to the next 4 KB boundary.  The loop then
 * walks forward one page at a time, calling kfree() on each page address.
 */
void freerange(void *vstart, void *vend){
    uint32_t p;
    p = PAGE_ALIGN((uint32_t)vstart);

    for(; p + PAGE_SIZE <= (uint32_t)vend; p += PAGE_SIZE)
        kfree((void *)p);
}

/*
 * kfree — return a single page to the allocator
 *
 * Parameter:
 *   va — the virtual address of the page (must be 4 KB-aligned)
 *
 * The page is filled with 0x01 to catch use-after-free errors: any code
 * that reads from the page after freeing it will see garbage, not valid data.
 *
 * The page is then prepended to the free list by casting it to a 'struct run'
 * and linking it in front of kmem.freelist.
 */
void kfree(void *va){
    struct run *r;

    /* Validate alignment and bounds before accepting the page. */
    if (((uint32_t)va % PAGE_SIZE != 0) || (uint32_t)va >= end){
        printf("kfree error!");
        return;
    }

    /* Overwrite the page contents to catch use-after-free bugs. */
    memset(va, 1, PAGE_SIZE);

    /* Push onto the free list: reuse the page's first bytes as a pointer. */
    r = (struct run *)va;
    r->next = kmem.freelist;
    kmem.freelist = r;
}

/*
 * kalloc — allocate one physical page (4 KB)
 *
 * Returns a pointer to a 4 KB-aligned page, or NULL if no pages are left.
 *
 * The allocated page is filled with 0x05 to catch use of uninitialized
 * memory: any code that forgets to initialize the page before reading it
 * will see 0x05 values instead of zeros or previous data.
 */
void* kalloc(void){
    struct run *r;

    /* Pop from the head of the free list. */
    r = kmem.freelist;
    if (r) {
        kmem.freelist = r->next;
        /* Overwrite freed-page poison (0x01) with new-page marker (0x05).
         * Also clears the 'next' pointer that was written by kfree. */
        memset((char*)r, 5, PAGE_SIZE);
    } else{
        printf("Error, can't allocate page\n");
        return (void*)r;
    }
    printf("Kalloc a new page: 0x%x, kmem is %x\n", r, kmem.freelist, kmem.freelist->next);
    return (void*)r;
}
