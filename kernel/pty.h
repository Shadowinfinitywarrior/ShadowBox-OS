#ifndef SHADOWBOX_PTY_H
#define SHADOWBOX_PTY_H

#define PTY_MAX 8
#define PTY_BUF_SIZE 4096

int pty_create(void);
int pty_write(int idx, const char *buf, size_t len);
int pty_read(int idx, char *buf, size_t len);
int pty_master_write(int idx, const char *buf, size_t len);
int pty_master_read(int idx, char *buf, size_t len);
void pty_subsystem_init(void);
void pty_destroy(int idx);

/* VFS wrapper prototypes */
#include "vfs.h"
uint32_t pty_vfs_read(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);
uint32_t pty_vfs_write(vfs_node_t *node, uint32_t offset, uint32_t size, uint8_t *buffer);

#endif
