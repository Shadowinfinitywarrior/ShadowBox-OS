// Enhanced Media Player for ShadowBox OS
// Features: Playlist support, playback controls, volume control, file info
// Compile with: $(CC) $(USER_CFLAGS) -no-pie -nostdlib userland/media_player_pcm.c -o media_player_pcm.elf
//
// Controls:
//   p - Play/Pause
//   s - Stop
//   n - Next track
//   b - Previous track
//   + - Volume up
//   - - Volume down
//   i - Show track info
//   l - Load playlist
//   a - Add to playlist
//   q - Quit

#include "sys.h"

#define MAX_PLAYLIST 32
#define MAX_PATH 256

static void print(const char *s) {
    sb_push(1, s, strlen(s));
}

static void print_uint(uint64_t val) {
    char buf[32];
    int idx = 0;
    if (val == 0) {
        buf[idx++] = '0';
    } else {
        while (val > 0 && idx < (int)sizeof(buf) - 1) {
            buf[idx++] = '0' + (val % 10);
            val /= 10;
        }
    }
    // reverse
    for (int i = idx - 1; i >= 0; i--) {
        sb_push(1, &buf[i], 1);
    }
}

static int readline(char *buf, int max) {
    int i = 0;
    while (i < max - 1) {
        char c;
        int r = sb_pull(0, &c, 1);
        if (r <= 0) break;
        if (c == '\n' || c == '\r') {
            sb_push(1, "\n", 1);
            break;
        }
        if (c == '\b' || c == 127) {
            if (i > 0) {
                sb_push(1, "\b \b", 3);
                i--;
            }
        } else {
            sb_push(1, &c, 1);
            buf[i++] = c;
        }
    }
    buf[i] = '\0';
    return i;
}

// Playlist management
static char playlist[MAX_PLAYLIST][MAX_PATH];
static int playlist_count = 0;
static int current_track = 0;
static int is_playing = 0;
static int volume = 50; // 0-100

static void add_to_playlist(const char *path) {
    if (playlist_count < MAX_PLAYLIST) {
        int i = 0;
        while (path[i] && i < MAX_PATH - 1) {
            playlist[playlist_count][i] = path[i];
            i++;
        }
        playlist[playlist_count][i] = '\0';
        playlist_count++;
        print("Added to playlist: ");
        print(path);
        print("\n");
    } else {
        print("Playlist full\n");
    }
}

static void show_playlist(void) {
    if (playlist_count == 0) {
        print("Playlist empty\n");
        return;
    }
    print("\n=== Playlist ===\n");
    for (int i = 0; i < playlist_count; i++) {
        if (i == current_track) print(" > ");
        else print("   ");
        print_uint(i + 1);
        print(". ");
        print(playlist[i]);
        print("\n");
    }
}

static void show_track_info(const char *path) {
    int fd = sb_acquire(path, 0);
    if (fd < 0) {
        print("Cannot open file\n");
        return;
    }
    
    char buf[512];
    uint64_t total = 0;
    while (1) {
        int r = sb_pull(fd, buf, sizeof(buf));
        if (r <= 0) break;
        total += r;
    }
    sb_release(fd);
    
    print("\n=== Track Info ===\n");
    print("File: ");
    print(path);
    print("\nSize: ");
    print_uint(total);
    print(" bytes\n");
    
    // Estimate duration (assuming 8kHz mono 16-bit PCM)
    uint64_t duration = total / 16000; // rough estimate in seconds
    print("Duration: ~");
    print_uint(duration);
    print(" seconds\n");
}

static void play_track(const char *path) {
    int fd = sb_acquire(path, 0);
    if (fd < 0) {
        print("Failed to open file: ");
        print(path);
        print("\n");
        return;
    }
    
    print("\nPlaying: ");
    print(path);
    print("\n");
    
    char buf[512];
    uint64_t total = 0;
    is_playing = 1;
    
    while (is_playing) {
        int r = sb_pull(fd, buf, sizeof(buf));
        if (r <= 0) break;
        total += r;
        // In a real implementation, we would send audio data to the audio device
        // For now, we just simulate playback by reading the data
    }
    
    sb_release(fd);
    is_playing = 0;
    
    print("Played ");
    print_uint(total);
    print(" bytes\n");
}

static void next_track(void) {
    if (playlist_count == 0) {
        print("No tracks in playlist\n");
        return;
    }
    current_track = (current_track + 1) % playlist_count;
    print("Next track: ");
    print(playlist[current_track]);
    print("\n");
}

static void prev_track(void) {
    if (playlist_count == 0) {
        print("No tracks in playlist\n");
        return;
    }
    current_track = (current_track - 1 + playlist_count) % playlist_count;
    print("Previous track: ");
    print(playlist[current_track]);
    print("\n");
}

static void volume_up(void) {
    if (volume < 100) {
        volume += 10;
        if (volume > 100) volume = 100;
        print("Volume: ");
        print_uint(volume);
        print("%\n");
    }
}

static void volume_down(void) {
    if (volume > 0) {
        volume -= 10;
        if (volume < 0) volume = 0;
        print("Volume: ");
        print_uint(volume);
        print("%\n");
    }
}

void _start(void) {
    print("\033[2J\033[H"); // Clear screen
    print("\033[1;36m");
    print("=== Enhanced Media Player ===\033[0m\n");
    print("Controls: p=play/pause, s=stop, n=next, b=prev, +=vol up, -=vol down\n");
    print("          i=info, l=list, a=add, q=quit\n\n");
    
    while (1) {
        print("\033[1;33m");
        print("Volume: ");
        print_uint(volume);
        print("% | ");
        if (is_playing) print("Playing");
        else print("Stopped");
        print("\033[0m\n");
        
        if (playlist_count > 0) {
            print("Current: ");
            print_uint(current_track + 1);
            print("/");
            print_uint(playlist_count);
            print(" - ");
            print(playlist[current_track]);
            print("\n");
        } else {
            print("No tracks loaded\n");
        }
        
        print("\n> ");
        
        char c;
        if (sb_pull(0, &c, 1) <= 0) break;
        
        if (c == 'p' || c == 'P') {
            if (playlist_count > 0) {
                if (is_playing) {
                    is_playing = 0;
                    print("Paused\n");
                } else {
                    play_track(playlist[current_track]);
                }
            } else {
                print("No tracks in playlist. Add a track first.\n");
            }
        } else if (c == 's' || c == 'S') {
            is_playing = 0;
            print("Stopped\n");
        } else if (c == 'n' || c == 'N') {
            next_track();
        } else if (c == 'b' || c == 'B') {
            prev_track();
        } else if (c == '+') {
            volume_up();
        } else if (c == '-') {
            volume_down();
        } else if (c == 'i' || c == 'I') {
            if (playlist_count > 0) {
                show_track_info(playlist[current_track]);
            } else {
                print("No track loaded\n");
            }
        } else if (c == 'l' || c == 'L') {
            show_playlist();
        } else if (c == 'a' || c == 'A') {
            print("Enter file path: ");
            char path[MAX_PATH];
            readline(path, sizeof(path));
            if (path[0] != '\0') {
                add_to_playlist(path);
                if (playlist_count == 1) {
                    current_track = 0;
                }
            }
        } else if (c == 'q' || c == 'Q') {
            print("Goodbye!\n");
            break;
        }
    }
    
    sb_terminate(0);
}
