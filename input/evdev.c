// Input evdev implementation for ShadowBox OS
// Provides a simple in-memory circular buffer for mouse events
// that the userland desktop reads via /dev/input.

#include "input.h"
#include "kernel.h"
#include "malloc.h"
#include "kstring.h"

/* Simple static buffer for events. */
static input_event_t ev_buffer[256];
static uint32_t ev_head = 0; // write position
static uint32_t ev_tail = 0; // read position

/* Store a mouse event in the circular buffer. */
static void evdev_push(uint8_t type, uint8_t code, int16_t x, int16_t y) {
 uint32_t next = (ev_head + 1) % 256;
 if (next == ev_tail) { // Buffer full – drop oldest event.
  ev_tail = (ev_tail + 1) % 256;
 }
 ev_buffer[ev_head].type = type;
 ev_buffer[ev_head].code = code;
 ev_buffer[ev_head].value = 0; // not used
 ev_buffer[ev_head].x = x;
 ev_buffer[ev_head].y = y;
 // timestamps left zero – not required for desktop.
 ev_head = next;
}

static int evdev_poll_event(input_event_t *ev) {
 if (ev_tail == ev_head) return 0; // no event
 *ev = ev_buffer[ev_tail];
 ev_tail = (ev_tail + 1) % 256;
 return 1;
}

void input_core_init(void) {
 // No special hardware init needed for this stub.
 ev_head = ev_tail = 0;
}

input_dev_t* input_allocate_device(void) {
 // Not used in this simplified model – return NULL.
 return NULL;
}

int input_register_device(input_dev_t *dev) {
 // No device registration needed for the mouse buffer.
 (void)dev;
 return 0;
}

void input_report_event(input_dev_t *dev, uint16_t type, uint16_t code, int32_t value) {
 // Directly push as an input event for simplicity.
 (void)dev;
 evdev_push((uint8_t)type, (uint8_t)code, (int16_t)value, 0);
}

void input_sync(input_dev_t *dev) {
 // No batching needed.
 (void)dev;
}
