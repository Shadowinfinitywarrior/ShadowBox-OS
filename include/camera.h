#ifndef SHADOWBOX_CAMERA_H
#define SHADOWBOX_CAMERA_H

#include "types.h"
#include "device.h"

// Pixel Formats
typedef enum {
    CAM_FMT_YUYV = 1,
    CAM_FMT_MJPEG,
    CAM_FMT_H264,
    CAM_FMT_RAW,
    CAM_FMT_RGB24
} cam_format_t;

// Camera Controls
typedef enum {
    CAM_CTRL_EXPOSURE,
    CAM_CTRL_WHITE_BALANCE,
    CAM_CTRL_FOCUS,
    CAM_CTRL_PRIVACY_LED
} cam_ctrl_t;

#define CAM_MAX_BUFFERS 16

// Hardware DMA Frame Buffer
typedef struct {
    uint32_t index;
    void *buffer_ptr;    // Zero-copy mmap target
    size_t length;
    uint64_t timestamp;  // Frame timestamp
    uint32_t sequence;
    uint8_t  ready;      // 1 = Producer ready, 0 = Consumer ready
} cam_frame_t;

// Kernel Camera Device Definition
typedef struct cam_device {
    device_t *base_dev;
    char node_path[32]; // e.g. "/dev/camera0"
    
    // Capabilities
    uint32_t max_width, max_height;
    uint32_t supported_formats; // Bitmask of 1 << cam_format_t
    uint8_t is_ir_camera;       // Face ID / Windows Hello support
    uint8_t has_privacy_led;
    
    // Active State
    cam_format_t current_fmt;
    uint32_t current_width;
    uint32_t current_height;
    uint32_t current_fps;
    uint8_t is_streaming;
    
    // Ring Buffer (Producer/Consumer model)
    cam_frame_t ring_buffer[CAM_MAX_BUFFERS];
    uint32_t ring_size;
    
    // Driver Operations (V4L2-like interface)
    int (*set_format)(struct cam_device *cam, cam_format_t fmt, uint32_t w, uint32_t h, uint32_t fps);
    int (*set_control)(struct cam_device *cam, cam_ctrl_t ctrl, int value);
    int (*stream_on)(struct cam_device *cam);
    int (*stream_off)(struct cam_device *cam);
    
    void *driver_data; // Backend context (UVC, MIPI CSI)
    struct cam_device *next;
} cam_device_t;


/* --- Kernel Subsystem APIs --- */
void camera_subsystem_init(void);
int camera_register(cam_device_t *cam);
int camera_unregister(cam_device_t *cam);
void camera_frame_ready_irq(cam_device_t *cam, uint32_t buffer_index, uint64_t timestamp);


/* --- Userspace API Abstraction (System Calls) --- */
cam_device_t* cam_open(const char *path);
void cam_close(cam_device_t *cam);
int cam_set_format(cam_device_t *cam, cam_format_t fmt, uint32_t width, uint32_t height, uint32_t fps);
int cam_stream_on(cam_device_t *cam);
int cam_stream_off(cam_device_t *cam);
cam_frame_t* cam_dequeue_frame(cam_device_t *cam);
void cam_enqueue_frame(cam_device_t *cam, cam_frame_t *frame);

#endif
