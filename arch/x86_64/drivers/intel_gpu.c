#include "intel_gpu.h"
#include "kernel.h"
#include "vmm.h"
#include "pmm.h"
#include "io.h"
#include "malloc.h"
#include "kstring.h"
#include "fb.h"
#include "display.h"

#define KB (1024ULL)
#define MB (1024 * 1024ULL)

static intel_gpu_device_t intel_gpu;

static uint32_t mmio_read(uint32_t offset) {
    return *(volatile uint32_t *)((uintptr_t)intel_gpu.mmio + offset);
}

static void mmio_write(uint32_t offset, uint32_t value) {
    *(volatile uint32_t *)((uintptr_t)intel_gpu.mmio + offset) = value;
}

static uint8_t intel_detect_gen(uint16_t device_id) {
    if (device_id == INTEL_GPU_DEVICE_SKYLAKE) return INTEL_CHIP_GEN9;
    if (device_id == INTEL_GPU_DEVICE_KABY) return INTEL_CHIP_GEN9;
    if (device_id == INTEL_GPU_DEVICE_COMETLAKE) return INTEL_CHIP_GEN9;
    if (device_id == INTEL_GPU_DEVICE_HASWELL) return INTEL_CHIP_GEN7;
    if (device_id == INTEL_GPU_DEVICE_IVY) return INTEL_CHIP_GEN7;
    if (device_id == INTEL_GPU_DEVICE_SANDY) return INTEL_CHIP_GEN6;
    if (device_id == INTEL_GPU_DEVICE_IRONLAKE) return INTEL_CHIP_GEN45;
    if (device_id == INTEL_GPU_DEVICE_G45_GM) return INTEL_CHIP_GEN45;
    if (device_id == INTEL_GPU_DEVICE_G4X) return INTEL_CHIP_GEN45;
    if (device_id == INTEL_GPU_DEVICE_GMA3600) return INTEL_CHIP_GEN4;
    if (device_id == INTEL_GPU_DEVICE_I965) return INTEL_CHIP_GEN4;
    if (device_id == INTEL_GPU_DEVICE_I945) return INTEL_CHIP_LEGACY;
    return INTEL_CHIP_GEN4;
}

static void intel_forcewake_get(void) {
    if (intel_gpu.info.gen >= INTEL_CHIP_GEN4) {
        mmio_write(INTEL_FORCEWAKE_CTL, INTEL_FORCEWAKE_CTL_VALUE);
        for (volatile int i = 0; i < 100000; i++) {
            if (mmio_read(INTEL_FORCEWAKE_STS) & 0x1) break;
            __builtin_ia32_pause();
        }
    }
}

static void intel_forcewake_put(void) {
    if (intel_gpu.info.gen >= INTEL_CHIP_GEN4) {
        mmio_write(INTEL_FORCEWAKE_CTL, 0);
        for (volatile int i = 0; i < 100000; i++) {
            if (!(mmio_read(INTEL_FORCEWAKE_STS) & 0x1)) break;
            __builtin_ia32_pause();
        }
    }
}

