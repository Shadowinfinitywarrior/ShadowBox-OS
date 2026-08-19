#include "hdmi_audio.h"
#include "kernel.h"
#include "vmm.h"
#include "pmm.h"
#include "kstring.h"
#include "io.h"
#include "malloc.h"

static hdmi_audio_device_t hdmi_audio;

static uint32_t hdmi_verb_send(uint8_t codec_addr, uint8_t node_id, uint8_t verb, uint16_t payload) {
    if (!hdmi_audio.initialized) return 0;

    uint32_t command = (codec_addr << 28) | (node_id << 20) | (verb << 8) | (payload & 0xFF);
    *(volatile uint32_t *)((uintptr_t)hdmi_audio.mmio_base + HDA_HDMI_CODEC_CMD) = command;

    uint32_t response = *(volatile uint32_t *)((uintptr_t)hdmi_audio.mmio_base + HDA_HDMI_CODEC_STS);

    /* Wait for response */
    for (volatile int i = 0; i < 1000000; i++) {
        response = *(volatile uint32_t *)((uintptr_t)hdmi_audio.mmio_base + HDA_HDMI_CODEC_STS);
        if (response != 0xFFFFFFFF) break;
        __builtin_ia32_pause();
    }

    return response;
}

static void hdmi_pin_sense_enable(hdmi_audio_pin_t *pin) {
    uint32_t resp = hdmi_verb_send(hdmi_audio.codec_addr, pin->pin_nid,
                                   HDA_VERB_SET_PIN_WIDGET_CONTROL, 0x01);
    (void)resp;

    /* Enable EAPD on this pin */
    hdmi_verb_send(hdmi_audio.codec_addr, pin->pin_nid,
                   HDA_VERB_SET_EAPD_BTLEN, 0x02);

    /* Set output amp to 0dB (0x7F = 0dB in 0.5dB steps) */
    hdmi_verb_send(hdmi_audio.codec_addr, pin->pin_nid,
                   HDA_VERB_SET_AMP_GAIN, 0x7F);

    pin->connected = 1;
    printk(KERN_INFO "HDMI-AUDIO: Pin NID 0x%02x enabled for output\n", pin->pin_nid);
}

static int hdmi_detect_sink_eld(hdmi_audio_pin_t *pin) {
    pin->jack_connected = 1;
    pin->monitor_count = 1;

    /* Read ELD (EDID-Like Data) from codec verb */
    uint32_t sense = hdmi_verb_send(hdmi_audio.codec_addr, pin->pin_nid,
                                    HDA_VERB_GET_PIN_SENSE, 0);

    if (sense & 0x80000000) {
        pin->jack_connected = 1;
        return 0;
    }

    pin->jack_connected = 0;
    pin->monitor_count = 0;
    return -1;
}

int hdmi_audio_detect_sink(hdmi_audio_pin_t *pin) {
    return hdmi_detect_sink_eld(pin);
}

int hdmi_audio_set_mode(hdmi_audio_device_t *dev, uint32_t rate, uint8_t channels, uint8_t bits) {
    if (!dev || !dev->initialized) return -1;

    /* Configure the audio widget parameters */
    uint32_t format = 0;
    if (bits == 24) format |= 0x02;
    format |= (channels <= 2) ? 0x01 : 0x02;

    if (rate == 44100) format |= 0x01;
    else if (rate == 48000) format |= 0x02;
    else if (rate == 96000) format |= 0x04;
    else if (rate == 192000) format |= 0x08;

    /* Set converter format */
    for (uint8_t i = 0; i < dev->pin_count; i++) {
        if (dev->pins[i].connected) {
            hdmi_verb_send(dev->codec_addr, dev->pins[i].pin_nid,
                           HDA_VERB_SET_PIN_PARAM, format);
        }
    }

    dev->sample_rate = rate;
    dev->channels = channels;
    dev->bit_depth = bits;

    printk(KERN_INFO "HDMI-AUDIO: Set mode rate=%u channels=%u bits=%u\n",
           rate, channels, bits);
    return 0;
}

static void hdmi_audio_register_sink(hdmi_audio_pin_t *pin) {
    audio_device_t *adev = (audio_device_t *)kmalloc(sizeof(audio_device_t));
    if (!adev) return;

    memset(adev, 0, sizeof(audio_device_t));
    adev->name = "hdmi";
    adev->base_dev = NULL;
    adev->routing_capabilities = 0x03; /* playback + capture */

    /* Read SAD from codec for supported formats */
    uint32_t resp = hdmi_verb_send(hdmi_audio.codec_addr, pin->pin_nid, 0x700, 0);
    if (resp != 0xFFFFFFFF) {
        pin->sad_count = 1;
        pin->sads[0].format_code = 1; /* PCM */
        pin->sads[0].num_channels = pin->connected ? 8 : 2;
        pin->sads[0].sample_rates = 0x0F; /* 44.1, 48, 96, 192 kHz */
        pin->sads[0].sample_size = 0x07;  /* 16/20/24-bit */
        adev->mixer = NULL; /* HDMI has no traditional mixer */
    }

    pin->connected = 1;
    if (audio_register_device(adev) == 0) {
        printk(KERN_INFO "HDMI-AUDIO: Registered audio device on pin 0x%02x\n", pin->pin_nid);
    }
}

