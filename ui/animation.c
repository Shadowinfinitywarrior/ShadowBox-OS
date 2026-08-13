/*
 * Animation subsystem implementation for ShadowBox OS.
 *
 * Provides a minimal spring and Bezier utility used by the Animation Viewer.
 */

#include "animation.h"

/* Global engine state */
static animation_engine_t engine = {
    .target_hz = 60,
    .reduced_motion = 0,
    .gpu_offload_supported = 0
};

void animation_engine_init(void) {
    /* Initialize with defaults – could be extended to read settings. */
    engine.target_hz = 60;
    engine.reduced_motion = 0;
    engine.gpu_offload_supported = 0;
}

void animation_engine_set_hz(uint32_t hz) {
    /* Clamp to a reasonable range (30‑144 Hz). */
    if (hz < 30) hz = 30;
    if (hz > 144) hz = 144;
    engine.target_hz = hz;
}

/* Spring physics step: basic Hooke's law with damping. */
float animation_step_spring(spring_physics_t *spring,
                           float current_val,
                           float target_val,
                           float dt) {
    if (!spring || dt <= 0.0f) {
        return current_val;
    }

    /* Compute displacement from target. */
    float displacement = current_val - target_val;

    /* Force = -k * displacement - c * velocity */
    float force = -spring->tension * displacement - spring->damping * spring->velocity;

    /* Acceleration = force / mass */
    float accel = force / spring->mass;

    /* Integrate velocity and position (explicit Euler). */
    spring->velocity += accel * dt;
    float new_val = current_val + spring->velocity * dt;

    return new_val;
}

/* Cubic Bezier evaluation – returns the interpolated Y value for a given t. */
float animation_step_bezier(bezier_curve_t *curve, float progress) {
    if (!curve) {
        return progress;
    }

    if (progress < 0.0f) progress = 0.0f;
    if (progress > 1.0f) progress = 1.0f;

    float t = progress;
    float u = 1.0f - t;

    /* Bernstein polynomial form. */
    float p0 = curve->p0_y;
    float p1 = curve->p1_y;
    float p2 = curve->p2_y;
    float p3 = curve->p3_y;

    float y =
        (u * u * u) * p0 +
        (3.0f * u * u * t) * p1 +
        (3.0f * u * t * t) * p2 +
        (t * t * t) * p3;

    return y;
}
