#ifndef SHADOWBOX_INPUT_H
#define SHADOWBOX_INPUT_H

#include "types.h"

// Unified Input Event Types
#define EV_KEY 0x01
#define EV_REL 0x02
#define EV_ABS 0x03

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
/*
 * input_dev_t - Input device descriptor
 */
typedef struct input_dev {
	char name[64];
	uint32_t capabilities;
	uint32_t head;
	uint32_t tail;
	void *dev_node;
} input_dev_t;

typedef struct {
    uint8_t type;
    uint8_t code;
    int16_t x;
    int16_t y;
    int16_t value;
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
