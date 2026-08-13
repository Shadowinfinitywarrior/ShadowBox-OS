#ifndef SHADOWBOX_LIBC_ENV_H
#define SHADOWBOX_LIBC_ENV_H

#include "types.h"

// C Standard Library (POSIX compatibility layer & musl/custom libc abstraction)

// Dynamic Linker / Loader (ld.so) Structures
typedef struct elf_shared_object {
    char name[256];
    uint64_t load_address;
    uint64_t dynamic_section;
    
    // Symbol resolution tables
    void *symtab;
    void *strtab;
    
    struct elf_shared_object *next;
} elf_shared_object_t;

// POSIX System Call Wrappers (libc ABI boundary to kernel)
int _posix_open(const char *pathname, int flags);
long _posix_read(int fd, void *buf, unsigned long count);
long _posix_write(int fd, const void *buf, unsigned long count);

// pthreads abstraction (maps heavily to task.h thread_t via futex)
typedef uint32_t pthread_t;
typedef struct {
    uint32_t lock;
} pthread_mutex_t;

int pthread_create(pthread_t *thread, void *attr, void *(*start_routine) (void *), void *arg);
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);

// Dynamic Linker API
void* dlopen(const char *filename, int flag);
void* dlsym(void *handle, const char *symbol);

#endif
