#include "wifi.h"
#include "desktop.h"
#include "kernel.h"
#include "pci.h"
#include "vmm.h"
#include "pmm.h"
#include "io.h"
#include "malloc.h"
#include "kstring.h"

static wifi_device_t wifi_dev;
static wifi_driver_ops_t wifi_ops;

/* Push a Wi-Fi state change into the kernel notification daemon. */
static void wifi_notify(int connected, const char *ssid) {
    notification_t n;
    memset(&n, 0, sizeof(n));
    const char *app = "Wi-Fi";
    const char *sum = connected ? "Connected" : "Disconnected";
    for (int i = 0; app[i] && i < 63; i++) n.app_name[i] = app[i];
    for (int i = 0; sum[i] && i < 127; i++) n.summary[i] = sum[i];
    if (ssid) {
        int j = 0;
        if (connected) {
            const char *pre = "Connected to ";
            for (; pre[j] && j < 63; j++) n.body[j] = pre[j];
        } else {
            const char *pre = "Disconnected from ";
            for (; pre[j] && j < 63; j++) n.body[j] = pre[j];
        }
        for (int k = 0; ssid[k] && j < 511; k++) n.body[j++] = ssid[k];
    } else if (!connected) {
        const char *m = "Network link lost";
        for (int k = 0; m[k] && k < 511; k++) n.body[k] = m[k];
    }
    n.priority = connected ? NOTIFY_PRIORITY_NORMAL : NOTIFY_PRIORITY_HIGH;
    notification_send(&n);
}

static void *wifi_eeprom_read(uint16_t offset) {
    /* In a real implementation, this would read the EEPROM via the PCI
       device's EEPROM access register. For stub purposes, return NULL. */
    (void)offset;
    return NULL;
}

static uint16_t wifi_read_eeprom_word(uint16_t offset) {
    if (!wifi_dev.eeprom_data || offset >= wifi_dev.eeprom_size) return 0;
    return ((uint16_t *)wifi_dev.eeprom_data)[offset / 2];
}

static void wifi_write_reg(uint16_t offset, uint32_t value) {
    if (wifi_dev.mmio_base) {
        *(volatile uint32_t *)((uintptr_t)wifi_dev.mmio_base + offset) = value;
    }
}

static uint32_t wifi_read_reg(uint16_t offset) {
    if (wifi_dev.mmio_base) {
        return *(volatile uint32_t *)((uintptr_t)wifi_dev.mmio_base + offset);
    }
    return 0;
}

static int wifi_eeprom_init(void) {
    /* Allocate and read EEPROM content */
    uint16_t eeprom_size = 512;
    wifi_dev.eeprom_size = eeprom_size;
    wifi_dev.eeprom_data = kmalloc(eeprom_size);
    if (!wifi_dev.eeprom_data) {
        printk(KERN_ERR "WIFI: Failed to allocate EEPROM buffer\n");
        return -1;
    }

    memset(wifi_dev.eeprom_data, 0, eeprom_size);

    /* Simulate reading EEPROM - in real hardware, we'd read via the EEPROM register */
    uint16_t *eeprom = (uint16_t *)wifi_dev.eeprom_data;

    /* Set up default MAC address */
    if (wifi_dev.vendor_id == WIFI_VENDOR_ATHEROS) {
        /* Atheros: MAC address is stored at offsets 0x00-0x05 in EEPROM */
        eeprom[0x1A / 2] = 0x00;  /* Placeholder */
    }

    /* If MAC wasn't read from EEPROM, generate a default one */
    if (wifi_dev.mac_addr[0] == 0) {
        wifi_dev.mac_addr[0] = 0x52;
        wifi_dev.mac_addr[1] = 0x54;
        wifi_dev.mac_addr[2] = 0x00;
        wifi_dev.mac_addr[3] = 0x12;
        wifi_dev.mac_addr[4] = (wifi_dev.device_id >> 8) & 0xFF;
        wifi_dev.mac_addr[5] = wifi_dev.device_id & 0xFF;
    }

    return 0;
}

