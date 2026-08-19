#ifndef SHADOWBOX_USB_AUDIO_H
#define SHADOWBOX_USB_AUDIO_H

#include "types.h"
#include "usb.h"
#include "audio.h"

#define USB_CLASS_AUDIO     0x01
#define USB_SUBCLASS_AUDIOCONTROL 0x01
#define USB_SUBCLASS_AUDIOSTREAMING 0x02

#define UAC1_FORMAT_TYPE_I   0x01
#define UAC1_FORMAT_PCM      0x0001
#define UAC1_FORMAT_PCMA     0x0002
#define UAC1_FORMAT_IEEE_FLOAT 0x0004

#define UAC_MAX_ENDPOINTS    4
#define UAC_MAX_FEATURES     8

typedef struct uac_feature_unit {
    uint8_t  id;
    uint16_t bitmaps;          /* Bitmap of present channels */
    uint8_t  mute;
    uint16_t volume;           /* Current volume in 0.25dB units */
    uint8_t  volume_min;
    uint8_t  volume_max;
    uint8_t  volume_res;
    uint8_t  can_mute;
    uint8_t  can_volume;
} uac_feature_unit_t;

typedef struct uac_terminal {
    uint8_t  id;
    uint16_t type;
    uint8_t  source_id[2];
    uint8_t  channel_count;
    uint8_t  channel_config;
} uac_terminal_t;

typedef struct uac_endpoint {
    uint8_t  addr;
    uint8_t  attributes;
    uint16_t max_packet_size;
    uint8_t  bInterval;
    uint8_t  format_type;
    uint32_t sample_rate;
    uint8_t  channels;
    uint8_t  bit_depth;
} uac_endpoint_t;

typedef struct uac_format {
    uint8_t  format_type;
    uint16_t formats;          /* Bitmap of supported PCM formats */
    uint32_t sample_rates[4];
    uint8_t  rate_count;
} uac_format_t;

typedef struct usb_audio_device {
    device_t *base_dev;
    uint8_t  dev_addr;
    uint8_t  interface_ac;    /* AudioControl interface */
    uint8_t  interface_as;    /* AudioStreaming interface */
    uint8_t  iface_setting_as;
    uint8_t  in_endpoint;
    uint8_t  out_endpoint;

    uac_terminal_t  input_term[UAC_MAX_FEATURES];
    uac_terminal_t  output_term[UAC_MAX_FEATURES];
    uac_feature_unit_t features[UAC_MAX_FEATURES];
    uac_endpoint_t  ep_in[UAC_MAX_ENDPOINTS];
    uac_endpoint_t  ep_out[UAC_MAX_ENDPOINTS];
    uac_format_t   format;

    uint8_t  feature_unit_count;
    uint8_t  endpoint_count;
    uint8_t  initialized;

    audio_device_t *audio_dev;
    struct usb_audio_device *next;
} usb_audio_device_t;

void usb_audio_init(void);
usb_audio_device_t *usb_audio_probe(uint8_t dev_addr);
int usb_audio_set_format(usb_audio_device_t *udev, uint32_t rate, uint8_t channels, uint8_t bits);
int usb_audio_set_volume(usb_audio_device_t *dev, uint8_t unit_id, uint8_t channel, uint16_t vol);
void usb_audio_irq_handler(usb_audio_device_t *dev);

#endif
