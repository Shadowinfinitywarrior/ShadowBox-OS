#include "vmm.h"
#include "pmm.h"
#include "kernel.h"
#include "spinlock.h"
#include "kstring.h"

extern uint64_t boot_pml4[];
extern uint64_t boot_pd[];
int pcid_supported = 0;
static spinlock_t vmm_lock;

static inline void invlpg(uint64_t addr) {
    __asm__ volatile("invlpg (%0)" ::"r" (addr) : "memory");
}

void vmm_map_page(uint64_t phys_addr, uint64_t virt_addr, uint32_t flags) {
    spin_lock_irqsave(&vmm_lock);
    uint64_t pml4_index = (virt_addr >> 39) & 0x1FF;
    uint64_t pdp_index = (virt_addr >> 30) & 0x1FF;
    uint64_t pd_index = (virt_addr >> 21) & 0x1FF;
    uint64_t pt_index = (virt_addr >> 12) & 0x1FF;

    uint64_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r" (cr3));
    uint64_t *pml4 = (uint64_t *)((cr3 & ~0xFFF) + 0xFFFFFFFF80000000);

    if (!(pml4[pml4_index] & PAGE_PRESENT)) {
        uint64_t new_pdp = (uint64_t)pmm_alloc_page();
        if (!new_pdp) panic("VMM: OOM mapping PDP");
        uint64_t *new_pdp_virt = (uint64_t *)(new_pdp + 0xFFFFFFFF80000000);
        memset(new_pdp_virt, 0, 512 * sizeof(uint64_t));
        pml4[pml4_index] = new_pdp | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    }

    uint64_t *pdp = (uint64_t *)((pml4[pml4_index] & ~0xFFF) + 0xFFFFFFFF80000000);

    if (!(pdp[pdp_index] & PAGE_PRESENT)) {
        uint64_t new_pd = (uint64_t)pmm_alloc_page();
        if (!new_pd) panic("VMM: OOM mapping PD");
        uint64_t *new_pd_virt = (uint64_t *)(new_pd + 0xFFFFFFFF80000000);
        memset(new_pd_virt, 0, 512 * sizeof(uint64_t));
        pdp[pdp_index] = new_pd | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    }

    uint64_t *pd = (uint64_t *)((pdp[pdp_index] & ~0xFFF) + 0xFFFFFFFF80000000);

    if (!(pd[pd_index] & PAGE_PRESENT)) {
        uint64_t new_pt = (uint64_t)pmm_alloc_page();
        if (!new_pt) panic("VMM: OOM mapping PT");
        uint64_t *new_pt_virt = (uint64_t *)(new_pt + 0xFFFFFFFF80000000);
        memset(new_pt_virt, 0, 512 * sizeof(uint64_t));
        pd[pd_index] = new_pt | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    } else if (pd[pd_index] & PAGE_LARGE) {
        uint64_t large_base = pd[pd_index] & ~0x1FFFFFULL;
        uint64_t large_flags = pd[pd_index] & 0xFFF;
        large_flags &= ~PAGE_LARGE;

        uint64_t new_pt = (uint64_t)pmm_alloc_page();
        if (!new_pt) panic("VMM: OOM splitting large page");
        uint64_t *new_pt_virt = (uint64_t *)(new_pt + 0xFFFFFFFF80000000);
        for (int i = 0; i < 512; i++) {
            new_pt_virt[i] = (large_base + i * 0x1000) | large_flags;
        }
        pd[pd_index] = new_pt | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
        __asm__ volatile("invlpg (%0)" :: "r" (virt_addr & ~0x1FFFFF) : "memory");
    }

    uint64_t *pt = (uint64_t *)((pd[pd_index] & ~0xFFF) + 0xFFFFFFFF80000000);
    pt[pt_index] = phys_addr | flags;
    invlpg(virt_addr);
    spin_unlock_irqrestore(&vmm_lock);
}

