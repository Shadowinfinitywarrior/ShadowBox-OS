#ifndef SHADOWBOX_ROOT_H
#define SHADOWBOX_ROOT_H

#include "session.h"

/* Initialize the root user session (UID 0). */
void root_init(void);

/* Retrieve the global root session. */
user_session_t* get_root_session(void);

#endif // SHADOWBOX_ROOT_H
