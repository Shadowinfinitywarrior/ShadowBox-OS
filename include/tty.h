#ifndef SHADOWBOX_TTY_H
#define SHADOWBOX_TTY_H

#include "vfs.h"

/*
 * tty_init - Initialize TTY subsystem and create /dev/tty node
 */
void tty_init(void);

extern vfs_node_t *tty_node;

#endif