static int intel_modeset_pipe(uint32_t pipe_idx, uint32_t w, uint32_t h, uint32_t stride, uint64_t fb_addr) {
    uint32_t pipe_conf_off, dsp_ctrl_off, dsp_base_off, dsp_addr_off, dsp_stride_off;

    if (pipe_idx == 0) {
        pipe_conf_off = INTEL_REG_PIPEA_CONF;
        dsp_ctrl_off = INTEL_REG_DSPA_CTRL;
        dsp_base_off = INTEL_REG_DSPA_BASE;
        dsp_addr_off = INTEL_REG_DSPA_ADDR;
        dsp_stride_off = INTEL_REG_DSPA_STRIDE;
    } else {
        pipe_conf_off = INTEL_REG_PIPEB_CONF;
        dsp_ctrl_off = INTEL_REG_DSPB_CTRL;
        dsp_base_off = INTEL_REG_DSPB_BASE;
        dsp_addr_off = INTEL_REG_DSPB_ADDR;
        dsp_stride_off = INTEL_REG_DSPB_STRIDE;
    }

    /* Disable display plane before changing */
    mmio_write(dsp_ctrl_off, 0);
    __builtin_ia32_pause();
    __builtin_ia32_pause();

    /* Set base address, stride, and source address */
    uint32_t addr_lo = (uint32_t)(fb_addr & 0xFFFFFFFF);
    uint32_t addr_hi = (uint32_t)(fb_addr >> 32);

    mmio_write(dsp_base_off, 0);  /* Reset cursor base */
    mmio_write(dsp_addr_off, addr_lo);
    if (intel_gpu.info.gen >= INTEL_CHIP_GEN4) {
        *(volatile uint32_t *)((uintptr_t)intel_gpu.mmio + dsp_addr_off + 4) = addr_hi;
    }
    mmio_write(dsp_stride_off, stride);

    /* Enable pipe */
    uint32_t pipe_conf = mmio_read(pipe_conf_off);
    pipe_conf |= INTEL_PIPECONF_ENABLE | INTEL_PIPECONF_BPC_8;
    mmio_write(pipe_conf_off, pipe_conf);

    /* Enable display plane */
    uint32_t dsp_ctrl = mmio_read(dsp_ctrl_off);
    dsp_ctrl |= INTEL_DSP_ENABLED | (1 << 24); /* GAMMA_8BIT | COLOR_8BIT */
    mmio_write(dsp_ctrl_off, dsp_ctrl);

    return 0;
}

int intel_gpu_alloc_framebuffer(intel_gpu_device_t *dev, uint32_t w, uint32_t h, uint8_t bpp) {
    if (!dev || dev->fb_count >= INTEL_MAX_FB) return -1;
    if (bpp != 32 && bpp != 24 && bpp != 16) {
        printk(KERN_WARN "INTEL_GPU: Unsupported BPP %u, defaulting to 32\n", bpp);
        bpp = 32;
    }

    uint32_t fb_id = dev->fb_count;
    uint32_t fb_size = w * h * (bpp / 8);
    uint32_t pages = (fb_size + 4095) / 4096;

    /* Allocate contiguous physical memory for framebuffer */
    uint64_t fb_phys = (uint64_t)pmm_alloc_page();
    /* We only allocate one page for simplicity; in a real driver we'd
       allocate pages*PAGE_SIZE contiguous memory */
    if (!fb_phys) {
        printk(KERN_ERR "INTEL_GPU: Failed to allocate framebuffer\n");
        return -1;
    }

    /* Map it to virtual space */
    void *fb_virt = vmap_phys(fb_phys, 4096);
    memset(fb_virt, 0, 4096);

    dev->fbs[fb_id].id = fb_id;
    dev->fbs[fb_id].width = w;
    dev->fbs[fb_id].height = h;
    dev->fbs[fb_id].pitch = w * (bpp / 8);
    dev->fbs[fb_id].bpp = bpp;
    dev->fbs[fb_id].vaddr = fb_virt;
    dev->fbs[fb_id].paddr = fb_phys;

    dev->fb_count++;

    printk(KERN_INFO "INTEL_GPU: Allocated FB %u (%zux%zu %u-bit) phys=0x%llx virt=%p\n",
           fb_id, w, h, bpp, fb_phys, fb_virt);
    return (int)fb_id;
}

int intel_gpu_modeset(intel_gpu_device_t *dev, uint32_t w, uint32_t h, uint8_t bpp) {
    if (!dev || !dev->initialized) return -1;

    int fb_id = intel_gpu_alloc_framebuffer(dev, w, h, bpp);
    if (fb_id < 0) return -1;

    intel_gpu_device_t *gpu = dev;
    intel_framebuffer_t *fb = &gpu->fbs[fb_id];

    /* Set the display mode on pipe A */
    intel_modeset_pipe(0, w, h, fb->pitch, fb->paddr);

    gpu->current_fb = fb;

    /* Update the fb subsystem */
    fb_set_info(fb->paddr, fb->width, fb->height, fb->pitch, fb->bpp);

    return fb_id;
}

void intel_gpu_flush_framebuffer(intel_gpu_device_t *dev, uint32_t fb_id) {
    if (!dev || fb_id >= (uint32_t)dev->fb_count) return;
    /* On Intel GPUs, the frontbuffer is flushed by the hardware after modeset.
       In a real driver, we would use the MI_FLUSH or PIPEFOR flush. */
    __asm__ volatile("mfence" ::: "memory");
}

