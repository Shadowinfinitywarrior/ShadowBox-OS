/*
 * HDA audio driver for ShadowBox OS.
 * Registers an audio_device and provides stub PCM handling.
 */

#include "audio.h"
#include "kernel.h"

// Forward declarations (functions defined below)
static int hda_open_stream(audio_device_t *dev, audio_stream_dir_t dir, audio_format_t *fmt);
static int hda_close_stream(audio_device_t *dev, audio_stream_dir_t dir);
static int hda_write_pcm(audio_device_t *dev, void *buffer, size_t bytes);
static int hda_read_pcm(audio_device_t *dev, void *buffer, size_t bytes);

// Define the audio_device instance for HDA
static audio_device_t hda_audio_dev = {
    .base_dev = NULL,
    .name = "hda",
    .open_stream = hda_open_stream,
    .close_stream = hda_close_stream,
    .write_pcm = hda_write_pcm,
    .read_pcm = hda_read_pcm,
    .mixer = NULL,
    .routing_capabilities = 0,
};

static int hda_open_stream(audio_device_t *dev, audio_stream_dir_t dir, audio_format_t *fmt) {
    // Stub: accept any format and direction
    (void)dev; (void)dir; (void)fmt;
    return 0;
}

static int hda_close_stream(audio_device_t *dev, audio_stream_dir_t dir) {
    (void)dev; (void)dir;
    return 0;
}

static int hda_write_pcm(audio_device_t *dev, void *buffer, size_t bytes) {
    // Log the write operation via printk
    // Use printk to output bytes count; format level info
    printk(KERN_INFO "hda: write_pcm %zu bytes\n", bytes);
    (void)dev; (void)buffer;
    return (int)bytes; // pretend all bytes were written
}

static int hda_read_pcm(audio_device_t *dev, void *buffer, size_t bytes) {
    // Stub: no data to read
    (void)dev; (void)buffer; (void)bytes;
    return 0;
}

void audio_hda_init(void) {
    // Ensure audio subsystem is initialized
    audio_subsystem_init();
    // Register the HDA audio device
    if (audio_register_device(&hda_audio_dev) == 0) {
        printk(KERN_INFO "hda: audio device registered successfully\n");
    } else {
        printk(KERN_ERR "hda: audio device registration failed\n");
    }
}
