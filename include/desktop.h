#ifndef SHADOWBOX_DESKTOP_H
#define SHADOWBOX_DESKTOP_H

#include "types.h"

// Notification Priority Levels
typedef enum {
    NOTIFY_PRIORITY_LOW,
    NOTIFY_PRIORITY_NORMAL,
    NOTIFY_PRIORITY_HIGH,
    NOTIFY_PRIORITY_URGENT
} notify_priority_t;

// Notification Daemon Node
typedef struct notification {
    uint32_t id;
    char app_name[64];
    char summary[128];
    char body[512];
    
    notify_priority_t priority;
    char group_id[64]; // For grouping related notifications (e.g. Chat app)
    
    // Interactive Action buttons
    char *action_labels[4];
    char *action_commands[4];
    uint8_t num_actions;
    
    struct notification *next;
} notification_t;

// App Launcher (Spotlight-style indexing)
typedef struct app_index_entry {
    char app_id[64];
    char display_name[64];
    char exec_path[256];
    char icon_path[256];
    uint64_t last_launched; // For computing "Recent" list
} app_index_entry_t;

// Daemon APIs
void notification_daemon_init(void);
uint32_t notification_send(notification_t *notify);
void notification_dismiss(uint32_t id);

/*
 * Compact, pointer-free snapshot of system status handed to userland via the
 * SYS_SYS_STATUS syscall. wifi_state mirrors wifi_state_t from wifi.h:
 *   0 uninitialized, 1 scanning, 2 associating, 3 associated, 4 connected, 5 disconnected
 */
typedef struct {
    uint8_t  wifi_state;
    char     wifi_ssid[33];
    int16_t  wifi_signal;    /* dBm */
    uint8_t  bt_available;   /* bluetooth stack present */
    uint8_t  bt_devices;     /* number of known BT devices */
    uint64_t uptime_ticks;
    uint64_t mem_total;
    uint64_t mem_used;
} sys_status_t;

/* Pointer-free snapshot of a pending notification (SYS_NOTIFY_PEEK). */
typedef struct {
    uint32_t id;
    uint8_t  priority;
    char     app_name[64];
    char     summary[128];
    char     body[512];
} sys_notify_t;

/* Notification daemon queries used by syscall layer. */
uint32_t notification_peek(sys_notify_t *out, uint32_t max);

void app_launcher_init(void);
app_index_entry_t** app_launcher_search(const char *query, uint32_t *result_count);
app_index_entry_t** app_launcher_get_recent(uint32_t max_results);

#endif
