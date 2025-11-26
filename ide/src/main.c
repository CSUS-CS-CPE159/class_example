/*
 * main.c -- IDE Example 
 */

#include <spede/flames.h>
#include <spede/machine/asmacros.h>
#include <spede/machine/io.h>
//#include <spede/machine/pic.h>
#include <spede/machine/proc_reg.h>
#include <spede/machine/seg.h>
#include <spede/stdio.h>
#include <spede/string.h>

#include "defs.h"
#include "fs.h"
#include "buf.h"
#include "file.h"
#include "stat.h"
#include "interrupts.h"

struct i386_gate *IDT_p;
extern void IdeEntry();
extern struct inode* iget(uint dev, uint inum);
extern void cat();

void ls(void){
    struct inode *ip;
    ip = namei("/");
    // Read data to ip
    struct file *f= filealloc();
    f->type = FD_INODE;
    f->ip = ip;
    f->off = 0;
    f->readable = 1;
    f->writable = 1;

    struct file *f1= filealloc();
    f1->type = FD_INODE;
    f1->off = 0;
    f1->readable = 1;
    f1->writable = 1;

    struct stat st;
    filestat(f, &st);

    struct dirent de;

    while(fileread(f, (char *)&de, sizeof(de)) == sizeof(de)){
        if(de.inum == 0)
            continue;
        printf("%s %d\n", de.name, de.inum);
        f1->ip = iget(1, de.inum);
        if(filestat(f1, &st) < 0) {
                printf("ls: cannot stat %s\n", de.name);
                continue;
        }
        cons_printf("%d\t %d\t %s\t\t%d\n", st.type, st.size, de.name, st.ino);
        iput(f1->ip);
    }
    cat();
    while(1){};
}

void cat(){
    char buf[512];
    struct file *f= filealloc();
    f->type = FD_INODE;
    f->off = 0;
    f->readable = 1;
    f->writable = 1;
    f->ip = iget(1, 2);
    while(fileread(f, (char*)buf, sizeof(buf)) == sizeof(buf)){
        printf("%s\n", buf);
    }
    iput(f->ip);
}
extern void IdeEntry();

int main(){
    // Initialize PIC, we need to reconfigure pic device
    picinit();
    // Initialize Interrupt
    interrupts_init();
	// Initialize the buffer cache
    binit();
	// Initialize the IDE device
    ideinit();
    // Enable interrupts
    interrupts_enable();

    unsigned short buf[512];
    memset(buf, 0, sizeof(buf));
	// Read the first block from ide device 1, and put this block into buffer
    struct buf * b = bread(1, 1);
    printf("%d\n", b->dev);
    b = bread(1, 2);
    b = bread(1, 29);
    b = bread(1, 0x2c);
    struct superblock sb;
    readsb(1, &sb);
    ls();
	return 0;
}