void vmm_unmap_page(uint64_t virt_addr) {
    spin_lock_irqsave(&vmm_lock);
    uint64_t pml4_index = (virt_addr >> 39) & 0x1FF;
    uint64_t pdp_index = (virt_addr >> 30) & 0x1FF;
    uint64_t pd_index = (virt_addr >> 21) & 0x1FF;
    uint64_t pt_index = (virt_addr >> 12) & 0x1FF;

    uint64_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r" (cr3));
    uint64_t *pml4 = (uint64_t *)((cr3 & ~0xFFF) + 0xFFFFFFFF80000000);

    uint64_t pml4e = pml4[pml4_index];
    if (!(pml4e & PAGE_PRESENT)) { spin_unlock_irqrestore(&vmm_lock); return; }
    if (pml4e & PAGE_LARGE) { spin_unlock_irqrestore(&vmm_lock); return; }
    uint64_t *pdp = (uint64_t *)((pml4e & ~0xFFF) + 0xFFFFFFFF80000000);

    uint64_t pdpe = pdp[pdp_index];
    if (!(pdpe & PAGE_PRESENT)) { spin_unlock_irqrestore(&vmm_lock); return; }
    if (pdpe & PAGE_LARGE) { spin_unlock_irqrestore(&vmm_lock); return; }
    uint64_t *pd = (uint64_t *)((pdpe & ~0xFFF) + 0xFFFFFFFF80000000);

    uint64_t pde = pd[pd_index];
    if (!(pde & PAGE_PRESENT)) { spin_unlock_irqrestore(&vmm_lock); return; }
    if (pde & PAGE_LARGE) { spin_unlock_irqrestore(&vmm_lock); return; }
    uint64_t *pt = (uint64_t *)((pde & ~0xFFF) + 0xFFFFFFFF80000000);

    uint64_t entry = pt[pt_index];
    if (entry & PAGE_PRESENT) {
        if (!(entry & PAGE_COW)) {
            // Free the physical page (assuming it's a regular allocation)
            extern void pmm_free_page(void*);
            pmm_free_page((void*)(entry & ~0xFFF));
        }
        pt[pt_index] = 0;
        invlpg(virt_addr);
    }
    spin_unlock_irqrestore(&vmm_lock);
}

void vmm_init(void) {
    spinlock_init(&vmm_lock);
    
    // Check if PCID is supported: CPUID EAX=1, ECX bit 17
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    if (ecx & (1 << 17)) {
        pcid_supported = 1;
        uint64_t cr4;
        __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
        cr4 |= (1 << 17);
        __asm__ volatile("mov %0, %%cr4" :: "r"(cr4));
        printk(KERN_INFO "ShadowBox VMM initialized (PCID enabled).\n");
    } else {
        pcid_supported = 0;
        printk(KERN_INFO "ShadowBox VMM initialized (PCID not supported, disabled).\n");
    }

    uint64_t *pml4 = (uint64_t *)((uint64_t)boot_pml4 + 0xFFFFFFFF80000000);
    pml4[0] = 0;

    // Allocate shared PDP for MMIO at PML4[256] (0xFFFF800000000000)
    if (!(pml4[256] & PAGE_PRESENT)) {
        uint64_t mmio_pdp = (uint64_t)pmm_alloc_page();
        if (!mmio_pdp) panic("VMM: OOM mapping MMIO PDP");
        memset((void*)(mmio_pdp + 0xFFFFFFFF80000000), 0, 4096);
        pml4[256] = mmio_pdp | PAGE_PRESENT | PAGE_WRITE;
    }

    uint64_t total_pages = pmm_total_pages();
    uint64_t total_phys = total_pages * PAGE_SIZE;
    uint64_t *pd = (uint64_t *)((uint64_t)boot_pd + 0xFFFFFFFF80000000);
    for (uint64_t i = 1; i < 512; i++) {
        uint64_t phys_start = i * 0x200000ULL;
        if (phys_start >= total_phys) break;
        pd[i] = phys_start | 0x83;
    }

    uint64_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r" (cr3));
    __asm__ volatile("mov %0, %%cr3" :: "r" (cr3));
}

