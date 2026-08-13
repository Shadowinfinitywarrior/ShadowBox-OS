#include "sys.h"
#include <errno.h>

/* Simple init system that launches input, window manager, and desktop services
 * as separate processes using sb_replicate (fork) and sb_morph (execve).
 * Each service is expected to be an ELF binary present in the initrd root.
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
	static const char *services[] = {
		"gui_demo.elf",
	};
	for (size_t i = 0; i < sizeof(services)/sizeof(services[0]); ++i) {
		start_service(services[i]);
	}
	// Emit test output to trigger syscall tracer
	sb_push(1, "TRACE_TEST\n", 11);
	// TRACE line removed

	/* Init process now becomes a simple reaper: wait for children */
	while (1) {
		// Wait for any child to exit; -1 means any pid, NULL status pointer, options=0
		sys_wait4((uint64_t)-1, 0, 0);
		// Yield the CPU
		syscall1(SYS_SCHED_YIELD, 0);
	}
}

/* ELF entry point wrapper — static to avoid duplicate global _start with head.S */
static void _start(void)
{
	init_main();
}
