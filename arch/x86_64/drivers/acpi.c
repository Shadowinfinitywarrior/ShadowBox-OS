#include "acpi.h"
#include "kernel.h"
#include "vmm.h"

void acpi_init(void) {
    printk(KERN_INFO "ACPI: Initializing Advanced Configuration and Power Interface...\n");
    uint8_t *start = (uint8_t*)(0x000E0000 + 0xFFFFFFFF80000000);
    uint8_t *end = (uint8_t*)(0x000FFFFF + 0xFFFFFFFF80000000);
    acpi_rsdp_t *rsdp = NULL;
    
    for (uint8_t *ptr = start; ptr < end; ptr += 16) {
        if (ptr[0] == 'R' && ptr[1] == 'S' && ptr[2] == 'D' && ptr[3] == ' ' &&
            ptr[4] == 'P' && ptr[5] == 'T' && ptr[6] == 'R' && ptr[7] == ' ') {
            
            // Validate checksum of the 20-byte RSDP structure
            uint8_t sum = 0;
            for (int i = 0; i < 20; i++) {
                sum += ptr[i];
            }
            if (sum == 0) {
                rsdp = (acpi_rsdp_t*)ptr;
                break;
            }
        }
    }
    
    if (rsdp) {
        printk(KERN_INFO "ACPI: Found RSDP at %x, OEM: %c%c%c%c%c%c\n", (uint32_t)(uint64_t)rsdp, 
               rsdp->oem_id[0], rsdp->oem_id[1], rsdp->oem_id[2], 
               rsdp->oem_id[3], rsdp->oem_id[4], rsdp->oem_id[5]);
        vmm_map_phys_range(rsdp->rsdt_address, 0x1000);
        printk(KERN_INFO "ACPI: RSDT mapped at %x\n", rsdp->rsdt_address);
    } else {
        printk(KERN_WARN "ACPI: RSDP not found in legacy memory region.\n");
    }
}
