#ifndef SHADOWBOX_BLUETOOTH_H
#define SHADOWBOX_BLUETOOTH_H

#include "types.h"
#include "net.h"
#include "pci.h"

#define BT_CLASS_MAJOR_NONE     0x00
#define BT_CLASS_MAJOR_BREDR    0x01
#define BT_CLASS_MAJOR_BLE      0x02
#define BT_CLASS_MAJOR_BREDR_BLE 0x03

#define BT_HCI_VENDOR_ATHEROS   0x0CF3
#define BT_HCI_VENDOR_BROADCOM  0x0A5C
#define BT_HCI_VENDOR_INTEL     0x8087
#define BT_HCI_VENDOR_QUALCOMM  0x05C1
#define BT_HCI_VENDOR_REALTEK   0x0BDA

/* Classic Bluetooth HCI command opcodes */
#define BT_HCI_OP_NOP              0x0000
#define BT_HCI_OP_SET_EVENT_MASK   0x0C01
#define BT_HCI_OP_RESET            0x0C03
#define BT_HCI_OP_SET_REVISION     0x0C15
#define BT_HCI_OP_SET_CONN_CTO     0x0C16
#define BT_HCI_OP_SET_ACL_CONN_REQ 0x0C18
#define BT_HCI_OP_SET_PKT_TYPE     0x0C15
#define BT_HCI_OP_SET_SCAN         0x0C1A
#define BT_HCI_OP_SET_CLASS_OF_DEV 0x0C23
#define BT_HCI_OP_SET_EVT_FILTER   0x0C05
#define BT_HCI_OP_SET_EVT_MASK     0x0C01
#define BT_HCI_OP_ENABLE_TEST_MODE 0x0C02

/* LE HCI command opcodes */
#define BT_HCI_OP_LE_SET_EVENT_MASK  0x2001
#define BT_HCI_OP_LE_SET_SCAN_PARAM  0x200B
#define BT_HCI_OP_LE_SET_SCAN_ENABLE 0x200C
#define BT_HCI_OP_LE_CREATE_CONN     0x200D
#define BT_HCI_OP_LE_CONN_CANCEL     0x200E

/* HCI Packet Types */
#define BT_HCI_PACKET_COMMAND      0x01
#define BT_HCI_PACKET_ACL_DATA     0x02
#define BT_HCI_PACKET_SCO_DATA     0x03
#define BT_HCI_PACKET_EVENT        0x04

/* Connection Types */
#define BT_CONN_TYPE_BREDR         0x01
#define BT_CONN_TYPE_LE            0x02
#define BT_CONN_TYPE_BREDR_LE      0x03

/* Connection States */
#define BT_CONN_STATE_DISCONNECTED 0
#define BT_CONN_STATE_CONNECTING   1
#define BT_CONN_STATE_CONNECTED    2
#define BT_CONN_STATE_DISCONNECTING 3

/* Address Types */
#define BT_ADDR_TYPE_PUBLIC      0x00
#define BT_ADDR_TYPE_RANDOM      0x01
#define BT_ADDR_TYPE_PUBLIC_ID   0x02
#define BT_ADDR_TYPE_RANDOM_ID   0x03

#define BT_BD_ADDR_LEN            6
#define BT_LOCAL_NAME_MAX         248
#define BT_MAX_DEVICES            16

/* Advertising Data Types */
#define BT_ADV_DATA_FLAGS         0x01
#define BT_ADV_DATA_COMPLETE_LOCAL_NAME 0x09
#define BT_ADV_DATA_INCOMPLETE_LOCAL_NAME 0x08
#define BT_ADV_DATA_SERVICE_UUID16 0x02
#define BT_ADV_DATA_SERVICE_UUID32 0x04
#define BT_ADV_DATA_SERVICE_UUID128 0x06

/* Connection Parameters */
typedef struct {
    uint16_t conn_handle;
    uint8_t  bd_addr[BT_BD_ADDR_LEN];
    uint8_t  addr_type;
    uint8_t  link_type;
    uint16_t conn_interval;
    uint16_t conn_latency;
    uint16_t supervision_timeout;
    uint8_t  state;
    uint8_t  security_level;
} bt_conn_t;

