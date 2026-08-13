#include "sys.h"
#include <string.h>

static void print(const char *s) {
    sb_push(1, s, strlen(s));
}

void _start(void) {
    const char *path = "graphify-out/graph.html";
    int fd = sb_acquire(path, 0);
    if (fd < 0) {
        print("Graphify Viewer: file not found: ");
        print(path);
        print("\n");
        sb_terminate(1);
        return;
    }
    char buf[4096];
    uint64_t got;
    while ((got = sb_pull(fd, buf, sizeof(buf))) > 0) {
        sb_push(1, buf, got);
    }
    sb_release(fd);
    sb_terminate(0);
}
