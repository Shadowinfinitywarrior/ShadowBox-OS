#include "hda.h"
#include "kernel.h"
#include "pci.h"
#include "vmm.h"
#include "pmm.h"
#include "kstring.h"

void hda_init(void) {
    printk(KERN_INFO "HDA: Initializing Intel High Definition Audio...\n");
    
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t dev = 0; dev < 32; dev++) {
            uint32_t vendor = pci_config_read(bus, dev, 0, 0) & 0xFFFF;
            if (vendor == 0xFFFF) continue;
            
            uint32_t class_info = pci_config_read(bus, dev, 0, 0x08);
            uint8_t class_code = (class_info >> 24) & 0xFF;
            uint8_t subclass = (class_info >> 16) & 0xFF;
            
            // Class 0x04 (Multimedia), Subclass 0x03 (Audio Device)
            if (class_code == 0x04 && subclass == 0x03) {
                printk(KERN_INFO "HDA: Found Intel HDA Controller at PCI %d:%d\n", bus, dev);
                
                // Enable Bus Mastering & Memory Space
                uint32_t cmd = pci_config_read(bus, dev, 0, 0x04);
                cmd |= (1 << 2) | (1 << 1);
                pci_config_write(bus, dev, 0, 0x04, cmd);
                
                // Read BAR0 (MMIO Base)
                uint32_t bar0_low = pci_config_read(bus, dev, 0, 0x10);
                uint64_t bar0 = bar0_low & 0xFFFFFFF0;
                if ((bar0_low & 0x06) == 0x04) { // 64-bit BAR
                    uint32_t bar0_high = pci_config_read(bus, dev, 0, 0x14);
                    bar0 |= ((uint64_t)bar0_high << 32);
                }
                
                printk(KERN_INFO "HDA: Base Address (MMIO): 0x%lx\n", bar0);
                
                // Map HDA MMIO (usually 16KB)
                uint64_t virt_bar0 = (uint64_t)vmap_phys(bar0, 16384);
                
                // Reset controller
                volatile uint32_t *gctl = (volatile uint32_t *)(virt_bar0 + 0x08);
                printk(KERN_INFO "HDA: Initial GCTL=0x%x\n", *gctl);
                *gctl &= ~1; // Clear CRST
                
                int timeout1 = 100000;
                while ((*gctl & 1) != 0 && timeout1-- > 0); // Wait for enter reset
                if (timeout1 <= 0) printk(KERN_WARN "HDA: Timeout waiting for CRST to clear! GCTL=0x%x\n", *gctl);
                
                *gctl |= 1; // Set CRST
                int timeout2 = 100000;
                while ((*gctl & 1) == 0 && timeout2-- > 0); // Wait for exit reset
                if (timeout2 <= 0) printk(KERN_WARN "HDA: Timeout waiting for CRST to set! GCTL=0x%x\n", *gctl);
                
                printk(KERN_INFO "HDA: Reset complete. GCTL=0x%x\n", *gctl);
                
                // Wait for Codec
                volatile uint16_t *statests = (volatile uint16_t *)(virt_bar0 + 0x0E);
                int codec_timeout = 1000000;
                while (*statests == 0 && codec_timeout > 0) {
                    codec_timeout--;
                }
                
                if (*statests == 0) {
                    printk(KERN_WARN "HDA: No codecs detected (STATESTS=0)!\n");
                    return;
                }
                
                uint16_t codecs = *statests;
                printk(KERN_INFO "HDA: Codecs detected (STATESTS=0x%x)\n", codecs);
                *statests = codecs; // Clear status
                
                int codec_id = -1;
                for (int i = 0; i < 15; i++) {
                    if (codecs & (1 << i)) {
                        codec_id = i;
                        break;
                    }
                }
                
                // Allocate CORB and RIRB (1 page each is enough)
                uint64_t corb_phys = (uint64_t)pmm_alloc_page();
                uint64_t rirb_phys = (uint64_t)pmm_alloc_page();
                
                // Zero them out
                memset((void*)(corb_phys + 0xFFFFFFFF80000000), 0, 4096);
                memset((void*)(rirb_phys + 0xFFFFFFFF80000000), 0, 4096);
                
                // Setup CORB
                volatile uint32_t *corblbase = (volatile uint32_t *)(virt_bar0 + 0x40);
                volatile uint32_t *corbubase = (volatile uint32_t *)(virt_bar0 + 0x44);
                volatile uint16_t *corbwp = (volatile uint16_t *)(virt_bar0 + 0x48);
                volatile uint16_t *corbrp = (volatile uint16_t *)(virt_bar0 + 0x4A);
                volatile uint8_t *corbctl = (volatile uint8_t *)(virt_bar0 + 0x4C);
                volatile uint8_t *corbsize = (volatile uint8_t *)(virt_bar0 + 0x4E);
                
                *corbctl = 0; // Stop CORB
                *corbsize = 2; // 256 entries
                *corblbase = (uint32_t)corb_phys;
                *corbubase = (uint32_t)(corb_phys >> 32);
                
                *corbwp = 0; // Reset write pointer
                *corbrp = (1 << 15); // Reset read pointer
                int timeout3 = 100000;
                while (!(*corbrp & (1 << 15)) && timeout3-- > 0); // Wait for reset to complete (bit=1)
                if (timeout3 <= 0) printk(KERN_WARN "HDA: Timeout waiting for CORB RP reset!\n");
                
                *corbrp = 0; // Clear reset bit
                int timeout4 = 100000;
                while ((*corbrp & (1 << 15)) && timeout4-- > 0); // Wait for bit to clear
                if (timeout4 <= 0) printk(KERN_WARN "HDA: Timeout waiting for CORB RP to clear!\n");
                
                *corbctl = 2; // Start CORB
                
                // Setup RIRB
                volatile uint32_t *rirblbase = (volatile uint32_t *)(virt_bar0 + 0x50);
                volatile uint32_t *rirbubase = (volatile uint32_t *)(virt_bar0 + 0x54);
                volatile uint16_t *rirbwp = (volatile uint16_t *)(virt_bar0 + 0x58);
                volatile uint16_t *rirbint = (volatile uint16_t *)(virt_bar0 + 0x5A);
                volatile uint8_t *rirbctl = (volatile uint8_t *)(virt_bar0 + 0x5C);
                volatile uint8_t *rirbsize = (volatile uint8_t *)(virt_bar0 + 0x5E);
                
                *rirbctl = 0; // Stop RIRB
                *rirbsize = 2; // 256 entries
                *rirblbase = (uint32_t)rirb_phys;
                *rirbubase = (uint32_t)(rirb_phys >> 32);
                
                *rirbwp = (1 << 15); // Reset write pointer
                
                *rirbint = 1; // Interrupt on 1 entry
                *rirbctl = 2; // Start RIRB
                
                printk(KERN_INFO "HDA: CORB and RIRB running. Sending test command to Codec %d...\n", codec_id);
                
                // Send GET_PARAMETER (Vendor ID) to Root Node (NID=0)
                // Verb: 0xF00, Parameter: 0x00 (Vendor ID)
                uint32_t *corb = (uint32_t *)(corb_phys + 0xFFFFFFFF80000000);
                uint32_t command = (codec_id << 28) | (0xF00 << 8) | 0x00; // Node 0
                
                // Read RIRB WP before sending command to detect synchronous responses
                uint16_t old_rirb_wp = *rirbwp;
                asm volatile("":::"memory");
                
                uint16_t wp = *corbwp;
                wp = (wp + 1) % 256;
                corb[wp] = command;
                asm volatile("":::"memory");
                *corbwp = wp; // Trigger controller to read command
                asm volatile("":::"memory");
                
                int timeout = 1000000;
                while (timeout > 0) {
                    uint16_t cur_wp = *rirbwp;
                    if (cur_wp != old_rirb_wp) break;
                    asm volatile("pause" ::: "memory");
                    timeout--;
                }
                
                if (timeout > 0) {
                    uint16_t rwp = *rirbwp & 0xFF; // Only 8 bits are used for index
                    uint32_t *rirb = (uint32_t *)(rirb_phys + 0xFFFFFFFF80000000);
                    uint32_t response = rirb[rwp * 2]; // 64-bit entries, lower 32 is response
                    uint32_t response_ex = rirb[rwp * 2 + 1]; // upper 32 is codec/node info
                    printk(KERN_INFO "HDA: Codec Response: 0x%x (Ex: 0x%x)\n", response, response_ex);
                    
                    uint16_t vendor_id = response >> 16;
                    uint16_t device_id = response & 0xFFFF;
                    printk(KERN_INFO "HDA: Detected Codec Vendor 0x%x, Device 0x%x\n", vendor_id, device_id);
                } else {
                    printk(KERN_WARN "HDA: Timeout waiting for RIRB response!\n");
                }
                
                return;
            }
        }
    }
}
