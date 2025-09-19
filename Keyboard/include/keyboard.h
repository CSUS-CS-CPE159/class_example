/**
 * CPE/CSC 159 - Operating System Pragmatics
 * California State University, Sacramento
 *
 * Keyboard Functions
 */
#ifndef KEYBOARD_H
#define KEYBOARD_H

// Keyboard data port
#define KBD_PORT_DATA           0x60

// Keyboard status port
#define KBD_PORT_STAT           0x64

// Special key definitions
#define KEY_NULL                0x00
#define KEY_ESCAPE              0x1B

// Directional Keys
#define KEY_HOME                0xE0
#define KEY_END                 0xE1
#define KEY_UP                  0xE2
#define KEY_DOWN                0xE3
#define KEY_LEFT                0xE4
#define KEY_RIGHT               0xE5
#define KEY_PAGE_UP             0xE6
#define KEY_PAGE_DOWN           0xE7
#define KEY_INSERT              0xE8
#define KEY_DELETE              0xE9

// Function Keys
#define KEY_F1                  0xF1
#define KEY_F2                  0xF2
#define KEY_F3                  0xF3
#define KEY_F4                  0xF4
#define KEY_F5                  0xF5
#define KEY_F6                  0xF6
#define KEY_F7                  0xF7
#define KEY_F8                  0xF8
#define KEY_F9                  0xF9
#define KEY_F10                 0xFA
#define KEY_F11                 0xFB
#define KEY_F12                 0xFC

#ifndef ASSEMBLER

/**
 * Initializes keyboard data structures and variables
 */
void keyboard_init(void);

/**
 * Scans for keyboard input and returns the raw character data
 * @return raw character data from the keyboard
 */
unsigned int keyboard_scan(void);

/**
 * Polls for a keyboard character to be entered.
 *
 * If a keyboard character data is present, will scan and return
 * the decoded keyboard output.
 *
 * @return decoded character or KEY_NULL (0) for any character
 *         that cannot be decoded
 */
unsigned int keyboard_poll(void);

/**
 * Blocks until a keyboard character has been entered
 * @return decoded character entered by the keyboard or KEY_NULL
 *         for any character that cannot be decoded
 */
unsigned int keyboard_getc(void);

/**
 * Processes raw keyboard input and decodes it.
 *
 * Should keep track of the keyboard status for the following keys:
 *   SHIFT, CTRL, ALT, CAPS, NUMLOCK
 *
 * For all other characters, they should be decoded/mapped to ASCII
 * or ASCII-friendly characters.
 *
 * For any character that cannot be mapped, KEY_NULL should be returned.
 */
unsigned int keyboard_decode(unsigned int c);
#endif
#endif
