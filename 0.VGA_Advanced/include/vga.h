/**
 * CPE/CSC 159 - Operating System Pragmatics
 * California State University, Sacramento
 *
 * VGA Definitions
 */
#ifndef VGA_H
#define VGA_H

#define VGA_BASE                ((unsigned short *)(0xB8000))
#define VGA_ATTR(bg, fg)        (((bg & 0xf) << 4) | (fg & 0xf))
#define VGA_CHAR(bg, fg, c)     (((VGA_ATTR((bg & 0xf), (fg & 0xf)) << 8)) | (c))

#define VGA_WIDTH               80
#define VGA_HEIGHT              25

#define VGA_COLOR_BLACK         0x0
#define VGA_COLOR_BLUE          0x1
#define VGA_COLOR_GREEN         0x2
#define VGA_COLOR_CYAN          0x3
#define VGA_COLOR_RED           0x4
#define VGA_COLOR_MAGENTA       0x5
#define VGA_COLOR_BROWN         0x6
#define VGA_COLOR_LIGHT_GREY    0x7

#define VGA_COLOR_DARK_GREY     0x8
#define VGA_COLOR_LIGHT_BLUE    0x9
#define VGA_COLOR_LIGHT_GREEN   0xA
#define VGA_COLOR_LIGHT_CYAN    0xB
#define VGA_COLOR_LIGHT_RED     0xC
#define VGA_COLOR_LIGHT_MAGENTA 0xD
#define VGA_COLOR_YELLOW        0xE
#define VGA_COLOR_WHITE         0xF

/**
 * Prints out a formatted string to the VGA display
 * @param fmt string format
 * @param ... variable list of parameters
 */
#define vga_printf(fmt, ...) { \
    char _vga_printf_buf[VGA_WIDTH * VGA_HEIGHT] = {0}; \
    snprintf(_vga_printf_buf, sizeof(_vga_printf_buf), (fmt), ##__VA_ARGS__); \
    vga_puts(_vga_printf_buf); \
}
#endif
