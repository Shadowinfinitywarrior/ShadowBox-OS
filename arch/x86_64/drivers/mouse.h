#ifndef MOUSE_H
#define MOUSE_H

#include "types.h"

// Process a single mouse data byte. Implemented in mouse.c.
void mouse_process_byte(uint8_t data);

// Starts the polling thread for environments without IRQ support. Implemented in mouse_poll.c.
void mouse_start_poll_thread(void);

#endif // MOUSE_H
