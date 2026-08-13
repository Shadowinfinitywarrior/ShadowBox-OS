#ifndef SHADOWBOX_SESSION_H
#define SHADOWBOX_SESSION_H

#include "types.h"

// Keyring (Secure credentials storage)
typedef struct keyring_entry {
    char service_name[64];
    char username[64];
    uint8_t *encrypted_secret;
    uint32_t secret_length;
    struct keyring_entry *next;
} keyring_entry_t;

// User Session State
typedef struct user_session {
    uint32_t uid;
    char username[32];
    char home_dir[256];
    
    // Environment Variables specific to user
    char **env_vars;
    
    // Session boundaries
    uint8_t is_active;
    keyring_entry_t *keyring; // Unlocked upon authentication
} user_session_t;

// Session Manager API
void session_manager_start(void); // Starts login screen (greeter)
int session_authenticate_user(const char *username, const char *password_hash);

// Session Lifecycle
user_session_t* session_create(uint32_t uid);
void session_destroy(user_session_t *session);

// Permission Agent (Polkit-like authentication elevation)
int permission_agent_request_auth(user_session_t *session, const char *action_id);

#endif
