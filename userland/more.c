#include "sys.h"

static void print(const char *s) { sb_push(1, s, strlen(s)); }

void _start(void) {
    print("Enter filename to view: ");
    char filename[128];
    int len = 0;
    while (len < 127) {
        char c;
        if (sb_pull(0, &c, 1) <= 0) break;
        if (c == '\n' || c == '\r') { print("\n"); break; }
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
    if (fd < 0) { print("File not found.\n"); sb_terminate(1); }

    char buf[1];
    int lines = 0;
    while (sb_pull(fd, buf, 1) > 0) {
        sb_push(1, buf, 1);
        if (buf[0] == '\n') {
            lines++;
            if (lines >= 20) {
                print("\033[7m--More--(Press Space to continue, q to quit)\033[0m");
                char cmd;
                while (1) {
                    if (sb_pull(0, &cmd, 1) > 0) {
                        if (cmd == ' ' || cmd == '\n' || cmd == '\r') {
                            print("\r                                            \r");
                            lines = 0;
                            break;
                        } else if (cmd == 'q' || cmd == 'Q') {
                            print("\r                                            \r");
                            sb_release(fd);
                            sb_terminate(0);
                        }
                    }
                }
            }
        }
    }
    sb_release(fd);
    sb_terminate(0);
}