static void wifi_init_net_device(void) {
    memset(&wifi_dev.base, 0, sizeof(net_device_t));
    wifi_dev.base.name = "wlan0";
    memcpy(wifi_dev.base.mac, wifi_dev.mac_addr, 6);
    wifi_dev.base.ip = 0;
    wifi_dev.base.netmask = 0;
    wifi_dev.base.gateway = 0;
    wifi_dev.base.send_packet = wifi_hw_send_packet;
    wifi_dev.base.next = NULL;
}

int wifi_hw_send_packet(net_device_t *dev, void *data, uint32_t len) {
    (void)dev;
    (void)data;
    (void)len;
    /* In a real driver, this would queue the frame to the TX ring */
    return len;
}

static int wifi_hardware_init(pci_device_t *pci_dev) {
    wifi_dev.vendor_id = pci_dev->vendor_id;
    wifi_dev.device_id = pci_dev->device_id;

    /* Read EEPROM for MAC and calibration data */
    wifi_eeprom_init();

    /* Enable bus mastering */
    pci_enable_bus_mastering(pci_dev);

    /* Map BAR0 (memory-mapped registers) */
    uint32_t bar0_low = pci_config_read(pci_dev->bus, pci_dev->device,
                                         pci_dev->function, 0x10);
    uint64_t bar0 = bar0_low & 0xFFFFFFF0;
    if ((bar0_low & 0x06) == 0x04) {
        uint32_t bar0_high = pci_config_read(pci_dev->bus, pci_dev->device,
                                              pci_dev->function, 0x14);
        bar0 |= ((uint64_t)bar0_high << 32);
    }

    wifi_dev.mmio_phys = bar0;
    wifi_dev.mmio_base = vmap_phys(bar0, 0x20000);  /* 128KB MMIO window */

    if (!wifi_dev.mmio_base) {
        printk(KERN_ERR "WIFI: Failed to map MMIO at 0x%llx\n", bar0);
        return -1;
    }

    printk(KERN_INFO "WIFI: MMIO mapped at 0x%llx (virt=%p)\n", bar0, wifi_dev.mmio_base);

    /* Reset the device */
    wifi_write_reg(0x0000, 0xFFFFFFFF);  /* Reset all registers */
    for (volatile int i = 0; i < 100000; i++) __builtin_ia32_pause();
    wifi_write_reg(0x0000, 0x00000000);

    /* Determine PHY type based on device ID */
    if (wifi_dev.vendor_id == WIFI_VENDOR_ATHEROS) {
        if (wifi_dev.device_id == WIFI_DEVICE_AR5008) {
            wifi_dev.phy_type = WIFI_PHY_HT;
            wifi_dev.capabilities = WIFI_CAP_24GHZ | WIFI_CAP_5GHZ | WIFI_CAP_HT;
        } else if (wifi_dev.device_id == WIFI_DEVICE_AR9280) {
            wifi_dev.phy_type = WIFI_PHY_HT;
            wifi_dev.capabilities = WIFI_CAP_24GHZ | WIFI_CAP_5GHZ | WIFI_CAP_HT | WIFI_CAP_ANT_DIV;
        } else if (wifi_dev.device_id == WIFI_DEVICE_AR93xx) {
            wifi_dev.phy_type = WIFI_PHY_HT;
            wifi_dev.capabilities = WIFI_CAP_24GHZ | WIFI_CAP_5GHZ | WIFI_CAP_HT;
        }
    }

    wifi_dev.state = WIFI_STATE_UNINITIALIZED;
    wifi_dev.tx_power_dbm = 20;

    return 0;
}

static wifi_bss_t *wifi_bss_alloc(void) {
    wifi_bss_t *bss = (wifi_bss_t *)kmalloc(sizeof(wifi_bss_t));
    if (!bss) return NULL;
    memset(bss, 0, sizeof(wifi_bss_t));
    return bss;
}

