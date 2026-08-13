#include "directory_watch.h"
#include "malloc.h"
#include "kstring.h"

/* Simple linked list of watch entries */
typedef struct dirwatch_entry {
    char path[256];
    dirwatch_cb_t cb;
    struct dirwatch_entry *next;
} dirwatch_entry_t;

static dirwatch_entry_t *watch_list = NULL;

int dirwatch_register(const char *path, dirwatch_cb_t cb) {
    if (!path || !cb) return -1;
    /* Allocate a new entry */
    dirwatch_entry_t *e = (dirwatch_entry_t *)kmalloc(sizeof(dirwatch_entry_t));
    if (!e) return -1;
    /* Copy path, truncate if necessary */
    strncpy(e->path, path, sizeof(e->path) - 1);
    e->path[sizeof(e->path) - 1] = '\0';
    e->cb = cb;
    e->next = watch_list;
    watch_list = e;
    return 0;
}

int dirwatch_unregister(const char *path, dirwatch_cb_t cb) {
    if (!path || !cb) return -1;
    dirwatch_entry_t *prev = NULL;
    dirwatch_entry_t *cur = watch_list;
    while (cur) {
        if (strcmp(cur->path, path) == 0 && cur->cb == cb) {
            if (prev) prev->next = cur->next; else watch_list = cur->next;
            kfree(cur);
            return 0;
        }
        prev = cur;
        cur = cur->next;
    }
    return -1;
}

void dirwatch_notify(const char *path, const char *event) {
    if (!path || !event) return;
    dirwatch_entry_t *cur = watch_list;
    while (cur) {
        if (strcmp(cur->path, path) == 0) {
            cur->cb(cur->path, event);
        }
        cur = cur->next;
    }
}
