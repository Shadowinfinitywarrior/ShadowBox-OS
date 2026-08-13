#include "types.h"
#include "kernel.h"
#include "boot.h"
#include "serial.h"
#include "hal/hal.h"
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

uint32_t multiboot_info_ptr = 0;

void __stack_chk_fail(void) {
    panic("Stack smashing detected");
}

static uint64_t initrd_start = 0;
static uint64_t initrd_end = 0;

static void bootloader_info_parse(uint32_t magic, uint32_t info_ptr) {
    boot_stage_begin("Bootloader info parse");

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
            fb_set_info(fb_tag->addr, fb_tag->width, fb_tag->height, fb_tag->pitch, fb_tag->bpp);
        }
        ptr_offs += (tag->size + 7) & ~7;
    }

    boot_stage_end();
}

static void arch_init(void) {
    boot_stage_begin("GDT setup");
    gdt_init();
    boot_stage_end();

    boot_stage_begin("IDT setup");
    idt_init();
    boot_stage_end();

    boot_stage_begin("PIC setup");
    pic_init();
    boot_stage_end();
}

static void mm_init(void) {
    boot_stage_begin("Physical Memory Manager init");
    pmm_init(multiboot_info_ptr);
    boot_stage_end();

    boot_stage_begin("Virtual Memory Manager init");
    vmm_init();
    boot_stage_end();

    boot_stage_begin("ASLR init");
    aslr_init();
    aslr_enable_smep();
    aslr_enable_smap();
    boot_stage_end();

    boot_stage_begin("Kernel Heap init");
    extern void malloc_init(void);
    malloc_init();
    slab_init();
    boot_stage_end();
}

static void vfs_storage_init(void) {
    boot_stage_begin("VFS init");
    vfs_init();
    boot_stage_end();

    boot_stage_begin("Block layer init");
    block_init();
    boot_stage_end();

    boot_stage_begin("PCI init");
    pci_init();
    boot_stage_end();

    boot_stage_begin("AHCI init");
    ahci_init();
    boot_stage_end();

    boot_stage_begin("HAL storage init");
    storage_init();
    boot_stage_end();

    boot_stage_begin("devfs init");
    devfs_init();
    boot_stage_end();

    boot_stage_begin("ext2 init");
    ext2_init();
    boot_stage_end();

    boot_stage_begin("procfs init");
    procfs_init();
    boot_stage_end();

    boot_stage_begin("sysfs init");
    sysfs_init();
    boot_stage_end();

    boot_stage_begin("tmpfs init");
    tmpfs_init();
    boot_stage_end();
}

static void kernel_subsys_init(void) {
    boot_stage_begin("IPC init");
    ipc_init();
    boot_stage_end();

    boot_stage_begin("Signal init");
    signal_init();
    boot_stage_end();

    boot_stage_begin("Kernel thread init");
    kthread_init();
    boot_stage_end();

    boot_stage_begin("Scheduler init");
    sched_init();
    boot_stage_end();

    boot_stage_begin("Network stack init");
    net_init();
    socket_init();
    boot_stage_end();

    boot_stage_begin("MMAP init");
    mmap_init();
    boot_stage_end();

    boot_stage_begin("Swap init");
    swap_init();
    boot_stage_end();
}

static void initrd_mount(void) {
    boot_stage_begin("Initrd load");
    if (initrd_start) {
        vmm_map_phys_range(initrd_start, initrd_end - initrd_start);
        tarfs_init(initrd_start + 0xFFFFFFFF80000000, initrd_end - initrd_start);
    } else {
        printk(KERN_WARN "Warning: No initrd module found!\n");
    }
    boot_stage_end();

    boot_stage_begin("Virtual FS mount");
    vfs_mount("/dev", devfs_root, 0);
    vfs_mount("/proc", procfs_root, 0);
    vfs_mount("/tmp", tmpfs_root, 0);
    vfs_mount("/sys", sysfs_root, 0);
    printk(KERN_INFO "ShadowBox: Filesystems mounted (/dev, /proc, /tmp, /sys)\n");
    boot_stage_end();
}

static void device_init(void) {
    boot_stage_begin("Input devices init");
    devfs_register_input();
    tty_init();
    boot_stage_end();

    boot_stage_begin("APIC init");
    apic_init();
    boot_stage_end();

    boot_stage_begin("PS/2 mouse init");
    extern void mouse_init(void);
    mouse_init();
    boot_stage_end();

    boot_stage_begin("PS/2 keyboard init");
    keyboard_init();
    boot_stage_end();

    boot_stage_begin("Trackpad init");
    extern void trackpad_init(void);
    trackpad_init();
    boot_stage_end();

    boot_stage_begin("PIT init (100Hz)");
    pit_init(100);
    boot_stage_end();

    boot_stage_begin("HDA audio init");
    hda_init();
    boot_stage_end();

    boot_stage_begin("USB xHCI init");
    usb_init();
    boot_stage_end();

    boot_stage_begin("Framebuffer init");
    fb_init();
    fb_console_init();
    boot_stage_end();

    boot_stage_begin("RTC init");
    rtc_init();
    boot_stage_end();

    boot_stage_begin("NIC init");
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
    boot_stage_end();
}

static void smp_bringup(void) {
    boot_stage_begin("SMP init");
    smp_init();
    boot_stage_end();
}

static void syscall_task_init(void) {
    boot_stage_begin("Syscall init");
    syscall_init();
    boot_stage_end();

    boot_stage_begin("Task init");
    task_init();
    boot_stage_end();
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

    bootloader_info_parse(magic, info_ptr);
    arch_init();
    mm_init();
    vfs_storage_init();
    kernel_subsys_init();
    initrd_mount();
    device_init();
    smp_bringup();

    boot_stage_begin("HAL init");
    hal_init();
    boot_stage_end();

    syscall_task_init();

    printk(KERN_INFO "ShadowBox syscall interface initialized.\n");
    boot_stages_summary();

    task_create_proc(init_user_thread, 0);

    __asm__ volatile("sti");

    for (;;) {
        __asm__ volatile("hlt");
    }
}
