#ifndef SHADOWBOX_ASSERT_H
#define SHADOWBOX_ASSERT_H

#include "types.h"
#include "kernel.h"

/*
 * BUG - Trigger a fatal kernel bug, halt the system
 * Prints bug info and stops execution
 */
#define BUG() do {                                      \
    printk(KERN_CRIT "BUG: at %s:%d in %s()\n",         \
           __FILE__, __LINE__, __func__);                \
    __asm__ volatile("cli; hlt");                       \
    while (1) __asm__ volatile("hlt");                  \
} while (0)

/*
 * BUG_ON - Conditionally trigger a fatal bug
 * @cond: Condition that should never be true
 */
#define BUG_ON(cond) do {                               \
    if (unlikely(cond)) {                                \
        printk(KERN_CRIT "BUG_ON(%s) at %s:%d in %s()\n", \
               #cond, __FILE__, __LINE__, __func__);     \
        __asm__ volatile("cli; hlt");                   \
        while (1) __asm__ volatile("hlt");              \
    }                                                   \
} while (0)

/*
 * WARN_ON - Issue a warning if condition is true
 * @cond: Condition to check
 * Returns: The value of cond
 */
#define WARN_ON(cond) ({                                \
    int __ret = !!(cond);                               \
    if (unlikely(__ret))                                 \
        printk(KERN_WARN "WARN_ON(%s) at %s:%d in %s()\n", \
               #cond, __FILE__, __LINE__, __func__);     \
    __ret;                                              \
})

/*
 * WARN - Issue a warning with formatted message
 * @cond: Condition to check
 * @fmt:  Printf-style format string
 * @...:  Format arguments
 */
#define WARN(cond, fmt, ...) do {                       \
    if (unlikely(cond))                                  \
        printk(KERN_WARN "WARN: " fmt " at %s:%d\n",    \
               ##__VA_ARGS__, __FILE__, __LINE__);       \
} while (0)

#endif
