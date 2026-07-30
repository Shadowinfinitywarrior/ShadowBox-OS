#include "sys.h"

static void print(const char *s) { sb_push(1, s, strlen(s)); }

static const char *fortunes[] = {
    "You will have a great day!",
    "UNIX is simple. It just takes a genius to understand its simplicity.",
    "A journey of a thousand miles begins with a single step.",
    "Bugs are a feature.",
    "404: Fortune not found.",
    "To err is human, to purr is feline.",
    "There's no place like 127.0.0.1",
    "Real programmers count from 0.",
    "I'm not lazy, I'm just on power-saving mode.",
    "May the source be with you."
};

void _start(void) {
    uint64_t tbuf[2]; sys_times(tbuf);
    uint64_t seed = tbuf[0];
    
    int index = seed % 10;
    print(fortunes[index]);
    print("\n");
    
    sb_terminate(0);
}
