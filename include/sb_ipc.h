#ifndef SB_IPC_H
#define SB_IPC_H

#include "types.h"

typedef struct sb_msg {
    uint64_t type;
    uint64_t data1;
    uint64_t data2;
    uint64_t data3;
    uint64_t data4;
} sb_msg_t;

// Message Types
#define SB_MSG_SYS       0x01
#define SB_MSG_VFS_READ  0x10
#define SB_MSG_VFS_WRITE 0x11
#define SB_MSG_VFS_OPEN  0x12
#define SB_MSG_IRQ       0x20

#endif
