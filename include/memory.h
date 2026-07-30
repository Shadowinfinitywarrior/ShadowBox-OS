#ifndef SHADOWBOX_MEMORY_H
#define SHADOWBOX_MEMORY_H

#include "types.h"

#define MEM_READ  (1 << 0)
#define MEM_WRITE (1 << 1)
#define MEM_EXEC  (1 << 2)

/*
 * user_ptr_t - User-space pointer with bounds
 * @ptr: Pointer to user memory
 * @len: Length of memory region
 */
typedef struct user_ptr {
    void *ptr;
    size_t len;
} user_ptr_t;

/*
 * is_user_page - Check if address is in user space
 * @addr: Address to check
 * Returns: true if user page, false otherwise
 */
bool is_user_page(const void *addr);

/*
 * validate_user_ptr - Validate user pointer with access flags
 * @uptr:  User pointer descriptor
 * @flags: Access flags (MEM_READ, MEM_WRITE, MEM_EXEC)
 * Returns: 0 on success, -1 on error
 */
int validate_user_ptr(user_ptr_t uptr, uint32_t flags);

/*
 * copy_from_user - Copy data from user space to kernel space
 * @kernel_dest: Kernel destination buffer
 * @user_src:    User source buffer
 * @len:         Number of bytes to copy
 * Returns: 0 on success, -1 on fault
 */
int copy_from_user(void *kernel_dest, const void *user_src, size_t len);

/*
 * copy_to_user - Copy data from kernel space to user space
 * @user_dest:   User destination buffer
 * @kernel_src:  Kernel source buffer
 * @len:         Number of bytes to copy
 * Returns: 0 on success, -1 on fault
 */
int copy_to_user(void *user_dest, const void *kernel_src, size_t len);

#endif
