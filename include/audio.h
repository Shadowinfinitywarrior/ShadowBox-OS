#ifndef SHADOWBOX_AUDIO_H
#define SHADOWBOX_AUDIO_H

#include "types.h"
#include "device.h"

// Audio Stream Direction
typedef enum {
    AUDIO_STREAM_PLAYBACK,
    AUDIO_STREAM_CAPTURE
} audio_stream_dir_t;

// PCM Stream Format
typedef struct {
    uint32_t sample_rate; // e.g., 44100, 48000
    uint8_t channels;     // 1 = Mono, 2 = Stereo, 6 = 5.1
    uint8_t bit_depth;    // 16, 24, 32
} audio_format_t;

// Audio Mixer / Volume Node
typedef struct audio_mixer {
    const char *name;
    uint8_t master_volume; // 0 - 100
    uint8_t muted;
    int (*set_volume)(struct audio_mixer *mixer, uint8_t vol);
    int (*set_mute)(struct audio_mixer *mixer, uint8_t mute);
} audio_mixer_t;

// Audio Device (HDA, UAC, HDMI)
typedef struct audio_device {
    device_t *base_dev;
    const char *name;
    
    // Streams
    int (*open_stream)(struct audio_device *dev, audio_stream_dir_t dir, audio_format_t *fmt);
    int (*close_stream)(struct audio_device *dev, audio_stream_dir_t dir);
    int (*write_pcm)(struct audio_device *dev, void *buffer, size_t bytes);
    int (*read_pcm)(struct audio_device *dev, void *buffer, size_t bytes);
    
    // Mixer Node
    audio_mixer_t *mixer;
    
    // Routing Graph
    uint32_t routing_capabilities;
} audio_device_t;

void audio_subsystem_init(void);
int audio_register_device(audio_device_t *audio);

// Routing Graph API
int audio_route_stream(audio_device_t *source, audio_device_t *sink);

#endif
