#ifndef SHADOWBOX_HAL_STORAGE_H
#define SHADOWBOX_HAL_STORAGE_H

#include "types.h"
#include "hal/hal.h"

// ============================================================================
// Storage Device Types
// ============================================================================

typedef enum {
    STORAGE_TYPE_SATA,
    STORAGE_TYPE_NVME,
    STORAGE_TYPE_SCSI,
    STORAGE_TYPE_SAS,
    STORAGE_TYPE_USB,
    STORAGE_TYPE_SD,
    STORAGE_TYPE_MMC,
    STORAGE_TYPE_ATA,
    STORAGE_TYPE_ATAPI,
    STORAGE_TYPE_FLOPPY,
    STORAGE_TYPE_CDROM,
    STORAGE_TYPE_VIRTIO,
    STORAGE_TYPE_RAMDISK,
    STORAGE_TYPE_NBD,
    STORAGE_TYPE_UNKNOWN
} storage_type_t;

// ============================================================================
// Storage Interface Types
// ============================================================================

typedef enum {
    STORAGE_INTERFACE_PATA,       // Parallel ATA (IDE)
    STORAGE_INTERFACE_SATA,       // Serial ATA
    STORAGE_INTERFACE_SCSI,       // SCSI
    STORAGE_INTERFACE_SAS,        // Serial Attached SCSI
    STORAGE_INTERFACE_NVME,       // NVMe
    STORAGE_INTERFACE_USB,        // USB Mass Storage
    STORAGE_INTERFACE_SD,         // SD Card
    STORAGE_INTERFACE_MMC,        // MMC
    STORAGE_INTERFACE_SPI,        // SPI Flash
    STORAGE_INTERFACE_I2C,        // I2C EEPROM
    STORAGE_INTERFACE_UFS,        // Universal Flash Storage
    STORAGE_INTERFACE_VIRTIO,     // Virtio
    STORAGE_INTERFACE_UNKNOWN
} storage_interface_t;

// ============================================================================
// Storage Protocol Types
// ============================================================================

typedef enum {
    STORAGE_PROTOCOL_AHCI,        // AHCI (SATA)
    STORAGE_PROTOCOL_NVME,        // NVMe
    STORAGE_PROTOCOL_SCSI,        // SCSI
    STORAGE_PROTOCOL_ATA,         // ATA/ATAPI
    STORAGE_PROTOCOL_USB,         // USB Mass Storage (UAS, BOT)
    STORAGE_PROTOCOL_SD,          // SD Card Protocol
    STORAGE_PROTOCOL_MMC,         // MMC Protocol
    STORAGE_PROTOCOL_SPI,         // SPI Flash Protocol
    STORAGE_PROTOCOL_I2C,         // I2C EEPROM Protocol
    STORAGE_PROTOCOL_VIRTIO,      // Virtio Block
    STORAGE_PROTOCOL_UNKNOWN
} storage_protocol_t;

// ============================================================================
// Storage Device Information
// ============================================================================

typedef struct {
    char vendor[41];            // Vendor string (null-terminated)
    char model[41];             // Model string (null-terminated)
    char serial[21];            // Serial number (null-terminated)
    char firmware[9];           // Firmware version (null-terminated)
    storage_type_t type;        // Device type
    storage_interface_t interface; // Interface type
    storage_protocol_t protocol; // Protocol type
    uint64_t capacity;          // Total capacity in bytes
    uint32_t sector_size;       // Sector size in bytes
    uint64_t sector_count;      // Total number of sectors
    uint32_t alignment;         // Required alignment in bytes
    uint32_t flags;             // Device flags
    uint16_t major_version;     // Major version
    uint16_t minor_version;     // Minor version
} storage_device_info_t;

// Storage device flags
#define STORAGE_FLAG_REMOVABLE     (1 << 0)
#define STORAGE_FLAG_WRITABLE      (1 << 1)
#define STORAGE_FLAG_READABLE      (1 << 2)
#define STORAGE_FLAG_BOOTABLE      (1 << 3)
#define STORAGE_FLAG_ENCRYPTED     (1 << 4)
#define STORAGE_FLAG_TRIM_SUPPORT  (1 << 5)
#define STORAGE_FLAG_NCQ_SUPPORT   (1 << 6)
#define STORAGE_FLAG_APST_SUPPORT  (1 << 7)
#define STORAGE_FLAG_HOTPLUG       (1 << 8)

// ============================================================================
// Storage Device Statistics
// ============================================================================

