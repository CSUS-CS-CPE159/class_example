/* 
 * events.h -- Keyboard
 */

#ifndef __EVENTS_H__
#define __EVENTS_H__

#define KEYBOARD_EVENT 0x21 // decimal: 33

#ifndef ASSEMBLER  // skip if ASSEMBLER defined (in assembly code)
void KeyboardEntry(); // defined in events.S
#endif

#endif
