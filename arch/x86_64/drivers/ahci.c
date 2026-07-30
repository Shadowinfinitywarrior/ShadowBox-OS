#include "ahci.h"
#include "kernel.h"
#include "pci.h"
#include "vmm.h"
#include "pmm.h"
#include "kstring.h"

// Basic implementation of AHCI

static HBA_MEM *abar;
static block_device_t ahci_dev;

#define HBA_PxCMD_ST    0x0001
#define HBA_PxCMD_FRE   0x0010
#define HBA_PxCMD_FR    0x4000
#define HBA_PxCMD_CR    0x8000

static int check_type(HBA_PORT *port) {
    uint32_t ssts = port->ssts;
    uint8_t ipm = (ssts >> 8) & 0x0F;
    uint8_t det = ssts & 0x0F;

    if (det != HBA_PORT_DET_PRESENT)
        return 0;
    if (ipm != HBA_PORT_IPM_ACTIVE)
        return 0;

    switch (port->sig) {
        case 0x00000101: return 1; // SATA drive
        case 0xEB140101: return 2; // SATAPI drive
        case 0xC33C0101: return 3; // Enclosure
        case 0x96690101: return 4; // Port multiplier
        default: return 0;
    }
}

static void start_cmd(HBA_PORT *port) {
    while (port->cmd & HBA_PxCMD_CR);
    port->cmd |= HBA_PxCMD_FRE;
    port->cmd |= HBA_PxCMD_ST;
}

static void stop_cmd(HBA_PORT *port) {
    port->cmd &= ~HBA_PxCMD_ST;
    port->cmd &= ~HBA_PxCMD_FRE;
    while(1) {
        if (port->cmd & HBA_PxCMD_FR) continue;
        if (port->cmd & HBA_PxCMD_CR) continue;
        break;
    }
}

static void port_rebase(HBA_PORT *port, int portno) {
    stop_cmd(port);
    
    // Allocate 1 page for this port's structures
    uint64_t phys_page = (uint64_t)pmm_alloc_page();
    if (!phys_page) panic("ahci: OOM in port_rebase");
    uint8_t *virt_page = (uint8_t*)(phys_page + 0xFFFFFFFF80000000);
    memset(virt_page, 0, 4096);
    
    // Command list (1024 bytes)
    port->clb = (uint32_t)(phys_page & 0xFFFFFFFF);
    port->clbu = (uint32_t)(phys_page >> 32);
    
    // FIS (256 bytes)
    port->fb = (uint32_t)((phys_page + 1024) & 0xFFFFFFFF);
    port->fbu = (uint32_t)((phys_page + 1024) >> 32);
    
    // 1 Command Table (256 bytes)
    HBA_CMD_HEADER *cmdheader = (HBA_CMD_HEADER*)(phys_page + 0xFFFFFFFF80000000);
    uint64_t cmd_tbl_phys = phys_page + 1024 + 256;
    cmdheader[0].prdtl = 1;
    cmdheader[0].ctba = (uint32_t)(cmd_tbl_phys & 0xFFFFFFFF);
    cmdheader[0].ctbau = (uint32_t)(cmd_tbl_phys >> 32);
    
    start_cmd(port);
}

static int ahci_read(UNUSED block_device_t *dev, uint64_t lba, uint32_t count, void *buffer) {
    HBA_PORT *port = &abar->ports[0];
    
    port->is = (uint32_t)-1;
    int spin = 0;
    int slot = 0;
    
    HBA_CMD_HEADER *cmdheader = (HBA_CMD_HEADER*)((uint64_t)port->clb | ((uint64_t)port->clbu << 32) | 0xFFFFFFFF80000000);
    cmdheader->cfl = sizeof(FIS_REG_H2D)/sizeof(uint32_t);
    cmdheader->w = 0;
    cmdheader->prdtl = 1;
    
    HBA_CMD_TBL *cmdtbl = (HBA_CMD_TBL*)((uint64_t)cmdheader->ctba | ((uint64_t)cmdheader->ctbau << 32) | 0xFFFFFFFF80000000);
    memset(cmdtbl, 0, sizeof(HBA_CMD_TBL));
    
    uint64_t buf_phys = (uint64_t)pmm_alloc_page();
    if (!buf_phys) return 0;
    
    cmdtbl->prdt_entry[0].dba = (uint32_t)buf_phys;
    cmdtbl->prdt_entry[0].dbau = (uint32_t)(buf_phys >> 32);
    cmdtbl->prdt_entry[0].dbc = (count * 512) - 1;
    cmdtbl->prdt_entry[0].i = 1;
    
    FIS_REG_H2D *cmdfis = (FIS_REG_H2D*)(&cmdtbl->cfis);
    cmdfis->fis_type = FIS_TYPE_REG_H2D;
    cmdfis->c = 1;
    cmdfis->command = 0x24; // READ DMA EXT
    
    cmdfis->lba0 = (uint8_t)lba;
    cmdfis->lba1 = (uint8_t)(lba >> 8);
    cmdfis->lba2 = (uint8_t)(lba >> 16);
    cmdfis->device = 1<<6;
    
    cmdfis->lba3 = (uint8_t)(lba >> 24);
    cmdfis->lba4 = (uint8_t)(lba >> 32);
    cmdfis->lba5 = (uint8_t)(lba >> 40);
    
    cmdfis->countl = (uint8_t)count;
    cmdfis->counth = (uint8_t)(count >> 8);
    
    while ((port->tfd & (0x80 | 0x08)) && spin < 1000000) spin++;
    if (spin == 1000000) return 0; // timeout
    
    port->ci = 1<<slot;
    
    while (1) {
        if ((port->ci & (1<<slot)) == 0) break;
        if (port->is & (1<<30)) return 0; // error
    }
    
    uint8_t *vbuf = (uint8_t*)(buf_phys + 0xFFFFFFFF80000000);
    memcpy(buffer, vbuf, count * 512);
    
    extern void pmm_free_page(void*);
    pmm_free_page((void*)buf_phys);
    
    return 1;
}

