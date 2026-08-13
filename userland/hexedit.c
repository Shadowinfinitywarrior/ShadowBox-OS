// Hex Editor for ShadowBox OS
// Simple terminal-based hex editor supporting view, navigation, edit, and save.
// Compile with: $(CC) $(USER_CFLAGS) -no-pie -nostdlib userland/hexedit.c -o hexedit.elf

#include "sys.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>



#define MAX_FILE_SIZE 65536 // 64 KiB maximum file size for editing
#define BYTES_PER_ROW 16

static void print(const char *s) {
    // Simple wrapper using sb_push to stdout (fd 1)
    size_t len = 0;
    while (s[len]) len++;
    sb_push(1, s, len);
}

static const char hex_chars[] = "0123456789ABCDEF";

static void print_hex2(uint8_t v) {
    char buf[3];
    buf[0] = hex_chars[(v >> 4) & 0xF];
    buf[1] = hex_chars[v & 0xF];
    buf[2] = ' ';
    sb_push(1, buf, 3);
}

static void print_hex6(size_t v) {
    char buf[7];
    for (int i = 5; i >= 0; i--) {
        buf[i] = hex_chars[v & 0xF];
        v >>= 4;
    }
    buf[6] = ' ';
    sb_push(1, buf, 7);
}

static void print_dec(size_t v) {
    char buf[32];
    int idx = 0;
    if (v == 0) {
        buf[idx++] = '0';
    } else {
        size_t tmp = v;
        while (tmp > 0) {
            buf[idx++] = '0' + (tmp % 10);
            tmp /= 10;
        }
        for (int i = 0; i < idx/2; i++) {
            char t = buf[i];
            buf[i] = buf[idx-1-i];
            buf[idx-1-i] = t;
        }
    }
    sb_push(1, buf, idx);
}

// Read a line of input from stdin (fd 0), echoing characters.
static int readline(char *buf, int max) {
    int i = 0;
    while (i < max - 1) {
        char c;
        if (sb_pull(0, &c, 1) <= 0) break;
        if (c == '\n' || c == '\r') {
            sb_push(1, "\n", 1);
            break;
        }
        if (c == '\b' || c == 127) { // Backspace
            if (i > 0) {
                sb_push(1, "\b \b", 3);
                i--;
            }
        } else {
            sb_push(1, &c, 1);
            buf[i++] = c;
        }
    }
    buf[i] = '\0';
    return i;
}

static int load_file(const char *path, uint8_t *buf, size_t *size) {
    int fd = sb_acquire(path, 0);
    if (fd < 0) return -1;
    int n = sb_pull(fd, buf, MAX_FILE_SIZE);
    sb_release(fd);
    if (n < 0) n = 0;
    *size = (size_t)n;
    return 0;
}

static int save_file(const char *path, const uint8_t *buf, size_t size) {
    // Create or truncate the file, then write all bytes.
    int fd = sb_acquire(path, 0x40 | 0x1 | 0x200); // CREATE | PUSH | TRUNC
    if (fd < 0) return -1;
    size_t off = 0;
    while (off < size) {
        size_t chunk = size - off;
        if (chunk > 4096) chunk = 4096;
        sb_push(fd, buf + off, chunk);
        off += chunk;
    }
    sb_release(fd);
    return 0;
}

static void display(const char *filename, const uint8_t *buf, size_t size, size_t cursor) {
    print("\033[2J\033[H"); // Clear screen
    print("Hex Editor - ");
    print(filename[0] ? filename : "(new)");
    print("\n\n");
    // Header row
    print("Offset   ");
    for (int b = 0; b < BYTES_PER_ROW; b++) {
        print_hex2((uint8_t)b);
    }
    print("  ASCII\n");
    print("-----------------------------------------------------------\n");

    size_t rows = (size + BYTES_PER_ROW - 1) / BYTES_PER_ROW;
    if (rows > 24) rows = 24; // Limit to visible rows similar to other UI
    for (size_t r = 0; r < rows; ++r) {
        print_hex6(r * BYTES_PER_ROW);
        for (int c = 0; c < BYTES_PER_ROW; ++c) {
            size_t idx = r * BYTES_PER_ROW + c;
            if (idx < size) {
                // Highlight cursor byte
                if (idx == cursor) {
                    print("\033[7m");
                    print_hex2(buf[idx]);
                    print("\033[0m");
                } else {
                    print_hex2(buf[idx]);
                }
            } else {
                print("   ");
            }
        }
        // ASCII representation
        print(" ");
        for (int c = 0; c < BYTES_PER_ROW; ++c) {
            size_t idx = r * BYTES_PER_ROW + c;
            if (idx < size) {
                char ch = buf[idx];
                if (ch >= 32 && ch < 127) {
                    char a[2] = {ch, 0};
                    print(a);
                } else {
                    print(".");
                }
            } else {
                print(" ");
            }
        }
        print("\n");
    }
    print("\nCommands: h(left) l(right) k(up) j(down) e(edit byte) g(goto offset) s(save) q(quit)\n");
    print("Cursor: ");
    print_dec(cursor);


    print("\n");
}

