#include "keyboard.h"
#include "io.h"
#include "kernel.h"
#include "task.h"

#define KB_PORT 0x60
#define KB_CMD 0x64

#define BUFFER_SIZE 256
static char kbd_buffer[BUFFER_SIZE];
static volatile int kbd_head = 0;
static volatile int kbd_tail = 0;

// Input event ring buffer for userland DE (input_event_t from keyboard.h)
static input_event_t input_ring[256];
static volatile int input_head = 0;
static volatile int input_tail = 0;

void input_push(uint8_t type, uint8_t code, int16_t x, int16_t y) {
    int next = (input_head + 1) % 256;
    if (next != input_tail) {
        input_ring[input_head].type = type;
        input_ring[input_head].code = code;
        input_ring[input_head].x = x;
        input_ring[input_head].y = y;
        __sync_synchronize();
        input_head = next;
    }
}

int input_poll_event(input_event_t *ev) {
    if (input_tail == input_head) return 0;
    *ev = input_ring[input_tail];
    __sync_synchronize();
    input_tail = (input_tail + 1) % 256;
    return 1;
}

static const char kbd_us_map[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', /* 9 */
  '9', '0', '-', '=', '\b', /* Backspace */
  '\t',         /* Tab */
  'q', 'w', 'e', 'r',   /* 19 */
  't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', /* Enter key */
    0,          /* 29   - Control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', /* 39 */
 '\'', '`',   0,        /* Left shift */
 '\\', 'z', 'x', 'c', 'v', 'b', 'n',            /* 49 */
  'm', ',', '.', '/',   0,                      /* Right shift */
  '*',
    0,  /* Alt */
  ' ',  /* Space bar */
    0,  /* Caps lock */
    0,  /* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,  /* < ... F10 */
    0,  /* 69 - Num lock*/
    0,  /* Scroll Lock */
    0,  /* Home key */
    0,  /* Up Arrow */
    0,  /* Page Up */
  '-',
    0,  /* Left Arrow */
    0,
    0,  /* Right Arrow */
  '+',
    0,  /* 79 - End key*/
    0,  /* Down Arrow */
    0,  /* Page Down */
    0,  /* Insert Key */
    0,  /* Delete Key */
    0,   0,   0,
    0,  /* F11 Key */
    0,  /* F12 Key */
    0, /* All other keys are undefined */
};

static uint8_t shift_pressed = 0;

void keyboard_handler(void) {
    uint8_t scancode = inb(KB_PORT);
    
    if (scancode & 0x80) {
        // Key release
        input_push(1, scancode & 0x7F, 0, 0);
        scancode &= 0x7F;
        if (scancode == 0x2A || scancode == 0x36) {
            shift_pressed = 0;
        }
    } else {
        // Key press
        if (scancode == 0x2A || scancode == 0x36) {
            shift_pressed = 1;
        } else {
            char c = kbd_us_map[scancode];
            if (c) {
                if (shift_pressed && c >= 'a' && c <= 'z') c -= 32; // Uppercase
                else if (shift_pressed && c == '1') c = '!';
                else if (shift_pressed && c == '2') c = '@';
                else if (shift_pressed && c == '3') c = '#';
                else if (shift_pressed && c == '4') c = '$';
                else if (shift_pressed && c == '5') c = '%';
                else if (shift_pressed && c == '6') c = '^';
                else if (shift_pressed && c == '7') c = '&';
                else if (shift_pressed && c == '8') c = '*';
                else if (shift_pressed && c == '9') c = '(';
                else if (shift_pressed && c == '0') c = ')';
                else if (shift_pressed && c == '-') c = '_';
                else if (shift_pressed && c == '=') c = '+';
                else if (shift_pressed && c == '[') c = '{';
                else if (shift_pressed && c == ']') c = '}';
                else if (shift_pressed && c == ';') c = ':';
                else if (shift_pressed && c == '\'') c = '"';
                else if (shift_pressed && c == ',') c = '<';
                else if (shift_pressed && c == '.') c = '>';
                else if (shift_pressed && c == '/') c = '?';
                else if (shift_pressed && c == '\\') c = '|';
                
                int next_head = (kbd_head + 1) % BUFFER_SIZE;
                if (next_head != kbd_tail) {
                    kbd_buffer[kbd_head] = c;
                    kbd_head = next_head;
                }
                input_push(0, scancode, 0, 0);
            }
        }
    }
}

void keyboard_init(void) {
    printk("Keyboard initialized.\n");
}

char keyboard_getchar(void) {
    while (kbd_head == kbd_tail) {
        yield(); // wait for key
    }
    char c = kbd_buffer[kbd_tail];
    kbd_tail = (kbd_tail + 1) % BUFFER_SIZE;
    return c;
}

int keyboard_has_char(void) {
    return kbd_head != kbd_tail;
}
