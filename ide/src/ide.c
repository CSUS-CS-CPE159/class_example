// Simple PIO-based (non-DMA) IDE driver code.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "x86.h"
#include "traps.h"
#include "fs.h"
#include "buf.h"
#include "interrupts.h"

#define SECTOR_SIZE   512
#define IDE_BSY       0x80
#define IDE_DRDY      0x40
#define IDE_DF        0x20
#define IDE_ERR       0x01

#define IDE_CMD_READ  0x20
#define IDE_CMD_WRITE 0x30

#define IDE_DATA_REG       0x1F0
#define IDE_SECTOR_COUNT   0x1F2
#define IDE_LBA_LOW        0x1F3
#define IDE_LBA_MID        0x1F4
#define IDE_LBA_HIGH       0x1F5
#define IDE_DRIVE_HEAD     0x1F6
#define IDE_COMMAND        0x1F7
#define IDE_STATUS         0x1F7

// idequeue points to the buf now being read/written to the disk.
// idequeue->qnext points to the next buf to be processed.

static struct buf *idequeue;

static int havedisk1;
static void idestart(struct buf*);

// Wait for IDE disk to become ready.
static int
idewait(int checkerr)
{
  int r;

  while(((r = inb(IDE_STATUS)) & (IDE_BSY|IDE_DRDY)) != IDE_DRDY) 
    ;
  if(checkerr && (r & (IDE_DF|IDE_ERR)) != 0)
    return -1;
  return 0;
}

void
ideinit(void)
{
  int i;

  // Register IDE to PIC interrupt
  interrupts_irq_register(IRQ_IDE, isr_entry_ide, ide_irq_handler);

  idewait(0);
  // Check if disk 1 is present
  outb(IDE_DRIVE_HEAD, 0xe0 | (1<<4));
  for(i=0; i<1000; i++){
    if(inb(IDE_STATUS) != 0){
      havedisk1 = 1;
      break;
    }
  }
  
  // Switch back to disk 0.
  outb(IDE_DRIVE_HEAD, 0xe0 | (0<<4));
}

// Start the request for b.  Caller must hold idelock.
static void
idestart(struct buf *b)
{
  if(b == 0)
    return;
  if(b->blockno >= FSSIZE)
    return;
  int sector_per_block =  BSIZE/SECTOR_SIZE;
  int sector = b->blockno * sector_per_block;

  if (sector_per_block > 7) return;

  idewait(0);

  outb(0x3f6, 0);  // generate interrupt
  outb(IDE_SECTOR_COUNT, sector_per_block);  // number of sectors
  // Set LBA low, mid, high parts
  outb(IDE_LBA_LOW, sector & 0xff);
  outb(IDE_LBA_MID, (sector >> 8) & 0xff);
  outb(IDE_LBA_HIGH, (sector >> 16) & 0xff);
  outb(IDE_DRIVE_HEAD, 0xe0 | ((b->dev&1)<<4) | ((sector>>24)&0x0f));
  if(b->flags & B_DIRTY){
    outb(IDE_COMMAND, IDE_CMD_WRITE);
    outsl(IDE_DATA_REG, b->data, BSIZE/4);
  } else {
    // Send the read sectors command
    outb(IDE_COMMAND, IDE_CMD_READ);
  }
}

// Interrupt handler.
void
ide_irq_handler(void)
{
  struct buf *b;

  // First queued buffer is the active request.
  if((b = idequeue) == 0){
    // cprintf("spurious IDE interrupt\n");
    return;
  }
  idequeue = b->qnext;

  // Read data if needed.
  if(!(b->flags & B_DIRTY) && idewait(1) >= 0)
    insl(0x1f0, b->data, BSIZE/4);
  
  // Wake process waiting for this buf.
  b->flags |= B_VALID;
  b->flags &= ~B_DIRTY;
  
  // Start disk on next buf in queue.
  if(idequeue != 0)
    idestart(idequeue);

}

//PAGEBREAK!
// Sync buf with disk. 
// If B_DIRTY is set, write buf to disk, clear B_DIRTY, set B_VALID.
// Else if B_VALID is not set, read buf from disk, set B_VALID.
void
iderw(struct buf *b)
{
  struct buf **pp;

  if(!(b->flags & B_BUSY))
    return;
  if((b->flags & (B_VALID|B_DIRTY)) == B_VALID)
    return;
  if(b->dev != 0 && !havedisk1)
    return;


  // Append b to idequeue.
  b->qnext = 0;
  for(pp=&idequeue; *pp; pp=&(*pp)->qnext)  //DOC:insert-queue
    ;
  *pp = b;
  
  // Start disk if necessary.
  if(idequeue == b)
    idestart(b);
  
  // Wait for request to finish.
  while((b->flags & (B_VALID|B_DIRTY)) != B_VALID){
  }

}
