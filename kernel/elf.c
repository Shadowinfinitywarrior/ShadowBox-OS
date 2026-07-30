#include "elf.h"
#include "vmm.h"
#include "pmm.h"
#include "kernel.h"
#include "kstring.h"
#include "task.h"
#include "errno.h"

int elf_load_segments(struct process *proc, const uint8_t *file_data, uint64_t *entry_out) {
    elf64_ehdr_t *ehdr = (elf64_ehdr_t *)file_data;

    if (ehdr->e_ident_mag != ELF_MAGIC) return -ENOEXEC;
    if (ehdr->e_ident_class != 2) return -ENOEXEC;

    uint64_t old_cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(old_cr3));
    if (proc->cr3 != old_cr3) {
        __asm__ volatile("mov %0, %%cr3" :: "r"(proc->cr3) : "memory");
    }

    uint64_t phoff = ehdr->e_phoff;
    uint16_t phnum = ehdr->e_phnum;
    uint16_t phentsize = ehdr->e_phentsize;

    for (uint16_t i = 0; i < phnum; i++) {
        elf64_phdr_t *phdr = (elf64_phdr_t *)(file_data + phoff + i * phentsize);

        if (phdr->p_type != PT_LOAD) continue;

        uint64_t virt_start = phdr->p_vaddr;
        uint64_t mem_sz = phdr->p_memsz;
        uint64_t file_sz = phdr->p_filesz;

        uint64_t page_start = virt_start & ~(PAGE_SIZE - 1);
        uint64_t page_end = (virt_start + mem_sz + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

        for (uint64_t addr = page_start; addr < page_end; addr += PAGE_SIZE) {
            uint32_t page_flags = PAGE_PRESENT | PAGE_USER;
            if (phdr->p_flags & 2) page_flags |= PAGE_WRITE;

            uint64_t phys = (uint64_t)pmm_alloc_page();
            if (!phys) {
                if (proc->cr3 != old_cr3) {
                    __asm__ volatile("mov %0, %%cr3" :: "r"(old_cr3) : "memory");
                }
                return -ENOMEM;
            }
            vmm_map_page(phys, addr, page_flags);
            memset((void*)addr, 0, PAGE_SIZE);
        }

        memcpy((void*)virt_start, file_data + phdr->p_offset, file_sz);
    }

    if (proc->cr3 != old_cr3) {
        __asm__ volatile("mov %0, %%cr3" :: "r"(old_cr3) : "memory");
    }

    *entry_out = ehdr->e_entry;
    return 0;
}
