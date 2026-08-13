// sys_sbrk.c — Simple wrapper providing sys_sbrk for C/C++ programs.
// This implementation mirrors the static _sbrk logic used in freestanding.c,
// exposing a globally visible function name expected by demo programs.

#include <stddef.h>
#include <stdint.h>

// Simple brk state – per‑process, static.
static uint64_t _brk_cur = 0;

// Allocate memory by adjusting the program break via the SYS_BRK system call (12).
void* sys_sbrk(long incr) {
    // Lazily fetch the current break if we haven't yet.
    if (!_brk_cur) {
        register long rax __asm__("rax") = 12;   // SYS_BRK
        register long rdi __asm__("rdi") = 0;    // request current break
        __asm__ volatile ("syscall" : "+r"(rax) : "r"(rdi) : "rcx", "r11", "memory");
        _brk_cur = (uint64_t)rax;
    }
    if (incr == 0) {
        return (void *)(uintptr_t)_brk_cur;
    }
    uint64_t new_brk = _brk_cur + (uint64_t)incr;
    register long rax __asm__("rax") = 12;
    register long rdi __asm__("rdi") = (long)new_brk;
    __asm__ volatile ("syscall" : "+r"(rax) : "r"(rdi) : "rcx", "r11", "memory");
    if ((int64_t)rax < 0) {
        return (void *)-1;
    }
    void *ret = (void *)(uintptr_t)_brk_cur;
    _brk_cur = (uint64_t)rax;
    return ret;
}
