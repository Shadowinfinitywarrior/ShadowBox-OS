#ifndef C_STD_H
#define C_STD_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void* malloc(size_t size);
void free(void* ptr);
void* realloc(void* ptr, size_t size);
void* memcpy(void* dst, const void* src, size_t n);
void* memmove(void* dst, const void* src, size_t n);
int strcmp(const char* s1, const char* s2);

#ifdef __cplusplus
}
#endif

#endif // C_STD_H