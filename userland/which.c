#include "sys.h"

/* which - locate an executable in the default search path */
static const char *search_dirs[] = {
    "/", "/bin", "/usr/bin", "/sbin", "/usr/sbin", "/tmp", "/dev", 0
};

static void print(const char *s) { sb_push(1, s, strlen(s)); }

int main(int argc, char **argv) {
    if (argc < 2) {
        print("Usage: which <program> [program...]\n");
        sb_terminate(1);
    }

    int found_any = 0;
    for (int a = 1; a < argc; a++) {
        const char *name = argv[a];
        /* If it has a path separator, test it directly */
        int has_slash = 0;
        for (const char *p = name; *p; p++) if (*p == '/') has_slash = 1;
        if (has_slash) {
            if (sys_access(name, 0) == 0) { print(name); print("\n"); found_any = 1; }
            continue;
        }
        for (int d = 0; search_dirs[d]; d++) {
            for (int try_elf = 0; try_elf < 2; try_elf++) {
                char path[160];
                int i = 0;
                const char *dp = search_dirs[d];
                while (*dp && i < 159) path[i++] = *dp++;
                if (i > 0 && path[i-1] != '/') path[i++] = '/';
                const char *np = name;
                while (*np && i < 159) path[i++] = *np++;
                if (try_elf) {
                    path[i++] = '.';
                    path[i++] = 'e';
                    path[i++] = 'l';
                    path[i++] = 'f';
                }
                path[i] = '\0';
                if (sys_access(path, 0) == 0) {
                    print(path);
                    print("\n");
                    found_any = 1;
                    break;
                }
            }
            if (found_any) break;
        }
        if (!found_any) { print(name); print(": not found\n"); }
        found_any = 0;
    }
    sb_terminate(0);
    return 0;
}