typedef struct {
    uint64_t read_count;        // Number of read operations
    uint64_t write_count;       // Number of write operations
    uint64_t read_bytes;        // Total bytes read
    uint64_t write_bytes;       // Total bytes written
    uint64_t read_errors;       // Number of read errors
    uint64_t write_errors;      // Number of write errors
    uint64_t io_errors;         // Number of I/O errors
    uint64_t timeout_errors;    // Number of timeout errors
    uint64_t media_errors;      // Number of media errors
    uint64_t total_time;        // Total time in microseconds
    uint64_t busy_time;         // Busy time in microseconds
} storage_stats_t;

// ============================================================================
// Storage Command Types
// ============================================================================

typedef enum {
    STORAGE_CMD_READ,
    STORAGE_CMD_WRITE,
    STORAGE_CMD_FLUSH,
    STORAGE_CMD_TRIM,
    STORAGE_CMD_SECURE_ERASE,
    STORAGE_CMD_IDENTIFY,
    STORAGE_CMD_SMART,
    STORAGE_CMD_STANDBY,
    STORAGE_CMD_SLEEP,
    STORAGE_CMD_WAKEUP,
    STORAGE_CMD_RESET,
    STORAGE_CMD_POWER_ON,
    STORAGE_CMD_POWER_OFF,
    STORAGE_CMD_GET_TEMPERATURE,
    STORAGE_CMD_GET_STATUS,
    STORAGE_CMD_SET_FEATURES,
    STORAGE_CMD_GET_FEATURES,
    STORAGE_CMD_ABORT,
    STORAGE_CMD_UNKNOWN
} storage_command_t;

// ============================================================================
// Storage Request Structure
// ============================================================================

typedef struct storage_request {
    storage_command_t command;  // Command type
    void *buffer;               // Data buffer
    uint64_t offset;            // Offset in bytes
    uint64_t size;              // Size in bytes
    uint32_t timeout;           // Timeout in milliseconds
    uint32_t flags;             // Request flags
    void *private_data;         // Private data for driver
    hal_status_t status;        // Completion status
    uint64_t actual_size;       // Actual bytes transferred
    uint64_t start_time;        // Request start time
    uint64_t end_time;          // Request end time
    struct storage_request *next; // Next request in queue
} storage_request_t;

// Storage request flags
#define STORAGE_REQ_SYNC          (1 << 0)
#define STORAGE_REQ_ASYNC         (1 << 1)
#define STORAGE_REQ_DMA           (1 << 2)
#define STORAGE_REQ_FUA           (1 << 3)  // Force Unit Access
#define STORAGE_REQ_NCQ           (1 << 4)  // Native Command Queuing
#define STORAGE_REQ_PRIORITY_HIGH (1 << 5)
#define STORAGE_REQ_PRIORITY_LOW  (1 << 6)

// ============================================================================
// Storage Device Operations
// ============================================================================

typedef struct storage_ops {
    // Device information
    hal_status_t (*get_info)(void *dev, storage_device_info_t *info);
    hal_status_t (*get_stats)(void *dev, storage_stats_t *stats);
    
    // Device control
    hal_status_t (*init)(void *dev);
    hal_status_t (*shutdown)(void *dev);
    hal_status_t (*reset)(void *dev);
    hal_status_t (*power_on)(void *dev);
    hal_status_t (*power_off)(void *dev);
    hal_status_t (*standby)(void *dev);
    
    // I/O operations
    hal_status_t (*read)(void *dev, void *buf, uint64_t offset, uint64_t size);
    hal_status_t (*write)(void *dev, const void *buf, uint64_t offset, uint64_t size);
    hal_status_t (*flush)(void *dev);
    hal_status_t (*trim)(void *dev, uint64_t offset, uint64_t size);
    
    // Async operations
    hal_status_t (*submit_request)(void *dev, storage_request_t *req);
    hal_status_t (*cancel_request)(void *dev, storage_request_t *req);
    
    // Advanced features
    hal_status_t (*identify)(void *dev, void *buf, uint32_t size);
    hal_status_t (*smart_read)(void *dev, void *buf, uint32_t size);
    hal_status_t (*set_features)(void *dev, uint32_t feature, uint32_t value);
    hal_status_t (*get_features)(void *dev, uint32_t feature, uint32_t *value);
    
    // Interrupt handling
    void (*irq_handler)(void *dev, uint32_t irq);
    
    // DMA operations
    hal_status_t (*dma_read)(void *dev, uint64_t phys, uint64_t offset, uint64_t size);
    hal_status_t (*dma_write)(void *dev, uint64_t phys, uint64_t offset, uint64_t size);
} storage_ops_t;

