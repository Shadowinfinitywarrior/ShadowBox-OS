#ifndef SHADOWBOX_HDMI_AUDIO_H
#define SHADOWBOX_HDMI_AUDIO_H

#include "types.h"
#include "pci.h"
#include "audio.h"

#define HDMI_AUDIO_MAX_PINS    8
#define HDMI_AUDIO_MAX_EAPDS   4
#define HDMI_AUDIO_MAX_SINK_CNT 3

/* Intel HDA HDMI/DP Pin Widget NIDs (typical) */
#define HDA_HDMI_PIN_NID_BASE 0x10
#define HDA_HDMI_PIN_CONFIG_DEFAULT  0x18  /* Default pin config register offset */

/* HDMI Audio Register Definitions (HDA MMIO) */
#define HDA_HDMI_CTRL_BASE     0x00
#define HDA_HDMI_CTRL_START    0x00
#define HDA_HDMI_GCTL          0x08
#define HDA_HDMI_GSTS          0x0C
#define HDA_HDMI_INTCTL        0x20
#define HDA_HDMI_CODEC_ID      0x24
#define HDA_HDMI_STATESTS      0x28
#define HDA_HDMI_WIDGET_PARAM   0x2C
#define HDA_HDMI_CODEC_CMD     0x40
#define HDA_HDMI_CODEC_STS     0x44
#define HDA_HDMI_PIN_SENSE_CTL 0x48

/* HDA Verb commands for HDMI audio */
#define HDA_VERB_SET_PIN_SENSE  0x707
#define HDA_VERB_GET_PIN_SENSE  0xF07
#define HDA_VERB_SET_PIN_WIDGET 0x701
#define HDA_VERB_SET_PIN_PARAM  0x700
#define HDA_VERB_SET_EAPD_BTLEN 0x781
#define HDA_VERB_SET_AMP_GAIN  0x300
#define HDA_VERB_SET_PIN_WIDGET_CONTROL 0x701

/* HDMI Audio InfoFrame types */
#define HDMI_AUDIO_INFOFRAME_TYPE  0x02
#define HDMI_AUDIO_INFOFRAME_LEN   0x0D

/* CEA Short Audio Descriptor */
typedef struct {
    uint8_t  format_code;
    uint8_t  num_channels;
    uint8_t  sample_rates;
    uint8_t  sample_size;
    uint32_t max_bitrate;
} hdmi_sad_t;

/* HDMI Audio Pin Configuration */
typedef struct {
    uint8_t  pin_nid;
    uint8_t  connected;
    uint8_t  jack_connected;
    uint32_t monitor_count;
    uint32_t eapd_pins[HDMI_AUDIO_MAX_EAPDS];
    hdmi_sad_t sads[HDMI_AUDIO_MAX_SINK_CNT];
    uint8_t  sad_count;
} hdmi_audio_pin_t;

typedef struct hdmi_audio_device {
    pci_device_t *pci_dev;
    void *mmio_base;       /* Mapped HDA MMIO base */
    uint64_t mmio_phys;
    uint8_t  codec_addr;
    uint8_t  pin_count;
    uint8_t  initialized;

    hdmi_audio_pin_t pins[HDMI_AUDIO_MAX_PINS];
    audio_device_t *audio_dev;

    /* Current stream configuration */
    uint32_t sample_rate;
    uint8_t  channels;
    uint8_t  bit_depth;
} hdmi_audio_device_t;

void hdmi_audio_init(pci_device_t *pci_dev);
int hdmi_audio_set_mode(hdmi_audio_device_t *dev, uint32_t rate, uint8_t channels, uint8_t bits);
int hdmi_audio_detect_sink(hdmi_audio_pin_t *pin);
void hdmi_audio_irq_handler(void);

#endif
