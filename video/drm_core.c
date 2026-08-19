#include "drm.h"
#include "kernel.h"
#include "pci.h"
#include "vmm.h"
#include "pmm.h"
#include "kstring.h"
#include "malloc.h"

#define DRM_MAX_DEVICES    8
#define DRM_MAX_CONNECTORS 16
#define DRM_MAX_CRTC       8
#define DRM_MAX_PLANES     16

static drm_driver_t *drm_drivers[DRM_MAX_DEVICES];
static device_t *drm_devices[DRM_MAX_DEVICES];
static uint32_t drm_driver_count = 0;

static drm_crtc_t crtc_table[DRM_MAX_CRTC];
static drm_encoder_t encoder_table[DRM_MAX_CRTC];
static drm_connector_t connector_table[DRM_MAX_CONNECTORS];
static drm_gem_object_t gem_objects[DRM_MAX_DEVICES];
static uint32_t crtc_count = 0;
static uint32_t encoder_count = 0;
static uint32_t connector_count = 0;

void drm_init(void) {
    printk(KERN_INFO "DRM: Initializing Direct Rendering Manager subsystem\n");
    memset(drm_drivers, 0, sizeof(drm_drivers));
    memset(drm_devices, 0, sizeof(drm_devices));
    memset(crtc_table, 0, sizeof(crtc_table));
    memset(encoder_table, 0, sizeof(encoder_table));
    memset(connector_table, 0, sizeof(connector_table));
    memset(gem_objects, 0, sizeof(gem_objects));
    drm_driver_count = 0;
    crtc_count = 0;
    encoder_count = 0;
    connector_count = 0;
    printk(KERN_INFO "DRM: Subsystem initialized (max_devices=%d)\n", DRM_MAX_DEVICES);
}

int drm_register_device(device_t *dev, drm_driver_t *driver) {
    if (!dev || !driver || drm_driver_count >= DRM_MAX_DEVICES)
        return -1;

    drm_devices[drm_driver_count] = dev;
    drm_drivers[drm_driver_count] = driver;

    /* Initialize GEM object for this device */
    gem_objects[drm_driver_count].handle = drm_driver_count;
    gem_objects[drm_driver_count].size = 0;
    gem_objects[drm_driver_count].vaddr = NULL;
    gem_objects[drm_driver_count].paddr = 0;

    /* Setup default CRTC */
    if (crtc_count < DRM_MAX_CRTC) {
        crtc_table[crtc_count].id = crtc_count;
        crtc_table[crtc_count].current_fb = 0;
        crtc_table[crtc_count].width = 1024;
        crtc_table[crtc_count].height = 768;
        encoder_table[encoder_count].id = encoder_count;
        encoder_table[encoder_count].crtc_id = crtc_count;
        encoder_count++;
        crtc_count++;
    }

    /* Setup default connector */
    if (connector_count < DRM_MAX_CONNECTORS) {
        connector_table[connector_count].id = connector_count;
        connector_table[connector_count].encoder_id = 0;
        connector_table[connector_count].type = DISP_CONN_DVI;
        connector_table[connector_count].connected = 1;
        connector_count++;
    }

    drm_driver_count++;
    dev->driver_data = driver;

    printk(KERN_INFO "DRM: Registered device '%s' with driver (id=%u, crtcs=%u, conn=%u)\n",
           dev->name, drm_driver_count - 1, crtc_count, connector_count);
    return 0;
}

int drm_gem_create(uint32_t dev_index, uint64_t size, drm_gem_object_t *obj) {
    if (dev_index >= drm_driver_count || !obj || !size)
        return -1;

    uint64_t phys = (uint64_t)pmm_alloc_page();
    if (!phys) return -1;

    void *vaddr = vmap_phys(phys, 4096);
    if (!vaddr) {
        pmm_free_page((void *)phys);
        return -1;
    }

    memset(vaddr, 0, size > 4096 ? 4096 : size);

    obj->handle = (uint32_t)(uintptr_t)vaddr;
    obj->size = size;
    obj->vaddr = vaddr;
    obj->paddr = phys;

    printk(KERN_DEBUG "DRM: Created GEM object handle=0x%x size=%llu phys=0x%llx\n",
           obj->handle, size, phys);
    return 0;
}

int drm_gem_map(uint32_t dev_index, drm_gem_object_t *obj) {
    if (dev_index >= drm_driver_count || !obj || !obj->paddr)
        return -1;

    void *vaddr = vmap_phys(obj->paddr, obj->size);
    if (!vaddr) return -1;

    obj->vaddr = vaddr;
    return 0;
}

int drm_gem_free(uint32_t dev_index, drm_gem_object_t *obj) {
    if (dev_index >= drm_driver_count || !obj)
        return -1;

    if (obj->vaddr) {
        /* Cannot truly unmap in this implementation */
    }
    if (obj->paddr) {
        pmm_free_page((void *)obj->paddr);
    }

    memset(obj, 0, sizeof(drm_gem_object_t));
    return 0;
}

int drm_modeset(uint32_t dev_index, drm_crtc_t *crtc, drm_connector_t *conn, uint32_t fb_id) {
    if (dev_index >= drm_driver_count || !crtc || !conn)
        return -1;

    drm_driver_t *driver = drm_drivers[dev_index];
    if (driver && driver->modeset) {
        return driver->modeset(drm_devices[dev_index], crtc, conn, fb_id);
    }

    /* Default modeset: update CRTC */
    crtc->current_fb = fb_id;
    conn->encoder_id = 0;

    printk(KERN_INFO "DRM: Modeset CRTC %u -> FB %u (connector %u, %zux%zu)\n",
           crtc->id, fb_id, conn->id, crtc->width, crtc->height);
    return 0;
}

int drm_submit_cmdbuf(uint32_t dev_index, void *cmd_buf, size_t size, uint32_t *fence_id) {
    if (dev_index >= drm_driver_count || !cmd_buf || !size)
        return -1;

    drm_driver_t *driver = drm_drivers[dev_index];
    if (driver && driver->submit_cmd_buffer) {
        return driver->submit_cmd_buffer(drm_devices[dev_index], cmd_buf, size, fence_id);
    }

    /* Default: no-op command submission */
    if (fence_id) *fence_id = 0;
    return 0;
}

uint32_t drm_get_device_count(void) {
    return drm_driver_count;
}

drm_driver_t *drm_get_driver(uint32_t index) {
    if (index >= drm_driver_count) return NULL;
    return drm_drivers[index];
}

device_t *drm_get_device(uint32_t index) {
    if (index >= drm_driver_count) return NULL;
    return drm_devices[index];
}

uint32_t drm_get_crtc_count(void) {
    return crtc_count;
}

uint32_t drm_get_connector_count(void) {
    return connector_count;
}