// ============================================================================
// Storage Device Structure
// ============================================================================

typedef struct storage_device {
    char name[32];            // Device name (e.g., "sda", "nvme0")
    uint32_t id;              // Device ID
    storage_type_t type;      // Device type
    storage_interface_t interface; // Interface type
    storage_protocol_t protocol; // Protocol type
    uint32_t bus;             // Bus number
    uint32_t device;          // Device number
    uint32_t function;        // Function number (for PCI)
    void *private_data;       // Private data for driver
    storage_ops_t *ops;       // Device operations
    storage_device_info_t info; // Device information
    storage_stats_t stats;    // Device statistics
    bool initialized;         // Device initialized
    bool present;            // Device present
    struct storage_device *next; // Next device in list
} storage_device_t;

// ============================================================================
// NVMe-Specific Definitions
// ============================================================================

#define NVME_QUEUE_DEPTH 64

typedef struct {
    uint32_t nsid;            // Namespace ID
    uint64_t capacity;        // Namespace capacity in bytes
    uint32_t sector_size;     // Sector size in bytes
    uint64_t sector_count;    // Number of sectors
    uint8_t lba_format;       // LBA format
} nvme_namespace_t;

typedef struct {
    uint16_t vid;             // Vendor ID
    uint16_t ssid;            // Subsystem ID
    uint8_t sn[20];           // Serial number
    uint8_t mn[40];           // Model number
    uint8_t fr[8];            // Firmware revision
    uint32_t max_namespaces;  // Maximum namespaces
    uint32_t num_namespaces;  // Number of namespaces
    nvme_namespace_t *namespaces; // Namespace array
} nvme_controller_info_t;

// ============================================================================
// SATA-Specific Definitions
// ============================================================================

typedef struct {
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t revision;
    uint8_t class_code;
    uint8_t subclass;
    uint8_t prog_if;
    uint32_t bar0;            // AHCI base address
    uint32_t bar1;
    uint32_t bar2;
    uint32_t bar3;
    uint32_t bar4;
    uint32_t bar5;
    uint8_t irq;
} sata_controller_info_t;

// ============================================================================
// Storage Abstraction Functions
// ============================================================================

/**
 * @brief Initialize storage abstraction layer
 * @return hal_status_t Status of initialization
 */
hal_status_t storage_init(void);

/**
 * @brief Enumerate all storage devices
 * @return int Number of devices found
 */
int storage_enumerate(void);

/**
 * @brief Get list of storage devices
 * @return storage_device_t* Pointer to first device
 */
storage_device_t* storage_get_devices(void);

/**
 * @brief Find storage device by name
 * @param name Device name (e.g., "sda", "nvme0")
 * @return storage_device_t* Pointer to device (NULL if not found)
 */
storage_device_t* storage_find_device(const char *name);

/**
 * @brief Find storage device by ID
 * @param id Device ID
 * @return storage_device_t* Pointer to device (NULL if not found)
 */
storage_device_t* storage_find_device_by_id(uint32_t id);

/**
 * @brief Find storage device by type
 * @param type Device type
 * @return storage_device_t* Pointer to first matching device
 */
storage_device_t* storage_find_device_by_type(storage_type_t type);

/**
 * @brief Initialize a storage device
 * @param dev Pointer to storage device
 * @return hal_status_t Status of initialization
 */
hal_status_t storage_device_init(storage_device_t *dev);

/**
 * @brief Shutdown a storage device
 * @param dev Pointer to storage device
 */
void storage_device_shutdown(storage_device_t *dev);

/**
 * @brief Read from a storage device
 * @param dev Pointer to storage device
 * @param buf Buffer to read into
 * @param offset Offset in bytes
 * @param size Size in bytes
 * @return hal_status_t Status of operation
 */
hal_status_t storage_read(storage_device_t *dev, void *buf, uint64_t offset, uint64_t size);

/**
 * @brief Write to a storage device
 * @param dev Pointer to storage device
 * @param buf Buffer to write from
 * @param offset Offset in bytes
 * @param size Size in bytes
 * @return hal_status_t Status of operation
 */
hal_status_t storage_write(storage_device_t *dev, const void *buf, uint64_t offset, uint64_t size);

/**
 * @brief Flush a storage device
 * @param dev Pointer to storage device
 * @return hal_status_t Status of operation
 */
hal_status_t storage_flush(storage_device_t *dev);

/**
 * @brief TRIM a storage device
 * @param dev Pointer to storage device
 * @param offset Offset in bytes
 * @param size Size in bytes
 * @return hal_status_t Status of operation
 */
