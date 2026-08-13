#ifndef SHADOWBOX_FAT32_H
#define SHADOWBOX_FAT32_H

#include "types.h"
#include "vfs.h"
#include "block.h"

void fat32_init(void);
int fat32_mount(void);

#endif
