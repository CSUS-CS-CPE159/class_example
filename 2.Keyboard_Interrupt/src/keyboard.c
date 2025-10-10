/**
 * CPE/CSC 159 Operating System Pragmatics
 * California State University, Sacramento
 *
 * Operating system entry point
 */
#include "spede/stdio.h"
#include "io.h"
#include "keyboard.h"

#define PIC_CONTROL 	0x20 	// Programmable Interrupt Controller (PIC).
#define PIC_EOI         0x20    // PIC End-of-Interrupt command
#define KEYBOARD_DONE 	0x60	// Set to PIC control when keyboard interrupt service done.


void keyboard_interrupt_handler(void) {
    int ext = 0;
	/* read keyboard data */
    unsigned char b = inportb(KBD_PORT_DATA);
	if (b == 0xE0) {
	    ext = 1;
        // read keyboard data again 
        b = inportb(KBD_PORT_DATA);
    }
	unsigned char c = b & 0x7F;

	/* translate keyboard scancode to ASCII code */
	unsigned char token = KEY_NULL;
	if (ext) {
	    token = keyboard_map_ext[c];
    } else {
	    token = (c < sizeof(keyboard_map_primary))
	        	? keyboard_map_primary[c] : KEY_NULL;	    
	}
	printf("keyboard scancode: %d, 0x%x, ascii code: %c\n", c, c, token);
    // Print the character on the VGA screen 
	*(unsigned short*)0xB8000 = 0x0700 + (unsigned int)c;       
	*(unsigned short*)0xB8002 = 0x0700 + (unsigned int)token;	

    /* Dismiss Keyboard event (IRQ 1). Otherwise, new event from keyboard won't be recognized 
    *  by CPU because hardware uses edge-trigger flipflop.
    */
    outportb(PIC_CONTROL, PIC_EOI); 
}
