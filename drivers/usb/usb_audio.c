#include "usb_audio.h"
#include "kernel.h"
#include "bus.h"
#include "driver.h"
#include "device.h"
#include "malloc.h"
#include "kstring.h"

extern bus_type_t usb_bus_type;

static usb_audio_device_t *uac_devices = NULL;
static uint8_t uac_device_count = 0;

static driver_t usb_audio_driver = {
    .name = "usb_audio",
    .probe = NULL,
    .remove = NULL,
    .bus = &usb_bus_type,
    .ops = NULL,
    .irq_handler = NULL,
    .suspend = NULL,
    .resume = NULL,
    .next = NULL,
};

static usb_audio_device_t *uac_alloc_device(void) {
    usb_audio_device_t *dev = (usb_audio_device_t *)kmalloc(sizeof(usb_audio_device_t));
    if (!dev) return NULL;
    memset(dev, 0, sizeof(usb_audio_device_t));
    dev->next = NULL;
    return dev;
}

int usb_audio_set_volume(usb_audio_device_t *dev, uint8_t unit_id, uint8_t channel, uint16_t vol) {
    if (!dev || !dev->initialized) return -1;

    for (int i = 0; i < dev->feature_unit_count; i++) {
        if (dev->features[i].id == unit_id) {
            if (!dev->features[i].can_volume) return -1;
            dev->features[i].volume = vol;
            printk(KERN_INFO "UAC: Volume set on unit %d ch %d to %d (0.25dB units)\n",
                   unit_id, channel, vol);
            return 0;
        }
    }
    return -1;
}

int usb_audio_set_format(usb_audio_device_t *dev, uint32_t rate, uint8_t channels, uint8_t bits) {
    if (!dev || !dev->initialized) return -1;

    /* Check supported sample rates */
    int found_rate = 0;
    for (int i = 0; i < dev->format.rate_count && i < 4; i++) {
        if (dev->format.sample_rates[i] == rate) {
            found_rate = 1;
            break;
        }
    }
    if (!found_rate && dev->format.sample_rates[0] != 0) {
        printk(KERN_WARN "UAC: Sample rate %u not supported\n", rate);
        return -1;
    }

    /* Check format support */
    if (!(dev->format.formats & UAC1_FORMAT_PCM)) {
        printk(KERN_WARN "UAC: PCM format not supported\n");
        return -1;
    }

    /* Configure endpoint */
    for (int i = 0; i < dev->endpoint_count; i++) {
        if (dev->ep_out[i].addr && dev->ep_out[i].attributes & 0x01) {
            dev->ep_out[i].sample_rate = rate;
            dev->ep_out[i].channels = channels;
            dev->ep_out[i].bit_depth = bits;
        }
    }

    dev->format.sample_rates[0] = rate;
    dev->format.rate_count = 1;
    dev->initialized = 1;

    /* Register with audio subsystem */
    if (dev->audio_dev) {
        audio_format_t fmt = {
            .sample_rate = rate,
            .channels = channels,
            .bit_depth = bits,
        };
        if (dev->audio_dev->open_stream) {
            dev->audio_dev->open_stream(dev->audio_dev, AUDIO_STREAM_PLAYBACK, &fmt);
        }
    }

    printk(KERN_INFO "UAC: Format set rate=%u ch=%u bits=%u\n", rate, channels, bits);
    return 0;
}

static int uac_parse_descriptors(usb_audio_device_t *dev, uint8_t *desc, uint16_t desc_len) {
    (void)dev; (void)desc; (void)desc_len;
    /* Descriptor parsing would happen here in a full implementation */
    return 0;
}

static audio_device_t *uac_create_audio_dev(usb_audio_device_t *udev) {
    audio_device_t *adev = (audio_device_t *)kmalloc(sizeof(audio_device_t));
    if (!adev) return NULL;

    memset(adev, 0, sizeof(audio_device_t));
    adev->name = "usb-audio";
    adev->base_dev = udev->base_dev;
    adev->routing_capabilities = 0x03;

    /* Create a mixer for the feature unit */
    if (udev->feature_unit_count > 0 && udev->features[0].can_volume) {
        adev->mixer = (audio_mixer_t *)kmalloc(sizeof(audio_mixer_t));
        if (adev->mixer) {
            adev->mixer->name = "usb-audio-mixer";
            adev->mixer->master_volume = udev->features[0].volume / 4;
            adev->mixer->muted = udev->features[0].mute;
        }
    }

    return adev;
}

static int uac_probe(device_t *dev) {
    printk(KERN_INFO "usb_audio: probe called for %s\n", dev->name);

    /* In a real implementation, we would parse USB descriptors here */
    /* For now, we check if this is an audio device by its interface class */
    if (!dev->bus_data) {
        /* No USB device data yet, skip */
        return -1;
    }

    return 0;
}

static void uac_remove(device_t *dev) {
    printk(KERN_INFO "usb_audio: remove called for %s\n", dev->name);
}

void usb_audio_init(void) {
    printk(KERN_INFO "USB-AUDIO: Initializing USB Audio Class driver\n");

    /* Register with USB bus driver framework */
    usb_audio_driver.probe = uac_probe;
    usb_audio_driver.remove = uac_remove;
    driver_register(&usb_audio_driver);

    audio_subsystem_init();
}

usb_audio_device_t *usb_audio_probe(uint8_t dev_addr) {
    usb_audio_device_t *udev = uac_alloc_device();
    if (!udev) return NULL;

    udev->dev_addr = dev_addr;
    udev->initialized = 0;

    /* In a real implementation, we would:
       1. Get USB device descriptors
       2. Parse configuration descriptors for AudioControl and AudioStreaming interfaces
       3. Parse feature units, terminals, and endpoint descriptors
       4. Set the AudioStreaming interface alternate setting
       5. Configure sample rate
     */

    uac_parse_descriptors(udev, NULL, 0);

    /* Create audio device and register */
    udev->audio_dev = uac_create_audio_dev(udev);
    if (udev->audio_dev) {
        audio_register_device(udev->audio_dev);
    }

    udev->initialized = 1;
    uac_device_count++;

    if (!uac_devices) {
        uac_devices = udev;
    }

    printk(KERN_INFO "USB-AUDIO: Device registered at address %d (features=%d)\n",
           dev_addr, udev->feature_unit_count);
    return udev;
}

void usb_audio_irq_handler(usb_audio_device_t *dev) {
    if (!dev || !dev->initialized) return;

    /* Handle audio data completion on endpoints */
    for (int i = 0; i < dev->endpoint_count; i++) {
        if (dev->ep_in[i].addr) {
            /* Data received on input endpoint (capture) */
            if (dev->audio_dev && dev->audio_dev->read_pcm) {
                dev->audio_dev->read_pcm(dev->audio_dev, NULL, dev->ep_in[i].max_packet_size);
            }
        }
        if (dev->ep_out[i].addr) {
            /* Playback data sent */
            printk(KERN_DEBUG "UAC: Playback transfer completed on ep 0x%02x\n",
                   dev->ep_out[i].addr);
        }
    }
}
