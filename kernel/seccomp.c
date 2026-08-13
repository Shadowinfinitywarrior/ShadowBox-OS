#include "security.h"
#include "kernel.h"

/* Minimal stub implementation of seccomp support.
 * This file provides placeholder functions so that the kernel build
 * succeeds. Real seccomp functionality can be added later.
 */

/* Register seccomp LSM module – currently a no‑op.
 * Returns 0 on success.
 */
int seccomp_register(void) {
    return 0;
}

/* Check a syscall against the sandbox restrictions.
 * In this stub implementation the check always succeeds.
 */
int seccomp_check_syscall(int syscall_num) {
    (void)syscall_num;
    return 0;
}

/* Initialise the seccomp subsystem – no initialisation required now. */
void seccomp_init(void) {
    /* No operation */
}
