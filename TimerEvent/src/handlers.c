/* 
 * handlers.c, Assignment -- Timer Event
 */
#include <spede/flames.h>
#include <spede/machine/asmacros.h>
#include <spede/machine/io.h>
#include <spede/machine/pic.h>
#include <spede/machine/proc_reg.h>
#include <spede/machine/seg.h>
#include <spede/stdio.h>
#include <spede/string.h>

#define name_len (unsigned)strlen(my_name) // my_name len

char my_name[] = "First Middle Family";
unsigned int i = 0;  // array index
unsigned int j = 0;
int tick_count = 0;  // cnt # of timer events

unsigned short *char_p = (unsigned short *)0xB8000 + 12 * 80 + 35; // video mem ptr

void TimerHandler(){
	if (tick_count == 0) {
		*(char_p) = 0xf00 + my_name[i];
	}
	tick_count ++;

	if(tick_count == 75){ // incr tick_count, loop every .75 seconds
		tick_count = 0;		
		char_p++; // incr ptr (cursor moves to next video mem location)
		i++; // incr i
		
        if(i == name_len){ // check len of my_name
			i = 0;
			char_p = (unsigned short *)0xB8000 + 12 * 80 + 35; // rst ptr

			for(j=0; j<name_len; j++){ // erase display
				char_p[j] = ' ' + 0xf00; // replace char with space
			}
		}
	}
	outportb(0x20, 0x60);  // 0x20 is PIC control, reg, 0x60 dismisses IRQ 0
}
