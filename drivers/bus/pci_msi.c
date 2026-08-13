/*
 * Minimal stub implementation for PCI MSI support.
 * This file provides the basic symbols required by the kernel build
 * system without implementing any real functionality.
 */

#ifndef PCI_MSI_C
#define PCI_MSI_C

#include <stdint.h>

/* Forward declaration of PCI device structure – the real definition
 * resides elsewhere in the kernel sources.  Using a forward declaration
 * avoids compilation errors when the full type is not available.
 */
struct pci_dev;

/* Message format used for MSI interrupts. */
struct pci_msi_msg {
    uint32_t address_lo;
    uint32_t address_hi;
    uint16_t data;
};

/* Allocate MSI vectors for a device.  Returns 0 on success.
 * The stub does not allocate anything; it simply pretends the operation
 * succeeded.
 */
static inline int pci_msi_alloc_vectors(struct pci_dev *dev, int nvec)
{
    (void)dev;
    (void)nvec;
    return 0;
}

/* Free previously allocated MSI vectors. */
static inline void pci_msi_free_vectors(struct pci_dev *dev)
{
    (void)dev;
}

/* Public wrapper used by other kernel code. */
int pci_msi_setup(struct pci_dev *dev, int nvec)
{
    return pci_msi_alloc_vectors(dev, nvec);
}

void pci_msi_teardown(struct pci_dev *dev)
{
    pci_msi_free_vectors(dev);
}

#endif /* PCI_MSI_C */
