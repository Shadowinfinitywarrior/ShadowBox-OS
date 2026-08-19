#ifndef SHADOWBOX_INTEL_GPU_H
#define SHADOWBOX_INTEL_GPU_H

#include "types.h"
#include "pci.h"
#include "display.h"

#define INTEL_GPU_VENDOR_ID 0x8086

#define INTEL_GPU_DEVICE_G4X      0x2A40
#define INTEL_GPU_DEVICE_I945     0x2782
#define INTEL_GPU_DEVICE_I965     0x2900
#define INTEL_GPU_DEVICE_GMA3600  0x0BE0
#define INTEL_GPU_DEVICE_G45_GM   0x2E10
#define INTEL_GPU_DEVICE_IRONLAKE 0x0046
#define INTEL_GPU_DEVICE_SANDY    0x0112
#define INTEL_GPU_DEVICE_IVY      0x0152
#define INTEL_GPU_DEVICE_HASWELL  0x0412
#define INTEL_GPU_DEVICE_SKYLAKE  0x1912
#define INTEL_GPU_DEVICE_KABY     0x5912
#define INTEL_GPU_DEVICE_COMETLAKE 0x9B41
#define INTEL_GPU_DEVICE_RAPTR    0x4680

/* Intel GPU MMIO Register Offsets */
#define INTEL_MMIO_BAR0         0x10

/* Display Engine Registers */
#define INTEL_REG_DSPA_CTRL       0x60040
#define INTEL_REG_DSPA_BASE       0x60030
#define INTEL_REG_DSPA_ADDR       0x60034
#define INTEL_REG_DSPA_STRIDE     0x60038
#define INTEL_REG_DSPB_CTRL       0x61040
#define INTEL_REG_DSPB_BASE       0x61030
#define INTEL_REG_DSPB_ADDR       0x61034
#define INTEL_REG_DSPB_STRIDE     0x61038
#define INTEL_REG_PIPEA_CONF      0x6000C
#define INTEL_REG_PIPEB_CONF      0x6100C
#define INTEL_REG_PFA_CTL         0x61180
#define INTEL_REG_PFB_CTL         0x61184

/* Legacy VGA registers */
#define INTEL_VGACNTL             0x44000
#define INTEL_VGACNTL_CPU_VGA_DIS (1 << 0)
#define INTEL_VGACNTL_GAMMEM_DIS  (1 << 1)

/* Transcoder registers (Ironlake+) */
#define INTEL_REG_PIPEA_CONF2     0x6AC00
#define INTEL_REG_PIPEB_CONF2     0x6BC00

/* Framebuffer and mode registers */
#define INTEL_DPLL_A              0x60440
#define INTEL_DPLL_B              0x60460
#define INTEL_FPA0                0x60450
#define INTEL_FPA1                0x60454

/* Ring buffer registers (gen4+) */
#define INTEL_RING_TAIL           0x2080
#define INTEL_RING_HEAD           0x2040
#define INTEL_RING_START          0x2044
#define INTEL_RING_LEN            0x2048
#define INTEL_RING_CTL            0x2044
#define INTEL_RING_STATUS         0x2088
#define INTEL_RING_IMR            0x208C
#define INTEL_RING_IPEIR          0x2088
#define INTEL_RING_IPEHR          0x208C

/* GT registers */
#define INTEL_REG_GT_RC6_EN       0x1000
#define INTEL_REG_GT_RC6_PWR      0x1004
#define INTEL_REG_GT_ILO_CMD      0x1010
#define INTEL_REG_GTIMR           0x10104
#define INTEL_REG_GTST              0x10110

/* Forcewake registers */
#define INTEL_FORCEWAKE_CTL       0xA070
#define INTEL_FORCEWAKE_STS       0x53D0
#define INTEL_FORCEWAKE_CTL_VALUE 0x1

/* Power well registers (Gen8+) */
#define INTEL_PW_REQ              0x46300
#define INTEL_PW_Status           0x46304
#define INTEL_PW_ACK              0x46308

/* Display modeset controls */
#define INTEL_PIPECONF_ENABLE     (1 << 31)
#define INTEL_PIPECONF_GAMMA_10BIT (1 << 25)
#define INTEL_PIPECONF_BPC_8     (0 << 20)

#define INTEL_DSP_ENABLED         (1 << 31)

/* Chipset info flags */
#define INTEL_CHIP_GEN9           9
#define INTEL_CHIP_GEN8           8
#define INTEL_CHIP_GEN7           7
#define INTEL_CHIP_GEN6           6
#define INTEL_CHIP_GEN45          5
#define INTEL_CHIP_GEN4           4
#define INTEL_CHIP_LEGACY         0

#define INTEL_MAX_FB              4

typedef struct intel_gpu_info {
    uint16_t device_id;
    uint16_t revision;
    uint8_t gen;
    uint32_t flags;
} intel_gpu_info_t;

typedef struct intel_framebuffer {
    uint32_t id;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint8_t  bpp;
    void *vaddr;
    uint64_t paddr;
} intel_framebuffer_t;

typedef struct intel_gpu_device {
    pci_device_t *pci_dev;
    void *mmio;
    uint64_t mmio_phys;
    intel_gpu_info_t info;
    intel_framebuffer_t fbs[INTEL_MAX_FB];
    intel_framebuffer_t *current_fb;
    int fb_count;
    uint8_t initialized;
    display_output_t *display;
} intel_gpu_device_t;

void intel_gpu_init(pci_device_t *pci_dev);
void intel_gpu_irq_handler(void);
int intel_gpu_modeset(intel_gpu_device_t *dev, uint32_t w, uint32_t h, uint8_t bpp);
int intel_gpu_alloc_framebuffer(intel_gpu_device_t *dev, uint32_t w, uint32_t h, uint8_t bpp);
void intel_gpu_flush_framebuffer(intel_gpu_device_t *dev, uint32_t fb_id);
intel_gpu_info_t *intel_gpu_get_info(void);

#endif
