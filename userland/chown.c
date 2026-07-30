#include "sys.h"

int atoi(const char *str) {
    int res = 0;
    int sign = 1;
    if (*str == '-') {
        sign = -1;
        str++;
    }
    while (*str) {
        if (*str >= '0' && *str <= '9') {
            res = res * 10 + (*str - '0');
        } else {
            break;
        }
        str++;
    }
    return res * sign;
}

static void print(const char *str) {
    sb_push(1, str, strlen(str));
}

int main(int argc, char **argv) {
    if (argc < 3) {
        print("Usage: chown <uid>:<gid> <file>\n");
        return 1;
    }
    
    int uid = -1;
    int gid = -1;
    
    char *colon = 0;
    char *str = argv[1];
    for (int i = 0; str[i]; i++) {
        if (str[i] == ':') {
            colon = &str[i];
            break;
        }
    }
    
    if (colon) {
        *colon = 0;
        if (str[0]) uid = atoi(str);
        if (colon[1]) gid = atoi(colon + 1);
        *colon = ':'; // restore
    } else {
        uid = atoi(str);
    }
    
    int ret = sys_chown(argv[2], uid, gid);
    if (ret < 0) {
        print("chown: failed to change ownership\n");
        return 1;
    }
    
    return 0;
}
