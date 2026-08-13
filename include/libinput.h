#ifndef SHADOWBOX_LIBINPUT_H
#define SHADOWBOX_LIBINPUT_H

#include "types.h"
#include "input.h"

// libinput-style semantic events
typedef enum {
    LIBINPUT_EVENT_NONE,
    LIBINPUT_EVENT_POINTER_MOTION,
    LIBINPUT_EVENT_POINTER_MOTION_ABSOLUTE,
    LIBINPUT_EVENT_POINTER_BUTTON,
    LIBINPUT_EVENT_POINTER_AXIS,
    LIBINPUT_EVENT_KEYBOARD_KEY,
    LIBINPUT_EVENT_TOUCH_DOWN,
    LIBINPUT_EVENT_TOUCH_UP,
    LIBINPUT_EVENT_TOUCH_MOTION,
    LIBINPUT_EVENT_GESTURE_SWIPE,
    LIBINPUT_EVENT_GESTURE_PINCH
} libinput_event_type_t;

// Input Configuration / Processing Profile
typedef struct {
    float accel_speed;             // -1.0 to 1.0 (Acceleration Profile)
    uint8_t pointer_accel_profile; // Flat vs Adaptive
    float deadzone_radius;         // Analog stick / Touchpad dead zone threshold
    float calibration_matrix[6];   // Touchscreen calibration matrix
} libinput_config_t;

// The Semantic Event returned to the Compositor
typedef struct {
    libinput_event_type_t type;
    uint64_t timestamp;
    union {
        struct { double dx; double dy; } pointer_motion;
        struct { uint32_t button; uint32_t state; } pointer_button;
        struct { uint32_t key; uint32_t state; } keyboard_key;
        struct { int32_t slot; double x; double y; } touch;
        struct { int fingers; double dx; double dy; } gesture_swipe;
        struct { int fingers; double scale; double angle; } gesture_pinch;
    } data;
} libinput_event_t;

void libinput_init(void);
void libinput_set_config(libinput_config_t *cfg);

// Reads from /dev/input/eventN, normalizes, applies gestures/accel, outputs semantic events
void libinput_dispatch(void); 
libinput_event_t* libinput_get_event(void);

#endif
