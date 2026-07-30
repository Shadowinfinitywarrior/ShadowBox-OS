#ifndef BOOT_H
#define BOOT_H

#include "types.h"

#define BOOT_STAGE_NAME_MAX 48

struct boot_stage {
    char name[BOOT_STAGE_NAME_MAX];
    uint64_t start_tick;
    uint64_t end_tick;
};

#define MAX_BOOT_STAGES 64

void boot_stage_begin(const char *name);
void boot_stage_end(void);
void boot_stages_summary(void);

#endif
