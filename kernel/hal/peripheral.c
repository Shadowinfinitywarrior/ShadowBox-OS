#include "hal/peripheral.h"
#include "kernel.h"
#include "pci.h"

static bool peripheral_initialized = false;
static peripheral_list_t peripheral_list;

hal_status_t peripheral_init(void) {
    if (peripheral_initialized) return HAL_SUCCESS;

    for (int i = 0; i < 64; i++)
        peripheral_list.devices[i].bus = PERIPHERAL_BUS_UNKNOWN;
    peripheral_list.device_count = 0;
    peripheral_initialized = true;
    return HAL_SUCCESS;
}

int peripheral_enumerate(peripheral_list_t *list) {
    if (!list) return 0;
    list->device_count = peripheral_list.device_count;
    for (int i = 0; i < peripheral_list.device_count && i < 64; i++)
        list->devices[i] = peripheral_list.devices[i];
    return list->device_count;
}

hal_status_t peripheral_read_config(peripheral_device_t *dev, uint32_t offset, uint32_t *value) {
    if (!dev || !value) return HAL_ERROR_IO;
    if (dev->bus != PERIPHERAL_BUS_PCI) return HAL_ERROR_UNSUPPORTED;
    *value = pci_config_read(dev->vendor_id & 0xFF, (dev->vendor_id >> 8) & 0xFF,
                             dev->device_id & 0xFF, offset);
    return HAL_SUCCESS;
}

hal_status_t peripheral_write_config(peripheral_device_t *dev, uint32_t offset, uint32_t value) {
    if (!dev) return HAL_ERROR_IO;
    if (dev->bus != PERIPHERAL_BUS_PCI) return HAL_ERROR_UNSUPPORTED;
    pci_config_write(dev->vendor_id & 0xFF, (dev->vendor_id >> 8) & 0xFF,
                     dev->device_id & 0xFF, offset, value);
    return HAL_SUCCESS;
}

hal_status_t i2c_init(void) {
    return HAL_ERROR_UNSUPPORTED;
}

hal_status_t spi_init(void) {
    return HAL_ERROR_UNSUPPORTED;
}
