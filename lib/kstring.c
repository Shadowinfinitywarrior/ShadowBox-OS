#include "kstring.h"

int strcmp(const char *a, const char *b) {
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return *(const unsigned char*)a - *(const unsigned char*)b;
}

int strncmp(const char *a, const char *b, uint64_t n) {
    for (uint64_t i = 0; i < n; i++) {
        if (a[i] != b[i]) return (unsigned char)a[i] - (unsigned char)b[i];
        if (!a[i]) return 0;
    }
    return 0;
}

uint64_t strlen(const char *s) {
    uint64_t len = 0;
    while (s[len]) len++;
    return len;
}

char *strcpy(char *dest, const char *src) {
    char *ret = dest;
    while ((*dest++ = *src++));
    return ret;
}

char *strncpy(char *dest, const char *src, uint64_t n) {
    uint64_t i;
    for (i = 0; i < n && src[i]; i++) dest[i] = src[i];
    for (; i < n; i++) dest[i] = 0;
    return dest;
}

char *strcat(char *dest, const char *src) {
    char *ret = dest;
    while (*dest) dest++;
    while ((*dest++ = *src++));
    return ret;
}

void *memset(void *dest, uint8_t val, uint64_t len) {
    uint8_t *d = (uint8_t *)dest;
    for (uint64_t i = 0; i < len; i++) d[i] = val;
    return dest;
}

void *memcpy(void *dest, const void *src, uint64_t len) {
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;
    for (uint64_t i = 0; i < len; i++) d[i] = s[i];
    return dest;
}

void *memmove(void *dest, const void *src, uint64_t len) {
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;
    if (d < s) {
        for (uint64_t i = 0; i < len; i++) d[i] = s[i];
    } else {
        for (uint64_t i = len; i > 0; i--) d[i - 1] = s[i - 1];
    }
    return dest;
}

int memcmp(const void *a, const void *b, uint64_t len) {
    const uint8_t *ca = (const uint8_t *)a;
    const uint8_t *cb = (const uint8_t *)b;
    for (uint64_t i = 0; i < len; i++) {
        if (ca[i] != cb[i]) return ca[i] - cb[i];
    }
    return 0;
}

int abs(int j) {
    return j < 0 ? -j : j;
}
