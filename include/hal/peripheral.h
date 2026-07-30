#ifndef SHADOWBOX_HAL_PERIPHERAL_H
#define SHADOWBOX_HAL_PERIPHERAL_H

#include "types.h"
#include "hal/hal.h"

typedef enum {
    PERIPHERAL_BUS_PCI,
    PERIPHERAL_BUS_USB,
    PERIPHERAL_BUS_I2C,
    PERIPHERAL_BUS_SPI,
    PERIPHERAL_BUS_PCIE,
    PERIPHERAL_BUS_ISA,
    PERIPHERAL_BUS_UNKNOWN
} peripheral_bus_t;

typedef struct {
    peripheral_bus_t bus;
    uint32_t vendor_id;
    uint32_t device_id;
    uint32_t revision;
    uint32_t irq;
    uint64_t mmio_base[6];
    uint64_t mmio_size[6];
    uint32_t mmio_count;
} peripheral_device_t;

typedef struct {
    uint32_t device_count;
    peripheral_device_t devices[64];
} peripheral_list_t;

hal_status_t peripheral_init(void);
int peripheral_enumerate(peripheral_list_t *list);
hal_status_t peripheral_read_config(peripheral_device_t *dev, uint32_t offset, uint32_t *value);
hal_status_t peripheral_write_config(peripheral_device_t *dev, uint32_t offset, uint32_t value);

hal_status_t usb_hal_init(void);
hal_status_t usb_hal_enumerate(void);

hal_status_t i2c_init(void);
hal_status_t i2c_read(uint8_t bus, uint8_t addr, uint8_t reg, uint8_t *data, uint32_t len);
hal_status_t i2c_write(uint8_t bus, uint8_t addr, uint8_t reg, const uint8_t *data, uint32_t len);

hal_status_t spi_init(void);
hal_status_t spi_transfer(uint8_t bus, const uint8_t *tx, uint8_t *rx, uint32_t len);

#endif
