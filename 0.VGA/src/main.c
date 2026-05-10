/**
 * Basic VGA Text Output Example
 *
 * Demonstrates how to write text to the VGA display directly by accessing
 * memory-mapped I/O at address 0xB8000.
 *
 * VGA Text Mode Details:
 * - Memory address: 0xB8000 (maps to VGA display memory)
 * - Each character cell uses 2 bytes:
 *   - Low byte: ASCII character code
 *   - High byte: Attribute (color and formatting)
 * - Screen layout: 80 characters x 25 lines
 * - Each row offset: 80 cells = 160 bytes
 *
 * Attribute byte format (0x0700 in this example):
 *   Bits 7-4: Background color (0 = black)
 *   Bits 3-0: Foreground color (7 = light gray)
 *
 * Learning Objectives:
 * - Understand memory-mapped I/O
 * - Learn VGA text mode addressing
 * - See how to write directly to hardware from C
 */

int main(){
    // VGA text memory is memory-mapped at 0xB8000
    // Each entry is a 16-bit word: [attribute_byte][ascii_char]
    unsigned short *vga_base = (unsigned short *)0xB8000;

    // Write "hello, world" directly using individual character writes
    // The attribute 0x0700 means: white text (7) on black background (0)
    *(vga_base + 0) = (unsigned short)0x0700 + 'h';
    *(vga_base + 1) = (unsigned short)0x0700 + 'e';
    *(vga_base + 2) = (unsigned short)0x0700 + 'l';
    *(vga_base + 3) = (unsigned short)0x0700 + 'l';
    *(vga_base + 4) = (unsigned short)0x0700 + 'o';
    *(vga_base + 5) = (unsigned short)0x0700 + ',';
    *(vga_base + 6) = (unsigned short)0x0700 + 'w';
    *(vga_base + 7) = (unsigned short)0x0700 + 'o';
    *(vga_base + 8) = (unsigned short)0x0700 + 'r';
    *(vga_base + 9) = (unsigned short)0x0700 + 'l';
    *(vga_base + 10) = (unsigned short)0x0700 + 'd';
    *(vga_base + 11) = (unsigned short)0x0700 + '!';

    // Demonstrate using a string and a loop for the second row
    // Row 2 starts at offset 80 (second row of 80 columns)
    char array[] = "hello world!";
    for (int i = 0; i < 12; i++) {
        // Attribute 0xB400: white text (4) on light red background (B)
        *(vga_base + 80 + i) = (unsigned short)0xB400 + array[i];
    }

    return 0;
}
