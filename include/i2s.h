#ifndef SHADOWBOX_I2S_H
#define SHADOWBOX_I2S_H

#include "types.h"
#include "audio.h"
#include "pci.h"

#define I2S_CLK_128FS        0
#define I2S_CLK_192FS        1
#define I2S_CLK_256FS        2
#define I2S_CLK_384FS        3
#define I2S_CLK_512FS        4

#define I2S_FMT_I2S           0
#define I2S_FMT_LEFT_J        1
#define I2S_FMT_RIGHT_J        2
#define I2S_FMT_DSP_A         3
#define I2S_FMT_DSP_B         4

#define I2S_BCLK_INV          (1 << 0)
#define I2S_WS_INV            (1 << 1)
#define I2S_MCLK_OUT          (1 << 2)

#define I2S_SLOT_WIDTH_16     16
#define I2S_SLOT_WIDTH_24     24
#define I2S_SLOT_WIDTH_32     32

#define I2S_MAX_CHANNELS      8

typedef struct i2s_config {
    uint32_t sample_rate;
    uint8_t  channels;
    uint8_t  bits_per_sample;
    uint8_t  slot_width;
    uint8_t  format;
    uint8_t  clock_src;
    uint32_t bclk_per_hr;   /* Bit clock dividers */
    uint32_t mclk_ratio;
    uint8_t  bclk_invert;
    uint8_t  ws_invert;
    uint8_t  mclk_out;
} i2s_config_t;

typedef struct i2s_device {
    pci_device_t *pci_dev;
    void *mmio_base;
    uint64_t mmio_phys;

    i2s_config_t config;
    uint8_t  initialized;

    /* DMA buffers for playback/capture */
    struct {
        void *buffer_virt;
        uint64_t buffer_phys;
        uint32_t buffer_size;
        uint32_t period_size;
        uint8_t  ready;
    } dma[2]; /* [0] = playback, [1] = capture */

    /* Register map (Intel HDA-style I2S) */
    struct {
        uint32_t ctl;
        uint32_t sts;
        uint32_t rate;
        uint32_t clk_ctl;
        uint32_t tx_ctl;
        uint32_t tx_sts;
        uint32_t rx_ctl;
        uint32_t rx_sts;
    } regs;

    audio_device_t *audio_dev;
} i2s_device_t;

/* I2S Register Offsets (typical Intel Baytrail/CHT I2S) */
#define I2S_REG_CTL         0x00
#define I2S_REG_STS         0x04
#define I2S_REG_RATE        0x08
#define I2S_REG_CLK_CTL     0x0C
#define I2S_REG_TX_CTL      0x10
#define I2S_REG_TX_STS      0x14
#define I2S_REG_RX_CTL      0x20
#define I2S_REG_RX_STS      0x24

/* I2S Control Bits */
#define I2S_CTL_SW_RST        (1 << 0)
#define I2S_CTL_DEV_RST       (1 << 1)
#define I2S_CTL_BCLK_EN       (1 << 4)
#define I2S_CTL_WS_EN         (1 << 5)
#define I2S_CTL_FSYNC_EN      (1 << 6)
#define I2S_CTL_MCLK_EN       (1 << 7)
#define I2S_CTL_SRMODE          (1 << 8)

/* I2S TX/RX Control */
#define I2S_TX_EN             (1 << 0)
#define I2S_TX_FIFO_THRESHOLD(n) (((n) & 0x3F) << 1)
#define I2S_RX_EN             (1 << 0)
#define I2S_RX_FIFO_THRESHOLD(n) (((n) & 0x3F) << 1)

/* I2S Format */
#define I2S_FORMAT_STANDARD   0x00
#define I2S_FORMAT_LEFT_J     0x01
#define I2S_FORMAT_RIGHT_J    0x02
#define I2S_FORMAT_PCM_A      0x03
#define I2S_FORMAT_PCM_B      0x04

void i2s_init(pci_device_t *pci_dev);
int i2s_configure(i2s_device_t *dev, i2s_config_t *config);
int i2s_start_tx(i2s_device_t *dev, void *buffer, uint32_t size);
int i2s_start_rx(i2s_device_t *dev, void *buffer, uint32_t size);
void i2s_irq_handler(void);
void i2s_set_volume(uint8_t volume, uint8_t muted);
uint32_t i2s_get_formats(void);

#endif
