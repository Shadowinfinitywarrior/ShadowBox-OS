#ifndef SHADOWBOX_BUS_H
#define SHADOWBOX_BUS_H

#include "device.h"
#include "driver.h"

struct bus_type {
    char name[32];
    
    // Bus-specific matching logic
    int (*match)(device_t *dev, driver_t *drv);
    
    // Bus-specific overrides
    int (*probe)(device_t *dev);
    int (*remove)(device_t *dev);
    int (*suspend)(device_t *dev);
    int (*resume)(device_t *dev);
    
    // Devices and Drivers associated with this bus
    device_t *devices;
    driver_t *drivers;
    
    struct bus_type *next;
};

int bus_register(bus_type_t *bus);
void bus_unregister(bus_type_t *bus);

/* Used by buses/devices to orchestrate binding */
void bus_add_device(device_t *dev);
void bus_add_driver(driver_t *drv);

void driver_framework_init(void);

#endif
