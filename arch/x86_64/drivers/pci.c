#include "pci.h"
#include "io.h"
#include "kernel.h"
#include "spinlock.h"
#include "malloc.h"
#include "kstring.h"

static spinlock_t pci_lock;
#define MAX_PCI_DEVICES 256
static pci_device_t pci_devices[MAX_PCI_DEVICES];
static int pci_device_count = 0;

uint32_t pci_config_read(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset) {
    uint32_t address = (uint32_t)(((uint32_t)bus << 16) | ((uint32_t)device << 11) |
                                   ((uint32_t)func << 8) | (offset & 0xFC) | 0x80000000);
    spin_lock_irqsave(&pci_lock);
    outl(PCI_CONFIG_ADDRESS, address);
    uint32_t value = inl(PCI_CONFIG_DATA);
    spin_unlock_irqrestore(&pci_lock);
    return value;
}

void pci_config_write(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint32_t value) {
    uint32_t address = (uint32_t)(((uint32_t)bus << 16) | ((uint32_t)device << 11) |
                                   ((uint32_t)func << 8) | (offset & 0xFC) | 0x80000000);
    spin_lock_irqsave(&pci_lock);
    outl(PCI_CONFIG_ADDRESS, address);
    outl(PCI_CONFIG_DATA, value);
    spin_unlock_irqrestore(&pci_lock);
}

static uint16_t pci_get_vendor_id(uint8_t bus, uint8_t device, uint8_t function) {
    return (uint16_t)(pci_config_read(bus, device, function, 0) & 0xFFFF);
}

void pci_check_function(uint8_t bus, uint8_t device, uint8_t function) {
    if (pci_device_count >= MAX_PCI_DEVICES) return;
    uint16_t vendor = pci_get_vendor_id(bus, device, function);
    if (vendor == 0xFFFF) return;

    uint32_t id_reg = pci_config_read(bus, device, function, 0);
    uint32_t class_reg = pci_config_read(bus, device, function, 8);
    uint32_t header_reg = pci_config_read(bus, device, function, 12);
    uint32_t bar0 = pci_config_read(bus, device, function, 0x10);
    uint32_t bar1 = pci_config_read(bus, device, function, 0x14);
    uint32_t bar2 = pci_config_read(bus, device, function, 0x18);
    uint32_t bar3 = pci_config_read(bus, device, function, 0x1C);
    uint32_t bar4 = pci_config_read(bus, device, function, 0x20);
    uint32_t bar5_val = pci_config_read(bus, device, function, 0x24);
    uint32_t irq_reg = pci_config_read(bus, device, function, 0x3C);

    pci_device_t *d = &pci_devices[pci_device_count++];
    memset(d, 0, sizeof(pci_device_t));
    d->bus = bus;
    d->device = device;
    d->function = function;
    d->vendor_id = vendor;
    d->device_id = (uint16_t)(id_reg >> 16);
    d->class_code = (uint8_t)(class_reg >> 24);
    d->subclass = (uint8_t)(class_reg >> 16);
    d->prog_if = (uint8_t)(class_reg >> 8);
    d->revision_id = (uint8_t)(class_reg);
    d->header_type = (uint8_t)(header_reg >> 16);
    d->bar0 = (uint16_t)(bar0 & 0xFFFE);
    d->bar1 = (uint16_t)(bar1 & 0xFFFE);
    d->bar2 = bar2;
    d->bar3 = bar3;
    d->bar4 = bar4;
    d->bar5 = bar5_val;
    d->irq_line = (uint8_t)(irq_reg);
    d->irq_pin = (uint8_t)(irq_reg >> 8);

    printk(KERN_INFO "PCI: %02x:%02x.%x %04x:%04x class=%02x subclass=%02x irq=%d BAR0=%04x\n",
           bus, device, function, d->vendor_id, d->device_id,
           d->class_code, d->subclass, d->irq_line, d->bar0);
}

void pci_check_device(uint8_t bus, uint8_t device) {
    uint16_t vendor = pci_get_vendor_id(bus, device, 0);
    if (vendor == 0xFFFF) return;
    pci_check_function(bus, device, 0);
    uint8_t header_type = (uint8_t)((pci_config_read(bus, device, 0, 12) >> 16) & 0xFF);
    if (header_type & 0x80) {
        for (uint8_t func = 1; func < 8; func++) {
            if (pci_get_vendor_id(bus, device, func) != 0xFFFF)
                pci_check_function(bus, device, func);
        }
    }
}

void pci_enumerate(void) {
    pci_device_count = 0;
    printk(KERN_INFO "PCI: Enumerating buses...\n");
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t device = 0; device < 32; device++) {
            pci_check_device((uint8_t)bus, device);
        }
    }
    printk(KERN_INFO "PCI: %d devices found.\n", pci_device_count);
}

void pci_init(void) {
    spinlock_init(&pci_lock);
    pci_enumerate();
}

pci_device_t *pci_find_device(uint16_t vendor_id, uint16_t device_id) {
    for (int i = 0; i < pci_device_count; i++) {
        if (pci_devices[i].vendor_id == vendor_id &&
            pci_devices[i].device_id == device_id)
            return &pci_devices[i];
    }
    return 0;
}

void pci_enable_bus_mastering(pci_device_t *dev) {
    uint32_t cmd_reg = pci_config_read(dev->bus, dev->device, dev->function, 4);
    cmd_reg |= 0x07;
    pci_config_write(dev->bus, dev->device, dev->function, 4, cmd_reg);
}