static int ahci_write(UNUSED block_device_t *dev, uint64_t lba, uint32_t count, const void *buffer) {
    HBA_PORT *port = &abar->ports[0];
    
    port->is = (uint32_t)-1;
    int spin = 0;
    int slot = 0;
    
    HBA_CMD_HEADER *cmdheader = (HBA_CMD_HEADER*)((uint64_t)port->clb | ((uint64_t)port->clbu << 32) | 0xFFFFFFFF80000000);
    cmdheader->cfl = sizeof(FIS_REG_H2D)/sizeof(uint32_t);
    cmdheader->w = 1;
    cmdheader->prdtl = 1;
    
    HBA_CMD_TBL *cmdtbl = (HBA_CMD_TBL*)((uint64_t)cmdheader->ctba | ((uint64_t)cmdheader->ctbau << 32) | 0xFFFFFFFF80000000);
    memset(cmdtbl, 0, sizeof(HBA_CMD_TBL));
    
    uint64_t buf_phys = (uint64_t)pmm_alloc_page();
    if (!buf_phys) return 0;
    uint8_t *vbuf = (uint8_t*)(buf_phys + 0xFFFFFFFF80000000);
    memcpy(vbuf, buffer, count * 512);
    
    cmdtbl->prdt_entry[0].dba = (uint32_t)buf_phys;
    cmdtbl->prdt_entry[0].dbau = (uint32_t)(buf_phys >> 32);
    cmdtbl->prdt_entry[0].dbc = (count * 512) - 1;
    cmdtbl->prdt_entry[0].i = 1;
    
    FIS_REG_H2D *cmdfis = (FIS_REG_H2D*)(&cmdtbl->cfis);
    cmdfis->fis_type = FIS_TYPE_REG_H2D;
    cmdfis->c = 1;
    cmdfis->command = 0x35; // WRITE DMA EXT
    
    cmdfis->lba0 = (uint8_t)lba;
    cmdfis->lba1 = (uint8_t)(lba >> 8);
    cmdfis->lba2 = (uint8_t)(lba >> 16);
    cmdfis->device = 1<<6;
    
    cmdfis->lba3 = (uint8_t)(lba >> 24);
    cmdfis->lba4 = (uint8_t)(lba >> 32);
    cmdfis->lba5 = (uint8_t)(lba >> 40);
    
    cmdfis->countl = (uint8_t)count;
    cmdfis->counth = (uint8_t)(count >> 8);
    
    while ((port->tfd & (0x80 | 0x08)) && spin < 1000000) spin++;
    if (spin == 1000000) return 0;
    
    port->ci = 1<<slot;
    
    while (1) {
        if ((port->ci & (1<<slot)) == 0) break;
        if (port->is & (1<<30)) return 0;
    }
    
    extern void pmm_free_page(void*);
    pmm_free_page((void*)buf_phys);
    
    return 1;
}

void ahci_init(void) {
    printk(KERN_INFO "AHCI: Scanning PCI for AHCI controller...\n");
    uint8_t ahci_bus = 0, ahci_dev_num = 0, ahci_func = 0;
    int found = 0;
    
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t device = 0; device < 32; device++) {
            uint16_t vendor = (uint16_t)(pci_config_read(bus, device, 0, 0) & 0xFFFF);
            if (vendor == 0xFFFF) continue;
            uint8_t class_id = (uint8_t)((pci_config_read(bus, device, 0, 8) >> 24) & 0xFF);
            uint8_t subclass = (uint8_t)((pci_config_read(bus, device, 0, 8) >> 16) & 0xFF);
            if (class_id == 0x01 && subclass == 0x06) {
                ahci_bus = bus; ahci_dev_num = device; ahci_func = 0;
                found = 1;
                break;
            }
        }
        if (found) break;
    }
    
    if (!found) {
        printk(KERN_WARN "AHCI: No controller found.\n");
        return;
    }
    
    printk(KERN_INFO "AHCI: Controller found at %d:%d:%d\n", ahci_bus, ahci_dev_num, ahci_func);
    
    uint32_t bar5 = pci_config_read(ahci_bus, ahci_dev_num, ahci_func, 0x24);
    uint32_t phys_abar = bar5 & 0xFFFFFFF0;
    
    // Map AHCI ABAR (usually 4KB)
    abar = (HBA_MEM*)vmap_phys((uint64_t)phys_abar, 4096);
    
    // enable pci busmastering
    uint32_t command_reg = pci_config_read(ahci_bus, ahci_dev_num, ahci_func, 0x04);
    pci_config_write(ahci_bus, ahci_dev_num, ahci_func, 0x04, command_reg | 0x04 | 0x02);
    
    // Enable global AHCI
    abar->ghc |= (1<<31);
    
    int pi = abar->pi;
    for (int i=0; i<32; i++) {
        if (pi & (1<<i)) {
            int dt = check_type(&abar->ports[i]);
            if (dt == 1) {
                printk(KERN_INFO "AHCI: SATA drive found on port %d\n", i);
                port_rebase(&abar->ports[i], i);
                
                ahci_dev.name = "sda";
                ahci_dev.block_size = 512;
                ahci_dev.total_blocks = 20480; // 10MB default
                ahci_dev.read_block = ahci_read;
                ahci_dev.write_block = ahci_write;
                ahci_dev.next = 0;
                block_register_device(&ahci_dev);
                break; // Just map the first SATA drive for now
            }
        }
    }
}
