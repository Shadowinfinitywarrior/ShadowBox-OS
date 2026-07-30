#include "sys.h"

static void print(const char *s) {
    sb_push(1, s, strlen(s));
}

void _start(void) {
    print("=== ShadowBox Text Editor ===\n");
    print("Enter filename to edit: ");
    char filename[128];
    int len = 0;
    while (len < 127) {
        char c;
        if (sb_pull(0, &c, 1) <= 0) break;
        if (c == '\n' || c == '\r') {
            print("\n");
            break;
        }
        if (c == '\b' || c == 127) {
            if (len > 0) { print("\b \b"); len--; }
        } else {
            sb_push(1, &c, 1);
            filename[len++] = c;
        }
    }
    filename[len] = '\0';
    if (len == 0) sb_terminate(1);

    int fd = sb_acquire(filename, 0);
    char buffer[4096];
    int content_len = 0;
    if (fd >= 0) {
        content_len = sb_pull(fd, buffer, 4095);
        if (content_len < 0) content_len = 0;
        sb_release(fd);
    }
    buffer[content_len] = '\0';

    print("\n--- File Content (Press Ctrl+D to save, Ctrl+C to abort) ---\n");
    print(buffer);

    while (content_len < 4095) {
        char c;
        if (sb_pull(0, &c, 1) <= 0) break;
        if (c == 4) break; // Ctrl+D
        if (c == 3) {      // Ctrl+C
            print("\nAborted.\n");
            sb_terminate(0);
        }
        if (c == '\r') c = '\n';
        if (c == '\b' || c == 127) {
            if (content_len > 0) { print("\b \b"); content_len--; }
        } else {
            sb_push(1, &c, 1);
            buffer[content_len++] = c;
        }
    }
    buffer[content_len] = '\0';
    
    fd = sb_acquire(filename, 0x40 | 0x1 | 0x200);
    if (fd >= 0) {
        sb_push(fd, buffer, content_len);
        sb_release(fd);
        print("\nSaved.\n");
    } else {
        print("\nFailed to save.\n");
    }
    sb_terminate(0);
}
