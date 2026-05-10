/**
 * Advanced VGA: Custom Font/Glyph Upload Example
 *
 * Demonstrates how to reprogram the VGA font ROM to display custom glyphs.
 * This is an advanced example showing:
 * - VGA sequencer and graphics controller registers
 * - Planar graphics memory access
 * - Font ROM modification
 * - Creating custom character bitmaps
 *
 * VGA Graphics Architecture:
 * - VGA memory is organized in 4 planes for various purposes
 * - Plane 0-1: Text color data
 * - Plane 2: Font/bitmap data (where we load custom glyphs)
 * - Plane 3: Reserved
 *
 * Key Concept:
 * By changing the mode and writing to plane 2 at 0xA0000, we can overwrite
 * the font ROM for any character code. Then when we write that character
 * to the text buffer at 0xB8000, VGA displays our custom bitmap.
 *
 * Reference: VGA has over 300 registers controlling various aspects!
 */
#include "io.h"

/* Standard VGA I/O Port Addresses */
#define VGA_SEQ_INDEX 0x3C4   /* Sequencer Index port - select which register */
#define VGA_SEQ_DATA  0x3C5   /* Sequencer Data port - read/write register value */
#define VGA_GC_INDEX  0x3CE   /* Graphics Controller Index port */
#define VGA_GC_DATA   0x3CF   /* Graphics Controller Data port */

typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

/**
 * Helper function to write to VGA registers via port I/O
 * 
 * VGA registers are accessed in a two-step process:
 * 1. Write the register index/number to the index port
 * 2. Write the value to the data port
 * 
 * @param port_idx  The index port address (e.g., VGA_SEQ_INDEX)
 * @param reg       Register number to modify
 * @param val       Value to write to the register
 */
void vga_write_reg(uint16_t port_idx, uint8_t reg, uint8_t val) {
    outportb(port_idx, reg);      /* Select which register */
    outportb(port_idx + 1, val);  /* Write the value */
}

/**
 * Upload a custom glyph (character bitmap) to VGA font ROM
 *
 * This function:
 * 1. Switches VGA to font-load mode (planar access)
 * 2. Writes a 16-byte bitmap to font ROM for a character
 * 3. Restores normal text mode
 *
 * The bitmap represents the character as an 8×16 pixel grid,
 * with each byte representing one row of the glyph.
 *
 * @param ascii_idx  ASCII code of character to replace (e.g., 'A'=65)
 * @param bitmap     Pointer to 16-byte bitmap data (one byte per row)
 */
void upload_glyph(uint8_t ascii_idx, uint8_t *bitmap) {
    // 0xA0000 is the start of plane 2 memory when in font-load mode
    uint8_t *vram = (uint8_t *)0xA0000;

    /* Step 1: Enter "Font Load Mode" - Switch to Planar access */
    vga_write_reg(VGA_SEQ_INDEX, 0x02, 0x04); /* Write only to Plane 2 */
    vga_write_reg(VGA_SEQ_INDEX, 0x04, 0x07); /* Disable Odd/Even addressing */
    vga_write_reg(VGA_GC_INDEX,  0x04, 0x02); /* Read from Plane 2 */
    vga_write_reg(VGA_GC_INDEX,  0x05, 0x00); /* Disable Odd/Even mode */
    vga_write_reg(VGA_GC_INDEX,  0x06, 0x00); /* Map VRAM to 0xA0000 */

    /* Step 2: Copy the bitmap to font ROM */
    /* Each character glyph occupies 32 bytes in VGA font ROM,
       regardless of font height (we use 16 bytes for 8×16 font) */
    for (int i = 0; i < 16; i++) {
        vram[ascii_idx * 32 + i] = bitmap[i];
    }

    /* Step 3: Restore "Text Mode" - Return to normal video output */
    vga_write_reg(VGA_SEQ_INDEX, 0x02, 0x03); /* Write to Planes 0 & 1 */
    vga_write_reg(VGA_SEQ_INDEX, 0x04, 0x03); /* Enable normal addressing */
    vga_write_reg(VGA_GC_INDEX,  0x04, 0x00); /* Read from Plane 0 */
    vga_write_reg(VGA_GC_INDEX,  0x05, 0x10); /* Restore Odd/Even mode */
    vga_write_reg(VGA_GC_INDEX,  0x06, 0x0E); /* Remap VRAM to 0xB8000 */
}

int main(){
    /* Custom bitmap for Omega symbol - 8x16 pixels */
    /* Each byte represents one row: bits are pixels (1=on, 0=off) */
    uint8_t omega_glyph[16] = {
        0x00, // Row 0:  00000000
        0x00, // Row 1:  00000000
        0x3C, // Row 2:  00111100 (top of omega)
        0x42, // Row 3:  01000010
        0x42, // Row 4:  01000010
        0x42, // Row 5:  01000010
        0x42, // Row 6:  01000010
        0x42, // Row 7:  01000010
        0x42, // Row 8:  01000010
        0x42, // Row 9:  01000010
        0x24, // Row 10: 00100100
        0x24, // Row 11: 00100100
        0x24, // Row 12: 00100100
        0x66, // Row 13: 01100110 (bottom of omega)
        0xFF, // Row 14: 11111111
        0x00  // Row 15: 00000000
    };

    /* Replace 'A' character (ASCII 65) with our omega symbol */
    upload_glyph('A', omega_glyph);

    /* Write to VGA display buffer to show the custom characters */
    unsigned short *vga_base = (unsigned short *)0xB8000;

    /* Display 'A' which now shows as omega symbol */
    *(vga_base + 0) = (unsigned short)0x0700 + 'A';
    *(vga_base + 2) = (unsigned short)0x0700 + 'A';

    /* Custom bitmap for Katakana 'A' (ア) character */
    uint8_t katakana_a[16] = {
        0x00, // 00000000
        0x00, // 00000000
        0x7F, // 01111111 (top line)
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

    /* Replace 'B' character with Katakana 'A' glyph */
    upload_glyph('B', katakana_a);

    /* Custom bitmap for Katakana 'HE' (ヘ) character */
    uint8_t katakana_he[16] = {
        0x00, // 00000000
        0x00, // 00000000
        0x08, // 00001000  (peak of the roof)
        0x1C, // 00011100
        0x36, // 00110110
        0x63, // 01100011
        0x41, // 01000001
        0x00, // (Remaining 9 bytes are 0x00 for padding)
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00
    };

    /* Replace 'C' character with Katakana 'HE' glyph */
    upload_glyph('C', katakana_he);

    /* Display the custom glyphs on rows 2 and 3 of the screen */
    /* Row 1 (offset 0): Show custom 'A' (omega) and 'B' (katakana A) */
    *(vga_base + 80 + 0) = (unsigned short)0xB400 + 'B';  /* Katakana A */
    *(vga_base + 80 + 2) = (unsigned short)0xB400 + 'B';

    /* Row 2 (offset 160): Show custom 'C' (katakana HE) */
    *(vga_base + 80*2 + 0) = (unsigned short)0xB400 + 'C'; /* Katakana HE */
    *(vga_base + 80*2 + 2) = (unsigned short)0xB400 + 'C';

    return 0;
}
