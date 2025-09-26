/* 
 * main.c -- Timer Event
 */

#include <spede/flames.h>
#include <spede/machine/asmacros.h>
#include <spede/machine/io.h>
#include <spede/machine/pic.h>
#include <spede/machine/proc_reg.h>
#include <spede/machine/seg.h>
#include <spede/stdio.h>
#include <spede/string.h>

extern void gdt_flush();
struct i386_descriptor gdt[7];
struct pseudo_descriptor gp;

void setup_gdt(){
//fill_descriptor(struct i386_descriptor *desc, unsigned base, unsigned limit,
//		unsigned char access, unsigned char sizebits)
    fill_descriptor(&gdt[0], 0x0, 0, 0, 0);
    // Kernel code segment
    fill_descriptor(&gdt[1], 0x0, 0xFFFFFFFF, 0x9A, 0xCF);
    // Kernel data segment 
    fill_descriptor(&gdt[2], 0x0, 0xFFFFFFFF, 0x92, 0xCF);
    // Process 1 code segment 
    fill_descriptor(&gdt[3], 0x000000, 0xFFFFFFFF, 0x9A, 0xCF);
    // Process 1 data segment 
    fill_descriptor(&gdt[4], 0x000000, 0xFFFFFFFF, 0x92, 0xCF);
    // Process 2 code segment 
    fill_descriptor(&gdt[5], 0x000000, 0xFFFFFFFF, 0x9A, 0xCF);
    // Process 2 data segment 
    fill_descriptor(&gdt[6], 0x00000, 0xFFFFFFFF, 0x92, 0xCF);
     
    //load the GDT
    gp.limit = (sizeof(struct i386_descriptor)*7) - 1;
    gp.linear_base = (uint32_t)&gdt;
    cons_printf("gdt base: 0x%x, limit: 0x%x \n", gp.linear_base, gp.limit);
    set_gdt(&gp);
    gdt_flush();
    cons_printf("successful load gdt\n");
}
