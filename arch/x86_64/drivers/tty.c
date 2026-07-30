#include "tty.h"
#include "keyboard.h"
#include "malloc.h"
#include "kernel.h"
#include "task.h"
#include "kstring.h"

vfs_node_t *tty_node = 0;

extern int serial_received(void);
extern char serial_read_char(void);

#define TTY_BUF_SIZE 1024
static char tty_line_buf[TTY_BUF_SIZE];
static int tty_buf_pos = 0;

static void tty_echo_char(char c) {
    if (c == '\n' || c == '\r') {
        printk("\n");
    } else if (c == '\b' || c == 127) {
        printk("\b \b");
    } else if (c >= ' ') {
        printk("%c", c);
    }
}

static int tty_read_pos = 0;
static int tty_line_ready = 0;

static uint32_t tty_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    (void)node; (void)offset;

    // Buffer input until a complete line (ended with newline) is ready
    while (!tty_line_ready) {
        while (!serial_received() && !keyboard_has_char()) {
            yield();
        }
        char c;
        if (keyboard_has_char()) {
            c = (char)keyboard_getchar();
        } else {
            c = (char)serial_read_char();
        }

        if (c == '\n' || c == '\r') {
            if (tty_buf_pos < TTY_BUF_SIZE - 1) {
                tty_line_buf[tty_buf_pos++] = '\n';
            }
            tty_echo_char('\n');
            tty_line_ready = 1;
            tty_read_pos = 0;
        } else if (c == '\b' || c == 127) {
            if (tty_buf_pos > 0) {
                tty_buf_pos--;
                tty_echo_char(c);
            }
        } else {
            if (tty_buf_pos < TTY_BUF_SIZE - 1) {
                tty_line_buf[tty_buf_pos++] = c;
            }
            tty_echo_char(c);
        }
    }

    // Copy from line buffer to user buffer
    uint32_t copied = 0;
    while (copied < size && tty_read_pos < tty_buf_pos) {
        buffer[copied++] = (uint8_t)tty_line_buf[tty_read_pos++];
    }

    // Reset line state if the entire line has been consumed
    if (tty_read_pos >= tty_buf_pos) {
        tty_buf_pos = 0;
        tty_read_pos = 0;
        tty_line_ready = 0;
    }

    return copied;
}

static uint32_t tty_write(vfs_node_t *node, uint32_t offset, uint32_t size, const uint8_t *buffer) {
    (void)node; (void)offset;
    for (uint32_t i = 0; i < size; i++) {
        printk("%c", buffer[i]);
    }
    return size;
}

void tty_init(void) {
    tty_node = (vfs_node_t *)kmalloc(sizeof(vfs_node_t));
    if (!tty_node) panic("tty_init: Out of memory");
    memset(tty_node, 0, sizeof(vfs_node_t));
    
    // Copy name "tty"
    tty_node->name[0] = 't';
    tty_node->name[1] = 't';
    tty_node->name[2] = 'y';
    tty_node->name[3] = '\0';
    
    tty_node->flags = FS_CHARDEVICE;
    tty_node->read = tty_read;
    tty_node->write = tty_write;
    
    printk(KERN_INFO "TTY VFS node initialized.\n");
}
