#include "sys.h"
#include "syscall.h"
#include "mmap.h"


static inline void* sys_mmap_inline(void* addr, size_t length, int prot, int flags, int fd, size_t offset) {
    register uint64_t rax __asm__("rax") = SYS_MMAP;
    register uint64_t rdi __asm__("rdi") = (uint64_t)addr;
    register uint64_t rsi __asm__("rsi") = length;
    register uint64_t rdx __asm__("rdx") = prot;
    register uint64_t r10 __asm__("r10") = flags;
    register uint64_t r8  __asm__("r8")  = (uint64_t)fd;
    register uint64_t r9  __asm__("r9")  = offset;
    __asm__ volatile ("syscall"
        : "+a" (rax)
        : "r" (rdi), "r" (rsi), "r" (rdx), "r" (r10), "r" (r8), "r" (r9)
        : "rcx", "r11", "memory");
    return (void*)rax;
}
static inline void print(const char *s) {
    size_t len = 0;
    while (s[len]) ++len;
    sb_push(1, s, len);
}

int main(void) {
    void *p = sys_mmap_inline(NULL, 4096, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if ((uint64_t)p > (uint64_t)-4095ULL) {
        print("mmap failed\n");
        sb_terminate(1);
    }
    // Write a RET instruction (0xC3)
    ((unsigned char*)p)[0] = 0xC3;
    print("Calling function\n");
    int (*func)(void) = p;
    func();
    print("Returned normally (unexpected)\n");
    sb_terminate(0);
    return 0;
}
