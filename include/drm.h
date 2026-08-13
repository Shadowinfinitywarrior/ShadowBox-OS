#ifndef SHADOWBOX_DRM_H
#define SHADOWBOX_DRM_H

#include "types.h"
#include "device.h"
#include "display.h" 

// Graphics Execution Manager (GEM) Memory Object
typedef struct drm_gem_object {
    uint32_t handle;
    uint64_t size;
    void *vaddr;     // CPU mapped address
    uint64_t paddr;  // GPU physical/virtual address
} drm_gem_object_t;

// KMS Pipeline: CRTC -> Encoder -> Connector
typedef struct drm_crtc {
    uint32_t id;
    uint32_t current_fb;
    uint32_t width, height;
} drm_crtc_t;

typedef struct drm_encoder {
    uint32_t id;
    uint32_t crtc_id;
} drm_encoder_t;

typedef struct drm_connector {
    uint32_t id;
    uint32_t encoder_id;
    display_connector_t type;
    uint8_t connected;
} drm_connector_t;

// GPU Driver Operations (Hardware specific i915, amdgpu, nouveau)
typedef struct drm_driver {
    // Memory Management (GEM / TTM)
    int (*gem_create)(device_t *dev, uint64_t size, drm_gem_object_t *obj);
    int (*gem_map)(device_t *dev, drm_gem_object_t *obj);
    int (*gem_free)(device_t *dev, drm_gem_object_t *obj);
    
    // Command Buffer & Synchronization
    int (*submit_cmd_buffer)(device_t *dev, void *cmd_buf, size_t size, uint32_t *fence_id);
    int (*wait_fence)(device_t *dev, uint32_t fence_id, uint64_t timeout_ns);
    
    // Kernel Mode Setting (KMS)
    int (*modeset)(device_t *dev, drm_crtc_t *crtc, drm_connector_t *conn, uint32_t fb_id);
} drm_driver_t;

void drm_init(void);
int drm_register_device(device_t *dev, drm_driver_t *driver);

#endif
