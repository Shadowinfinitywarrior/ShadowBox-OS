#ifndef SHADOWBOX_ELF_H
#define SHADOWBOX_ELF_H

#include "types.h"

#define ELF_MAGIC 0x464C457F

/*
 * elf64_ehdr_t - ELF64 executable header
 */
typedef struct {
    uint32_t e_ident_mag;
    uint8_t  e_ident_class;
    uint8_t  e_ident_data;
    uint8_t  e_ident_version;
    uint8_t  e_ident_osabi;
    uint8_t  e_ident_abiversion;
    uint8_t  e_ident_pad[7];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} __attribute__((packed)) elf64_ehdr_t;

/*
 * elf64_phdr_t - ELF64 program header
 */
typedef struct {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} __attribute__((packed)) elf64_phdr_t;

#define PT_LOAD 1

struct process;

/*
 * elf_load_segments - Load ELF segments into a process
 * @proc:      Target process
 * @file_data: ELF file data in memory
 * @entry_out: Output for entry point address
 * Returns: 0 on success, -1 on failure
 */
int elf_load_segments(struct process *proc, const uint8_t *file_data, uint64_t *entry_out);

#endif
