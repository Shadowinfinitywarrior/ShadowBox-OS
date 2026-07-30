#include "sys.h"
#include "string.h" // Wait, I might need to implement strlen and print

int print(const char *str) {
    int len = 0;
    while (str[len]) len++;
    return sb_push(1, str, len);
}

void _start(void) {
    print("Hello from spawned process!\n");
    sb_terminate(0);
}
