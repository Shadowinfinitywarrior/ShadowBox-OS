#include "sys.h"

// Simple substring search (case sensitive)
static int contains_substring(const char *haystack, const char *needle) {
    if (!*needle) return 1;
    for (; *haystack; ++haystack) {
        const char *h = haystack;
        const char *n = needle;
        while (*h && *n && *h == *n) {
            ++h; ++n;
        }
        if (!*n) return 1; // match found
    }
    return 0;
}

static int my_strlen(const char *s) {
    int i = 0;
    while (s[i]) ++i;
    return i;
}

static void print(const char *s) {
    sb_push(1, s, my_strlen(s));
}

static int strcmp(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return *(const unsigned char*)a - *(const unsigned char*)b;
}

// Recursive directory search
static void search_dir(const char *base, const char *pattern) {
    int fd = sb_acquire(base, 0);
    if (fd < 0) return;
    struct dirent de;
    while (sys_getdents(fd, &de, sizeof(de)) > 0) {
        if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) continue;
        // Build full path
        char full[512];
        int pos = 0;
        // copy base
        const char *p = base;
        while (*p && pos < 511) full[pos++] = *p++;
        // ensure '/' separator
        if (pos > 0 && full[pos-1] != '/' && pos < 511) full[pos++] = '/';
        // copy name
        const char *n = de.name;
        while (*n && pos < 511) full[pos++] = *n++;
        full[pos] = '\0';
        // Print if name matches pattern
        if (contains_substring(de.name, pattern)) {
            print(full);
            print("\n");
        }
        // Try to treat as directory and recurse
        int subfd = sb_acquire(full, 0);
        if (subfd >= 0) {
            struct dirent test;
            int r = sys_getdents(subfd, &test, sizeof(test));
            sb_release(subfd);
            if (r > 0) {
                search_dir(full, pattern);
            }
        }
    }
    sb_release(fd);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        print("Usage: search <pattern> [path]\n");
        sb_terminate(1);
    }
    const char *pattern = argv[1];
    const char *path = (argc >= 3) ? argv[2] : ".";
    search_dir(path, pattern);
    sb_terminate(0);
    return 0;
}
