#ifndef SHADOWBOX_ERROR_H
#define SHADOWBOX_ERROR_H

#include "errno.h"

extern __thread int current_errno;

#define SET_ERRNO(err) (current_errno = (err))
#define GET_ERRNO() (current_errno)
#define CLEAR_ERRNO() (current_errno = 0)

#define CHECK_ERR(ptr) do { if ((ptr) == NULL) { SET_ERRNO(ENOMEM); return NULL; } } while(0)
#define CHECK_ERR_INT(val) do { if ((val) < 0) { SET_ERRNO(-(val)); return -1; } } while(0)

/*
 * propagate_errno - Propagate negative errno to thread-local storage
 * @ret: Return value (negative errno or >= 0)
 * Returns: 0 on success, -1 on error
 */
static inline int propagate_errno(int ret) {
    if (ret < 0) {
        SET_ERRNO(-ret);
        return -1;
    }
    return ret;
}

/*
 * propagate_errno_ptr - Check NULL pointer and set errno
 * @ptr: Pointer to check
 * Returns: ptr, or NULL with errno set to ENOMEM
 */
static inline void* propagate_errno_ptr(void* ptr) {
    if (ptr == NULL) {
        SET_ERRNO(ENOMEM);
    }
    return ptr;
}

/*
 * strerror - Get error string for errno value
 * @errnum: Error number
 * Returns: Pointer to error string
 */
const char* strerror(int errnum);

/*
 * strerror_r - Get error string (reentrant)
 * @errnum: Error number
 * @buf:    Destination buffer
 * @buflen: Buffer length
 * Returns: Pointer to error string
 */
const char* strerror_r(int errnum, char *buf, size_t buflen);

/*
 * log_error - Log an error message
 * @func:   Function name
 * @errnum: Error number
 */
void log_error(const char *func, int errnum);

/*
 * log_warning - Log a warning message
 * @func: Function name
 * @msg:  Warning message
 */
void log_warning(const char *func, const char *msg);

/*
 * log_info - Log an info message
 * @func: Function name
 * @msg:  Info message
 */
void log_info(const char *func, const char *msg);

/*
 * error_context_t - Error context tracking entry
 */
typedef struct error_context {
    const char *function;
    const char *file;
    int line;
    int error_code;
    struct error_context *next;
} error_context_t;

/*
 * push_error_context - Push error context onto stack
 * @function:   Function name
 * @file:       Source file name
 * @line:       Source line number
 * @error_code: Error code
 */
void push_error_context(const char *function, const char *file, int line, int error_code);

/*
 * pop_error_context - Pop error context from stack
 */
void pop_error_context(void);

/*
 * print_error_stack - Print all error contexts
 */
void print_error_stack(void);

/*
 * ASSERT - Assert condition or panic
 * @cond: Condition that must be true
 */
#define ASSERT(cond) do { \
    if (!(cond)) { \
        push_error_context(__func__, __FILE__, __LINE__, EINVAL); \
        panic("Assertion failed: " #cond); \
    } \
} while(0)

/*
 * ASSERT_PTR - Assert pointer is non-NULL, else return NULL
 * @ptr: Pointer to check
 */
#define ASSERT_PTR(ptr) do { \
    if ((ptr) == NULL) { \
        push_error_context(__func__, __FILE__, __LINE__, EFAULT); \
        SET_ERRNO(EFAULT); \
        return NULL; \
    } \
} while(0)

/*
 * ASSERT_RET - Assert condition or return error value
 * @cond: Condition that must be true
 * @ret:  Return value on failure
 */
#define ASSERT_RET(cond, ret) do { \
    if (!(cond)) { \
        push_error_context(__func__, __FILE__, __LINE__, EINVAL); \
        SET_ERRNO(EINVAL); \
        return (ret); \
    } \
} while(0)

/*
 * RETURN_ERROR - Set errno and return -1
 * @err: Error number
 */
#define RETURN_ERROR(err) do { \
    SET_ERRNO(err); \
    push_error_context(__func__, __FILE__, __LINE__, err); \
    return -1; \
} while(0)

/*
 * RETURN_ERROR_PTR - Set errno and return NULL
 * @err: Error number
 */
#define RETURN_ERROR_PTR(err) do { \
    SET_ERRNO(err); \
    push_error_context(__func__, __FILE__, __LINE__, err); \
    return NULL; \
} while(0)

/*
 * SYSCALL_CHECK - Check syscall return, set errno on failure
 * @ret: Return value to check
 */
#define SYSCALL_CHECK(ret) do { \
    if ((ret) < 0) { \
        SET_ERRNO(-(ret)); \
        push_error_context(__func__, __FILE__, __LINE__, -(ret)); \
        return -1; \
    } \
} while(0)

#endif
