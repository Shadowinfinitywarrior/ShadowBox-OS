#ifndef SHADOWBOX_TYPES_H
#define SHADOWBOX_TYPES_H

typedef __builtin_va_list va_list;
#define va_start __builtin_va_start
#define va_arg   __builtin_va_arg
#define va_end   __builtin_va_end

typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;
typedef unsigned int        uint32_t;
typedef unsigned long long  uint64_t;

typedef signed char         int8_t;
typedef short               int16_t;
typedef int                 int32_t;
typedef long long           int64_t;

typedef uint64_t            size_t;
typedef int64_t             ssize_t;
typedef uint64_t            uintptr_t;
typedef int64_t             intptr_t;

typedef int32_t             pid_t;
typedef uint32_t            uid_t;
typedef uint32_t            gid_t;
typedef int64_t             time_t;
typedef int32_t             clockid_t;
typedef uint32_t            mode_t;
typedef uint64_t            off_t;
typedef int64_t             nlink_t;
typedef uint64_t            dev_t;
typedef uint64_t            ino_t;

typedef int32_t             bool;
#define true                1
#define false               0

#define NULL                ((void *)0)

#define offsetof(type, member) ((size_t)&((type *)0)->member)

#define PAGE_SIZE           4096

#endif
