#ifndef SHADOWBOX_KCONFIG_H
#define SHADOWBOX_KCONFIG_H

#define CONFIG_MAX_CPUS           4
#define CONFIG_MAX_PID            32768
#define CONFIG_MAX_FDS            256
#define CONFIG_MAX_MOUNTS         16
#define CONFIG_MAX_PIPES          64
#define CONFIG_SCHED_GRANULARITY_NS 10000000
#define CONFIG_KERNEL_STACK_SIZE  16384
#define CONFIG_HEAP_START         0xFFFFFFFFC1000000
#define CONFIG_HEAP_PTE           0xFFFFFFFF90000000
#define CONFIG_USER_STACK_TOP     0x8000000000
#define CONFIG_USER_BRK_BASE      0x60000000
#define CONFIG_VGA_BASE           0xFFFFFFFF800B8000
#define CONFIG_FRAMEBUFFER_WIDTH  1024
#define CONFIG_FRAMEBUFFER_HEIGHT 768
#define CONFIG_FRAMEBUFFER_DEPTH  32
#define CONFIG_SCHED_WEIGHT_NICE  1024
#define CONFIG_VFS_PATH_MAX       128
#define CONFIG_INPUT_RING_SIZE    256

#define CONFIG_SMP                1
#define CONFIG_PREEMPT            1
#define CONFIG_ASLR               1
#define CONFIG_SMEP               1
#define CONFIG_SMAP               1
#define CONFIG_KASLR              1
#define CONFIG_BUDDY_ALLOC        1
#define CONFIG_BLOCK_CACHE        1
#define CONFIG_DENTRY_CACHE       1
#define CONFIG_RTL8139            1
#define CONFIG_SCHED_CFS          1
#define CONFIG_SCHED_RT           1
#define CONFIG_STACK_CANARY       1
#define CONFIG_WX_PROTECT         1
#define CONFIG_IPC_SYSV           1

#endif
