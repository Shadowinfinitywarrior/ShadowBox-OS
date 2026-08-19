#include "i2s.h"
#include "kernel.h"
#include "vmm.h"
#include "pmm.h"
#include "io.h"
#include "malloc.h"
#include "kstring.h"
#include "pci.h"

static i2s_device_t i2s_dev;

static uint32_t i2s_read_reg(uint32_t offset) {
    return *(volatile uint32_t *)((uintptr_t)i2s_dev.mmio_base + offset);
}

static void i2s_write_reg(uint32_t offset, uint32_t value) {
    *(volatile uint32_t *)((uintptr_t)i2s_dev.mmio_base + offset) = value;
}

int i2s_configure(i2s_device_t *dev, i2s_config_t *config) {
    if (!dev || !config || !dev->initialized) return -1;

    if (config->sample_rate > 192000 || config->sample_rate < 8000) {
        printk(KERN_WARN "I2S: Invalid sample rate %u\n", config->sample_rate);
        return -1;
    }

    if (config->channels > I2S_MAX_CHANNELS) {
        printk(KERN_WARN "I2S: Too many channels %u (max %d)\n",
               config->channels, I2S_MAX_CHANNELS);
        return -1;
    }

    memcpy(&dev->config, config, sizeof(i2s_config_t));

    /* Calculate BCLK divider: BCLK = sample_rate * slot_width * channels */
    /* Assuming 24MHz input clock */
    uint32_t bclk = config->sample_rate * config->slot_width * config->channels;
    dev->config.bclk_per_hr = 24000000 / bclk;

    /* Set sample rate register */
    i2s_write_reg(I2S_REG_RATE, config->sample_rate);

    /* Configure format based on I2S format type */
    uint32_t fmt_reg = 0;
    switch (config->format) {
        case I2S_FORMAT_STANDARD:  /* I2S */
            fmt_reg |= 0x00;
            break;
        case I2S_FORMAT_LEFT_J:
            fmt_reg |= 0x01;
            break;
        case I2S_FORMAT_RIGHT_J:
            fmt_reg |= 0x02;
            break;
        default:
            fmt_reg |= 0x00;
            break;
    }

    /* Configure slot width */
    fmt_reg |= (config->slot_width == 32) ? 0x02 : 0x00;

    /* Clock inversion settings */
    if (config->bclk_invert) fmt_reg |= I2S_BCLK_INV;
    if (config->ws_invert) fmt_reg |= I2S_WS_INV;

    /* Calculate MCLK ratio */
    dev->config.mclk_ratio = 256;

    /* Configure TX path */
    uint32_t tx_ctl = I2S_TX_EN | I2S_TX_FIFO_THRESHOLD(16);
    if (config->bclk_invert) tx_ctl |= I2S_BCLK_INV;
    if (config->ws_invert) tx_ctl |= I2S_WS_INV;
    i2s_write_reg(I2S_REG_TX_CTL, tx_ctl);
    i2s_write_reg(I2S_REG_TX_STS, 0xFFFFFFFF);

    /* Configure RX path */
    uint32_t rx_ctl = I2S_RX_EN | I2S_RX_FIFO_THRESHOLD(16);
    if (config->bclk_invert) rx_ctl |= I2S_BCLK_INV;
    if (config->ws_invert) rx_ctl |= I2S_WS_INV;
    i2s_write_reg(I2S_REG_RX_CTL, rx_ctl);
    i2s_write_reg(I2S_REG_RX_STS, 0xFFFFFFFF);

    /* Enable clocks: clear reset, enable bit clock, frame sync */
    uint32_t ctl = I2S_CTL_DEV_RST | I2S_CTL_BCLK_EN | I2S_CTL_WS_EN | I2S_CTL_FSYNC_EN;
    if (config->mclk_out) ctl |= I2S_CTL_MCLK_EN;
    i2s_write_reg(I2S_REG_CTL, ctl);

    dev->regs.ctl = ctl;
    dev->regs.rate = config->sample_rate;
    dev->regs.clk_ctl = fmt_reg;
    dev->regs.tx_ctl = tx_ctl;
    dev->regs.rx_ctl = rx_ctl;

    printk(KERN_INFO "I2S: Configured rate=%u Hz ch=%u bits=%u slot_w=%u fmt=%u bclk_div=%u\n",
           config->sample_rate, config->channels,
           config->bits_per_sample, config->slot_width,
           config->format, dev->config.bclk_per_hr);
    return 0;
}

static int i2s_alloc_dma_buffer(uint8_t dir, uint32_t size) {
    uint64_t phys = (uint64_t)pmm_alloc_page();
    if (!phys) return -1;

    void *virt = vmap_phys(phys, 4096);
    if (!virt) {
        pmm_free_page((void *)phys);
        return -1;
    }

    memset(virt, 0, size > 4096 ? 4096 : size);

    i2s_dev.dma[dir].buffer_virt = virt;
    i2s_dev.dma[dir].buffer_phys = phys;
    i2s_dev.dma[dir].buffer_size = size;
    i2s_dev.dma[dir].ready = 0;

    return 0;
}

