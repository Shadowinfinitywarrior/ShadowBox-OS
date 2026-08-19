#include "desktop.h"
#include "malloc.h"

/* Simple in-memory notification daemon.
 * Stores notifications in a linked list. No synchronization is
 * performed – this is sufficient for the current single‑threaded
 * kernel build.
 */

static notification_t *notification_head = NULL;
static uint32_t next_notify_id = 1;

void notification_daemon_init(void) {
    notification_head = NULL;
    next_notify_id = 1;
}

uint32_t notification_send(notification_t *notify) {
    if (!notify)
        return 0;
    notification_t *node = (notification_t *)kmalloc(sizeof(notification_t));
    if (!node)
        return 0;
    /* Shallow copy – string fields are copied by value and pointer
     * fields (action_labels/commands) are kept as‑is.
     */
    *node = *notify;
    node->id = next_notify_id++;
    node->next = NULL;
    /* Insert at head for O(1) insertion. */
    node->next = notification_head;
    notification_head = node;
    return node->id;
}

void notification_dismiss(uint32_t id) {
    notification_t *prev = NULL;
    notification_t *cur = notification_head;
    while (cur) {
        if (cur->id == id) {
            if (prev)
                prev->next = cur->next;
            else
                notification_head = cur->next;
            kfree(cur);
            return;
        }
        prev = cur;
        cur = cur->next;
    }
    /* Not found – silently ignore. */
}

/* Copy up to `max` pending notifications into `out` (newest first).
 * Returns the number of notifications copied. */
uint32_t notification_peek(sys_notify_t *out, uint32_t max) {
    if (!out || max == 0)
        return 0;
    uint32_t n = 0;
    notification_t *cur = notification_head;
    while (cur && n < max) {
        sys_notify_t *dst = &out[n];
        dst->id = cur->id;
        dst->priority = (uint8_t)cur->priority;
        for (int i = 0; i < 63 && cur->app_name[i]; i++) dst->app_name[i] = cur->app_name[i];
        dst->app_name[63] = 0;
        for (int i = 0; i < 127 && cur->summary[i]; i++) dst->summary[i] = cur->summary[i];
        dst->summary[127] = 0;
        for (int i = 0; i < 511 && cur->body[i]; i++) dst->body[i] = cur->body[i];
        dst->body[511] = 0;
        n++;
        cur = cur->next;
    }
    return n;
}
