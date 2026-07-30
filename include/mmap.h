#ifndef SHADOWBOX_MMAP_H
#define SHADOWBOX_MMAP_H

#include "types.h"

#define PROT_NONE       0x0
#define PROT_READ       0x1
#define PROT_WRITE      0x2
#define PROT_EXEC       0x4

#define MAP_SHARED      0x01
#define MAP_PRIVATE     0x02
#define MAP_FIXED       0x10
#define MAP_ANONYMOUS   0x20

/*
 * mmap_init - Initialize mmap subsystem
 */
void mmap_init(void);

/*
 * sys_mmap - Map files or devices into memory
 * @addr:    Hint for mapping address
 * @length:  Length of mapping
 * @prot:    Memory protection flags
 * @flags:   Mapping type and options
 * @fd:      File descriptor
 * @offset:  File offset
 * Returns:  Mapped address
 */
void* sys_mmap(void *addr, size_t length, int prot, int flags, int fd, size_t offset);

/*
 * sys_munmap - Unmap memory region
 * @addr:   Address to unmap
 * @length: Length of region
 * Returns: 0 on success, -1 on error
 */
int sys_munmap(void *addr, size_t length);

#endif