static int parse_hex_digit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
}

static int parse_hex_byte(const char *s, uint8_t *out) {
    int hi = parse_hex_digit(s[0]);
    int lo = parse_hex_digit(s[1]);
    if (hi < 0 || lo < 0) return -1;
    *out = (uint8_t)((hi << 4) | lo);
    return 0;
}

int main(void) {
    char filename[128] = {0};
    uint8_t data[MAX_FILE_SIZE];
    size_t file_size = 0;
    size_t cursor = 0;

    print("Enter filename (empty for new): ");
    readline(filename, sizeof(filename));
    if (filename[0]) {
        if (load_file(filename, data, &file_size) != 0) {
            print("Failed to load file, starting empty.\n");
            file_size = 0;
        }
    }
    // Main loop
    while (1) {
        display(filename, data, file_size, cursor);
        // Read a single command character
        char c;
        // Wait for input byte
        if (sb_pull(0, &c, 1) <= 0) continue;
        // Echo for user feedback
        sb_push(1, &c, 1);
        if (c == '\r' || c == '\n') continue;
        // Navigation
        if (c == 'h') {
            if (cursor > 0) cursor--;
        } else if (c == 'l') {
            if (cursor + 1 < file_size) cursor++;
        } else if (c == 'k') {
            if (cursor >= BYTES_PER_ROW) cursor -= BYTES_PER_ROW;
        } else if (c == 'j') {
            if (cursor + BYTES_PER_ROW < file_size) cursor += BYTES_PER_ROW;
        } else if (c == 'g') {
            // Go to offset
            print("\nGoto offset (hex): ");
            char offbuf[16];
            readline(offbuf, sizeof(offbuf));
            if (offbuf[0]) {
                // parse hex number
                uint64_t off = 0;
                for (int i = 0; offbuf[i]; ++i) {
                    int v = parse_hex_digit(offbuf[i]);
                    if (v < 0) break;
                    off = (off << 4) | (uint64_t)v;
                }
                if (off < file_size) cursor = (size_t)off;
            }
        } else if (c == 'e') {
            // Edit current byte
            print("\nNew byte (hex, 2 chars): ");
            char bytebuf[4];
            readline(bytebuf, sizeof(bytebuf));
            if (strlen(bytebuf) >= 2) {
                uint8_t newval;
                if (parse_hex_byte(bytebuf, &newval) == 0) {
                    if (cursor >= file_size) {
                        // extend file if needed (up to max)
                        if (cursor < MAX_FILE_SIZE) {
                            data[cursor] = newval;
                            file_size = cursor + 1;
                        }
                    } else {
                        data[cursor] = newval;
                    }
                }
            }
        } else if (c == 's') {
            if (filename[0]) {
                if (save_file(filename, data, file_size) == 0) {
                    print("\nSaved.\n");
                } else {
                    print("\nSave failed.\n");
                }
            } else {
                print("\nEnter filename to save: ");
                char newname[128];
                readline(newname, sizeof(newname));
                if (newname[0]) {
            {
                int i = 0;
                for (; i < sizeof(filename)-1 && newname[i]; ++i) {
                    filename[i] = newname[i];
                }
                filename[i] = '\0';
            }
                    if (save_file(filename, data, file_size) == 0) {
                        print("\nSaved.\n");
                    } else {
                        print("\nSave failed.\n");
                    }
                }
            }
        } else if (c == 'q') {
            // Quit
            break;
        }
        // Simple paging: if cursor beyond visible rows, adjust? Not required for basic implementation.
    }
    return 0;
}
