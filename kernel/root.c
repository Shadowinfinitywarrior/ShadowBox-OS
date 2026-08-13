#include "root.h"
#include "session.h"
#include "kernel.h"
#include <string.h>

static user_session_t *root_session = NULL;

/* Initialize the root session with UID 0. */
void root_init(void) {
    if (root_session) return; // Already initialized
    root_session = session_create(0);
    if (root_session) {
        /* Mark as active and set default root attributes. */
        root_session->is_active = 1;
        strncpy(root_session->username, "root", sizeof(root_session->username) - 1);
        root_session->username[sizeof(root_session->username) - 1] = '\0';
        /* Set home directory to root, can be empty or "/". */
        strncpy(root_session->home_dir, "/", sizeof(root_session->home_dir) - 1);
        root_session->home_dir[sizeof(root_session->home_dir) - 1] = '\0';
        printk(KERN_INFO "[root] session initialized (uid=0)\n");
    } else {
        printk(KERN_ERR "[root] failed to allocate root session\n");
    }
}

user_session_t* get_root_session(void) {
    return root_session;
}
