#include "types.h"
#include "kernel.h"
#include "serial.h"
#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "keyboard.h"
#include "tty.h"
#include "vfs.h"
#include "pit.h"
#include "pmm.h"
#include "vmm.h"
#include "task.h"
#include "syscall.h"
#include "elf.h"
#include "multiboot2.h"
#include "tarfs.h"
#include "malloc.h"
#include "pci.h"
#include "apic.h"
#include "slab.h"
#include "block.h"
#include "ext2.h"
#include "devfs.h"
#include "smp.h"
#include "ipc.h"
#include "net.h"
#include "acpi.h"
#include "ahci.h"
#include "usb.h"
#include "fb.h"
#include "e1000.h"
#include "rtc.h"
#include "mmap.h"
#include "swap.h"
#include "procfs.h"
#include "sysfs.h"
#include "signal.h"
#include "kthread.h"
#include "socket.h"
#include "sched.h"
#include "tmpfs.h"
#include "aslr.h"
#include "buddy.h"
#include "rtl8139.h"
#include "hda.h"

void __stack_chk_fail(void) {
    panic("Stack smashing detected");
}

void init_user_thread(void *arg) {
    (void)arg;
    struct process *cur = get_current_process();
    if (!fs_root) {
        printk(KERN_ERR "No filesystem root, cannot load shell\n");
        return;
    }

    vfs_node_t *shell_node = vfs_finddir(fs_root, "desktop.elf");
    if (!shell_node) {
        printk(KERN_ERR "Failed to find desktop.elf in initrd!\n");
        return;
    }

    uint8_t *shell_data = kmalloc(shell_node->length);
    vfs_read(shell_node, 0, shell_node->length, shell_data);

    uint64_t entry_point = 0;
    if (elf_load_segments(get_current_process(), shell_data, &entry_point) < 0) {
        printk(KERN_ERR "Failed to load shell ELF!\n");
        kfree(shell_data);
        return;
    }
    kfree(shell_data);

    uint64_t user_stack = aslr_get_stack_base();
    __asm__ volatile("mov %0, %%cr3" :: "r"(cur->cr3) : "memory");

    for (int i = 0; i < 6; i++) {
        uint64_t phys = (uint64_t)pmm_alloc_page();
        vmm_map_page(phys, user_stack - i * 0x1000, PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
    }

    static const uint8_t exit_trampoline[] = {
        0x48, 0xc7, 0xc0, 0x3c, 0x00, 0x00, 0x00,
        0x48, 0xc7, 0xc7, 0x00, 0x00, 0x00, 0x00,
        0x0f, 0x05
    };
    uint8_t *trampoline = (uint8_t *)(user_stack - 32);
    for (int i = 0; i < sizeof(exit_trampoline); i++) trampoline[i] = exit_trampoline[i];

    uint64_t initial_rsp = user_stack - 64 - 8;
    initial_rsp &= ~0xF;

    uint64_t *ustack = (uint64_t *)initial_rsp;
    ustack[0] = (uint64_t)trampoline;
    ustack[1] = 0;
    ustack[2] = 0;

    extern void tss_set_stack(uint64_t rsp0);
    tss_set_stack(cur->kstack + KERNEL_STACK_SIZE);
    extern void syscall_set_kernel_stack(uint64_t stack);
    syscall_set_kernel_stack(cur->kstack + KERNEL_STACK_SIZE);

    cur->brk_start = aslr_get_heap_base();
    cur->brk_end = cur->brk_start;
    cur->mmap_base = aslr_get_mmap_base();
    cur->stack_base = user_stack;

    extern vfs_node_t *tty_node;
    struct file *f = kmalloc(sizeof(struct file));
    f->node = tty_node;
    f->offset = 0;
    f->flags = 0;
    f->refcount = 0;
    process_fd_install(cur, f);
    process_fd_install(cur, f);
    process_fd_install(cur, f);

    printk(KERN_INFO "ShadowBox: jumping to user mode at %llx...\n", entry_point);
    extern void switch_to_user_mode(uint64_t rip, uint64_t rsp);
    switch_to_user_mode(entry_point, initial_rsp);
}

uint32_t multiboot_info_ptr = 0;

void kernel_main(uint32_t magic, uint32_t info_ptr) {
    multiboot_info_ptr = info_ptr;
    serial_init();

    if (magic != 0x36d76289 && magic != 0x2BADB002) {
        printk(KERN_ERR "ShadowBox: Not booted by Multiboot, magic is 0x%x\n", magic);
        return;
    }

    printk(KERN_INFO "ShadowBox v" SHADOWBOX_VERSION " booting on " SHADOWBOX_ARCH "...\n");

    uint64_t cr0, cr4;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~(1ULL << 2);
    cr0 |= (1ULL << 1);
    __asm__ volatile("mov %0, %%cr0" :: "r"(cr0));

    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (1ULL << 9);
    cr4 |= (1ULL << 10);
    __asm__ volatile("mov %0, %%cr4" :: "r"(cr4));

    uint16_t *vga = (uint16_t *)0xFFFFFFFF800B8000;
    for (int i = 0; i < 80 * 25; i++) vga[i] = 0x0F00 | ' ';

    uint64_t initrd_start = 0;
    uint64_t initrd_end = 0;
    uint32_t fb_width = 0, fb_height = 0, fb_pitch = 0;
    uint64_t fb_addr = 0;

    uint32_t total_size = *(uint32_t *)(uint64_t)info_ptr;
    uint32_t ptr_offs = info_ptr + 8;
    while (ptr_offs < info_ptr + total_size) {
        struct multiboot_tag *tag = (struct multiboot_tag *)(uint64_t)ptr_offs;
        if (tag->type == 0) break;
        if (tag->type == 3) {
            struct multiboot_tag_module *mod = (struct multiboot_tag_module *)tag;
            initrd_start = mod->mod_start;
            initrd_end = mod->mod_end;
        }
        if (tag->type == 8) {
            struct {
                uint32_t type;
                uint32_t size;
                uint64_t addr;
                uint32_t pitch;
                uint32_t width;
                uint32_t height;
                uint8_t bpp;
            } *fb_tag = (void *)tag;
            fb_width = fb_tag->width;
            fb_height = fb_tag->height;
            fb_pitch = fb_tag->pitch;
            fb_addr = fb_tag->addr;
            fb_set_info(fb_addr, fb_width, fb_height, fb_pitch, fb_tag->bpp);
        }
        ptr_offs += (tag->size + 7) & ~7;
    }

    gdt_init();
    idt_init();
    pic_init();

    pmm_init(info_ptr);
    vmm_init();
    printk(KERN_INFO "MAIN: after vmm_init\n");

    printk(KERN_INFO "MAIN: calling aslr_init\n");
    aslr_init();
    aslr_enable_smep();
    aslr_enable_smap();

    extern void malloc_init(void);
    malloc_init();
    slab_init();

    vfs_init();
    block_init();

    pci_init();
    ahci_init();

    devfs_init();
    ext2_init();
    procfs_init();
    sysfs_init();
    tmpfs_init();

    ipc_init();
    signal_init();
    kthread_init();
    sched_init();

    net_init();
    socket_init();

    mmap_init();
    swap_init();

    if (initrd_start) {
        vmm_map_phys_range(initrd_start, initrd_end - initrd_start);
        tarfs_init(initrd_start + 0xFFFFFFFF80000000, initrd_end - initrd_start);
    } else {
        printk(KERN_WARN "Warning: No initrd module found!\n");
    }

    vfs_mount("/dev", devfs_root, 0);
    vfs_mount("/proc", procfs_root, 0);
    vfs_mount("/tmp", tmpfs_root, 0);
    vfs_mount("/sys", sysfs_root, 0);
    printk(KERN_INFO "ShadowBox: Filesystems mounted (/dev, /proc, /tmp, /sys)\n");

    devfs_register_input();
    tty_init();
    apic_init();
    keyboard_init();
    extern void mouse_init(void);
    mouse_init();
    extern void trackpad_init(void);
    trackpad_init();
    pit_init(100);

    #include "hda.h"
    hda_init();

    usb_init();
    fb_init();
    fb_console_init();
    rtc_init();

    {
        pci_device_t *pci_dev = pci_find_device(RTL8139_VENDOR_ID, RTL8139_DEVICE_ID);
        if (pci_dev) rtl8139_init(pci_dev);
    }
    {
        pci_device_t *pci_dev = pci_find_device(E1000_VENDOR_ID, E1000_DEVICE_ID);
        if (pci_dev) {
            extern void e1000_init(pci_device_t *);
            extern void e1000_irq_handler(void);
            e1000_init(pci_dev);
            irq_register_handler(pci_dev->irq_line, e1000_irq_handler);
            pic_clear_mask(pci_dev->irq_line);
        }
    }

    smp_init();

    syscall_init();
    task_init();

    task_create_proc(init_user_thread, 0);

    __asm__ volatile("sti");

    for (;;) {
        __asm__ volatile("hlt");
    }
}