uint64_t vmm_create_address_space(void) {
    spin_lock_irqsave(&vmm_lock);

    // Allocate new PML4
    uint64_t pml4_phys = (uint64_t)pmm_alloc_page();
    if (!pml4_phys) panic("VMM: OOM creating PML4");

    // Allocate new PDP for user space (covers PML4[0], addresses 0x0 - 0x7FFFFFFFFF)
    uint64_t user_pdp_phys = (uint64_t)pmm_alloc_page();
    if (!user_pdp_phys) panic("VMM: OOM creating user PDP");

    // Allocate new PD for user space (covers PDP[0], first 1GB)
    uint64_t user_pd_phys = (uint64_t)pmm_alloc_page();
    if (!user_pd_phys) panic("VMM: OOM creating user PD");

    uint64_t *new_pml4 = (uint64_t *)(pml4_phys + 0xFFFFFFFF80000000);
    uint64_t *user_pdp = (uint64_t *)(user_pdp_phys + 0xFFFFFFFF80000000);
    uint64_t *user_pd = (uint64_t *)(user_pd_phys + 0xFFFFFFFF80000000);
    uint64_t *old_pml4 = (uint64_t *)((uint64_t)boot_pml4 + 0xFFFFFFFF80000000);

    memset(new_pml4, 0, 256 * sizeof(uint64_t));
    memset(user_pdp, 0, 512 * sizeof(uint64_t));
    memset(user_pd, 0, 512 * sizeof(uint64_t));

    // Map first 1GB (512 x 2MB large pages) as identity + higher-half
    // PD[i] maps virt 0x0 + i*2MB to phys 0x0 + i*2MB (with USER flags)
    for (int i = 0; i < 512; i++) {
        user_pd[i] = ((uint64_t)i * 0x200000ULL) | 0x83 | PAGE_USER;
    }

    // Set up PDP[0] -> user_pd (covers addresses 0x0 - 0x3FFFFFFF = 1GB)
    user_pdp[0] = user_pd_phys | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;

    // PML4[0] -> user_pdp (covers addresses 0x0 - 0x7FFFFFFF_FFFFFFFF = 128TB lower half)
    new_pml4[0] = user_pdp_phys | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;

    // Copy kernel higher-half mappings (entries 256-511)
    for (int i = 256; i < 512; i++) new_pml4[i] = old_pml4[i];

    spin_unlock_irqrestore(&vmm_lock);
    return pml4_phys;
}

uint64_t vmm_fork_address_space(uint64_t parent_cr3) {
    uint64_t child_cr3 = vmm_create_address_space();

    uint64_t *parent_pml4 = (uint64_t *)((parent_cr3 & ~0xFFF) + 0xFFFFFFFF80000000);
    uint64_t *child_pml4 = (uint64_t *)((child_cr3 & ~0xFFF) + 0xFFFFFFFF80000000);

    for (int pml4_i = 0; pml4_i < 256; pml4_i++) {
        if (!(parent_pml4[pml4_i] & PAGE_PRESENT)) continue;

        uint64_t parent_pdp_phys = parent_pml4[pml4_i] & ~0xFFF;
        uint64_t *parent_pdp = (uint64_t *)(parent_pdp_phys + 0xFFFFFFFF80000000);

        uint64_t child_pdp_phys = (uint64_t)pmm_alloc_page();
        if (!child_pdp_phys) panic("VMM: OOM forking address space PDP");
        uint64_t *child_pdp = (uint64_t *)(child_pdp_phys + 0xFFFFFFFF80000000);
        memset(child_pdp, 0, 512 * sizeof(uint64_t));

        child_pml4[pml4_i] = child_pdp_phys | (parent_pml4[pml4_i] & 0xFFF);

        for (int pdp_i = 0; pdp_i < 512; pdp_i++) {
            if (!(parent_pdp[pdp_i] & PAGE_PRESENT)) continue;
            if (parent_pdp[pdp_i] & PAGE_LARGE) continue;

            uint64_t parent_pd_phys = parent_pdp[pdp_i] & ~0xFFF;
            uint64_t *parent_pd = (uint64_t *)(parent_pd_phys + 0xFFFFFFFF80000000);

            uint64_t child_pd_phys = (uint64_t)pmm_alloc_page();
            if (!child_pd_phys) panic("VMM: OOM forking address space PD");
            uint64_t *child_pd = (uint64_t *)(child_pd_phys + 0xFFFFFFFF80000000);
            memset(child_pd, 0, 512 * sizeof(uint64_t));

            child_pdp[pdp_i] = child_pd_phys | (parent_pdp[pdp_i] & 0xFFF);

            for (int pd_i = 0; pd_i < 512; pd_i++) {
                if (!(parent_pd[pd_i] & PAGE_PRESENT)) continue;
                if (parent_pd[pd_i] & PAGE_LARGE) continue;

                uint64_t parent_pt_phys = parent_pd[pd_i] & ~0xFFF;
                uint64_t *parent_pt = (uint64_t *)(parent_pt_phys + 0xFFFFFFFF80000000);

                uint64_t child_pt_phys = (uint64_t)pmm_alloc_page();
                uint64_t *child_pt = (uint64_t *)(child_pt_phys + 0xFFFFFFFF80000000);

                for (int pt_i = 0; pt_i < 512; pt_i++) {
                    uint64_t entry = parent_pt[pt_i];
                    if (!(entry & PAGE_PRESENT)) {
                        child_pt[pt_i] = 0;
                        continue;
                    }

                    if (entry & PAGE_WRITE) {
                        entry &= ~PAGE_WRITE;
                        entry |= PAGE_COW;
                        parent_pt[pt_i] = entry;
                        child_pt[pt_i] = entry;
                    } else {
                        child_pt[pt_i] = entry;
                    }
                }

                child_pd[pd_i] = child_pt_phys | (parent_pd[pd_i] & 0xFFF);
            }
        }
    }

    uint64_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    __asm__ volatile("mov %0, %%cr3" :: "r"(cr3) : "memory");

    return child_cr3;
}

