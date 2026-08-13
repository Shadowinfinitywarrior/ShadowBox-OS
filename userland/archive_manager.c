#include "sys.h"
/* tar_header - TAR archive file header */
struct tar_header {
    char filename[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    char typeflag[1];
};
#include <string.h>

static void print(const char *s) {
    sb_push(1, s, strlen(s));
}

/* Convert octal size string to integer */
static uint32_t parse_size(const char *in) {
    uint32_t size = 0;
    uint32_t mult = 1;
    for (int i = 11; i >= 0; i--) {
        size += ((in[i] - '0') * mult);
        mult *= 8;
    }
    return size;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        print("Usage: archive_manager <tarfile> [list]\n");
        sb_terminate(1);
    }

    const char *tar_path = argv[1];
    int fd = sb_acquire(tar_path, 0);
    if (fd < 0) {
        print("Failed to open archive\n");
        sb_terminate(1);
    }

    uint8_t block[512];
    while (1) {
        uint64_t r = sb_pull(fd, block, 512);
        if (r < 512) break;
        struct tar_header *hdr = (struct tar_header *)block;
        if (hdr->filename[0] == '\0')
            break;

        uint32_t size = parse_size(hdr->size);
        print(hdr->filename);
        print("\n");

        /* Skip the file contents */
        uint32_t skip_blocks = (size + 511) / 512;
        for (uint32_t i = 0; i < skip_blocks; i++) {
            if (sb_pull(fd, block, 512) != 512)
                break;
        }
    }

    sb_release(fd);
    sb_terminate(0);
    return 0;
}
