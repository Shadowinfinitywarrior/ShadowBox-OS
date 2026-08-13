#include "sys.h"

static inline int print(const char *s) {
    int len = 0;
    while (s[len]) len++;
    sb_push(1, s, len);
    return len;
}

static void print_uint(uint64_t v) {
    char buf[32];
    int i = 0;
    if (v == 0) {
        buf[i++] = '0';
    } else {
        uint64_t n = v;
        while (n > 0) {
            buf[i++] = '0' + (n % 10);
            n /= 10;
        }
        // reverse
        for (int l = 0, r = i - 1; l < r; ++l, --r) {
            char tmp = buf[l];
            buf[l] = buf[r];
            buf[r] = tmp;
        }
    }
    buf[i++] = '\n';
    sb_push(1, buf, i);
}

void _start(void) {
    // Simple clipboard set/get test
    char msg[] = "hello clipboard";
    uint64_t len = sizeof(msg) - 1;
    uint64_t set_ret = syscall3(SYS_CLIPBOARD_SET, (uint64_t)msg, len, 0);
    print("set returned: ");
    print_uint(set_ret);

    char out[64];
    uint64_t get_ret = syscall3(SYS_CLIPBOARD_GET, (uint64_t)out, sizeof(out), 0);
    print("get returned: ");
    print_uint(get_ret);
    if (get_ret < sizeof(out)) out[get_ret] = '\0';
    print("clipboard content: ");
    print(out);
    print("\n");

    // Buffer size limit test (>4096 bytes)
    char big[5000];
    for (int i = 0; i < 5000; ++i) big[i] = 'A' + (i % 26);
    uint64_t set_big = syscall3(SYS_CLIPBOARD_SET, (uint64_t)big, sizeof(big), 0);
    print("set big returned: ");
    print_uint(set_big);

    char big_out[6000];
    uint64_t get_big = syscall3(SYS_CLIPBOARD_GET, (uint64_t)big_out, sizeof(big_out), 0);
    print("get big returned: ");
    print_uint(get_big);
    // Show first 64 chars of the retrieved data
    uint64_t preview_len = (get_big < 64) ? get_big : 64;
    if (preview_len > 0) {
        char preview[65];
        for (uint64_t i = 0; i < preview_len; ++i) preview[i] = big_out[i];
        preview[preview_len] = '\0';
        print("first 64 chars: ");
        print(preview);
        print("\n");
    }

    sb_terminate(0);
}