int vmm_handle_cow_fault(uint64_t fault_addr) {
    uint64_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    uint64_t *pml4 = (uint64_t *)((cr3 & ~0xFFF) + 0xFFFFFFFF80000000);

    uint64_t pml4_idx = (fault_addr >> 39) & 0x1FF;
    uint64_t pdp_idx = (fault_addr >> 30) & 0x1FF;
    uint64_t pd_idx = (fault_addr >> 21) & 0x1FF;
    uint64_t pt_idx = (fault_addr >> 12) & 0x1FF;

    uint64_t pml4e = pml4[pml4_idx];
    if (!(pml4e & PAGE_PRESENT)) return 0;
    if (pml4e & PAGE_LARGE) return 0;

    uint64_t *pdp = (uint64_t *)((pml4e & ~0xFFF) + 0xFFFFFFFF80000000);
    uint64_t pdpe = pdp[pdp_idx];
    if (!(pdpe & PAGE_PRESENT)) return 0;
    if (pdpe & PAGE_LARGE) return 0;

    uint64_t *pd = (uint64_t *)((pdpe & ~0xFFF) + 0xFFFFFFFF80000000);
    uint64_t pde = pd[pd_idx];
    if (!(pde & PAGE_PRESENT)) return 0;
    if (pde & PAGE_LARGE) return 0;

    uint64_t *pt = (uint64_t *)((pde & ~0xFFF) + 0xFFFFFFFF80000000);
    uint64_t entry = pt[pt_idx];

    if (!(entry & PAGE_PRESENT)) return 0;
    if (!(entry & PAGE_COW)) return 0;

    uint64_t old_phys = entry & ~0xFFF;
    uint64_t new_phys = (uint64_t)pmm_alloc_page();
    if (!new_phys) {
        panic("VMM: OOM in COW fault");
        return 0;
    }

    uint8_t *old_virt = (uint8_t *)(old_phys + 0xFFFFFFFF80000000);
    uint8_t *new_virt = (uint8_t *)(new_phys + 0xFFFFFFFF80000000);
    printk(KERN_DEBUG "cow: fault=%p cr3=%p pml4e=%p pdpe=%p pde=%p entry=%p old=%p new=%p\n",
           (void*)fault_addr, (void*)(cr3 & ~0xFFF),
           (void*)pml4e, (void*)pdpe, (void*)pde, (void*)entry,
           (void*)old_phys, (void*)new_phys);
    memcpy(new_virt, old_virt, 4096);

    pt[pt_idx] = new_phys | (entry & 0xEFF) | PAGE_WRITE;
    invlpg(fault_addr);
    return 1;
}

