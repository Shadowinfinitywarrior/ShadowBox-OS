#ifndef SHADOWBOX_ANIMATION_H
#define SHADOWBOX_ANIMATION_H

#include "types.h"

// Animation Curves (Bezier Interpolation)
typedef struct bezier_curve {
    float p0_x, p0_y;
    float p1_x, p1_y;
    float p2_x, p2_y;
    float p3_x, p3_y;
} bezier_curve_t;

// Spring Physics System
typedef struct spring_physics {
    float mass;
    float tension;
    float damping;
    float velocity;
} spring_physics_t;

// Animation Engine Global State
typedef struct animation_engine {
    uint32_t target_hz; // 60, 90, 120, 144
    uint8_t reduced_motion; // Accessibility mode flag
    uint8_t gpu_offload_supported;
} animation_engine_t;

void animation_engine_init(void);
void animation_engine_set_hz(uint32_t hz);

// Compute next frame step for animations
float animation_step_spring(spring_physics_t *spring, float current_val, float target_val, float dt);
float animation_step_bezier(bezier_curve_t *curve, float progress); // progress 0.0 to 1.0

#endif
