#ifndef SHADOWBOX_KEYBOARD_H
#define SHADOWBOX_KEYBOARD_H

#include "types.h"

#include "input.h"

/*
 * keyboard_handler - IRQ handler for keyboard interrupts
 */
void keyboard_handler(void);

/*
 * keyboard_init - Initialize keyboard controller
 */
void keyboard_handle_scancode(uint8_t scancode);
void keyboard_init(void);

/*
 * keyboard_getchar - Read a character from keyboard (blocking)
 * Returns: ASCII character
 */
char keyboard_getchar(void);

/*
 * keyboard_has_char - Check if a character is available
 * Returns: 1 if character available, 0 otherwise
 */
int keyboard_has_char(void);

/*
 * mouse_init - Initialize the PS/2 mouse controller
 */
void mouse_init(void);

/*
 * mouse_handler - IRQ handler for mouse interrupts
 */
void mouse_handler(void);

#endif
