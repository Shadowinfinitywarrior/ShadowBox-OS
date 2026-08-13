#include "session.h"
#include "malloc.h"
#include "kernel.h"
#include <string.h>

/*
 * Minimal stub implementation of session management for the kernel.
 * Provides basic skeleton functions declared in include/session.h.
 * No real authentication or keyring handling – just enough to compile.
 */

void session_manager_start(void) {
    printk(KERN_INFO "[session] manager started (stub)\n");
    /* In a full implementation this would launch the login greeter. */
}

int session_authenticate_user(const char *username, const char *password_hash) {
    printk(KERN_INFO "[session] authenticate user %s (stub)\n", username);
    /* Always succeed for the stub. */
    return 0;
}

user_session_t* session_create(uint32_t uid) {
    user_session_t *sess = (user_session_t *)kmalloc(sizeof(user_session_t));
    if (!sess) {
        printk(KERN_ERR "[session] allocation failed\n");
        return NULL;
    }
    memset(sess, 0, sizeof(user_session_t));
    sess->uid = uid;
    sess->is_active = 0;
    sess->keyring = NULL;
    sess->env_vars = NULL;
    printk(KERN_INFO "[session] created session uid=%u (stub)\n", uid);
    return sess;
}

void session_destroy(user_session_t *session) {
    if (!session) return;
    /* In a real system we would free env_vars and keyring entries. */
    kfree(session);
    printk(KERN_INFO "[session] destroyed session (stub)\n");
}

int permission_agent_request_auth(user_session_t *session, const char *action_id) {
    printk(KERN_INFO "[session] permission request for action %s (stub)\n", action_id);
    /* Stub always grants permission. */
    return 0;
}
