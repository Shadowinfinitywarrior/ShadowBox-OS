#ifndef SHADOWBOX_DEVICE_H
#define SHADOWBOX_DEVICE_H

#include "types.h"
#include "sysfs.h"

typedef struct bus_type bus_type_t;
typedef struct driver driver_t;

typedef enum {
    BUS_TYPE_UNKNOWN = 0,
    BUS_TYPE_PCI,
    BUS_TYPE_USB,
    BUS_TYPE_I2C,
    BUS_TYPE_SPI,
    BUS_TYPE_PLATFORM
} bus_id_t;

typedef struct device {
    char name[64];
    bus_id_t bus_id;
    bus_type_t *bus;
    driver_t *driver;
    
    void *bus_data;    // Bus-specific data (e.g. pci_device_t*)
    void *driver_data; // Driver-specific data (e.g. state)
    
    struct device *parent;
    struct device *next;
    
    kobject_t kobj;    // For sysfs integration
} device_t;

/* Device Management */
int device_register(device_t *dev);
void device_unregister(device_t *dev);

#endif
