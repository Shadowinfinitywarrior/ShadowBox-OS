#include "usb.h"
#include "kernel.h"
#include "pci.h"
#include "pmm.h"
#include "vmm.h"
#include "malloc.h"

void usb_init(void) {
    printk(KERN_INFO "USB: Initializing Universal Serial Bus stack...\n");
    
    // Scan PCI for USB Controllers
    // Class 0x0C (Serial Bus), Subclass 0x03 (USB)
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t dev = 0; dev < 32; dev++) {
            uint32_t vendor = pci_config_read(bus, dev, 0, 0) & 0xFFFF;
            if (vendor == 0xFFFF) continue;
            
            uint32_t class_info = pci_config_read(bus, dev, 0, 0x08);
            uint8_t class_code = (class_info >> 24) & 0xFF;
            uint8_t subclass = (class_info >> 16) & 0xFF;
            uint8_t prog_if = (class_info >> 8) & 0xFF;
            
            if (class_code == 0x0C && subclass == 0x03) {
                if (prog_if == 0x30) {
                    printk(KERN_INFO "USB: Found xHCI (USB 3.0) Controller at PCI %d:%d\n", bus, dev);
                    
                    // Enable Bus Mastering and Memory Space
                    uint32_t cmd = pci_config_read(bus, dev, 0, 0x04);
                    cmd |= (1 << 2) | (1 << 1); // Bus Master | Memory Space
                    pci_config_write(bus, dev, 0, 0x04, cmd);
                    
                    // Read BAR0 (Assume 64-bit for modern xHCI)
                    uint32_t bar0_low = pci_config_read(bus, dev, 0, 0x10);
                    uint32_t bar0_high = pci_config_read(bus, dev, 0, 0x14);
                    uint64_t bar0 = ((uint64_t)bar0_high << 32) | (bar0_low & 0xFFFFFFF0);
                    
                    printk(KERN_INFO "USB: xHCI Base Address (MMIO): 0x%lx\n", bar0);
                    
                    // Map xHCI MMIO (usually 8KB)
                    uint64_t virt_bar0 = (uint64_t)vmap_phys(bar0, 8192);
                    
                    uint8_t caplength = *(volatile uint8_t *)(virt_bar0);
                    uint64_t opbase = virt_bar0 + caplength;
                    
                    // Allocate DCBAA (Device Context Base Address Array) - needs 2048 bytes, aligned
                    uint64_t dcbaa_phys = (uint64_t)pmm_alloc_page();
                    uint64_t virt_dcbaa = dcbaa_phys + 0xFFFFFFFF80000000;
                    
                    // Allocate Command Ring
                    uint64_t cmd_ring_phys = (uint64_t)pmm_alloc_page();
                    uint64_t virt_cmd_ring = cmd_ring_phys + 0xFFFFFFFF80000000;
                    
                    // Zero them
                    for (int i=0; i<4096; i++) {
                        ((uint8_t*)virt_dcbaa)[i] = 0;
                        ((uint8_t*)virt_cmd_ring)[i] = 0;
                    }
                    
                    // Write DCBAAP (offset 0x30 from opbase)
                    *(volatile uint64_t *)(opbase + 0x30) = dcbaa_phys;
                    
                    // Write CRCR (offset 0x18 from opbase)
                    *(volatile uint64_t *)(opbase + 0x18) = cmd_ring_phys | 1; // 1 = Ring Cycle State
                    
                    printk(KERN_INFO "USB: xHCI DCBAA (0x%lx) and Command Ring (0x%lx) initialized\n", dcbaa_phys, cmd_ring_phys);
                    
                } else if (prog_if == 0x20) {
                    printk(KERN_INFO "USB: Found EHCI (USB 2.0) Controller at PCI %d:%d\n", bus, dev);
                } else if (prog_if == 0x10) {
                    printk(KERN_INFO "USB: Found OHCI (USB 1.0) Controller at PCI %d:%d\n", bus, dev);
                } else if (prog_if == 0x00) {
                    printk(KERN_INFO "USB: Found UHCI (USB 1.0) Controller at PCI %d:%d\n", bus, dev);
                }
            }
        }
    }
}
