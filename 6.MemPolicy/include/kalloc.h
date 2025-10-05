#define PAGE_SIZE               0x00001000                  // 4096 is 0x1000
#define PHYSTOP                 0x08000000                  // 128MB
#define PAGE_START              0x00400000                  // 4MB
#define PAGE_STOP               0x00800000                  // 8MB

#define PT_PAGE_MASK            (~(PAGE_SIZE-1))    /* Trim offset */
#define PAGE_ALIGN(addr)        (((uint32)(addr)+PAGE_SIZE-1)&PT_PAGE_MASK)


// 1KB      0x00000400
// 2KB      0x00000800
// 4KB      0x00001000
// 8KB      0x00002000
// 16KB     0x00004000
// 32KB     0x00008000
// 64KB     0x00010000
// 128KB    0x00020000
// 256KB    0x00040000
// 512KB    0x00080000
// 1MB      0x00100000
// 2MB      0x00200000
// 4MB      0x00400000
// 8MB      0x00800000
// 16MB     0x01000000
// 32MB     0x02000000
// 64MB     0x04000000
// 128MB    0x08000000
// 256MB    0x10000000
// 512MB    0x20000000
// 1GB      0x40000000
// 4GB      0xFFFFFFFF

void freerange(void *vstart, void *vend);

void kinit();
void* kalloc();
void kfree(void *);