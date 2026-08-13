// Minimal stub implementation for SPI master driver
// This file provides just enough definitions to compile and register
// a driver for the SPI bus. No actual hardware functionality is implemented.

#include "driver.h"
#include "bus.h"
#include "device.h"

/* Probe function – called when a device is matched to this driver */
static int spi_master_probe(device_t *dev)
{
    // No real initialization; simply succeed.
    (void)dev;
    return 0;
}

/* Remove function – called when the driver is detached */
static int spi_master_remove(device_t *dev)
{
    (void)dev;
    return 0;
}

/* Optional suspend/resume – left as NULL for now */

static driver_t spi_master_driver = {
    .name = "spi_master",
    .probe = spi_master_probe,
    .remove = spi_master_remove,
    .suspend = NULL,
    .resume = NULL,
    .ops = NULL,
    .irq_handler = NULL,
    .bus = NULL,
    .next = NULL,
};

/* Register the driver at module load time */
static void __attribute__((constructor)) spi_master_register(void)
{
    driver_register(&spi_master_driver);
}
