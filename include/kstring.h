#ifndef SHADOWBOX_STRING_H
#define SHADOWBOX_STRING_H

#include "types.h"

int      strcmp(const char *a, const char *b);
int      strncmp(const char *a, const char *b, uint64_t n);
uint64_t strlen(const char *s);
char    *strcpy(char *dest, const char *src);
char    *strncpy(char *dest, const char *src, uint64_t n);
char    *strcat(char *dest, const char *src);
void    *memset(void *dest, uint8_t val, uint64_t len);
void    *memcpy(void *dest, const void *src, uint64_t len);
void    *memmove(void *dest, const void *src, uint64_t len);
int      memcmp(const void *a, const void *b, uint64_t len);
int      abs(int j);

#endif