typedef struct {
    uint8_t  bd_addr[BT_BD_ADDR_LEN];
    char     local_name[BT_LOCAL_NAME_MAX + 1];
    uint8_t  name_len;
    uint32_t class_of_device;
    int16_t  rssi;
    uint8_t  is_le;
    uint8_t  connectable;
    uint8_t  scannable;
    uint8_t  adv_data_len;
    uint8_t  adv_data[31];
    struct bt_remote_device *next;
} bt_remote_device_t;

typedef struct {
    uint8_t  type;
    uint8_t  length;
    uint8_t  data[30];
} bt_advertising_data_t;

typedef struct {
    uint8_t  address[BT_BD_ADDR_LEN];
    uint8_t  address_type;
    int16_t  rssi;
    uint8_t  adv_length;
    uint8_t  adv_data[31];
} bt_scan_result_t;

typedef struct bt_hci_dev {
    net_device_t base;
    pci_device_t *pci_dev;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t  hci_rev;
    uint8_t  lmp_rev;
    uint8_t  sub_class;
    uint8_t  class_major;

    void *mmio_base;
    uint64_t mmio_phys;
    void *dma_buffer;
    uint64_t dma_phys;
    uint32_t dma_size;

    uint8_t  bd_addr[BT_BD_ADDR_LEN];
    uint8_t  initialized;
    uint32_t flags;

    bt_remote_device_t *devices;
    bt_conn_t connections[BT_MAX_DEVICES];
    uint8_t conn_count;

    uint8_t tx_power;
    uint8_t le_tx_power;
    uint8_t min_conn_interval;
    uint8_t max_conn_interval;
    uint8_t latency;
    uint16_t supervision_timeout;

    void *driver_data;
    struct bt_hci_dev *next;
} bt_hci_dev_t;

typedef struct bt_driver_ops {
    int (*init)(pci_device_t *pci_dev);
    void (*send_command)(bt_hci_dev_t *dev, uint16_t opcode, void *data, uint8_t len);
    void (*send_acl)(bt_hci_dev_t *dev, uint16_t handle, void *data, uint16_t len);
    void (*send_sco)(bt_hci_dev_t *dev, uint16_t handle, void *data, uint8_t len);
    int (*start_le_scan)(bt_hci_dev_t *dev, uint8_t active);
    int (*connect_le)(bt_hci_dev_t *dev, uint8_t *addr, uint8_t addr_type);
    int (*disconnect)(bt_hci_dev_t *dev, uint16_t conn_handle, uint8_t reason);
    int (*set_scan_params)(bt_hci_dev_t *dev, uint8_t scan_type, uint16_t interval, uint16_t window);
    int (*set_adv_params)(bt_hci_dev_t *dev, uint8_t adv_type, uint16_t interval,
                          uint8_t *addr, uint8_t addr_type);
    int (*set_adv_data)(bt_hci_dev_t *dev, uint8_t *data, uint8_t len);
    int (*set_le_tx_power)(bt_hci_dev_t *dev, uint8_t power);
    void (*irq_handler)(void);
    void (*led_on)(bt_hci_dev_t *dev);
    void (*led_off)(bt_hci_dev_t *dev);
} bt_driver_ops_t;

void bt_hci_init(pci_device_t *pci_dev);
void bt_hci_send_command(bt_hci_dev_t *dev, uint16_t opcode, void *data, uint8_t len);
int bt_start_le_scan(bt_hci_dev_t *dev, uint8_t active);
int bt_connect_le(bt_hci_dev_t *dev, uint8_t *addr, uint8_t addr_type);
int bt_disconnect(bt_hci_dev_t *dev, uint16_t conn_handle, uint8_t reason);
int bt_set_le_tx_power(bt_hci_dev_t *dev, uint8_t power);
void bt_led_on(bt_hci_dev_t *dev);
void bt_led_off(bt_hci_dev_t *dev);
void bt_irq_handler(void);
bt_hci_dev_t *bt_get_device(void);
int bt_send_packet(net_device_t *dev, void *data, uint32_t len);
void bt_handle_packet(bt_hci_dev_t *dev, uint8_t *packet, uint32_t len);

#endif
