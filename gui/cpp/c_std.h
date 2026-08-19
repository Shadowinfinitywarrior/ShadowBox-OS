#ifndef C_STD_H
#define C_STD_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Freestanding C++17 compatible declarations
// These match what the C++ standard library provides without noexcept
// to avoid conflicts with the host's stdlib

void* malloc(size_t size) noexcept;
void free(void* ptr) noexcept;
void* realloc(void* ptr, size_t size) noexcept;
void* memcpy(void* dst, const void* src, size_t n) noexcept;
void* memmove(void* dst, const void* src, size_t n) noexcept;
int strcmp(const char* s1, const char* s2) noexcept;

#ifdef __cplusplus
}
#endif

#endif // C_STD_H