#ifndef SHADOWBOX_COMPILER_H
#define SHADOWBOX_COMPILER_H

#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define barrier() __asm__ volatile("" ::: "memory")

#define __packed   __attribute__((packed))
#define __aligned(x) __attribute__((aligned(x)))
#define __noreturn __attribute__((noreturn))
#define __unused   __attribute__((unused))
#define UNUSED     __attribute__((unused))
#define __used     __attribute__((used))
#define __section(x) __attribute__((section(x)))
#define __printf(a,b) __attribute__((format(printf,a,b)))

#define container_of(ptr, type, member) ({ \
    const typeof(((type *)0)->member) *__mptr = (ptr); \
    (type *)((char *)__mptr - offsetof(type, member)); })

#define min(a,b) ({ typeof(a) _a = (a); typeof(b) _b = (b); _a < _b ? _a : _b; })
#define max(a,b) ({ typeof(a) _a = (a); typeof(b) _b = (b); _a > _b ? _a : _b; })
#define clamp(v,lo,hi) min(max((v),(lo)),(hi))
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define NO_RETURN __attribute__((noreturn))

#define STACK_CANARY_VALUE 0xDEADBEEFCAFEBABEULL

static inline void set_stack_canary(void) {
    uint64_t canary = STACK_CANARY_VALUE;
    __asm__ volatile("mov %0, %%gs:0x28" :: "r"(canary));
}

#endif
