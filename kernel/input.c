#include "input.h"

// Ring buffer for unified input events
#define INPUT_RING_SIZE 256

static input_event_t input_ring[INPUT_RING_SIZE];
static volatile int input_head = 0;
static volatile int input_tail = 0;

void input_push(uint8_t type, uint8_t code, int16_t x, int16_t y) {
    // Disable interrupts for atomic access from IRQ context
    uint64_t flags;
    __asm__ volatile("pushfq\npop %0\ncli\n" : "=r"(flags) : : "memory");
    
    int next = (input_head + 1) % INPUT_RING_SIZE;
    if (next != input_tail) {
        input_ring[input_head].type = type;
        input_ring[input_head].code = code;
        input_ring[input_head].x = x;
        input_ring[input_head].y = y;
        __sync_synchronize();
        input_head = next;
    }
    
    // Restore interrupt state
    __asm__ volatile("push %0\npopfq\n" : : "r"(flags) : "memory");
}

int input_poll_event(input_event_t *ev) {
    // Disable interrupts for atomic access from userland context
    uint64_t flags;
    __asm__ volatile("pushfq\npop %0\ncli\n" : "=r"(flags) : : "memory");
    
    if (input_tail == input_head) {
        __asm__ volatile("push %0\npopfq\n" : : "r"(flags) : "memory");
        return 0;
    }
    *ev = input_ring[input_tail];
    __sync_synchronize();
    input_tail = (input_tail + 1) % INPUT_RING_SIZE;
    
    // Restore interrupt state
    __asm__ volatile("push %0\npopfq\n" : : "r"(flags) : "memory");
    return 1;
}
