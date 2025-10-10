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


// Ordinary Scan Codes (US 102-key): Primary keymap
static const char keyboard_map_primary[] = {
    KEY_NULL,           /* 0x00 - Null */
    KEY_ESCAPE,         /* 0x01 - Escape  */
    '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',
    '\b',               /* 0x0e - Backspace */
    '\t',               /* 0x0f - Tab */
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']',
    '\n',               /* 0x1e - Enter */
    KEY_NULL,           /* 0x1d - Left Ctrl */
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    KEY_NULL,           /* 0x2a - Left Shift */
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
    KEY_NULL,           /* 0x36 - Right Shift */
    KEY_NULL,           /* 0x37 - Print Screen */
    KEY_NULL,           /* 0x38 - Left Alt */
    ' ',                /* 0x39 - Spacebar */
    KEY_NULL,           /* 0x3a - CapsLock */
    KEY_F1,             /* 0x3b - F1 */
    KEY_F2,             /* 0x3c - F2 */
    KEY_F3,             /* 0x3d - F3 */
    KEY_F4,             /* 0x3e - F4 */
    KEY_F5,             /* 0x3f - F5 */
    KEY_F6,             /* 0x40 - F6 */
    KEY_F7,             /* 0x41 - F7 */
    KEY_F8,             /* 0x42 - F8 */
    KEY_F9,             /* 0x43 - F9 */
    KEY_F10,            /* 0x44 - F10 */
    KEY_NULL,           /* 0x45 - NumLock */
    KEY_NULL,           /* 0x46 - ScrollLock */
    '7',                /* 0x47 - Numpad 7 */
    KEY_UP, //'8',                /* 0x48 - Numpad 8 */
    '9',                /* 0x49 - Numpad 9 */
    '-',                /* 0x4a - Numpad Minus */
    KEY_LEFT, //'4',                /* 0x4b - Numpad 4 */
    '5',                /* 0x4c - Numpad 5 */
    KEY_RIGHT, //'6',                /* 0x4d - Numpad 6 */
    '+',                /* 0x4e - Numpad Plus */
    '1',                /* 0x4f - Numpad 1 */
    KEY_DOWN, //'2',                /* 0x50 - Numpad 2 */
    '3',                /* 0x51 - Numpad 3 */
    KEY_INSERT,         /* 0x52 - Insert */
    KEY_DELETE,         /* 0x53 - Delete */
    // add more key
};

// Escaped Scan Codes (US 102-Key)
static const unsigned char keyboard_map_ext[128] = {
    // … initialize all to KEY_NULL, then set the few we care about:
    [0x48] = KEY_UP,
    [0x50] = KEY_DOWN,
    [0x4B] = KEY_LEFT,
    [0x4D] = KEY_RIGHT,
    [0x52] = KEY_INSERT,
    [0x53] = KEY_DELETE,
    // add HOME(0x47), END(0x4F), PGUP(0x49), PGDN(0x51) if you like
};

void keyboard_interrupt_handler(void);
 
#endif 

