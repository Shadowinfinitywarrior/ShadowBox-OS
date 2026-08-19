#ifndef SHADOWBOX_AC97_H
#define SHADOWBOX_AC97_H

#include "types.h"
#include "pci.h"
#include "audio.h"

#define AC97_VENDOR_ID    0x8086
#define AC97_DEVICE_ID    0x2415  /* Intel ICH AC'97 */
#define AC97_PAYLOAD_VENDOR_ID 0x10DE  /* NVIDIA MCP */
#define AC97_PAYLOAD_DEVICE_ID 0x03AA

/* AC'97 Bus Master Register Offsets (I/O port relative) */
#define AC97_BM_BASE        0x1C00
#define AC97_BM_PCM_BASE    0x1C00
#define AC97_BM_PCM_CUR     0x00
#define AC97_BM_PCM_FMT     0x02
#define AC97_BM_PCM_LBA     0x04
#define AC97_BM_PCM_LBAH    0x08
#define AC97_BM_PCM_LINK    0x0C
#define AC97_BM_PCM_CTL     0x10
#define AC97_BM_PCM_STS      0x14

/* AC'97 Mixer Register Offsets (I/O port) */
#define AC97_MIXER_RESET      0x00
#define AC97_MIXER_MASTER_VOL 0x02
#define AC97_MIXER_HEADPHONE  0x04
#define AC97_MIXER_BEEP       0x06
#define AC97_MIXER_PHONE      0x08
#define AC97_MIXER_MIC        0x0A
#define AC97_MIXER_LINE_IN    0x0C
#define AC97_MIXER_CD         0x18
#define AC97_MIXER_VIDEO      0x10
#define AC97_MIXER_PCM        0x1C
#define AC97_MIXER_PCUSOUND   0x1E
#define AC97_MIXER_RECORD_GAIN 0x1A
#define AC97_MIXER_REC_SEL    0x1E
#define AC97_MIXER_GENERAL_PU 0x20
#define AC97_MIXER_POWERDOWN  0x26

/* AC'97 Bus Master Control Bits */
#define AC97_BM_CTL_XFER_START  0x01
#define AC97_BM_CTL_XFER_PAUSE  0x02
#define AC97_BM_CTL_BM_RESET    0x04

/* AC'97 PCM Format Bits */
#define AC97_PCM_FMT_16BIT      0x01
#define AC97_PCM_FMT_20BIT      0x03
#define AC97_PCM_FMT_24BIT      0x05
#define AC97_PCM_STEREO         0x00
#define AC97_PCM_MONO           0x10
#define AC97_PCM_RATE_48K       0x20
#define AC97_PCM_RATE_44K       0x40
#define AC97_PCM_RATE_22K       0x60
#define AC97_PCM_RATE_11K       0x80

/* AC'97 Codec Register (via mixer port) */
#define AC97_CODEC_VENDOR_ID1   0x7C
#define AC97_CODEC_VENDOR_ID2   0x7E
#define AC97_CODEC_RESET        0x00

/* AC'97 NID (Node ID) for Power Up */
#define AC97_PCM_FRONT_DAC_RATE 0x2
#define AC97_PCM_SURROUND_RATE  0x4
#define AC97_PCM_LFE_RATE       0x6

typedef struct ac97_codec {
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t  index;
    uint16_t capabilities;
    uint32_t sample_rate;
    uint8_t  channels;
    uint8_t  bit_depth;
} ac97_codec_t;

typedef struct ac97_device {
    pci_device_t *pci_dev;
    uint16_t nisd;  /* Negotiated Input Sample Depth */
    uint16_t noso;  /* Non-0 if codec is ready */
    uint16_t mixer_base;  /* Mixer I/O port base */
    uint16_t bm_base;     /* Bus Master I/O port base */

    ac97_codec_t primary_codec;
    ac97_codec_t ext_codec[3];
    uint8_t codec_count;

    audio_device_t *audio_dev;

    /* DMA Buffer for PCM playback */
    uint32_t pcm_buffer_phys;
    void *pcm_buffer_virt;
    uint32_t pcm_buffer_size;

    uint8_t initialized;
} ac97_device_t;

void ac97_init(pci_device_t *pci_dev);
void ac97_irq_handler(void);
uint16_t ac97_mixer_read(ac97_device_t *dev, uint8_t index);
void ac97_mixer_write(ac97_device_t *dev, uint8_t index, uint16_t value);
int ac97_set_sample_rate(uint32_t rate);
uint32_t ac97_get_capabilities(uint8_t codec_index);

#endif
