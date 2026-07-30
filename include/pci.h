#ifndef SHADOWBOX_PCI_H
#define SHADOWBOX_PCI_H

#include "types.h"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

/*
 * pci_device_t - PCI device information
 * @bus:        PCI bus number
 * @device:     Device number on bus
 * @function:   Function number
 * @vendor_id:  Vendor identifier
 * @device_id:  Device identifier
 * @class_code: Base class code
 * @subclass:   Subclass code
 * @prog_if:    Programming interface
 * @revision_id: Revision identifier
 * @header_type: Header type
 */
typedef struct {
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class_code;
    uint8_t subclass;
    uint8_t prog_if;
    uint8_t revision_id;
    uint8_t header_type;
    uint16_t bar0;
    uint16_t bar1;
    uint32_t bar2;
    uint32_t bar3;
    uint32_t bar4;
    uint32_t bar5;
    uint8_t irq_line;
    uint8_t irq_pin;
} pci_device_t;

/*
 * pci_init - Initialize PCI subsystem and enumerate devices
 */
void pci_init(void);

/*
 * pci_config_read - Read from PCI configuration space
 * @bus:    PCI bus number
 * @device: Device number
 * @func:   Function number
 * @offset: Register offset
 * Returns: 32-bit value read
 */
uint32_t pci_config_read(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset);

/*
 * pci_config_write - Write to PCI configuration space
 * @bus:    PCI bus number
 * @device: Device number
 * @func:   Function number
 * @offset: Register offset
 * @value:  Value to write
 */
void pci_config_write(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint32_t value);

/*
 * pci_enumerate - Enumerate all PCI devices
 */
void pci_enumerate(void);

/*
 * pci_check_device - Check if a PCI device exists
 * @bus:    PCI bus number
 * @device: Device number
 */
void pci_check_device(uint8_t bus, uint8_t device);

/*
 * pci_check_function - Probe a specific PCI function
 * @bus:      PCI bus number
 * @device:   Device number
 * @function: Function number
 */
void pci_check_function(uint8_t bus, uint8_t device, uint8_t function);

pci_device_t *pci_find_device(uint16_t vendor_id, uint16_t device_id);
void pci_enable_bus_mastering(pci_device_t *dev);

#endif
