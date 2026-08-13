#include "bus.h"
#include "driver.h"
#include "kernel.h"

/* Extern declaration for the global I2C bus type defined in kernel/driver.c */
extern bus_type_t i2c_bus_type;

/* Minimal stub probe implementation – simply reports success */
static int i2c_hid_probe(device_t *dev)
{
    printk(KERN_INFO "i2c_hid: probe stub for device %s\n", dev->name);
    return 0; /* success */
}

/* Minimal stub remove implementation – just logs */
static void i2c_hid_remove(device_t *dev)
{
    printk(KERN_INFO "i2c_hid: remove stub for device %s\n", dev->name);
}

/* Driver definition – registers with the existing I2C bus */
static driver_t i2c_hid_driver = {
    .name   = "i2c_hid",
    .probe  = i2c_hid_probe,
    .remove = i2c_hid_remove,
    .bus    = &i2c_bus_type,
    .ops    = NULL,
    .irq_handler = NULL,
};

/* Initialization entry point – called from kernel init sequence */
void i2c_hid_init(void)
{
    driver_register(&i2c_hid_driver);
    printk(KERN_INFO "i2c_hid driver registered\n");
}