void hdmi_audio_init(pci_device_t *pci_dev) {
    if (!pci_dev) {
        printk(KERN_ERR "HDMI-AUDIO: No PCI device\n");
        return;
    }

    memset(&hdmi_audio, 0, sizeof(hdmi_audio));
    hdmi_audio.pci_dev = pci_dev;

    /* Check for HDMI Audio device: Class 0x04 (multimedia), Subclass 0x03 (HDA) */
    if (pci_dev->class_code == 0x04 && pci_dev->subclass == 0x03) {
        /* Could be HDA audio - check if it supports HDMI output */
        printk(KERN_INFO "HDMI-AUDIO: Found HDA-compatible device at PCI %02x:%02x\n",
               pci_dev->bus, pci_dev->device);
    }

    pci_enable_bus_mastering(pci_dev);

    /* Read BAR0 (MMIO base) */
    uint32_t bar0_low = pci_config_read(pci_dev->bus, pci_dev->device,
                                         pci_dev->function, 0x10);
    uint64_t bar0 = bar0_low & 0xFFFFFFF0;
    if ((bar0_low & 0x06) == 0x04) {
        uint32_t bar0_high = pci_config_read(pci_dev->bus, pci_dev->device,
                                              pci_dev->function, 0x14);
        bar0 |= ((uint64_t)bar0_high << 32);
    }

    hdmi_audio.mmio_phys = bar0;
    /* Map 4KB of MMIO space */
    hdmi_audio.mmio_base = vmap_phys(bar0, 4096);

    if (!hdmi_audio.mmio_base) {
        printk(KERN_ERR "HDMI-AUDIO: Failed to map MMIO at 0x%lx\n", bar0);
        return;
    }

    /* Reset HDA controller */
    volatile uint32_t *gctl = (volatile uint32_t *)((uintptr_t)hdmi_audio.mmio_base + HDA_HDMI_GCTL);
    *gctl &= ~1; /* Clear reset */
    for (volatile int i = 0; i < 100000; i++) __builtin_ia32_pause();
    *gctl |= 1;  /* Set reset */
    for (volatile int i = 0; i < 100000; i++) __builtin_ia32_pause();

    /* Enable unsolicited responses */
    outl(0x20, 0x00); /* PCI_COMMAND, enable memory + bus master */
    *(volatile uint32_t *)((uintptr_t)hdmi_audio.mmio_base + HDA_HDMI_CODEC_ID) = 1;

    /* Scan for HDMI output pins (NIDs 0x10-0x17) */
    uint8_t pin_count = 0;
    for (uint8_t nid = HDA_HDMI_PIN_NID_BASE; nid < HDA_HDMI_PIN_NID_BASE + HDMI_AUDIO_MAX_PINS; nid++) {
        uint32_t param = hdmi_verb_send(hdmi_audio.codec_addr, nid, 0xF00, 0);
        if (param == 0xFFFFFFFF || param == 0) continue;

        uint8_t widget_type = (param >> 20) & 0x0F;
        if (widget_type == 0x04) { /* Pin Complex */
            hdmi_audio.pins[pin_count].pin_nid = nid;
            hdmi_audio.pins[pin_count].connected = 0;
            pin_count++;
            printk(KERN_INFO "HDMI-AUDIO: Found pin complex at NID 0x%02x\n", nid);
        }
    }

    hdmi_audio.pin_count = pin_count;

    if (pin_count == 0) {
        printk(KERN_WARN "HDMI-AUDIO: No HDMI output pins detected\n");
        return;
    }

    /* Detect connected sinks and enable */
    for (uint8_t i = 0; i < pin_count; i++) {
        if (hdmi_detect_sink_eld(&hdmi_audio.pins[i]) == 0) {
            hdmi_pin_sense_enable(&hdmi_audio.pins[i]);
            hdmi_audio_register_sink(&hdmi_audio.pins[i]);
        }
    }

    /* Initialize audio subsystem and register */
    audio_subsystem_init();
    hdmi_audio.sample_rate = 48000;
    hdmi_audio.channels = 2;
    hdmi_audio.bit_depth = 16;
    hdmi_audio.initialized = 1;

    printk(KERN_INFO "HDMI-AUDIO: Initialized (codec=0x%x, pins=%d)\n",
           hdmi_audio.codec_addr, pin_count);
}

void hdmi_audio_irq_handler(void) {
    if (!hdmi_audio.initialized) return;

    volatile uint32_t *status = (volatile uint32_t *)
        ((uintptr_t)hdmi_audio.mmio_base + HDA_HDMI_STATESTS);
    uint32_t sts = *status;

    if (sts & 0x01) {
        /* unsolicited response - jack plug/unplug */
        volatile uint32_t *unsol = (volatile uint32_t *)
            ((uintptr_t)hdmi_audio.mmio_base + HDA_HDMI_CODEC_STS);
        uint32_t resp = *unsol;
        uint8_t nid = (resp >> 20) & 0x7F;
        uint8_t state = (resp >> 8) & 0x1;

        for (uint8_t i = 0; i < hdmi_audio.pin_count; i++) {
            if (hdmi_audio.pins[i].pin_nid == nid) {
                hdmi_audio.pins[i].jack_connected = state;
                printk(KERN_INFO "HDMI-AUDIO: Pin 0x%02x %sconnected\n",
                       nid, state ? "con" : "dis");
            }
        }
        *status = sts;
    }
}
