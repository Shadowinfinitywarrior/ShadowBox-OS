#include "camera.h"
#include "kernel.h"
#include "malloc.h"
#include "kstring.h"

static cam_device_t *camera_list = 0;
static int camera_count = 0;

void camera_subsystem_init(void) {
    printk(KERN_INFO "CAMERA: Initialized V4L2-compatible Video Subsystem\n");
}

int camera_register(cam_device_t *cam) {
    if (!cam) return -1;
    
    // Auto-assign device node path if empty
    if (cam->node_path[0] == '\0') {
        char buf[16] = "/dev/camera";
        buf[11] = '0' + camera_count;
        buf[12] = '\0';
        for (int i = 0; i < 16; i++) cam->node_path[i] = buf[i];
    }
    
    // Initialize empty ring buffer
    cam->ring_size = CAM_MAX_BUFFERS;
    for (uint32_t i = 0; i < cam->ring_size; i++) {
        cam->ring_buffer[i].index = i;
        cam->ring_buffer[i].ready = 0; // Owned by producer (hardware)
    }
    
    cam->next = camera_list;
    camera_list = cam;
    camera_count++;
    
    printk(KERN_INFO "CAMERA: Registered %s (IR=%d)\n", cam->node_path, cam->is_ir_camera);
    return 0;
}

// Hardware IRQ callback when DMA transfer finishes
void camera_frame_ready_irq(cam_device_t *cam, uint32_t buffer_index, uint64_t timestamp) {
    if (!cam || buffer_index >= cam->ring_size) return;
    
    // Mark buffer as ready for consumer (userspace)
    cam->ring_buffer[buffer_index].timestamp = timestamp;
    cam->ring_buffer[buffer_index].ready = 1; 
}


/* --- Userspace API Implementations --- */

cam_device_t* cam_open(const char *path) {
    cam_device_t *curr = camera_list;
    while (curr) {
        int match = 1;
        for (int i = 0; path[i] || curr->node_path[i]; i++) {
            if (path[i] != curr->node_path[i]) { match = 0; break; }
        }
        if (match) return curr;
        curr = curr->next;
    }
    return 0; // Not found
}

void cam_close(cam_device_t *cam) {
    if (cam && cam->is_streaming) {
        cam_stream_off(cam);
    }
}

int cam_set_format(cam_device_t *cam, cam_format_t fmt, uint32_t width, uint32_t height, uint32_t fps) {
    if (!cam) return -1;
    if (cam->is_streaming) return -2; // Cannot change format while streaming
    
    if (!(cam->supported_formats & (1 << fmt))) {
        printk(KERN_WARN "CAMERA: Format %d not supported by device\n", fmt);
        return -3;
    }
    
    // Call down to hardware driver
    if (cam->set_format) {
        int ret = cam->set_format(cam, fmt, width, height, fps);
        if (ret != 0) return ret;
    }
    
    cam->current_fmt = fmt;
    cam->current_width = width;
    cam->current_height = height;
    cam->current_fps = fps;
    return 0;
}

int cam_stream_on(cam_device_t *cam) {
    if (!cam || cam->is_streaming) return -1;
    
    if (cam->has_privacy_led && cam->set_control) {
        cam->set_control(cam, CAM_CTRL_PRIVACY_LED, 1);
    }
    
    if (cam->stream_on) {
        cam->stream_on(cam);
    }
    
    cam->is_streaming = 1;
    return 0;
}

int cam_stream_off(cam_device_t *cam) {
    if (!cam || !cam->is_streaming) return -1;
    
    if (cam->stream_off) {
        cam->stream_off(cam);
    }
    
    if (cam->has_privacy_led && cam->set_control) {
        cam->set_control(cam, CAM_CTRL_PRIVACY_LED, 0);
    }
    
    cam->is_streaming = 0;
    return 0;
}

cam_frame_t* cam_dequeue_frame(cam_device_t *cam) {
    if (!cam || !cam->is_streaming) return 0;
    
    // Scan ring buffer for a ready frame (Zero-copy logic)
    for (uint32_t i = 0; i < cam->ring_size; i++) {
        if (cam->ring_buffer[i].ready == 1) {
            cam->ring_buffer[i].ready = 2; // Mark as held by userspace
            return &cam->ring_buffer[i];
        }
    }
    return 0; // No frames ready
}

void cam_enqueue_frame(cam_device_t *cam, cam_frame_t *frame) {
    if (!cam || !frame) return;
    
    // Return buffer to hardware producer pool
    frame->ready = 0;
}
