#ifndef SHADOWBOX_DRIVER_H
#define SHADOWBOX_DRIVER_H

#include "device.h"

typedef struct {
    uint32_t (*read)(device_t *dev, uint32_t offset, uint32_t size, uint8_t *buffer);
    uint32_t (*write)(device_t *dev, uint32_t offset, uint32_t size, uint8_t *buffer);
    uint32_t (*ioctl)(device_t *dev, uint32_t request, void *arg);
} ops_t;

typedef void (irq_fn)(device_t *dev);

struct driver {
    char name[64];
    
    // Core Driver Methods
    int (*probe)(device_t *dev);
    int (*remove)(device_t *dev);
    int (*suspend)(device_t *dev);
    int (*resume)(device_t *dev);
    
    // File/Device Operations
    ops_t *ops;
    
    // Interrupt Handler
    irq_fn *irq_handler;
    
    // Internal linking
    bus_type_t *bus;
    struct driver *next;
};

/* Driver Registration */
int driver_register(driver_t *drv);
void driver_unregister(driver_t *drv);

#endif
