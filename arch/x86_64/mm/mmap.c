#include "mmap.h"
#include "kernel.h"

void mmap_init(void) {
    printk(KERN_DEBUG "MMAP: Initializing mmap and shared memory support...\n");
}

void* sys_mmap(UNUSED void *addr, UNUSED size_t length, UNUSED int prot, UNUSED int flags, UNUSED int fd, UNUSED size_t offset) {
    return (void*)-1;
}

int sys_munmap(UNUSED void *addr, UNUSED size_t length) {
    return -1;
}
