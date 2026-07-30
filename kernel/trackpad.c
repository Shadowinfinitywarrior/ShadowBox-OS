#include "trackpad.h"
#include "input.h"
#include "kernel.h"

static trackpad_report_t last_report;
static int is_gesturing = 0;

void trackpad_init(void) {
    printk(KERN_INFO "TRACKPAD: Initialized Multi-touch Gesture Engine\n");
    for (int i = 0; i < MAX_FINGERS; i++) last_report.fingers[i].active = 0;
    last_report.finger_count = 0;
}

void trackpad_process_report(trackpad_report_t *report) {
    if (report->finger_count == 1 && last_report.finger_count == 1) {
        // Single finger move -> Mouse move
        int dx = report->fingers[0].x - last_report.fingers[0].x;
        int dy = report->fingers[0].y - last_report.fingers[0].y;
        if (dx != 0 || dy != 0) {
            input_push(INPUT_EVENT_MOUSE_MOVE, 0, dx, dy);
        }
    } else if (report->finger_count == 2 && last_report.finger_count == 2) {
        // Two fingers -> Scroll
        int dy = report->fingers[0].y - last_report.fingers[0].y;
        if (dy != 0) {
            input_push(INPUT_EVENT_GESTURE, GESTURE_SCROLL, 0, dy);
        }
    } else if (report->finger_count == 3 && last_report.finger_count == 3) {
        // Three fingers -> Swipe (like macOS expose/workspace switch)
        int dx = report->fingers[0].x - last_report.fingers[0].x;
        if (dx > 50 && !is_gesturing) {
            input_push(INPUT_EVENT_GESTURE, GESTURE_SWIPE, 1, 0); // Swipe Right
            is_gesturing = 1;
        } else if (dx < -50 && !is_gesturing) {
            input_push(INPUT_EVENT_GESTURE, GESTURE_SWIPE, -1, 0); // Swipe Left
            is_gesturing = 1;
        }
    }
    
    if (report->finger_count == 0) is_gesturing = 0; // Reset gesture lock on finger lift
    
    for (int i = 0; i < MAX_FINGERS; i++) {
        last_report.fingers[i] = report->fingers[i];
    }
    last_report.finger_count = report->finger_count;
}
