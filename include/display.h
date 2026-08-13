#ifndef SHADOWBOX_DISPLAY_H
#define SHADOWBOX_DISPLAY_H

#include "types.h"
#include "device.h"

// Connector Types
typedef enum {
    DISP_CONN_VGA,
    DISP_CONN_DVI,
    DISP_CONN_HDMI,
    DISP_CONN_DISPLAYPORT,
    DISP_CONN_USB_C_ALT,
    DISP_CONN_THUNDERBOLT
} display_connector_t;

// EDID / Monitor Capabilities
typedef struct {
    char manufacturer[4];
    uint16_t product_code;
    uint32_t serial_number;
    uint16_t max_width;
    uint16_t max_height;
    uint8_t refresh_rates[8];
    uint8_t supports_hdr;
} display_edid_t;

// Display Device (Monitor Output)
typedef struct display_output {
    device_t *base_dev;
    display_connector_t type;
    uint8_t connected;
    
    display_edid_t edid;
    
    // Current Mode
    uint32_t current_width;
    uint32_t current_height;
    uint8_t current_refresh; // Hz
    
    // Multi-monitor layout coordinates (Virtual Desktop Space)
    int32_t virtual_x;
    int32_t virtual_y;
    
    int (*set_mode)(struct display_output *disp, uint32_t w, uint32_t h, uint8_t refresh);
    int (*dpms_set)(struct display_output *disp, int state); // DPMS Suspend/On
} display_output_t;

void display_subsystem_init(void);
int display_register_output(display_output_t *disp);

// Multi-monitor Layout Management
void display_set_layout(display_output_t *disp, int32_t x, int32_t y);

#endif
