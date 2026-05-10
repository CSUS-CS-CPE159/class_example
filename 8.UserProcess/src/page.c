/*
 * page.c — User-process page table construction
 *
 * WHAT THIS FILE DEMONSTRATES:
 *   - How x86 two-level paging works: a page directory (level 1) points to
 *     page tables (level 2), each of which maps 4 KB pages of virtual memory
 *     to physical page frames.
 *   - How a user process gets its own virtual address space by receiving a
 *     private page directory that initially shares the kernel's lower mappings.
 *   - How page permission bits control which privilege ring (kernel vs. user)
 *     can access each page, and whether the page is writable.
 *
 * CONCEPTS TO LEARN:
 *   - Virtual address layout (32-bit x86 with 4 KB pages):
 *       bits [31:22] — page directory index (which page table to use)
 *       bits [21:12] — page table index     (which page frame to use)
 *       bits [11:0]  — byte offset within the page
 *
 *   - Page flags (stored in the low 12 bits of each directory/table entry):
 *       PAGE_PRESENT (bit 0): the entry is valid; the CPU will use it.
 *       PAGE_WRITE   (bit 1): the page is writable; 0 = read-only.
 *       PAGE_USER    (bit 2): Ring-3 code may access this page; 0 = kernel-only.
 *
 *   - User-process virtual memory layout used here:
 *       0x80000000 (2 GB) — user stack  (top of page dir entry 511)
 *       0x80000080         — user code EIP (entry point inside page dir entry 512)
 *
 * HOW IT FITS:
 *   NewUserProcHandler() in handlers.c calls setup_pagetable() to build a
 *   fresh page directory for the new user process.  The resulting CR3 value
 *   is stored in proc->pagetable and loaded into CR3 by context_switch()
 *   whenever that process runs.
 *
 * MEMORY LAYOUT — five pages per user process:
 *   Page 1: page directory       — top-level table with 1024 entries
 *   Page 2: page table for stack — maps one page at virtual 0x80000000
 *   Page 3: stack page           — the actual user stack memory
 *   Page 4: page table for code  — maps one page at virtual 0x80000000 + 4MB
 *   Page 5: code page            — user binary is copied here
 */
#include "spede.h"
#include "data.h"
#include "types.h"
#include "kalloc.h"

/*
 * kpagemake — allocate one blank physical page for use as a page directory
 *             or page table
 *
 * Calls kalloc() to get a 4 KB-aligned page, then zeroes it out so that
 * all entries start as "not present."  A zeroed page directory means no
 * virtual addresses are mapped yet.
 *
 * Returns a pagetable_t (uint32_t pointer) to the new page, or 0 on error.
 */
pagetable_t kpagemake(void){
    pagetable_t kpgtbl = 0;

    kpgtbl = (pagetable_t)kalloc();
    /* Zero all 1024 entries so every VA starts unmapped (PAGE_PRESENT = 0). */
    memset((void *)kpgtbl, 0, PAGE_SIZE);
    return kpgtbl;
}

/*
 * setup_pagetable — build a complete two-level page table for a user process
 *
 * Parameters:
 *   func      — pointer to the user binary in kernel memory (source for copy)
 *   func_size — size of the user binary in bytes
 *
 * Returns a pointer to the new page directory (suitable for loading into CR3).
 *
 * Virtual address mapping created:
 *
 *   Virtual 0x7FFFF000 (dir[511], table[1023])  →  stack physical page
 *     User ESP is initialized to 0x80000000 - 16, which is inside this page.
 *
 *   Virtual 0x80000000 (dir[512], table[0])      →  code physical page
 *     EIP is set to 0x80000080 (the binary's entry point within this page).
 *
 * Why copy the first five kernel page directory entries?
 *   When the user process runs and an interrupt fires, the CPU switches to
 *   kernel mode but the page table is still the user's.  The kernel's code
 *   and data must be reachable, so we copy the first five entries from the
 *   kernel page directory.  This gives the user page table access to kernel
 *   virtual addresses without mapping all of kernel space.
 */
size_t* setup_pagetable(void *func, size_t func_size) {
    size_t *page_directory;
    size_t *page_table;
    size_t *page;

    /* Allocate and zero the new page directory. */
    page_directory = (size_t *)kpagemake();

    /* ---- Inherit kernel mappings ----------------------------------------
     * Copy the first five page-directory entries from the kernel's page table.
     * Each entry covers 4 MB of virtual address space (1024 pages × 4 KB).
     * Entries 0–4 cover VA 0x00000000 – 0x013FFFFF, where the kernel lives.
     * Without these entries, the kernel would be unreachable after an
     * interrupt switches from user to kernel mode. */
    size_t *kernel_pagetable_directory = (size_t *)kernel_cr3;
    page_directory[0] = kernel_pagetable_directory[0];
    page_directory[1] = kernel_pagetable_directory[1];
    page_directory[2] = kernel_pagetable_directory[2];
    page_directory[3] = kernel_pagetable_directory[3];
    page_directory[4] = kernel_pagetable_directory[4];

    /* ---- Map the user stack at virtual 0x7FFFF000 ----------------------
     *
     * VA 0x7FFFF000 decoded:
     *   bits [31:22] = 0x1FF = 511  → page_directory[511]
     *   bits [21:12] = 0x3FF = 1023 → page_table[1023]
     *   bits [11:0]  = offset within the 4 KB page
     *
     * The stack grows downward from 0x80000000, so the first page of stack
     * is at VA 0x7FFFF000.  User ESP is set to 0x80000000 - 16 in handlers.c. */
    page_table = (size_t *)kpagemake(); /* page table for stack region */
    page       = (size_t *)kpagemake(); /* physical page that IS the stack */

    /* Link the stack physical page into page_table[1023]. */
    page_table[1023] = (size_t)page | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    /* Link the page table into directory entry 511. */
    page_directory[511] = ((size_t)page_table) | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;

    /* ---- Map the user code at virtual 0x80000000 -----------------------
     *
     * VA 0x80000000 decoded:
     *   bits [31:22] = 0x200 = 512  → page_directory[512]
     *   bits [21:12] = 0x000 = 0    → page_table[0]
     *   bits [11:0]  = offset (0x80 for the entry point)
     *
     * Note: directory entry 512 maps a different 4 MB region than entry 511,
     * even though 0x80000000 = 2 GB looks close to 0x7FFFF000.  Directory 511
     * covers 0x7FC00000–0x7FFFFFFF (stack), directory 512 covers
     * 0x80000000–0x803FFFFF (code). */
    page_table = (size_t *)kpagemake(); /* page table for code region */
    page       = (size_t *)kpagemake(); /* physical page that will hold code */

    /* Link the code physical page into page_table[0]. */
    page_table[0] = (size_t)page | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    /* Link the page table into directory entry 512. */
    page_directory[512] = ((size_t)page_table) | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;

    /* Copy the user binary from its kernel-memory location to the freshly
     * allocated code page.  After this, the virtual address 0x80000000 in
     * the user process's address space contains the actual instructions. */
    memcpy(page, func, func_size);
    printf("Copy function code to a new allocated physical address: 0x%x, 0x%x\n", func, page);

    return page_directory; /* this becomes the process's CR3 value */
}
