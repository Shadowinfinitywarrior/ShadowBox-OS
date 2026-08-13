#include "sys.h"
#include "elf.h"

static void print(const char *s) { sb_push(1, s, strlen(s)); }

static void print_uint64(uint64_t v) {
    char buf[21];
    int i = 20;
    buf[i] = '\0';
    if (v == 0) {
        buf[--i] = '0';
    } else {
        while (v > 0 && i > 0) {
            buf[--i] = '0' + (v % 10);
            v /= 10;
        }
    }
    print(&buf[i]);
}

static void print_hex64(uint64_t v) {
    char buf[19]; // 0x + 16 hex digits + \0
    const char *hex = "0123456789abcdef";
    int i = 18;
    buf[i] = '\0';
    if (v == 0) {
        buf[--i] = '0';
    } else {
        while (v > 0 && i > 2) {
            buf[--i] = hex[v & 0xF];
            v >>= 4;
        }
    }
    buf[--i] = 'x';
    buf[--i] = '0';
    print(&buf[i]);
}

static void newline(void) { print("\n"); }

void _start(void) {
    print("Enter ELF filename: ");
    char filename[128];
    int len = 0;
    while (len < 127) {
        char c;
        if (sb_pull(0, &c, 1) <= 0) break;
        if (c == '\n' || c == '\r') { newline(); break; }
        if (c == '\b' || c == 127) {
            if (len > 0) { print("\b \b"); len--; }
        } else {
            sb_push(1, &c, 1);
            filename[len++] = c;
        }
    }
    filename[len] = '\0';
    if (len == 0) { sb_terminate(1); }

    int fd = sb_acquire(filename, 0);
    if (fd < 0) { print("File not found.\n"); sb_terminate(1); }

    // Read ELF header
    elf64_ehdr_t ehdr;
    if (sb_pull(fd, &ehdr, sizeof(ehdr)) != sizeof(ehdr)) {
        print("Failed to read ELF header.\n");
        sb_release(fd);
        sb_terminate(1);
    }
    if (ehdr.e_ident_mag != ELF_MAGIC) {
        print("Not a valid ELF file.\n");
        sb_release(fd);
        sb_terminate(1);
    }
    print("ELF Header:\n");
    print("  Entry point: ");
    print_hex64(ehdr.e_entry);
    newline();
    print("  Program header count: ");
    print_uint64(ehdr.e_phnum);
    newline();
    // Seek to program header table
    sys_lseek(fd, ehdr.e_phoff, 0); // SEEK_SET
    for (uint16_t i = 0; i < ehdr.e_phnum; ++i) {
        elf64_phdr_t phdr;
        if (sb_pull(fd, &phdr, sizeof(phdr)) != sizeof(phdr)) break;
        print("  ["); print_uint64(i); print("] Type: ");
        if (phdr.p_type == PT_LOAD) print("PT_LOAD"); else { print_uint64(phdr.p_type); }
        print(", Flags: "); print_uint64(phdr.p_flags);
        print(", Offset: "); print_hex64(phdr.p_offset);
        print(", Vaddr: "); print_hex64(phdr.p_vaddr);
        print(", Filesz: "); print_uint64(phdr.p_filesz);
        print(", Memsz: "); print_uint64(phdr.p_memsz);
        newline();
    }
    sb_release(fd);
    sb_terminate(0);
}