int wifi_scan(wifi_device_t *dev, wifi_bss_t **out_list) {
    if (!dev || !dev->mmio_base) return -1;

    printk(KERN_INFO "WIFI: Scanning for networks...\n");

    /* Send scan command to hardware */
    /* In a real driver, this would set the device into scan mode on all channels */
    wifi_write_reg(0x8000, 0);  /* Scan command register */

    /* Simulate finding a few access points */
    wifi_bss_t *list = NULL;

    for (int i = 0; i < 3; i++) {
        wifi_bss_t *bss = wifi_bss_alloc();
        if (!bss) continue;

        bss->channel.channel = (uint8_t)(i + 1);
        bss->channel.freq_mhz = 2412 + i * 5;
        bss->channel.band = WIFI_BAND_2GHZ;
        bss->signal_dbm = -50 - i * 10;
        bss->caps = 0x0411; /* ESS + WMM */
        bss->encryption = WIFI_ENC_WPA2;
        bss->rate_count = 4;
        bss->rates[0].rate_mbps = 1; bss->rates[0].phy = WIFI_PHY_CCK;
        bss->rates[1].rate_mbps = 6; bss->rates[1].phy = WIFI_PHY_OFDM;
        bss->rates[2].rate_mbps = 12; bss->rates[2].phy = WIFI_PHY_OFDM;
        bss->rates[3].rate_mbps = 54; bss->rates[3].phy = WIFI_PHY_OFDM;

        /* Synthesize SSID */
        char ssid_buf[16];
        memset(ssid_buf, 0, sizeof(ssid_buf));
        strcpy(ssid_buf, "ShadowNet");
        ssid_buf[9] = '0' + i;
        strcpy(bss->ssid, ssid_buf);
        bss->ssid_len = strlen(ssid_buf);

        /* Generate fake BSSID */
        for (int j = 0; j < WIFI_BSSID_LEN; j++)
            bss->bssid[j] = (uint8_t)((i + 1) * j + 0x10);

        bss->next = list;
        list = bss;
        dev->state = WIFI_STATE_SCANNING;
    }

    if (out_list) *out_list = list;
    dev->state = WIFI_STATE_SCANNING;

    printk(KERN_INFO "WIFI: Scan complete (found %d networks)\n", 3);
    return 3;
}

int wifi_connect(wifi_device_t *dev, const char *ssid, wifi_encryption_t enc, const char *passphrase) {
    if (!dev || !ssid) return -1;

    printk(KERN_INFO "WIFI: Connecting to '%s' (encryption=%d)\n", ssid, enc);
    dev->state = WIFI_STATE_ASSOCIATING;

    /* Set the SSID in the device */
    wifi_write_reg(0x8010, (uint32_t)(uintptr_t)ssid);

    /* Configure security based on encryption type */
    uint32_t sec_config = 0;
    switch (enc) {
        case WIFI_ENC_OPEN: sec_config = 0x00; break;
        case WIFI_ENC_WEP:  sec_config = 0x01; break;
        case WIFI_ENC_WPA:  sec_config = 0x02; break;
        case WIFI_ENC_WPA2: sec_config = 0x03; break;
        case WIFI_ENC_WPA3: sec_config = 0x04; break;
    }

    wifi_write_reg(0x8020, sec_config);
    wifi_write_reg(0x8030, (uint32_t)(uintptr_t)passphrase);

    /* Trigger association */
    wifi_write_reg(0x8040, 1);

    /* Wait for association (in real hardware, this would be interrupt-driven) */
    for (volatile int i = 0; i < 1000000; i++) {
        uint32_t status = wifi_read_reg(0x8050);
        if (status & 0x01) break;
        __builtin_ia32_pause();
    }

    uint32_t status = wifi_read_reg(0x8050);
    if (status & 0x01) {
        dev->state = WIFI_STATE_CONNECTED;
        printk(KERN_INFO "WIFI: Connected! (status=0x%x)\n", status);
        if (dev->current_bss)
            wifi_notify(1, dev->current_bss->ssid);
        else
            wifi_notify(1, ssid);
        return 0;
    }

    dev->state = WIFI_STATE_DISCONNECTED;
    printk(KERN_ERR "WIFI: Connection failed (status=0x%x)\n", status);
    wifi_notify(0, ssid);
    return -1;
}

int wifi_disconnect(wifi_device_t *dev) {
    if (!dev) return -1;

    printk(KERN_INFO "WIFI: Disconnecting...\n");
    wifi_write_reg(0x8040, 2);  /* Disconnect command */
    dev->state = WIFI_STATE_DISCONNECTED;
    if (dev->current_bss)
        wifi_notify(0, dev->current_bss->ssid);
    dev->current_bss = NULL;
    return 0;
}

