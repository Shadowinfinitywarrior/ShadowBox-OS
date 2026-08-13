#include "kernel.h"
#include "types.h"

/* Minimal kernel module subsystem stub */

typedef struct kernel_module {
    const char *name;
    int (*init)(void);
    int (*exit)(void);
    struct kernel_module *next;
} kernel_module_t;

static kernel_module_t *module_list = NULL;

int register_kernel_module(kernel_module_t *mod) {
    if (!mod) return -1;
    mod->next = module_list;
    module_list = mod;
    printk(KERN_INFO "module: registered %s\n", mod->name);
    return 0;
}

void init_all_modules(void) {
    kernel_module_t *m = module_list;
    while (m) {
        if (m->init) m->init();
        m = m->next;
    }
}

void exit_all_modules(void) {
    kernel_module_t *m = module_list;
    while (m) {
        if (m->exit) m->exit();
        m = m->next;
    }
}

/* Placeholder module example (does nothing) */
static int dummy_init(void) { return 0; }
static int dummy_exit(void) { return 0; }

static kernel_module_t dummy_module = {
    .name = "dummy",
    .init = dummy_init,
    .exit = dummy_exit,
    .next = NULL,
};

/* Register dummy on kernel start */
__attribute__((constructor)) static void register_dummy_module(void) {
    register_kernel_module(&dummy_module);
}
