#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "types.h"

typedef struct {
    uint8_t type;    // 0=key press, 1=key release, 2=mouse move, 3=mouse button
    uint8_t code;    // scancode for keys, button mask for mouse
    int16_t x;
    int16_t y;
} input_event_t;

void input_push(uint8_t type, uint8_t code, int16_t x, int16_t y);
int input_poll_event(input_event_t *ev);

void keyboard_handler(void);
void keyboard_init(void);
char keyboard_getchar(void); // blocking read
int keyboard_has_char(void);

#endif