hal_status_t storage_trim(storage_device_t *dev, uint64_t offset, uint64_t size);

/**
 * @brief Get device information
 * @param dev Pointer to storage device
 * @param info Pointer to storage_device_info_t structure to fill
 * @return hal_status_t Status of operation
 */
hal_status_t storage_get_info(storage_device_t *dev, storage_device_info_t *info);

/**
 * @brief Get device statistics
 * @param dev Pointer to storage device
 * @param stats Pointer to storage_stats_t structure to fill
 * @return hal_status_t Status of operation
 */
hal_status_t storage_get_stats(storage_device_t *dev, storage_stats_t *stats);

/**
 * @brief Submit an async storage request
 * @param dev Pointer to storage device
 * @param req Pointer to storage_request_t structure
 * @return hal_status_t Status of operation
 */
hal_status_t storage_submit_request(storage_device_t *dev, storage_request_t *req);

/**
 * @brief Wait for a storage request to complete
 * @param dev Pointer to storage device
 * @param req Pointer to storage_request_t structure
 * @param timeout Timeout in milliseconds (0 = wait forever)
 * @return hal_status_t Status of operation
 */
hal_status_t storage_wait_request(storage_device_t *dev, storage_request_t *req, uint32_t timeout);

/**
 * @brief Cancel a storage request
 * @param dev Pointer to storage device
 * @param req Pointer to storage_request_t structure
 * @return hal_status_t Status of operation
 */
hal_status_t storage_cancel_request(storage_device_t *dev, storage_request_t *req);

// ============================================================================
// NVMe-Specific Functions
// ============================================================================

/**
 * @brief Initialize NVMe abstraction
 * @return hal_status_t Status of initialization
 */
hal_status_t nvme_init(void);

/**
 * @brief Enumerate NVMe controllers
 * @return int Number of controllers found
 */
int nvme_enumerate(void);

/**
 * @brief Get NVMe controller by ID
 * @param id Controller ID
 * @return storage_device_t* Pointer to device (NULL if not found)
 */
storage_device_t* nvme_get_controller(uint32_t id);

/**
 * @brief Get NVMe controller information
 * @param dev Pointer to storage device
 * @param info Pointer to nvme_controller_info_t structure to fill
 * @return hal_status_t Status of operation
 */
hal_status_t nvme_get_controller_info(storage_device_t *dev, nvme_controller_info_t *info);

// ============================================================================
// SATA/AHCI-Specific Functions
// ============================================================================

/**
 * @brief Initialize SATA abstraction
 * @return hal_status_t Status of initialization
 */
hal_status_t sata_init(void);

/**
 * @brief Enumerate SATA controllers
 * @return int Number of controllers found
 */
int sata_enumerate(void);

/**
 * @brief Get SATA controller by ID
 * @param id Controller ID
 * @return storage_device_t* Pointer to device (NULL if not found)
 */
storage_device_t* sata_get_controller(uint32_t id);

/**
 * @brief Get SATA controller information
 * @param dev Pointer to storage device
 * @param info Pointer to sata_controller_info_t structure to fill
 * @return hal_status_t Status of operation
 */
hal_status_t sata_get_controller_info(storage_device_t *dev, sata_controller_info_t *info);

// ============================================================================
// Storage Helper Functions
// ============================================================================

/**
 * @brief Check if a storage device is bootable
 * @param dev Pointer to storage device
 * @return bool true if bootable
 */
bool storage_is_bootable(storage_device_t *dev);

/**
 * @brief Check if a storage device supports TRIM
 * @param dev Pointer to storage device
 * @return bool true if TRIM is supported
 */
bool storage_supports_trim(storage_device_t *dev);

/**
 * @brief Check if a storage device supports NCQ
 * @param dev Pointer to storage device
 * @return bool true if NCQ is supported
 */
bool storage_supports_ncq(storage_device_t *dev);

/**
 * @brief Check if a storage device is removable
 * @param dev Pointer to storage device
 * @return bool true if removable
 */
bool storage_is_removable(storage_device_t *dev);

/**
 * @brief Get storage device type string
 * @param type Storage type
 * @return const char* Type string
 */
const char* storage_type_to_string(storage_type_t type);

/**
 * @brief Get storage interface type string
 * @param interface Storage interface
 * @return const char* Interface string
 */
const char* storage_interface_to_string(storage_interface_t interface);

/**
 * @brief Get storage protocol type string
 * @param protocol Storage protocol
 * @return const char* Protocol string
 */
const char* storage_protocol_to_string(storage_protocol_t protocol);

#endif // SHADOWBOX_HAL_STORAGE_H