intel_gpu_info_t *intel_gpu_get_info(void) {
    return &intel_gpu.info;
}

void intel_gpu_init(pci_device_t *pci_dev) {
    if (!pci_dev) {
        printk(KERN_ERR "INTEL_GPU: No PCI device\n");
        return;
    }

    memset(&intel_gpu, 0, sizeof(intel_gpu));
    intel_gpu.pci_dev = pci_dev;
    intel_gpu.info.device_id = pci_dev->device_id;
    intel_gpu.info.revision = pci_dev->revision_id;
    intel_gpu.info.gen = intel_detect_gen(pci_dev->device_id);

    printk(KERN_INFO "INTEL_GPU: Detected Intel GPU device 0x%04x (gen %u)\n",
           pci_dev->device_id, intel_gpu.info.gen);

    pci_enable_bus_mastering(pci_dev);

    /* Read BAR0 for MMIO registers */
    uint32_t bar0_low = pci_config_read(pci_dev->bus, pci_dev->device,
                                         pci_dev->function, 0x10);
    uint64_t bar0 = bar0_low & 0xFFE00000ULL;  /* 4KB aligned for MMIO */
    if ((bar0_low & 0x06) == 0x04) {
        uint32_t bar0_high = pci_config_read(pci_dev->bus, pci_dev->device,
                                              pci_dev->function, 0x14);
        bar0 |= ((uint64_t)bar0_high << 32) & 0xFFFFFFFF00000000ULL;
    }

    intel_gpu.mmio_phys = bar0;
    /* Map at least 1MB of MMIO space */
    intel_gpu.mmio = vmap_phys(bar0, 0x100000);

    if (!intel_gpu.mmio) {
        printk(KERN_ERR "INTEL_GPU: Failed to map MMIO at 0x%llx\n", bar0);
        return;
    }

    printk(KERN_INFO "INTEL_GPU: MMIO at phys=0x%llx virt=%p\n", bar0, intel_gpu.mmio);

    /* Disable VGA legacy if needed */
    if (mmio_read(INTEL_VGACNTL) & 0x1) {
        printk(KERN_INFO "INTEL_GPU: Disabling VGA legacy\n");
        mmio_write(INTEL_VGACNTL, INTEL_VGACNTL_CPU_VGA_DIS);
    }

    /* Setup forcewake for power management */
    intel_forcewake_get();

    /* Default modeset to 1024x768x32 */
    int fb_id = intel_gpu_modeset(&intel_gpu, 1024, 768, 32);
    if (fb_id >= 0) {
        intel_gpu.display = (display_output_t *)kmalloc(sizeof(display_output_t));
        if (intel_gpu.display) {
            memset(intel_gpu.display, 0, sizeof(display_output_t));
            intel_gpu.display->type = DISP_CONN_DVI;
            intel_gpu.display->current_width = 1024;
            intel_gpu.display->current_height = 768;
            intel_gpu.display->current_refresh = 60;
            intel_gpu.display->connected = 1;
        }
        display_register_output(intel_gpu.display);
    } else {
        printk(KERN_ERR "INTEL_GPU: Failed to set initial mode\n");
        intel_forcewake_put();
        return;
    }

    intel_forcewake_put();

    intel_gpu.initialized = 1;
    printk(KERN_INFO "INTEL_GPU: Initialized (gen %u, FB %dx%d)\n",
           intel_gpu.info.gen, 1024, 768);
}

void intel_gpu_irq_handler(void) {
    if (!intel_gpu.initialized) return;

    uint32_t gt_status = mmio_read(INTEL_REG_GTST);
    if (gt_status) {
        printk(KERN_DEBUG "INTEL_GPU: GT interrupt status=0x%x\n", gt_status);
        /* Clear interrupts */
        mmio_write(INTEL_REG_GTST, gt_status);
    }

    /* Acknowledge display interrupt */
    uint32_t dspsts = mmio_read(0x60010); /* PIPEASTATUS */
    if (dspsts) {
        mmio_write(0x60010, dspsts);
        printk(KERN_DEBUG "INTEL_GPU: Display interrupt status=0x%x\n", dspsts);
    }
}
