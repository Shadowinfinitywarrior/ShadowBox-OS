#ifndef SHADOWBOX_KERNEL_H
#define SHADOWBOX_KERNEL_H

#include "types.h"
#include "io.h"
#include "compiler.h"
#include "kconfig.h"

#ifndef SHADOWBOX_VERSION
#define SHADOWBOX_VERSION      "0.2.0"
#endif
#define SHADOWBOX_NAME         "ShadowBox"
#define SHADOWBOX_COPYRIGHT    "Copyright (c) 2026 ShadowBox Contributors"
#define SHADOWBOX_ARCH         "x86_64"

#define KERN_EMERG   "<0>" 
#define KERN_ALERT   "<1>"
#define KERN_CRIT    "<2>"
#define KERN_ERR     "<3>"
#define KERN_WARN    "<4>"
#define KERN_NOTICE  "<5>"
#define KERN_INFO    "<6>"
#define KERN_DEBUG   "<7>"

void printk(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void NO_RETURN panic(const char *msg);

#endif
