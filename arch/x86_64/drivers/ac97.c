#include "ac97.h"
#include "kernel.h"
#include "vmm.h"
#include "pmm.h"
#include "kstring.h"
#include "io.h"
#include "malloc.h"

static ac97_device_t ac97_dev;

static void ac97_codec_reset(void) {
    outw(ac97_dev.mixer_base + AC97_MIXER_RESET, 0x0000);
    for (volatile int i = 0; i < 200000; i++) __builtin_ia32_pause();
    outw(ac97_dev.mixer_base + AC97_MIXER_RESET, 0x0000);
    for (volatile int i = 0; i < 200000; i++) __builtin_ia32_pause();
}

static uint16_t ac97_read_codec(uint8_t codec_index, uint8_t reg) {
    if (codec_index == 0) {
        return inw(ac97_dev.mixer_base + reg);
    } else {
        uint16_t command = 0x80 | (codec_index << 8) | reg;
        outw(ac97_dev.mixer_base + AC97_CODEC_VENDOR_ID1, command);
        for (volatile int i = 0; i < 500000; i++) __builtin_ia32_pause();
        return inw(ac97_dev.mixer_base + AC97_CODEC_VENDOR_ID1);
    }
}

static void ac97_write_codec(uint8_t codec_index, uint8_t reg, uint16_t value) {
    if (codec_index == 0) {
        outw(ac97_dev.mixer_base + reg, value);
    } else {
        uint16_t command = 0x80 | (codec_index << 8) | reg;
        outw(ac97_dev.mixer_base + AC97_CODEC_VENDOR_ID1, command);
        for (volatile int i = 0; i < 500000; i++) __builtin_ia32_pause();
        outw(ac97_dev.mixer_base + AC97_CODEC_VENDOR_ID1, value);
    }
}

uint16_t ac97_mixer_read(ac97_device_t *dev, uint8_t index) {
    if (!dev || !dev->initialized) return 0;
    return ac97_read_codec(0, index);
}

void ac97_mixer_write(ac97_device_t *dev, uint8_t index, uint16_t value) {
    if (!dev || !dev->initialized) return;
    ac97_write_codec(0, index, value);
}

int ac97_set_sample_rate(uint32_t rate) {
    if (!ac97_dev.initialized) return -1;

    uint16_t format = 0;
    if (ac97_dev.primary_codec.bit_depth == 24)
        format |= AC97_PCM_FMT_24BIT;
    else if (ac97_dev.primary_codec.bit_depth == 20)
        format |= AC97_PCM_FMT_20BIT;
    else
        format |= AC97_PCM_FMT_16BIT;

    if (ac97_dev.primary_codec.channels == 1)
        format |= AC97_PCM_MONO;

    if (rate == 44100)
        format |= AC97_PCM_RATE_44K;
    else if (rate == 22050)
        format |= AC97_PCM_RATE_22K;
    else if (rate == 11025)
        format |= AC97_PCM_RATE_11K;
    else
        format |= AC97_PCM_RATE_48K;

    outw(ac97_dev.bm_base + AC97_BM_PCM_FMT, format);
    ac97_dev.primary_codec.sample_rate = rate;
    return 0;
}

static void ac97_setup_dma_buffer(uint32_t size) {
    ac97_dev.pcm_buffer_phys = (uint32_t)(uintptr_t)pmm_alloc_page();
    ac97_dev.pcm_buffer_virt = (void *)(ac97_dev.pcm_buffer_phys + 0xFFFFFFFF80000000ULL);
    memset(ac97_dev.pcm_buffer_virt, 0, size);
    ac97_dev.pcm_buffer_size = size;

    outl(ac97_dev.bm_base + AC97_BM_PCM_LBA, ac97_dev.pcm_buffer_phys);
    outw(ac97_dev.bm_base + AC97_BM_PCM_LBAH, ac97_dev.pcm_buffer_phys >> 16);
    outb(ac97_dev.bm_base + AC97_BM_PCM_LINK, size & 0xFF);
    outb(ac97_dev.bm_base + AC97_BM_PCM_LINK + 1, (size >> 8) & 0xFF);
    outb(ac97_dev.bm_base + AC97_BM_PCM_LINK + 2, (size >> 16) & 0xFF);
    outb(ac97_dev.bm_base + AC97_BM_PCM_LINK + 3, (size >> 24) & 0xFF);
}

static int ac97_detect_codecs(void) {
    uint16_t vid1 = ac97_read_codec(0, AC97_CODEC_VENDOR_ID1);
    uint16_t vid2 = ac97_read_codec(0, AC97_CODEC_VENDOR_ID2);

    if (vid1 == 0 || vid1 == 0xFFFF) {
        printk(KERN_ERR "AC97: No codec detected at primary (VID1=0x%x)\n", vid1);
        return -1;
    }

    ac97_dev.primary_codec.vendor_id = vid1;
    ac97_dev.primary_codec.device_id = vid2;
    ac97_dev.primary_codec.sample_rate = 48000;
    ac97_dev.primary_codec.channels = 2;
    ac97_dev.primary_codec.bit_depth = 16;

    printk(KERN_INFO "AC97: Found primary codec vendor=0x%04x device=0x%04x\n",
           vid1, vid2);

    ac97_dev.codec_count = 1;

    /* Try to detect secondary codecs (index 1 and 2) */
    for (int i = 1; i < 3; i++) {
        uint16_t sec_vid1 = ac97_read_codec(i, AC97_CODEC_VENDOR_ID1);
        if (sec_vid1 != 0 && sec_vid1 != 0xFFFF) {
            uint16_t sec_vid2 = ac97_read_codec(i, AC97_CODEC_VENDOR_ID2);
            ac97_dev.ext_codec[ac97_dev.codec_count - 1].vendor_id = sec_vid1;
            ac97_dev.ext_codec[ac97_dev.codec_count - 1].device_id = sec_vid2;
            ac97_dev.ext_codec[ac97_dev.codec_count - 1].index = i;
            ac97_dev.codec_count++;
            printk(KERN_INFO "AC97: Found secondary codec %d vendor=0x%04x device=0x%04x\n",
                   i, sec_vid1, sec_vid2);
        }
    }

    return 0;
}

