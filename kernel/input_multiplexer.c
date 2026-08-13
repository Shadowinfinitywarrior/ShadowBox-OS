// Input Multiplexer – simple device registration and event forwarding
// This component provides a minimal framework for registering input devices
// and forwarding their events into the kernel's unified input ring buffer.
// It is deliberately lightweight: it does not implement per‑device buffers,
// but offers an API compatible with the existing input core.

#include "kernel.h"
#include "input.h"
#include "malloc.h"

#define MAX_INPUT_DEVICES 16

static input_dev_t *registered_devices[MAX_INPUT_DEVICES];
static unsigned int device_count = 0;

/* Initialise the multiplexer – clear device list. */
void input_multiplexer_init(void) {
    for (unsigned i = 0; i < MAX_INPUT_DEVICES; ++i) {
        registered_devices[i] = NULL;
    }
    device_count = 0;
    // No additional kernel input core initialisation required here.
}

/* Allocate a new input device structure. */
input_dev_t* input_multiplexer_allocate_device(void) {
    input_dev_t *dev = (input_dev_t*)kmalloc(sizeof(input_dev_t));
    if (!dev) return NULL;
    // Zero‑initialize the device descriptor.
    for (int i = 0; i < 64; ++i) dev->name[i] = 0;
    dev->capabilities = 0;
    dev->head = dev->tail = 0;
    dev->dev_node = NULL;
    // Event buffer will be cleared lazily when events are read.
    return dev;
}

/* Register a device with the multiplexer. Returns 0 on success, -1 on failure. */
int input_multiplexer_register_device(input_dev_t *dev) {
    if (!dev) return -1;
    if (device_count >= MAX_INPUT_DEVICES) return -1;
    registered_devices[device_count++] = dev;
    // Optionally expose the device capabilities to the core.
    // For now we simply store the caps; callers may invoke input_set_last_device_caps.
    return 0;
}

/* Forward a raw event from a device into the central input ring buffer.
   This is a thin wrapper around input_push, translating generic EV_* types
   to the kernel's internal INPUT_EVENT_* identifiers where reasonable. */
void input_multiplexer_report_event(input_dev_t *dev, uint16_t type, uint16_t code, int32_t value) {
    (void)dev; // Currently unused – the multiplexer does not maintain per‑device state.
    uint8_t ev_type = INPUT_EVENT_KEY_PRESS; // Default fallback.
    int16_t x = 0, y = 0;

    switch (type) {
        case EV_KEY:
            ev_type = (value) ? INPUT_EVENT_KEY_PRESS : INPUT_EVENT_KEY_RELEASE;
            break;
        case EV_REL:
            // Relative motion – treat as mouse move. "code" can indicate axis.
            ev_type = INPUT_EVENT_MOUSE_MOVE;
            // For simplicity we pack the motion delta into x; y remains 0.
            x = (int16_t)value;
            break;
        case EV_ABS:
            // Absolute coordinates – also map to mouse move.
            ev_type = INPUT_EVENT_MOUSE_MOVE;
            x = (int16_t)value;
            break;
        default:
            // Unknown/unsupported type – forward as a generic key press.
            ev_type = INPUT_EVENT_KEY_PRESS;
            break;
    }
    input_push(ev_type, code, x, y);
}

/* Poll devices – placeholder for future implementation.
   Returns the number of events processed (currently always 0). */
int input_multiplexer_poll(void) {
    // In this minimal stub we have no per‑device event queues.
    // Real drivers would read their device buffers here and call
    // input_multiplexer_report_event for each pending event.
    return 0;
}
