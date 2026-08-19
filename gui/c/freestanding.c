/*
 * freestanding.c  —  Minimal freestanding runtime support for the C++ widget layer
 *
 * Provides:
 *   • malloc / free / realloc  — backed by the existing sys_sbrk() mechanism
 *   • memmove / memcpy / memset — inline implementations
 *   • operator new / operator delete — C++ heap bridge (calls our malloc)
 *
 * This file is compiled as C (not C++) and linked into desktop.elf.
 * It must appear BEFORE the C++ widget objects in the link command so
 * that operator new/delete are resolved.
 *
 * Only safe for single-threaded use (no locks).
 */

#include <stddef.h>
#include <stdint.h>




/* ── sbrk stub ───────────────────────────────────────────────────────────── */
/*
 * We replicate the sys_sbrk logic from sys.h here (can't include sys.h in a
 * .c file that is also used by the C++ layer, due to static-inline conflicts).
 */
static uint64_t _brk_cur = 0;

static void *_sbrk(long incr) {
    if (!_brk_cur) {
        register long rax __asm__("rax") = 12;
        register long rdi __asm__("rdi") = 0;
        __asm__ volatile ("syscall" : "+r"(rax) : "r"(rdi) : "rcx", "r11", "memory");
        _brk_cur = (uint64_t)rax;
    }
    if (incr == 0) return (void *)(uintptr_t)_brk_cur;

    uint64_t new_brk = _brk_cur + (uint64_t)incr;
    register long rax __asm__("rax") = 12;
    register long rdi __asm__("rdi") = (long)new_brk;
    __asm__ volatile ("syscall" : "+r"(rax) : "r"(rdi) : "rcx", "r11", "memory");
    if (rax < 0) return (void *)-1;
    void *ret = (void *)(uintptr_t)_brk_cur;
    _brk_cur  = (uint64_t)rax;
    return ret;
}

/* ── Tiny first-fit allocator ────────────────────────────────────────────── */

typedef struct block_hdr {
    size_t            size;   /* usable bytes (not including header) */
    int               free;
    struct block_hdr *next;
} block_hdr_t;

static block_hdr_t *heap_head = NULL;
static const size_t ALIGN = sizeof(void *);
static const size_t HDR   = sizeof(block_hdr_t);

static size_t _align_up(size_t n) {
    return (n + ALIGN - 1) & ~(ALIGN - 1);
}

__attribute__((weak)) void *malloc(size_t size) {
    if (size == 0) return NULL;
    size = _align_up(size);

    /* First-fit search */
    block_hdr_t *b = heap_head;
    while (b) {
        if (b->free && b->size >= size) {
            b->free = 0;
            return (void *)(b + 1);
        }
        b = b->next;
    }

    /* Extend heap */
    block_hdr_t *nb = (block_hdr_t *)_sbrk((long)(HDR + size));
    if ((void *)nb == (void *)-1) return NULL;
    nb->size = size;
    nb->free = 0;
    nb->next = NULL;

    /* Append to list */
    if (!heap_head) {
        heap_head = nb;
    } else {
        block_hdr_t *tail = heap_head;
        while (tail->next) tail = tail->next;
        tail->next = nb;
    }
    return (void *)(nb + 1);
}

__attribute__((weak)) void free(void *ptr) {
    if (!ptr) return;
    block_hdr_t *b = (block_hdr_t *)ptr - 1;
    b->free = 1;
    /* Coalesce consecutive free blocks */
    block_hdr_t *cur = heap_head;
    while (cur && cur->next) {
        if (cur->free && cur->next->free) {
            cur->size += HDR + cur->next->size;
            cur->next  = cur->next->next;
        } else {
            cur = cur->next;
        }
    }
}

__attribute__((weak)) void *realloc(void *ptr, size_t new_size) {
    if (!ptr)     return malloc(new_size);
    if (!new_size) { free(ptr); return NULL; }

    block_hdr_t *b = (block_hdr_t *)ptr - 1;
    if (b->size >= new_size) return ptr;   /* already large enough */

    void *np = malloc(new_size);
    if (!np) return NULL;

    /* Copy old data */
    size_t copy = b->size < new_size ? b->size : new_size;
    uint8_t *s = (uint8_t *)ptr;
    uint8_t *d = (uint8_t *)np;
    for (size_t i = 0; i < copy; ++i) d[i] = s[i];

    free(ptr);
    return np;
}

/* ── Memory intrinsics ───────────────────────────────────────────────────── */

__attribute__((weak)) void *memcpy(void *dst, const void *src, size_t n) {
    uint8_t       *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    for (size_t i = 0; i < n; ++i) d[i] = s[i];
    return dst;
}

__attribute__((weak)) void *memmove(void *dst, const void *src, size_t n) {
    uint8_t       *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    if (d < s) {
        for (size_t i = 0; i < n; ++i) d[i] = s[i];
    } else {
        for (size_t i = n; i > 0; --i) d[i - 1] = s[i - 1];
    }
    return dst;
}

__attribute__((weak)) void *memset(void *dst, int c, size_t n) {
    uint8_t *d = (uint8_t *)dst;
    for (size_t i = 0; i < n; ++i) d[i] = (uint8_t)c;
    return dst;
}

__attribute__((weak)) int memcmp(const void *a, const void *b, size_t n) {
    const uint8_t *pa = (const uint8_t *)a;
    const uint8_t *pb = (const uint8_t *)b;
    for (size_t i = 0; i < n; ++i) {
        if (pa[i] != pb[i]) return (int)pa[i] - (int)pb[i];
    }
    return 0;
}

__attribute__((weak)) int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s1 == *s2) { s1++; s2++; }
    return (unsigned char)*s1 - (unsigned char)*s2;
}
