#ifndef SHADOWBOX_WIFI_H
#define SHADOWBOX_WIFI_H

#include "types.h"
#include "net.h"
#include "pci.h"

#define WIFI_VENDOR_ATHEROS     0x168C
#define WIFI_VENDOR_INTEL       0x8086
#define WIFI_VENDOR_BROADCOM    0x14E4
#define WIFI_VENDOR_REALTEK     0x10EC
#define WIFI_VENDOR_QUALCOMM_ATHEROS 0x168C

#define WIFI_DEVICE_AR5008      0x0024
#define WIFI_DEVICE_AR9280      0x002B
#define WIFI_DEVICE_AR93xx      0x0030
#define WIFI_DEVICE_AR9485      0x0030
#define WIFI_DEVICE_IWL_6205    0x0280
#define WIFI_DEVICE_IWL_6235    0x0886
#define WIFI_DEVICE_IWL_7260    0x08B1
#define WIFI_DEVICE_IWL_7265    0x095A
#define WIFI_DEVICE_IWL_8265    0x24FD
#define WIFI_DEVICE_BCM4313     0x4313
#define WIFI_DEVICE_BCM43224    0x0576
#define WIFI_DEVICE_RTL8188     0x8179
#define WIFI_DEVICE_RTL8723BE   0x8723
#define WIFI_DEVICE_RTL8821AE    0x8179

#define WIFI_BAND_2GHZ          0
#define WIFI_BAND_5GHZ          1

#define WIFI_PROTO_B     0x01
#define WIFI_PROTO_G     0x02
#define WIFI_PROTO_N     0x04
#define WIFI_PROTO_AC    0x08
#define WIFI_PROTO_AX    0x10

#define WIFI_CAP_24GHZ   0x01
#define WIFI_CAP_5GHZ    0x02
#define WIFI_CAP_ANT_DIV 0x04
#define WIFI_CAP_HT      0x08
#define WIFI_CAP_VHT     0x10
#define WIFI_CAP_HE      0x20

#define WIFI_MAX_SSID     32
#define WIFI_MAX_KEY      64
#define WIFI_BSSID_LEN    6
#define WIFI_MAX_RATES    32

typedef enum {
    WIFI_STATE_UNINITIALIZED,
    WIFI_STATE_SCANNING,
    WIFI_STATE_ASSOCIATING,
    WIFI_STATE_ASSOCIATED,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_DISCONNECTED
} wifi_state_t;

typedef enum {
    WIFI_ENC_OPEN,
    WIFI_ENC_WEP,
    WIFI_ENC_WPA,
    WIFI_ENC_WPA2,
    WIFI_ENC_WPA3
} wifi_encryption_t;

typedef enum {
    WIFI_PHY_CCK,
    WIFI_PHY_OFDM,
    WIFI_PHY_HT,
    WIFI_PHY_VHT,
    WIFI_PHY_HE
} wifi_phy_t;

typedef struct wifi_rate {
    uint32_t rate_mbps;
    wifi_phy_t phy;
    uint8_t  mcs_index;
} wifi_rate_t;

typedef struct wifi_channel {
    uint8_t  channel;
    uint32_t freq_mhz;
    uint8_t  band;
} wifi_channel_t;

typedef struct wifi_bss {
    uint8_t  bssid[WIFI_BSSID_LEN];
    char     ssid[WIFI_MAX_SSID + 1];
    uint32_t ssid_len;
    wifi_channel_t channel;
    int16_t  signal_dbm;
    uint16_t caps;
    wifi_encryption_t encryption;
    wifi_rate_t rates[WIFI_MAX_RATES];
    uint8_t  rate_count;
    struct wifi_bss *next;
} wifi_bss_t;

typedef struct wifi_station {
    uint8_t  mac[WIFI_BSSID_LEN];
    uint16_t aid;
    int16_t  signal_dbm;
    uint32_t tx_bytes;
    uint32_t rx_bytes;
    uint32_t tx_packets;
    uint32_t rx_packets;
    struct wifi_station *next;
} wifi_station_t;

typedef struct wifi_device {
    net_device_t base;
    pci_device_t *pci_dev;
    wifi_state_t state;
    wifi_bss_t *bss_list;
    wifi_bss_t *current_bss;
    wifi_station_t *stations;
    wifi_station_t *associated_sta;

    uint8_t mac_addr[WIFI_BSSID_LEN];
    uint16_t device_id;
    uint16_t vendor_id;
    wifi_phy_t phy_type;
    uint32_t capabilities;
    wifi_channel_t current_channel;
    uint32_t channel_count;

    uint8_t tx_power_dbm;
    uint8_t antenna_diversity;
    uint8_t initialized;

    void *mmio_base;
    uint64_t mmio_phys;
    void *eeprom_data;
    uint16_t eeprom_size;

    void *driver_data;
} wifi_device_t;

typedef struct wifi_driver_ops {
    int (*init)(pci_device_t *pci_dev);
    int (*probe)(wifi_device_t *dev);
    void (*remove)(wifi_device_t *dev);
    int (*scan)(wifi_device_t *dev);
    int (*connect)(wifi_device_t *dev, const char *ssid, const uint8_t *bssid,
                   wifi_encryption_t enc, const char *passphrase);
    int (*disconnect)(wifi_device_t *dev);
    int (*send_frame)(wifi_device_t *dev, void *data, uint32_t len);
    void (*irq_handler)(void);
    int (*set_channel)(wifi_device_t *dev, uint8_t channel);
    int (*set_tx_power)(wifi_device_t *dev, uint8_t power_dbm);
    void (*led_on)(wifi_device_t *dev);
    void (*led_off)(wifi_device_t *dev);
} wifi_driver_ops_t;

void wifi_hw_init(pci_device_t *pci_dev);
int wifi_scan(wifi_device_t *dev, wifi_bss_t **out_list);
int wifi_connect(wifi_device_t *dev, const char *ssid, wifi_encryption_t enc, const char *passphrase);
int wifi_disconnect(wifi_device_t *dev);
int wifi_set_channel(wifi_device_t *dev, uint8_t channel);
int wifi_set_tx_power(wifi_device_t *dev, uint8_t power_dbm);
void wifi_led_on(wifi_device_t *dev);
void wifi_led_off(wifi_device_t *dev);
void wifi_irq_handler(void);
wifi_device_t *wifi_get_device(void);
int wifi_hw_send_packet(net_device_t *dev, void *data, uint32_t len);

#endif
