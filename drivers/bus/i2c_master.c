/* Minimal I2C master bus stub */

#include "bus.h"
#include "device.h"
#include "driver.h"
#include "kernel.h"

/* Forward declarations */
static int i2c_master_match(device_t *dev, driver_t *drv) { return 0; }
static int i2c_master_probe(device_t *dev) { return 0; }
static int i2c_master_remove(device_t *dev) { return 0; }

/* I2C master bus type */
bus_type_t i2c_master_bus = {
    .name = "i2c_master",
    .match = i2c_master_match,
    .probe = i2c_master_probe,
    .remove = i2c_master_remove,
    .suspend = NULL,
    .resume = NULL,
    .devices = NULL,
    .drivers = NULL,
    .next = NULL,
};

/* Initialization function to be called from driver framework */
void i2c_master_init(void) {
    bus_register(&i2c_master_bus);
}