int wifi_set_channel(wifi_device_t *dev, uint8_t channel) {
    if (!dev) return -1;

    dev->current_channel.channel = channel;
    if (channel >= 1 && channel <= 13) {
        dev->current_channel.band = WIFI_BAND_2GHZ;
        dev->current_channel.freq_mhz = 2412 + (channel - 1) * 5;
    } else if (channel >= 36 && channel <= 165) {
        dev->current_channel.band = WIFI_BAND_5GHZ;
        dev->current_channel.freq_mhz = 5180 + (channel - 36) * 5;
    }

    /* Set channel in hardware */
    wifi_write_reg(0x8060, channel);
    wifi_write_reg(0x8060, wifi_read_reg(0x8060) | (1 << 8)); /* Channel load */

    return 0;
}

int wifi_set_tx_power(wifi_device_t *dev, uint8_t power_dbm) {
    if (!dev) return -1;
    dev->tx_power_dbm = power_dbm;
    wifi_write_reg(0x8070, power_dbm);
    return 0;
}

void wifi_led_on(wifi_device_t *dev) {
    if (!dev) return;
    wifi_write_reg(0x8080, 1);
}

void wifi_led_off(wifi_device_t *dev) {
    if (!dev) return;
    wifi_write_reg(0x8080, 0);
}

void wifi_irq_handler(void) {
    if (!wifi_dev.initialized) return;

    uint32_t status = wifi_read_reg(0x8004);  /* Interrupt status register */

    if (status & 0x01) {
        /* RX complete */
        wifi_write_reg(0x8004, 0x01);
        printk(KERN_DEBUG "WIFI: RX complete interrupt\n");
    }
    if (status & 0x02) {
        /* TX complete */
        wifi_write_reg(0x8004, 0x02);
        printk(KERN_DEBUG "WIFI: TX complete interrupt\n");
    }
    if (status & 0x04) {
        /* Scan complete */
        wifi_write_reg(0x8004, 0x04);
        printk(KERN_INFO "WIFI: Scan complete\n");
    }
    if (status & 0x08) {
        /* Association complete */
        wifi_write_reg(0x8004, 0x08);
        wifi_dev.state = WIFI_STATE_CONNECTED;
        printk(KERN_INFO "WIFI: Association complete\n");
    }
    if (status & 0x40) {
        /* Disconnect */
        wifi_write_reg(0x8004, 0x40);
        wifi_dev.state = WIFI_STATE_DISCONNECTED;
        printk(KERN_INFO "WIFI: Disconnected (deauth)\n");
    }
}

wifi_device_t *wifi_get_device(void) {
    return &wifi_dev;
}

void wifi_hw_init(pci_device_t *pci_dev) {
    memset(&wifi_dev, 0, sizeof(wifi_dev));
    wifi_dev.pci_dev = pci_dev;

    if (wifi_hardware_init(pci_dev) != 0) {
        printk(KERN_ERR "WIFI: Hardware initialization failed\n");
        return;
    }

    /* Set default channel */
    wifi_dev.channel_count = 14;
    wifi_set_channel(&wifi_dev, 6);

    /* Initialize net device */
    wifi_init_net_device();

    /* Register with network stack */
    net_register_device(&wifi_dev.base);

    /* Set up driver ops */
    wifi_ops.init = wifi_hw_init;
    wifi_ops.probe = NULL;
    wifi_ops.remove = NULL;
    wifi_ops.scan = wifi_scan;
    wifi_ops.connect = wifi_connect;
    wifi_ops.disconnect = wifi_disconnect;
    wifi_ops.send_frame = NULL;
    wifi_ops.irq_handler = wifi_irq_handler;
    wifi_ops.set_channel = wifi_set_channel;
    wifi_ops.set_tx_power = wifi_set_tx_power;
    wifi_ops.led_on = wifi_led_on;
    wifi_ops.led_off = wifi_led_off;

    wifi_dev.driver_data = &wifi_ops;
    wifi_dev.initialized = 1;
    printk(KERN_INFO "WIFI: Initialized (vendor=0x%04x dev=0x%04x mac=%02x:%02x:%02x:%02x:%02x:%02x)\n",
           pci_dev->vendor_id, pci_dev->device_id,
           wifi_dev.mac_addr[0], wifi_dev.mac_addr[1], wifi_dev.mac_addr[2],
           wifi_dev.mac_addr[3], wifi_dev.mac_addr[4], wifi_dev.mac_addr[5]);
}
