#ifndef SHADOWBOX_FCNTL_H
#define SHADOWBOX_FCNTL_H

#define SB_MODE_PULL    0
#define SB_MODE_PUSH    1
#define SB_MODE_PULLPUSH      2
#define SB_MODE_CREATE     0x40
#define O_EXCL      0x80
#define O_NOCTTY    0x100
#define SB_MODE_TRUNC     0x200
#define SB_MODE_APPEND    0x400
#define O_NONBLOCK  0x800
#define O_CLOEXEC   0x10000

#define F_DUPFD     0
#define F_GETFD     1
#define F_SETFD     2
#define F_GETFL     3
#define F_SETFL     4

#endif
