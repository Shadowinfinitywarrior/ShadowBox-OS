#ifndef SHADOWBOX_DIRECTORY_WATCH_H
#define SHADOWBOX_DIRECTORY_WATCH_H

/* Directory Watch Component
 * Provides a simple registration API for monitoring directory changes.
 * This is a minimal stub implementation; it records callbacks and can be
 * invoked manually via dirwatch_notify(). Integration with existing VFS
 * operations can be added later.
 */

#include "types.h"

/* Callback prototype: receives the watched path and an event string. */
typedef void (*dirwatch_cb_t)(const char *watched_path, const char *event);

/* Register a callback for a specific directory path.
 * Returns 0 on success, -1 on allocation failure.
 */
int dirwatch_register(const char *path, dirwatch_cb_t cb);

/* Unregister a previously registered callback.
 * Returns 0 on success, -1 if not found.
 */
int dirwatch_unregister(const char *path, dirwatch_cb_t cb);

/* Notify all registered watchers that an event occurred on a path.
 * This function is intended to be called by VFS code when a directory
 * changes (e.g., file create/delete). It iterates over the watch list
 * and calls matching callbacks.
 */
void dirwatch_notify(const char *path, const char *event);

#endif /* SHADOWBOX_DIRECTORY_WATCH_H */
