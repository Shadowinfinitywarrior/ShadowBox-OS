#include <stdio.h>
#include <stdint.h>
#include <string.h>
static uint32_t parse_size(const char *in) {
    uint32_t size = 0;
    uint32_t j;
    uint32_t count = 1;
    for (j = 11; j > 0; j--, count *= 8) {
        size += ((in[j - 1] - '0') * count);
    }
    return size;
}
int main() {
    FILE *f = fopen("initrd.tar", "rb");
    char buf[512];
    while (fread(buf, 1, 512, f) == 512) {
        if (buf[0] == 0) break;
        uint32_t sz = parse_size(buf + 124);
        printf("%s: %u\n", buf, sz);
        uint32_t blocks = (sz + 511) / 512;
        fseek(f, blocks * 512, SEEK_CUR);
    }
    return 0;
}
