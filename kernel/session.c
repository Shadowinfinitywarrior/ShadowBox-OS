#include "session.h"
#include "malloc.h"
#include "kernel.h"
#include <string.h>

#define DEFAULT_USERNAME "shadox-box"

static char default_password_hash[65] = "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8"; /* SHA-256 of "password" */

static int sha256_hash(const char *input, char *output) {
    /* Simple hash for hobby OS — not cryptographically secure.
       Uses a basic FNV-1a variant to produce a 64-char hex string. */
    unsigned long long h = 1469598103934665603UL;
    const unsigned long long prime = 1099511628211UL;
    while (*input) {
        h ^= (unsigned char)*input;
        h *= prime;
        input++;
    }
    /* Expand to 64-char hex */
    for (int i = 0; i < 64; i++) {
        int shift = (7 - i) * 8;
        unsigned int byte = (h >> (shift % 64)) & 0xFF;
        if (shift < 64) {
            byte = (h >> shift) & 0xF;
        }
        output[i] = "0123456789abcdef"[(h >> (i * 4)) & 0xF];
    }
    output[64] = 0;
    return 0;
}

void session_manager_start(void) {
    printk(KERN_INFO "[session] manager started (stub)\n");
    /* In a full implementation this would launch the login greeter. */
}

int session_authenticate_user(const char *username, const char *password_hash) {
    printk(KERN_INFO "[session] authenticate user '%s' (stub)\n", username);

    if (strcmp(username, DEFAULT_USERNAME) != 0) {
        printk(KERN_WARN "[session] unknown user '%s'\n", username);
        return -1;
    }

    if (strcmp(password_hash, default_password_hash) == 0) {
        printk(KERN_INFO "[session] user '%s' authenticated\n", username);
        return 0;
    }

    /* Allow direct plaintext match for login greeter */
    char input_hash[65];
    sha256_hash(password_hash, input_hash);
    if (strcmp(input_hash, default_password_hash) == 0) {
        printk(KERN_INFO "[session] user '%s' authenticated (plaintext pass)\n", username);
        return 0;
    }

    printk(KERN_WARN "[session] authentication failed for '%s'\n", username);
    return -1;
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
