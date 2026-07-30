#include "sys.h"

int atoi_octal(const char *str) {
    int res = 0;
    while (*str) {
        if (*str >= '0' && *str <= '7') {
            res = (res << 3) + (*str - '0');
        } else {
            return -1; // invalid
        }
        str++;
    }
    return res;
}

static void print(const char *str) {
    sb_push(1, str, strlen(str));
}

int main(int argc, char **argv) {
    if (argc < 3) {
        print("Usage: chmod <mode> <file>\n");
        return 1;
    }
    
    int mode = atoi_octal(argv[1]);
    if (mode < 0) {
        print("chmod: invalid mode\n");
        return 1;
    }
    
    int ret = sys_chmod(argv[2], mode);
    if (ret < 0) {
        print("chmod: failed to change permissions\n");
        return 1;
    }
    
    return 0;
}
