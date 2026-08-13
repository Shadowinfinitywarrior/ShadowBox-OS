// KernelConfigView.cpp — Implementation of the kernel configuration view component
#include "KernelConfigView.hpp"
#include "kconfig.h"

// Helper macros to turn macro values into strings.
#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)
#define KCONFIG_ENTRY(name) #name " = " STRINGIFY(name)

// List of config entries to display. The order mirrors include/kconfig.h.
static const char* const config_entries[] = {
    KCONFIG_ENTRY(CONFIG_MAX_CPUS),
    KCONFIG_ENTRY(CONFIG_MAX_PID),
    KCONFIG_ENTRY(CONFIG_MAX_FDS),
    KCONFIG_ENTRY(CONFIG_MAX_MOUNTS),
    KCONFIG_ENTRY(CONFIG_MAX_PIPES),
    KCONFIG_ENTRY(CONFIG_SCHED_GRANULARITY_NS),
    KCONFIG_ENTRY(CONFIG_KERNEL_STACK_SIZE),
    KCONFIG_ENTRY(CONFIG_HEAP_START),
    KCONFIG_ENTRY(CONFIG_HEAP_PTE),
    KCONFIG_ENTRY(CONFIG_USER_STACK_TOP),
    KCONFIG_ENTRY(CONFIG_USER_BRK_BASE),
    KCONFIG_ENTRY(CONFIG_VGA_BASE),
    KCONFIG_ENTRY(CONFIG_FRAMEBUFFER_WIDTH),
    KCONFIG_ENTRY(CONFIG_FRAMEBUFFER_HEIGHT),
    KCONFIG_ENTRY(CONFIG_FRAMEBUFFER_DEPTH),
    KCONFIG_ENTRY(CONFIG_SCHED_WEIGHT_NICE),
    KCONFIG_ENTRY(CONFIG_VFS_PATH_MAX),
    KCONFIG_ENTRY(CONFIG_INPUT_RING_SIZE),
    // Feature flags – all compile‑time booleans (1 = enabled)
    KCONFIG_ENTRY(CONFIG_SMP),
    KCONFIG_ENTRY(CONFIG_PREEMPT),
    KCONFIG_ENTRY(CONFIG_ASLR),
    KCONFIG_ENTRY(CONFIG_SMEP),
    KCONFIG_ENTRY(CONFIG_SMAP),
    KCONFIG_ENTRY(CONFIG_KASLR),
    KCONFIG_ENTRY(CONFIG_BUDDY_ALLOC),
    KCONFIG_ENTRY(CONFIG_BLOCK_CACHE),
    KCONFIG_ENTRY(CONFIG_DENTRY_CACHE),
    KCONFIG_ENTRY(CONFIG_RTL8139),
    KCONFIG_ENTRY(CONFIG_SCHED_CFS),
    KCONFIG_ENTRY(CONFIG_SCHED_RT),
    KCONFIG_ENTRY(CONFIG_STACK_CANARY),
    KCONFIG_ENTRY(CONFIG_WX_PROTECT),
    KCONFIG_ENTRY(CONFIG_IPC_SYSV),
};

KernelConfigView::KernelConfigView(Widget* parent)
    : Window(parent)
{
    // Basic window configuration
    set_title("Kernel Config");
    set_pos(200, 150);
    set_size(420, 300);

    // Scrollable area that will contain the config list
    scroll_view_ = new ScrollView(this);
    // Position the scroll view to fill the client area (simple approximation)
    scroll_view_->set_pos(0, 0);
    scroll_view_->set_size(400, 280);

    // Container widget for the labels – not visible itself, just holds children
    content_ = new Widget(scroll_view_);
    content_->set_pos(0, 0);

    const int entry_count = sizeof(config_entries) / sizeof(config_entries[0]);
    const int line_h = 20; // Approximate height for each label
    // Size container enough for all entries
    content_->set_size(380, entry_count * line_h + 4);

    // Create a label for each config entry.
    for (int i = 0; i < entry_count; ++i) {
        Label* lbl = new Label(content_);
        lbl->set_text(config_entries[i]);
        lbl->set_pos(5, i * line_h);
        lbl->set_size(370, line_h);
    }

    // Attach container to scroll view.
    scroll_view_->set_content(content_);
}
