#include "bus.h"
#include "driver.h"
#include "device.h"
#include "kernel.h"
#include "malloc.h"
#include "kstring.h"

static bus_type_t *bus_list = 0;

/* Global Bus Declarations */
bus_type_t pci_bus_type = { .name = "pci", .match = 0, .probe = 0, .remove = 0 };
bus_type_t usb_bus_type = { .name = "usb", .match = 0, .probe = 0, .remove = 0 };
bus_type_t i2c_bus_type = { .name = "i2c", .match = 0, .probe = 0, .remove = 0 };
bus_type_t spi_bus_type = { .name = "spi", .match = 0, .probe = 0, .remove = 0 };

void driver_framework_init(void) {
    printk(KERN_INFO "Initializing Driver Framework (Bus/Device/Driver Core)\n");
    bus_list = 0;
    
    bus_register(&pci_bus_type);
    bus_register(&usb_bus_type);
    bus_register(&i2c_bus_type);
    bus_register(&spi_bus_type);
}

int bus_register(bus_type_t *bus) {
    if (!bus) return -1;
    bus->devices = 0;
    bus->drivers = 0;
    
    bus->next = bus_list;
    bus_list = bus;
    
    printk(KERN_INFO "bus: registered bus '%s'\n", bus->name);
    return 0;
}

void bus_unregister(bus_type_t *bus) {
    if (!bus) return;
    
    bus_type_t **curr = &bus_list;
    while (*curr) {
        if (*curr == bus) {
            *curr = bus->next;
            break;
        }
        curr = &(*curr)->next;
    }
}

static void try_bind_device(device_t *dev, driver_t *drv) {
    if (dev->driver) return; // Already bound
    if (!dev->bus || !dev->bus->match) return;
    
    if (dev->bus->match(dev, drv) == 0) {
        printk(KERN_INFO "driver: matching driver '%s' to device '%s' on bus '%s'\n",
               drv->name, dev->name, dev->bus->name);
               
        // Probe via bus override, else driver directly
        int ret = 0;
        if (dev->bus->probe) {
            ret = dev->bus->probe(dev);
        } else if (drv->probe) {
            ret = drv->probe(dev);
        }
        
        if (ret == 0) {
            dev->driver = drv;
            printk(KERN_INFO "driver: bound '%s' to '%s' successfully\n", drv->name, dev->name);
        } else {
            printk(KERN_WARN "driver: probe failed for '%s' (%d)\n", dev->name, ret);
        }
    }
}

void bus_add_device(device_t *dev) {
    if (!dev || !dev->bus) return;
    
    dev->next = dev->bus->devices;
    dev->bus->devices = dev;
    
    printk(KERN_INFO "bus: added device '%s' to bus '%s'\n", dev->name, dev->bus->name);
    
    // Try to bind to any existing driver on this bus
    driver_t *drv = dev->bus->drivers;
    while (drv) {
        try_bind_device(dev, drv);
        if (dev->driver) break;
        drv = drv->next;
    }
}

void bus_add_driver(driver_t *drv) {
    if (!drv || !drv->bus) return;
    
    drv->next = drv->bus->drivers;
    drv->bus->drivers = drv;
    
    printk(KERN_INFO "bus: added driver '%s' to bus '%s'\n", drv->name, drv->bus->name);
    
    // Try to bind to any existing, unbound devices on this bus
    device_t *dev = drv->bus->devices;
    while (dev) {
        if (!dev->driver) {
            try_bind_device(dev, drv);
        }
        dev = dev->next;
    }
}

int device_register(device_t *dev) {
    if (!dev || !dev->bus) return -1;
    bus_add_device(dev);
    return 0;
}

void device_unregister(device_t *dev) {
    if (!dev || !dev->bus) return;
    
    if (dev->driver) {
        if (dev->bus->remove) {
            dev->bus->remove(dev);
        } else if (dev->driver->remove) {
            dev->driver->remove(dev);
        }
        dev->driver = 0;
    }
    
    device_t **curr = &dev->bus->devices;
    while (*curr) {
        if (*curr == dev) {
            *curr = dev->next;
            break;
        }
        curr = &(*curr)->next;
    }
}

int driver_register(driver_t *drv) {
    if (!drv || !drv->bus) return -1;
    bus_add_driver(drv);
    return 0;
}

void driver_unregister(driver_t *drv) {
    if (!drv || !drv->bus) return;
    
    // Find all devices bound to this driver and unbind them
    device_t *dev = drv->bus->devices;
    while (dev) {
        if (dev->driver == drv) {
            if (drv->bus->remove) {
                drv->bus->remove(dev);
            } else if (drv->remove) {
                drv->remove(dev);
            }
            dev->driver = 0;
            printk(KERN_INFO "driver: unbound '%s' from '%s'\n", drv->name, dev->name);
        }
        dev = dev->next;
    }
    
    driver_t **curr = &drv->bus->drivers;
    while (*curr) {
        if (*curr == drv) {
            *curr = drv->next;
            break;
        }
        curr = &(*curr)->next;
    }
}