void vmm_map_phys_range(uint64_t phys_start, uint64_t size) {
    spin_lock_irqsave(&vmm_lock);
    uint64_t page_start = phys_start & ~(PAGE_SIZE - 1);
    uint64_t page_end = (phys_start + size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    for (uint64_t phys = page_start; phys < page_end; phys += PAGE_SIZE) {
        uint64_t virt = phys + 0xFFFF800000000000ULL;
        uint64_t pml4_index = (virt >> 39) & 0x1FF;
        uint64_t pdp_index = (virt >> 30) & 0x1FF;
        uint64_t pd_index = (virt >> 21) & 0x1FF;
        uint64_t pt_index = (virt >> 12) & 0x1FF;

        uint64_t cr3;
        __asm__ volatile("mov %%cr3, %0" : "=r" (cr3));
        uint64_t *pml4 = (uint64_t *)((cr3 & ~0xFFF) + 0xFFFFFFFF80000000);

        if (!(pml4[pml4_index] & PAGE_PRESENT)) {
            uint64_t new_pdp = (uint64_t)pmm_alloc_page();
            if (!new_pdp) panic("VMM: OOM mapping phys range PDP");
            uint64_t *new_pdp_virt = (uint64_t *)(new_pdp + 0xFFFFFFFF80000000);
            memset(new_pdp_virt, 0, 512 * sizeof(uint64_t));
            pml4[pml4_index] = new_pdp | PAGE_PRESENT | PAGE_WRITE;
        }

        uint64_t *pdp = (uint64_t *)((pml4[pml4_index] & ~0xFFF) + 0xFFFFFFFF80000000);
        if (!(pdp[pdp_index] & PAGE_PRESENT)) {
            uint64_t new_pd = (uint64_t)pmm_alloc_page();
            if (!new_pd) panic("VMM: OOM mapping phys range PD");
            uint64_t *new_pd_virt = (uint64_t *)(new_pd + 0xFFFFFFFF80000000);
            memset(new_pd_virt, 0, 512 * sizeof(uint64_t));
            pdp[pdp_index] = new_pd | PAGE_PRESENT | PAGE_WRITE;
        }

        uint64_t *pd = (uint64_t *)((pdp[pdp_index] & ~0xFFF) + 0xFFFFFFFF80000000);
        if (!(pd[pd_index] & PAGE_PRESENT)) {
            uint64_t new_pt = (uint64_t)pmm_alloc_page();
            if (!new_pt) panic("VMM: OOM mapping phys range PT");
            uint64_t *new_pt_virt = (uint64_t *)(new_pt + 0xFFFFFFFF80000000);
            memset(new_pt_virt, 0, 512 * sizeof(uint64_t));
            pd[pd_index] = new_pt | PAGE_PRESENT | PAGE_WRITE;
        } else if (pd[pd_index] & PAGE_LARGE) {
            uint64_t large_base = pd[pd_index] & ~0x1FFFFFULL;
            uint64_t large_flags = pd[pd_index] & 0xFFF;
            large_flags &= ~PAGE_LARGE;

            uint64_t new_pt = (uint64_t)pmm_alloc_page();
            if (!new_pt) panic("VMM: OOM splitting phys range large page");
            uint64_t *new_pt_virt = (uint64_t *)(new_pt + 0xFFFFFFFF80000000);
            for (int i = 0; i < 512; i++) {
                new_pt_virt[i] = (large_base + i * 0x1000) | large_flags;
            }
            pd[pd_index] = new_pt | PAGE_PRESENT | PAGE_WRITE;
            __asm__ volatile("invlpg (%0)" :: "r" (virt & ~0x1FFFFF) : "memory");
        }

        uint64_t *pt = (uint64_t *)((pd[pd_index] & ~0xFFF) + 0xFFFFFFFF80000000);
        pt[pt_index] = phys | PAGE_PRESENT | PAGE_WRITE;
        invlpg(virt);
    }
    spin_unlock_irqrestore(&vmm_lock);
}

int vmm_handle_demand_page(uint64_t fault_addr) {
    uint64_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    uint64_t *pml4 = (uint64_t *)((cr3 & ~0xFFF) + 0xFFFFFFFF80000000);

    uint64_t pml4_idx = (fault_addr >> 39) & 0x1FF;
    uint64_t pdp_idx = (fault_addr >> 30) & 0x1FF;
    uint64_t pd_idx = (fault_addr >> 21) & 0x1FF;
    uint64_t pt_idx = (fault_addr >> 12) & 0x1FF;

    uint64_t pml4e = pml4[pml4_idx];
    if (!(pml4e & PAGE_PRESENT)) return 0;
    if (pml4e & PAGE_LARGE) return 0;

    uint64_t *pdp = (uint64_t *)((pml4e & ~0xFFF) + 0xFFFFFFFF80000000);
    uint64_t pdpe = pdp[pdp_idx];
    if (!(pdpe & PAGE_PRESENT)) return 0;
    if (pdpe & PAGE_LARGE) return 0;

    uint64_t *pd = (uint64_t *)((pdpe & ~0xFFF) + 0xFFFFFFFF80000000);
    uint64_t pde = pd[pd_idx];
    if (!(pde & PAGE_PRESENT)) return 0;
    if (pde & PAGE_LARGE) return 0;

    uint64_t *pt = (uint64_t *)((pde & ~0xFFF) + 0xFFFFFFFF80000000);
    uint64_t entry = pt[pt_idx];

    // If it's already present, it's not a demand page fault
    if (entry & PAGE_PRESENT) return 0;

    // Check if it's marked for demand paging
    if (!(entry & PAGE_DEMAND)) return 0;

    uint64_t new_phys = (uint64_t)pmm_alloc_page();
    if (!new_phys) {
        panic("VMM: OOM in Demand Paging");
        return 0;
    }

    uint8_t *new_virt = (uint8_t *)(new_phys + 0xFFFFFFFF80000000);
    memset(new_virt, 0, 4096);

    // Map it properly (remove PAGE_DEMAND, add PAGE_PRESENT)
    pt[pt_idx] = new_phys | (entry & ~PAGE_DEMAND) | PAGE_PRESENT;
    invlpg(fault_addr);
    
    return 1;
}

static uint64_t vmalloc_addr = 0xFFFFFFFF90000000;

void *vmalloc(uint64_t size) {
    if (size == 0) return 0;
    uint64_t total_size = size + PAGE_SIZE;
    uint64_t pages = (total_size + PAGE_SIZE - 1) / PAGE_SIZE;
    uint64_t start_addr = vmalloc_addr;
    for (uint64_t i = 0; i < pages; i++) {
        uint64_t phys = (uint64_t)pmm_alloc_page();
        if (!phys) panic("vmalloc: Out of memory");
        vmm_map_page(phys, vmalloc_addr, PAGE_PRESENT | PAGE_WRITE);
        vmalloc_addr += PAGE_SIZE;
    }
    *(uint64_t*)start_addr = pages;
    return (void *)(start_addr + PAGE_SIZE);
}

void *vmap_phys(uint64_t phys_start, uint64_t size) {
    if (size == 0) return 0;
    uint64_t page_start = phys_start & ~(PAGE_SIZE - 1);
    uint64_t page_end = (phys_start + size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    uint64_t pages = (page_end - page_start) / PAGE_SIZE;
    uint64_t start_addr = vmalloc_addr;
    
    for (uint64_t i = 0; i < pages; i++) {
        vmm_map_page(page_start + i * PAGE_SIZE, vmalloc_addr, PAGE_PRESENT | PAGE_WRITE | 0x10);
        vmalloc_addr += PAGE_SIZE;
    }
    
    return (void *)(start_addr + (phys_start & (PAGE_SIZE - 1)));
}

void vfree(void *ptr) {
    if (!ptr) return;
    uint64_t start_addr = (uint64_t)ptr - PAGE_SIZE;
    uint64_t pages = *(uint64_t*)start_addr;
    for (uint64_t i = 0; i < pages; i++) {
        uint64_t virt = start_addr + i * PAGE_SIZE;
        uint64_t pml4_index = (virt >> 39) & 0x1FF;
        uint64_t pdp_index = (virt >> 30) & 0x1FF;
        uint64_t pd_index = (virt >> 21) & 0x1FF;
        uint64_t pt_index = (virt >> 12) & 0x1FF;

        uint64_t cr3;
        __asm__ volatile("mov %%cr3, %0" : "=r" (cr3));
        uint64_t *pml4 = (uint64_t *)((cr3 & ~0xFFF) + 0xFFFFFFFF80000000);
        uint64_t pml4e = pml4[pml4_index];
        if (!(pml4e & PAGE_PRESENT)) continue;
        if (pml4e & PAGE_LARGE) continue;
        uint64_t *pdp = (uint64_t *)((pml4e & ~0xFFF) + 0xFFFFFFFF80000000);
        uint64_t pdpe = pdp[pdp_index];
        if (!(pdpe & PAGE_PRESENT)) continue;
        if (pdpe & PAGE_LARGE) continue;
        uint64_t *pd = (uint64_t *)((pdpe & ~0xFFF) + 0xFFFFFFFF80000000);
        uint64_t pde = pd[pd_index];
        if (!(pde & PAGE_PRESENT)) continue;
        if (pde & PAGE_LARGE) continue;
        uint64_t *pt = (uint64_t *)((pde & ~0xFFF) + 0xFFFFFFFF80000000);
        
        uint64_t phys = pt[pt_index] & ~0xFFF;
        if (phys) pmm_free_page((void*)phys);
        pt[pt_index] = 0;
        invlpg(virt);
    }
}

int vmm_set_page_flags(uint64_t virt_addr, uint32_t set_flags, uint32_t clear_flags) {
    spin_lock_irqsave(&vmm_lock);
    uint64_t pml4_index = (virt_addr >> 39) & 0x1FF;
    uint64_t pdp_index = (virt_addr >> 30) & 0x1FF;
    uint64_t pd_index = (virt_addr >> 21) & 0x1FF;
    uint64_t pt_index = (virt_addr >> 12) & 0x1FF;

    uint64_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r" (cr3));
    uint64_t *pml4 = (uint64_t *)((cr3 & ~0xFFF) + 0xFFFFFFFF80000000);

    uint64_t pml4e = pml4[pml4_index];
    if (!(pml4e & PAGE_PRESENT)) { spin_unlock_irqrestore(&vmm_lock); return -1; }
    if (pml4e & PAGE_LARGE) { spin_unlock_irqrestore(&vmm_lock); return -1; }
    uint64_t *pdp = (uint64_t *)((pml4e & ~0xFFF) + 0xFFFFFFFF80000000);

    uint64_t pdpe = pdp[pdp_index];
    if (!(pdpe & PAGE_PRESENT)) { spin_unlock_irqrestore(&vmm_lock); return -1; }
    if (pdpe & PAGE_LARGE) { spin_unlock_irqrestore(&vmm_lock); return -1; }
    uint64_t *pd = (uint64_t *)((pdpe & ~0xFFF) + 0xFFFFFFFF80000000);

    uint64_t pde = pd[pd_index];
    if (!(pde & PAGE_PRESENT)) { spin_unlock_irqrestore(&vmm_lock); return -1; }
    if (pde & PAGE_LARGE) { spin_unlock_irqrestore(&vmm_lock); return -1; }
    uint64_t *pt = (uint64_t *)((pde & ~0xFFF) + 0xFFFFFFFF80000000);

    uint64_t entry = pt[pt_index];
    if (!(entry & PAGE_PRESENT)) { spin_unlock_irqrestore(&vmm_lock); return -1; }

    entry = (entry & ~clear_flags) | set_flags;
    pt[pt_index] = entry;
    invlpg(virt_addr);
    spin_unlock_irqrestore(&vmm_lock);
    return 0;
}

void vmm_destroy_address_space(uint64_t cr3) {
    if (!cr3) return;
    uint64_t *pml4 = (uint64_t *)((cr3 & ~0xFFF) + 0xFFFFFFFF80000000);

    for (int pml4_i = 0; pml4_i < 256; pml4_i++) {
        if (!(pml4[pml4_i] & PAGE_PRESENT)) continue;
        if (pml4[pml4_i] & PAGE_LARGE) continue;

        uint64_t pdp_phys = pml4[pml4_i] & ~0xFFF;
        uint64_t *pdp = (uint64_t *)(pdp_phys + 0xFFFFFFFF80000000);

        for (int pdp_i = 0; pdp_i < 512; pdp_i++) {
            if (!(pdp[pdp_i] & PAGE_PRESENT)) continue;
            if (pdp[pdp_i] & PAGE_LARGE) continue;

            uint64_t pd_phys = pdp[pdp_i] & ~0xFFF;
            uint64_t *pd = (uint64_t *)(pd_phys + 0xFFFFFFFF80000000);

            for (int pd_i = 0; pd_i < 512; pd_i++) {
                if (!(pd[pd_i] & PAGE_PRESENT)) continue;
                if (pd[pd_i] & PAGE_LARGE) continue;

                uint64_t pt_phys = pd[pd_i] & ~0xFFF;
                uint64_t *pt = (uint64_t *)(pt_phys + 0xFFFFFFFF80000000);

                for (int pt_i = 0; pt_i < 512; pt_i++) {
                    uint64_t entry = pt[pt_i];
                    if (entry & PAGE_PRESENT) {
                        if (!(entry & PAGE_COW)) {
                            pmm_free_page((void*)(entry & ~0xFFF));
                        }
                    }
                }
                pmm_free_page((void*)pt_phys);
            }
            pmm_free_page((void*)pd_phys);
        }
            pmm_free_page((void*)pdp_phys);
    }
    pmm_free_page((void*)(cr3 & ~0xFFF));
}

