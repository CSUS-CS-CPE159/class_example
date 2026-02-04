/* 
 * main.c -- Reprogramming VGA font 
 * VGA has over 300 register! 
 */
#include "io.h"

/* Standard VGA Ports */
#define VGA_SEQ_INDEX 0x3C4
#define VGA_SEQ_DATA  0x3C5
#define VGA_GC_INDEX  0x3CE
#define VGA_GC_DATA   0x3CF

typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

/* Helper to write to VGA registers */
void vga_write_reg(uint16_t port_idx, uint8_t reg, uint8_t val) {
    outportb(port_idx, reg);
    outportb(port_idx + 1, val);
}

void upload_glyph(uint8_t ascii_idx, uint8_t *bitmap) {
    // 0xA0000 is the start of Plane 2 when mapped
    uint8_t *vram = (uint8_t *)0xA0000;

    // 1. Enter "Font Load Mode" (Switch to Planar access)
    vga_write_reg(VGA_SEQ_INDEX, 0x02, 0x04); // Write only to Plane 2
    vga_write_reg(VGA_SEQ_INDEX, 0x04, 0x07); // Disable Odd/Even addressing
    vga_write_reg(VGA_GC_INDEX,  0x04, 0x02); // Read from Plane 2
    vga_write_reg(VGA_GC_INDEX,  0x05, 0x00); // Disable Odd/Even mode
    vga_write_reg(VGA_GC_INDEX,  0x06, 0x00); // Map VRAM to 0xA0000


    // 2. Copy the bitmap.
    // VGA hardware allocates 32 bytes per glyph regardless of height.
    for (int i = 0; i < 16; i++) {
        vram[ascii_idx * 32 + i] = bitmap[i];
    }

    // Restore "text mode" (Normal Video output)
    vga_write_reg(VGA_SEQ_INDEX, 0x02, 0x03); // Write to Plane 0 & 1
    vga_write_reg(VGA_SEQ_INDEX, 0x04, 0x03); // Disable Odd/Even addressing
    vga_write_reg(VGA_GC_INDEX,  0x04, 0x00); // Read from Plane 0
    vga_write_reg(VGA_GC_INDEX,  0x05, 0x10); // Disable Odd/Even mode
    vga_write_reg(VGA_GC_INDEX,  0x06, 0x0E); // Remap VRAM to 0xB0000
}

int main(){
    // Bitmap for Omega (U+03A9) -- 8x16 pixels
    uint8_t omega_glyph[16] = {
        0x00, 0x00, 0x3C, 0x42, 0x42, 0x42, 0x42, 0x42,
        0x42, 0x42, 0x24, 0x24, 0x24, 0x66, 0xFF, 0x00
    };

    // Replace 'A' (65) with our new symbol
    upload_glyph('A', omega_glyph);
    // Display "hello, world" in the first row
    unsigned short *vga_base = (unsigned short *)0xB8000;	
    *(vga_base + 0) = (unsigned short )0x0700 + 'A';
    *(vga_base + 2) = (unsigned short )0x0700 + 'A';

    // Katakana 'A' (ア) - 8x16 bitmap
    uint8_t katakana_a[16] = {
        0x00, // 00000000  
        0x00, // 00000000 
        0x7F, // 01111111  
        0x82, // 10000010
        0x04, // 00000100 
        0x08, // 00001000 
        0x10, // 00010000 
        0x10, // 00010000 
        0x10, // 00010000  
        0x10, // 00010000
        0x10, // 00010000
        0x10, // 00010000
        0x10, // 00010000
        0x10, // 00010000
        0x00, // 00000000
        0x00  // 00000000
    };
    upload_glyph('B', katakana_a);
    unsigned char katakana_he[16] = {
        0x00, // 00000000
        0x00, // 00000000
        0x08, // 00001000  <-- Peak of the roof
        0x1C, // 00011100
        0x36, // 00110110
        0x63, // 01100011
        0x41, // 01000001
        0x00, // (Remaining 9 bytes are 0x00 for padding)
    // ...
    };
    upload_glyph('C', katakana_he);
 
    // Display Japanese Katakana 'A' (ア) in the second row 
    *(vga_base + 80 + 0) = (unsigned short)0xB400 + 'B';
    *(vga_base + 80 + 2) = (unsigned short)0xB400 + 'B';
    
    // Display Japanese Katakana "HE" (ヘ) in the second row 
    *(vga_base + 80*2 + 0) = (unsigned short)0xB400 + 'C';
    *(vga_base + 80*2 + 2) = (unsigned short)0xB400 + 'C';

    return 0;
}
