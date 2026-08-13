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

void app_launcher_init(void);
app_index_entry_t** app_launcher_search(const char *query, uint32_t *result_count);
app_index_entry_t** app_launcher_get_recent(uint32_t max_results);

#endif
