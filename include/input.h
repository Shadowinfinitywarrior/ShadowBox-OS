#ifndef SHADOWBOX_INPUT_H
#define SHADOWBOX_INPUT_H

#include "types.h"

// Unified Input Event Types
#define INPUT_EVENT_KEY_PRESS   0
#define INPUT_EVENT_KEY_RELEASE 1
#define INPUT_EVENT_MOUSE_MOVE  2
#define INPUT_EVENT_MOUSE_BTN   3
#define INPUT_EVENT_GESTURE     4

// Gesture Codes
#define GESTURE_SCROLL 0
#define GESTURE_PINCH  1
#define GESTURE_SWIPE  2

/*
 * input_event_t - Unified input event structure
 * Used by all input drivers (keyboard, mouse, touchpad, usb hid)
 * to communicate with the GUI compositor.
 */
typedef struct {
    uint8_t type;
    uint8_t code;
    int16_t x;
    int16_t y;
    uint16_t reserved;
} input_event_t;

/*
 * input_push - Push an input event into the global event ring buffer
 */
void input_push(uint8_t type, uint8_t code, int16_t x, int16_t y);

/*
 * input_poll_event - Poll the next input event
 * Returns: 1 if event available, 0 otherwise
 */
int input_poll_event(input_event_t *ev);

#endif
