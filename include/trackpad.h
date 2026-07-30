#ifndef SHADOWBOX_TRACKPAD_H
#define SHADOWBOX_TRACKPAD_H

#include "types.h"

#define MAX_FINGERS 5

typedef struct {
    uint8_t active;
    uint16_t x;
    uint16_t y;
    uint8_t pressure;
} trackpad_finger_t;

typedef struct {
    trackpad_finger_t fingers[MAX_FINGERS];
    uint8_t finger_count;
} trackpad_report_t;

void trackpad_init(void);
void trackpad_process_report(trackpad_report_t *report);

#endif