static void ac97_configure_mixer(void) {
    /* Set master volume to 75% (0x0000 = max, 0xFFFE = min) */
    uint16_t master_vol = (0x10 << 8) | 0x10;  /* Left=75%, Right=75% */
    ac97_mixer_write(&ac97_dev, AC97_MIXER_MASTER_VOL, master_vol);

    /* Enable PCM output at full volume */
    ac97_mixer_write(&ac97_dev, AC97_MIXER_PCM, 0x8000 | 0x8000);

    /* Set general purpose register - enable speakers */
    ac97_write_codec(0, AC97_MIXER_GENERAL_PU, 0x0002);

    printk(KERN_INFO "AC97: Mixer configured (master vol, PCM enabled)\n");
}

static audio_device_t *ac97_create_audio_dev(void) {
    audio_device_t *adev = (audio_device_t *)kmalloc(sizeof(audio_device_t));
    if (!adev) return NULL;

    memset(adev, 0, sizeof(audio_device_t));
    adev->name = "ac97";
    adev->base_dev = NULL;
    adev->routing_capabilities = 0x01; /* playback only */
    adev->mixer = (audio_mixer_t *)kmalloc(sizeof(audio_mixer_t));
    if (adev->mixer) {
        adev->mixer->name = "ac97-mixer";
        adev->mixer->master_volume = 75;
        adev->mixer->muted = 0;
        adev->mixer->set_volume = NULL;
        adev->mixer->set_mute = NULL;
    }

    return adev;
}

void ac97_init(pci_device_t *pci_dev) {
    if (!pci_dev) {
        printk(KERN_ERR "AC97: No PCI device\n");
        return;
    }

    memset(&ac97_dev, 0, sizeof(ac97_dev));

    /* Verify class code: Class 0x04 (multimedia), Subclass 0x01 (audio device) */
    if (pci_dev->class_code != 0x04 || pci_dev->subclass != 0x01) {
        printk(KERN_WARN "AC97: PCI device class/subclass mismatch (class=%02x sub=%02x)\n",
               pci_dev->class_code, pci_dev->subclass);
    }

    ac97_dev.pci_dev = pci_dev;

    /* Read BARs for mixer and bus master I/O ports */
    /* BAR0 = Mixer I/O port, BAR1 = Bus Master I/O port */
    ac97_dev.mixer_base = pci_dev->bar0 & 0xFFFE;
    ac97_dev.bm_base = pci_dev->bar1 & 0xFFFE;

    if (!ac97_dev.mixer_base || !ac97_dev.bm_base) {
        printk(KERN_ERR "AC97: Invalid BAR0/BAR1 bases (mixer=0x%x, bm=0x%x)\n",
               ac97_dev.mixer_base, ac97_dev.bm_base);
        return;
    }

    pci_enable_bus_mastering(pci_dev);

    /* Allocate and map DMA buffer for PCM */
    ac97_setup_dma_buffer(65536);

    /* Reset codec */
    ac97_codec_reset();

    /* Detect codecs */
    if (ac97_detect_codecs() != 0) {
        printk(KERN_ERR "AC97: Codec detection failed\n");
        return;
    }

    /* Initialize codec */
    ac97_dev.primary_codec.capabilities = ac97_read_codec(0, AC97_CODEC_VENDOR_ID2);

    /* Configure mixer */
    ac97_configure_mixer();

    /* Enable PCM playback via bus master */
    outb(ac97_dev.bm_base + AC97_BM_PCM_CTL, 0);
    ac97_set_sample_rate(48000);
    outb(ac97_dev.bm_base + AC97_BM_PCM_CTL, AC97_BM_CTL_XFER_START);

    /* Register with audio subsystem */
    ac97_dev.audio_dev = ac97_create_audio_dev();
    if (ac97_dev.audio_dev) {
        audio_subsystem_init();
        if (audio_register_device(ac97_dev.audio_dev) == 0) {
            printk(KERN_INFO "AC97: Audio device registered with subsystem\n");
        }
    }

    ac97_dev.initialized = 1;
    printk(KERN_INFO "AC97: Initialized on PCI %02x:%02x (mixer=0x%x, bm=0x%x, dma=0x%x)\n",
           pci_dev->bus, pci_dev->device, ac97_dev.mixer_base,
           ac97_dev.bm_base, ac97_dev.pcm_buffer_phys);
}

void ac97_irq_handler(void) {
    if (!ac97_dev.initialized) return;

    uint8_t status = inb(ac97_dev.bm_base + AC97_BM_PCM_STS);

    if (status & 0x01) {
        /* Buffer completion interrupt */
        outb(ac97_dev.bm_base + AC97_BM_PCM_STS, 0x01);
        printk(KERN_DEBUG "AC97: PCM buffer completion interrupt\n");
    }

    if (status & 0x02) {
        /* Buffer interrupt */
        outb(ac97_dev.bm_base + AC97_BM_PCM_STS, 0x02);
        printk(KERN_DEBUG "AC97: PCM buffer interrupt\n");
    }
}

uint32_t ac97_get_capabilities(uint8_t codec_index) {
    if (codec_index == 0)
        return ac97_dev.primary_codec.capabilities;
    if (codec_index < ac97_dev.codec_count)
        return ac97_dev.ext_codec[codec_index - 1].capabilities;
    return 0;
}
