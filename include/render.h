#ifndef SHADOWBOX_RENDER_H
#define SHADOWBOX_RENDER_H

#include "types.h"
#include "compositor.h"

// Dark/Light mode theme tokens
typedef struct theme_tokens {
    uint8_t is_dark_mode;
    uint32_t color_background;
    uint32_t color_surface;
    uint32_t color_primary;
    uint32_t color_text;
} theme_tokens_t;

// Display List Command (App CPU Thread output -> GPU Thread input)
typedef enum {
    DL_CMD_DRAW_RECT,
    DL_CMD_DRAW_TEXT,
    DL_CMD_DRAW_PATH,
    DL_CMD_APPLY_SHADER
} display_cmd_type_t;

typedef struct display_list_node {
    display_cmd_type_t type;
    void *cmd_data;
    struct display_list_node *next;
} display_list_node_t;

// Rendering Backend Context (Compositor GPU Thread)
typedef struct {
    wl_buffer_t *target;
    theme_tokens_t theme;
    uint8_t hw_accelerated;
} render_context_t;

void render_engine_init(void);

// App Thread: Construct Display List during Paint pass
display_list_node_t* render_create_display_list(void);
void render_append_dl(display_list_node_t *dl, display_cmd_type_t type, void *data);

// Compositor GPU Thread: Rasterize & Composite Display List -> Framebuffer -> VSync Flip
void render_execute_display_list(render_context_t *ctx, display_list_node_t *dl);

// GPU-Accelerated Shaders & Effects (Rasterize stage)
void render_shader_acrylic_glass(render_context_t *ctx, float x, float y, float w, float h); // Frosted glass + Gaussian blur
void render_shader_sdf_rounded_corners(render_context_t *ctx, float x, float y, float w, float h, float radius);
void render_shader_drop_shadow(render_context_t *ctx, float x, float y, float w, float h, float elevation);
void render_shader_alpha_composite(render_context_t *ctx, float alpha);

#endif
