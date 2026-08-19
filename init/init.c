#include "sys.h"
#include <errno.h>

/* Simple init system that launches the login screen as the primary
 * userland service. The login greeter authenticates the user and then
 * spawns the desktop shell (desktop.elf).
 * Each service is an ELF binary present in the initrd root.
 */

static void start_service(const char *path)
{
	uint64_t pid = sb_replicate();
	if (pid == (uint64_t)-1) {
		return;
	}

	if (pid == 0) {
		// child: replace image with the service binary
		sb_morph(path, 0, 0);
		// If exec fails, terminate with error status
		sb_terminate(1);
	}
}

/* _start is the entry point for the init ELF */
void init_main(void)
{
	// Launch the login screen first — it handles authentication
	// and spawns desktop.elf on successful login.
	start_service("login.elf");

	// Emit test output to trigger syscall tracer
	sb_push(1, "ShadowBox OS init: login.elf launched\n", 39);

	/* Init process now becomes a simple reaper: wait for children */
	while (1) {
		sys_wait4((uint64_t)-1, 0, 0);
		syscall1(SYS_SCHED_YIELD, 0);
	}
}

/* ELF entry point wrapper — static to avoid duplicate global _start with head.S */
static void _start(void)
{
	init_main();
}
