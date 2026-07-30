#include "memory.h"
#include "kstring.h"
#include "errno.h"

#define USER_SPACE_END 0x00007FFFFFFFFFFF

bool is_user_page(const void *addr) {
    return (uint64_t)addr < USER_SPACE_END;
}

int validate_user_ptr(user_ptr_t uptr, uint32_t flags) {
    (void)flags;

    uint64_t start = (uint64_t)uptr.ptr;
    uint64_t end = start + uptr.len;

    if (end < start) return -EFAULT;

    if (end > USER_SPACE_END) return -EFAULT;

    return 0;
}

int copy_from_user(void *kernel_dest, const void *user_src, size_t len) {
    user_ptr_t uptr = { (void*)user_src, len };
    if (validate_user_ptr(uptr, MEM_READ) != 0) {
        return -EFAULT;
    }
    memcpy(kernel_dest, user_src, len);
    return 0;
}

int copy_to_user(void *user_dest, const void *kernel_src, size_t len) {
    user_ptr_t uptr = { (void*)user_dest, len };
    if (validate_user_ptr(uptr, MEM_WRITE) != 0) {
        return -EFAULT;
    }
    memcpy(user_dest, kernel_src, len);
    return 0;
}
