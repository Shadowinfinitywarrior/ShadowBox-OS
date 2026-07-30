#ifndef SHADOWBOX_VMM_H
#define SHADOWBOX_VMM_H

#include "types.h"

#define PAGE_PRESENT  0x01
#define PAGE_WRITE    0x02
#define PAGE_USER     0x04
#define PAGE_LARGE    0x80
#define PAGE_COW      0x100
#define PAGE_DEMAND   0x200

/*
 * vmm_init - Initialize virtual memory manager
 */
void vmm_init(void);

/*
 * vmm_map_page - Map a virtual page to a physical page
 * @phys_addr: Physical address
 * @virt_addr: Virtual address
 * @flags:     Page flags (PAGE_*)
 */
void vmm_map_page(uint64_t phys_addr, uint64_t virt_addr, uint32_t flags);

/*
 * vmm_unmap_page - Unmap a virtual page
 * @virt_addr: Virtual address to unmap
 */
void vmm_unmap_page(uint64_t virt_addr);

/*
 * vmm_map_phys_range - Map a range of physical memory
 * @phys_start: Start physical address
 * @size:       Size of region
 */
void vmm_map_phys_range(uint64_t phys_start, uint64_t size);

/*
 * vmm_create_address_space - Create a new page table hierarchy
 * Returns: Physical address of new top-level page table (cr3)
 */
uint64_t vmm_create_address_space(void);

/*
 * vmm_fork_address_space - Fork an address space for COW
 * @parent_cr3: Parent's page table address
 * Returns: New page table address
 */
uint64_t vmm_fork_address_space(uint64_t parent_cr3);

/*
 * vmm_handle_cow_fault - Handle copy-on-write page fault
 * @fault_addr: Address that triggered the fault
 * Returns: 0 on success, -1 on error
 */
int vmm_handle_cow_fault(uint64_t fault_addr);

/*
 * vmm_handle_demand_page - Handle demand paging fault
 * @fault_addr: Address that triggered the fault
 * Returns: 0 on success, -1 on error
 */
int vmm_handle_demand_page(uint64_t fault_addr);

/*
 * vmalloc - Allocate virtual memory
 * @size: Size in bytes
 * Returns: Virtual address, or NULL on failure
 */
void *vmalloc(uint64_t size);
void *vmap_phys(uint64_t phys_start, uint64_t size);

/*
 * vfree - Free virtual memory
 * @ptr: Virtual address to free
 */
void vfree(void *ptr);

/*
 * vmm_set_page_flags - Set/clear page table flags for a virtual address
 * @virt_addr:   Virtual address
 * @set_flags:   Flags to set (OR'd into current flags)
 * @clear_flags: Flags to clear (AND'd out of current flags)
 * Returns: 0 on success, -1 if page not present
 */
int vmm_set_page_flags(uint64_t virt_addr, uint32_t set_flags, uint32_t clear_flags);

/*
 * vmm_destroy_address_space - Free an address space
 * @cr3: Physical address of top-level page table
 */
void vmm_destroy_address_space(uint64_t cr3);

extern int pcid_supported;

#endif