int i2s_start_tx(i2s_device_t *dev, void *buffer, uint32_t size) {
    if (!dev || !dev->initialized || !buffer || !size) return -1;

    uint32_t words = size / 4;
    uint32_t *data = (uint32_t *)buffer;

    for (uint32_t i = 0; i < words; i++) {
        while (i2s_read_reg(I2S_REG_TX_STS) & 0x3F) {
            __builtin_ia32_pause();
        }
        i2s_write_reg(I2S_REG_TX_STS, data[i]);
    }

    dev->dma[0].ready = 1;
    return (int)size;
}

int i2s_start_rx(i2s_device_t *dev, void *buffer, uint32_t size) {
    if (!dev || !dev->initialized || !buffer || !size) return -1;

    uint32_t words = size / 4;
    uint32_t *data = (uint32_t *)buffer;

    for (uint32_t i = 0; i < words; i++) {
        while (!(i2s_read_reg(I2S_REG_RX_STS) & 0x3F)) {
            __builtin_ia32_pause();
        }
        data[i] = i2s_read_reg(I2S_REG_RX_STS);
    }

    dev->dma[1].ready = 1;
    return (int)size;
}

void i2s_set_volume(uint8_t volume, uint8_t muted) {
    if (volume > 100) volume = 100;

    if (i2s_dev.audio_dev && i2s_dev.audio_dev->mixer) {
        i2s_dev.audio_dev->mixer->master_volume = volume;
        i2s_dev.audio_dev->mixer->muted = muted ? 1 : 0;
    }

    /* In hardware, volume would be set via I2S amp control or codec registers */
    printk(KERN_INFO "I2S: Volume=%u%% muted=%u\n", volume, muted);
}

uint32_t i2s_get_formats(void) {
    return I2S_FORMAT_STANDARD | I2S_FORMAT_LEFT_J | I2S_FORMAT_RIGHT_J;
}

void i2s_irq_handler(void) {
    if (!i2s_dev.initialized) return;

    uint32_t status = i2s_read_reg(I2S_REG_STS);

    if (status & 0x01) {
        i2s_write_reg(I2S_REG_TX_STS, 0x01);
        i2s_dev.dma[0].ready = 0;
    }
    if (status & 0x02) {
        i2s_write_reg(I2S_REG_RX_STS, 0x02);
        i2s_dev.dma[1].ready = 1;
    }
}

void i2s_init(pci_device_t *pci_dev) {
    if (!pci_dev) {
        printk(KERN_ERR "I2S: No PCI device\n");
        return;
    }

    memset(&i2s_dev, 0, sizeof(i2s_dev));
    i2s_dev.pci_dev = pci_dev;

    printk(KERN_INFO "I2S: Found I2S controller at PCI %02x:%02x (vendor=0x%04x dev=0x%04x)\n",
           pci_dev->bus, pci_dev->device, pci_dev->vendor_id, pci_dev->device_id);

    pci_enable_bus_mastering(pci_dev);

    /* Read BAR0 for MMIO registers */
    uint32_t bar0_low = pci_config_read(pci_dev->bus, pci_dev->device,
                                         pci_dev->function, 0x10);
    uint64_t bar0 = bar0_low & 0xFFFFFFF0;
    if ((bar0_low & 0x06) == 0x04) {
        uint32_t bar0_high = pci_config_read(pci_dev->bus, pci_dev->device,
                                              pci_dev->function, 0x14);
        bar0 |= ((uint64_t)bar0_high << 32);
    }

    i2s_dev.mmio_phys = bar0;
    i2s_dev.mmio_base = vmap_phys(bar0, 4096);

    if (!i2s_dev.mmio_base) {
        printk(KERN_ERR "I2S: Failed to map MMIO at 0x%llx\n", bar0);
        return;
    }

    /* Allocate DMA buffers */
    i2s_alloc_dma_buffer(0, 0x1000); /* playback */
    i2s_alloc_dma_buffer(1, 0x1000); /* capture */

    /* Default configuration */
    i2s_config_t config = {
        .sample_rate = 48000,
        .channels = 2,
        .bits_per_sample = 16,
        .slot_width = I2S_SLOT_WIDTH_32,
        .format = I2S_FORMAT_STANDARD,
        .clock_src = I2S_CLK_256FS,
        .bclk_per_hr = 0,
        .mclk_ratio = 0,
        .bclk_invert = 0,
        .ws_invert = 0,
        .mclk_out = 1,
    };

    i2s_configure(&i2s_dev, &config);

    /* Register with audio subsystem */
    audio_subsystem_init();

    i2s_dev.audio_dev = (audio_device_t *)kmalloc(sizeof(audio_device_t));
    if (i2s_dev.audio_dev) {
        memset(i2s_dev.audio_dev, 0, sizeof(audio_device_t));
        i2s_dev.audio_dev->name = "i2s";
        i2s_dev.audio_dev->routing_capabilities = 0x03;
        i2s_dev.audio_dev->mixer = (audio_mixer_t *)kmalloc(sizeof(audio_mixer_t));
        if (i2s_dev.audio_dev->mixer) {
            i2s_dev.audio_dev->mixer->name = "i2s-mixer";
            i2s_dev.audio_dev->mixer->master_volume = 75;
            i2s_dev.audio_dev->mixer->muted = 0;
        }
        audio_register_device(i2s_dev.audio_dev);
    }

    i2s_dev.initialized = 1;
    printk(KERN_INFO "I2S: Initialized on PCI %02x:%02x (MMIO=0x%llx)\n",
           pci_dev->bus, pci_dev->device, bar0);
}
