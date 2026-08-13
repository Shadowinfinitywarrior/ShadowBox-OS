#include "kernel.h"
#include "malloc.h"
#include "kstring.h"
#include "vfs.h"
#include "pty.h"

#define PTY_MAX 8
#define PTY_BUF_SIZE 4096

typedef struct {
    int master_fd;   /* Placeholder for master side file descriptor */
    int slave_fd;    /* Placeholder for slave side file descriptor */
    char name[32];   /* Device name e.g., "pty0" */
    char m2s_buf[PTY_BUF_SIZE]; /* Master to Slave buffer */
    uint32_t m2s_head;
    uint32_t m2s_tail;
    char s2m_buf[PTY_BUF_SIZE]; /* Slave to Master buffer */
    uint32_t s2m_head;
    uint32_t s2m_tail;
} pty_device_t;

static pty_device_t pty_devices[PTY_MAX];
static int pty_count = 0;

void pty_subsystem_init(void) {
    printk(KERN_INFO "PTY: subsystem initialized\n");
    for (int i = 0; i < PTY_MAX; ++i) {
        pty_devices[i].master_fd = -1;
        pty_devices[i].slave_fd = -1;
        pty_devices[i].name[0] = '\0';
        pty_devices[i].m2s_head = pty_devices[i].m2s_tail = 0;
        pty_devices[i].s2m_head = pty_devices[i].s2m_tail = 0;
    }
    pty_count = 0;
}

int pty_create(void) {
    if (pty_count >= PTY_MAX)
        return -1;
    int idx = pty_count++;
    pty_device_t *dev = &pty_devices[idx];
    dev->master_fd = -1;
    dev->slave_fd = -1;
    dev->m2s_head = dev->m2s_tail = 0;
    dev->s2m_head = dev->s2m_tail = 0;
    /* Manually construct PTY name "pty<idx>" without snprintf */
    int _pos = 0;
    dev->name[_pos++] = 'p';
    dev->name[_pos++] = 't';
    dev->name[_pos++] = 'y';
    int _num = idx;
    char _buf[12];
    int _len = 0;
    do { _buf[_len++] = '0' + (_num % 10); _num /= 10; } while (_num);
    while (_len--) dev->name[_pos++] = _buf[_len];
    dev->name[_pos] = '\0';
    printk(KERN_INFO "PTY: created %s\n", dev->name);
    return idx;
}

/* Slave side write (process writes to PTY) */
int pty_write(int idx, const char *buf, size_t len) {
    if (idx < 0 || idx >= pty_count) return -1;
    pty_device_t *dev = &pty_devices[idx];
    int written = 0;
    for (size_t i = 0; i < len; ++i) {
        uint32_t next = (dev->m2s_head + 1) % PTY_BUF_SIZE;
        if (next == dev->m2s_tail) break; // buffer full
        dev->m2s_buf[dev->m2s_head] = buf[i];
        dev->m2s_head = next;
        ++written;
    }
    return written;
}

/* Slave side read (process reads from PTY) */
int pty_read(int idx, char *buf, size_t len) {
    if (idx < 0 || idx >= pty_count) return -1;
    pty_device_t *dev = &pty_devices[idx];
    int taken = 0;
    for (size_t i = 0; i < len; ++i) {
        if (dev->s2m_head == dev->s2m_tail) break; // empty
        buf[i] = dev->s2m_buf[dev->s2m_tail];
        dev->s2m_tail = (dev->s2m_tail + 1) % PTY_BUF_SIZE;
        ++taken;
    }
    return taken;
}

/* Master side write (desktop writes to PTY) */
int pty_master_write(int idx, const char *buf, size_t len) {
    if (idx < 0 || idx >= pty_count) return -1;
    pty_device_t *dev = &pty_devices[idx];
    int written = 0;
    for (size_t i = 0; i < len; ++i) {
        uint32_t next = (dev->s2m_head + 1) % PTY_BUF_SIZE;
        if (next == dev->s2m_tail) break; // buffer full
        dev->s2m_buf[dev->s2m_head] = buf[i];
        dev->s2m_head = next;
        ++written;
    }
    return written;
}

/* Master side read (desktop reads output from PTY) */
int pty_master_read(int idx, char *buf, size_t len) {
    if (idx < 0 || idx >= pty_count) return -1;
    pty_device_t *dev = &pty_devices[idx];
    int taken = 0;
    for (size_t i = 0; i < len; ++i) {
        if (dev->m2s_head == dev->m2s_tail) break; // empty
        buf[i] = dev->m2s_buf[dev->m2s_tail];
        dev->m2s_tail = (dev->m2s_tail + 1) % PTY_BUF_SIZE;
        ++taken;
    }
    return taken;
}

/* VFS read/write wrappers for PTY slave device */
uint32_t pty_vfs_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    (void)offset;
    int idx = (int)(uintptr_t)node->impl;
    int n = pty_read(idx, (char*)buffer, size);
    return n < 0 ? 0 : (uint32_t)n;
}
uint32_t pty_vfs_write(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer) {
    (void)offset;
    int idx = (int)(uintptr_t)node->impl;
    int n = pty_write(idx, (char*)buffer, size);
    return n < 0 ? 0 : (uint32_t)n;
}

/* Syscall wrapper for PTY creation */
static uint64_t sys_pty_create(uint64_t unused1, uint64_t unused2, uint64_t unused3, uint64_t unused4, uint64_t unused5) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    return (uint64_t)pty_create();
}

void pty_destroy(int idx) {
    if (idx < 0 || idx >= pty_count) return;
    pty_device_t *dev = &pty_devices[idx];
    dev->master_fd = -1;
    dev->slave_fd = -1;
    dev->name[0] = 0;
    dev->m2s_head = dev->m2s_tail = 0;
    dev->s2m_head = dev->s2m_tail = 0;
}
