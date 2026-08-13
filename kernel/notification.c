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
