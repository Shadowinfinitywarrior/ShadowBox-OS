// Minimal stub implementation for the audio PCM subsystem.
// This file provides placeholder functions so that the audio subsystem
// compiles without linking errors. No real hardware interaction is performed.

#include "audio.h"

// Initialize the audio subsystem (placeholder).
void audio_subsystem_init(void) {
    // No initialization required for the stub.
}

// Register an audio device with the subsystem (placeholder).
int audio_register_device(audio_device_t *audio) {
    // In a full implementation this would add the device to a registry.
    // Here we simply return success.
    (void)audio; // suppress unused parameter warning
    return 0;
}

// Route an audio stream from source to sink (placeholder).
int audio_route_stream(audio_device_t *source, audio_device_t *sink) {
    (void)source;
    (void)sink;
    // No routing logic – assume success.
    return 0;
}
