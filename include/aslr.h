#ifndef SHADOWBOX_ASLR_H
#define SHADOWBOX_ASLR_H

#include "types.h"

void aslr_init(void);
uint64_t aslr_random_addr(uint64_t base, uint64_t range, uint64_t align);

uint64_t aslr_get_mmap_base(void);
uint64_t aslr_get_stack_base(void);
uint64_t aslr_get_heap_base(void);
uint64_t aslr_get_kernel_base(void);
uint64_t aslr_get_kernel_offset(void);

void aslr_enable_smep(void);
void aslr_enable_smap(void);

#endif
