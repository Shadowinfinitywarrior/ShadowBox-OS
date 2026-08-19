CC = x86_64-linux-gnu-gcc
LD = x86_64-linux-gnu-ld
AS = x86_64-linux-gnu-gcc

VERSION := $(shell git describe --tags --always --dirty 2>/dev/null || echo "v0.2.0")

CFLAGS = -Wall -Wextra -ffreestanding -mcmodel=kernel -mno-red-zone -fno-pie -O2 -Iinclude -MMD -DSHADOWBOX_VERSION=\"$(VERSION)\" -fno-stack-protector
USER_CFLAGS = -Wall -Wextra -ffreestanding -mcmodel=small -mno-red-zone -fno-pie -O2 -Iinclude -Iuserland -fno-stack-protector -U_FORTIFY_SOURCE
ASFLAGS = -Wa,--divide -Iinclude -D__ASSEMBLY__
LDFLAGS = -nostdlib -z max-page-size=0x1000 -T link.ld

ARCH_BOOT_OBJS = \
arch/x86_64/boot/head.o \
arch/x86_64/boot/idt.o \
arch/x86_64/boot/isr_stubs.o \
arch/x86_64/boot/ap_trampoline.o

ARCH_KERNEL_OBJS = \
arch/x86_64/kernel/switch.o \
arch/x86_64/kernel/syscall_entry.o \
arch/x86_64/kernel/smp.o \
arch/x86_64/kernel/gdt.o \
arch/x86_64/kernel/idt.o

ARCH_MM_OBJS = \
arch/x86_64/mm/pmm.o \
arch/x86_64/mm/vmm.o \
arch/x86_64/mm/buddy.o \
arch/x86_64/mm/malloc.o \
arch/x86_64/mm/slab.o \
arch/x86_64/mm/mmap.o \
arch/x86_64/mm/swap.o \
arch/x86_64/mm/aslr.o \
arch/x86_64/mm/smep_smap.o

ARCH_DRV_OBJS = \
arch/x86_64/drivers/pic.o \
arch/x86_64/drivers/serial.o \
arch/x86_64/drivers/pit.o \
arch/x86_64/drivers/keyboard.o \
arch/x86_64/drivers/mouse.o \
arch/x86_64/drivers/mouse_poll.o \
arch/x86_64/drivers/tty.o \
arch/x86_64/drivers/pci.o \
arch/x86_64/drivers/apic.o \
arch/x86_64/drivers/acpi.o \
arch/x86_64/drivers/ahci.o \
arch/x86_64/drivers/usb.o \
arch/x86_64/drivers/hda.o \
arch/x86_64/drivers/fb.o \
arch/x86_64/drivers/rtc.o \
arch/x86_64/drivers/rtl8139.o \
arch/x86_64/drivers/e1000.o \
arch/x86_64/drivers/fb_double.o \
arch/x86_64/drivers/ramdisk.o \
arch/x86_64/drivers/tickless.o \
arch/x86_64/drivers/virtio_net.o \
arch/x86_64/drivers/ac97.o \
arch/x86_64/drivers/i2s.o \
arch/x86_64/drivers/intel_gpu.o \
arch/x86_64/drivers/virtio_gpu.o

HAL_OBJS = \
kernel/hal/hal.o \
kernel/hal/cpu.o \
kernel/hal/memory.o \
kernel/hal/storage.o \
kernel/hal/peripheral.o \
kernel/hal/alloc_wrapper.o

KERNEL_OBJS = \
kernel/main.o \
kernel/boot.o \
kernel/printk.o \
kernel/task.o \
kernel/vfs.o \
kernel/syscall.o \
kernel/elf.o \
kernel/tarfs.o \
kernel/ipc.o \
kernel/micro_ipc.o \
kernel/signal.o \
kernel/kthread.o \
kernel/sched.o \
kernel/memory.o \
kernel/time.o \
kernel/input.o \
kernel/trackpad.o \
kernel/hid.o \
kernel/hid_kbd.o \
kernel/keyboard_layouts.o \
kernel/keyboard_shortcuts.o \
kernel/driver.o \
kernel/softirq.o \
kernel/idle.o \
kernel/power.o \
kernel/root.o \
kernel/notification.o \
kernel/irq_monitor.o \
kernel/wait.o \
kernel/session.o \
kernel/module.o \
kernel/entropy.o \
kernel/namespace.o \
kernel/seccomp.o \
kernel/rng.o \
kernel/camera.o \
kernel/test_priority.o \
kernel/calendar.o \
kernel/input_multiplexer.o \
kernel/syscall_tracer.o \
kernel/preemptive_sched.o \
kernel/pty.o \
kernel/security.o \
kernel/stubs.o \
$(HAL_OBJS)

KERNEL_ACPI_OBJS = \
kernel/acpi/aml.o \
kernel/acpi/battery.o \
kernel/acpi/parser.o \
kernel/acpi/thermal.o

KERNEL_MEM_OBJS = \
kernel/mem/bcache.o \
kernel/mem/hugepage.o \
kernel/mem/lru.o \
kernel/mem/numa.o

KERNEL_SCHED_OBJS = \
kernel/sched/cfs.o \
kernel/sched/loadbal.o \
kernel/sched/rt.o

KERNEL_DRV_OBJS = \
kernel/drivers/input/usb_hid_driver.o

FS_OBJS = \
fs/block.o \
fs/ext2.o \
fs/devfs.o \
fs/procfs.o \
fs/sysfs.o \
fs/tmpfs.o \
fs/ext4.o \
fs/fat32.o \
fs/shadowfs.o \
fs/directory_watch.o

NET_OBJS = \
net/net.o \
net/tcp.o \
net/udp.o \
net/socket.o \
net/bluetooth.o \
net/ipv6.o \
net/wifi.o \
net/dns.o \
net/ntp.o

LIB_OBJS = \
lib/kstring.o

AUDIO_OBJS = \
audio/hda.o \
audio/mixer.o \
audio/pcm.o

DRIVER_BUS_OBJS = \
drivers/bus/i2c_hid.o \
drivers/bus/i2c_master.o \
drivers/bus/pci_msi.o \
drivers/bus/spi_master.o

DRIVER_USB_OBJS = \
drivers/usb/ehci.o \
drivers/usb/usb_audio.o \
drivers/usb/usb_core.o \
drivers/usb/usb_hid.o \
drivers/usb/usb_hub.o \
drivers/usb/usb_msc.o \
drivers/usb/usb_uvc.o \
drivers/usb/xhci.o

GUI_C_OBJS = \
gui/c/draw.o \
gui/c/fb_draw.o \
gui/c/freestanding.o \
gui/c/gui_bridge.o \
gui/c/screensaver.o \
gui/c/sys_sbrk.o \
gui/c/taskbar.o

INIT_OBJS = \
init/init.o \
init/service.o

INPUT_OBJS = \
input/accel.o \
input/evdev.o \
input/gesture.o \
input/input_router.o

POWER_OBJS = \
power/backlight.o \
power/cpufreq.o \
power/suspend.o

UI_OBJS = \
ui/animation.o \
ui/animation_viewer.o \
ui/color_picker.o \
ui/diff_viewer.o \
ui/font.o \
ui/gui_impl.o \
ui/layout.o \
ui/textinput.o \
ui/theme.o \
ui/widget.o \
	ui/icon.o

WM_OBJS = \
wm/decorations.o \
wm/focus.o \
wm/window.o \
wm/wm_core.o \
wm/workspace.o

GUI_OBJS = \
gui/dirty_region.o \
gui/wallpaper_engine.o \
$(GUI_C_OBJS) \
$(UI_OBJS) \
$(WM_OBJS)

VIDEO_OBJS = \
video/drm_core.o \
video/display.o

OBJS = $(ARCH_BOOT_OBJS) $(ARCH_KERNEL_OBJS) $(ARCH_MM_OBJS) $(ARCH_DRV_OBJS) \
$(KERNEL_OBJS) $(KERNEL_ACPI_OBJS) $(KERNEL_MEM_OBJS) $(KERNEL_SCHED_OBJS) \
$(KERNEL_DRV_OBJS) \
$(FS_OBJS) $(NET_OBJS) $(LIB_OBJS) \
$(AUDIO_OBJS) $(DRIVER_BUS_OBJS) $(DRIVER_USB_OBJS) \
$(VIDEO_OBJS) \
$(GUI_OBJS) $(INIT_OBJS) $(INPUT_OBJS) $(POWER_OBJS)

DEPS = $(OBJS:.o=.d)

.PHONY: all iso run run-nox clean debug disasm size help format

all: iso

kernel/main.o: kernel/main.c

%.elf: userland/%.c
	$(CC) $(USER_CFLAGS) -no-pie -nostdlib $< -o $@

shell.elf: userland/shell.c
edit.elf: userland/edit.c
desktop.elf: userland/desktop.c userland/desktop_icons.c gui/c/fb_draw.c gui/c/draw.c gui/asm/draw.o gui/c/fb_stub.c
	$(CC) $(USER_CFLAGS) -no-pie -nostdlib $^ -o $@
terminal.elf: userland/terminal.c
calc.elf: userland/calc.c
netstat.elf: userland/netstat.c
sysfetch.elf: userland/sysfetch.c
strings.elf: userland/strings.c
colors.elf: userland/colors.c
guess.elf: userland/guess.c
more.elf: userland/more.c
factor.elf: userland/factor.c
matrix.elf: userland/matrix.c
seq.elf: userland/seq.c
wc.elf: userland/wc.c
uname.elf: userland/uname.c
fortune.elf: userland/fortune.c
rot13.elf: userland/rot13.c
cmp.elf: userland/cmp.c
clear.elf: userland/clear.c
sleep.elf: userland/sleep.c
chmod.elf: userland/chmod.c
chown.elf: userland/chown.c
rev.elf: userland/rev.c
sort.elf: userland/sort.c
tee.elf: userland/tee.c
which.elf: userland/which.c
yes.elf: userland/yes.c
true.elf: userland/true.c
false.elf: userland/false.c
search.elf: userland/search.c
hexedit.elf: userland/hexedit.c
login.elf: userland/login.c
ping.elf: userland/ping.c
date.elf: userland/date.c
uptime.elf: userland/uptime.c
nslookup.elf: userland/nslookup.c
ntpdate.elf: userland/ntpdate.c

os.bin: $(OBJS) link.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(AS) $(ASFLAGS) -c $< -o $@

ahci_disk.img:
	dd if=/dev/zero of=ahci_disk.img bs=1M count=10 2>/dev/null
	mke2fs -t ext2 -F ahci_disk.img

initrd.tar: shell.elf hello.elf desktop.elf terminal.elf edit.elf calc.elf netstat.elf sysfetch.elf strings.elf colors.elf guess.elf more.elf factor.elf matrix.elf seq.elf rev.elf wc.elf uname.elf fortune.elf rot13.elf cmp.elf clear.elf sleep.elf chmod.elf chown.elf sort.elf tee.elf which.elf yes.elf true.elf false.elf search.elf hexedit.elf login.elf ping.elf date.elf uptime.elf nslookup.elf ntpdate.elf userland/wallpaper.bmp userland/logo.bmp userland/test_data.txt icons/*.bmp
	tar -cf initrd.tar shell.elf hello.elf desktop.elf terminal.elf edit.elf calc.elf netstat.elf sysfetch.elf strings.elf colors.elf guess.elf more.elf factor.elf matrix.elf seq.elf rev.elf wc.elf uname.elf fortune.elf rot13.elf cmp.elf clear.elf sleep.elf chmod.elf chown.elf sort.elf tee.elf which.elf yes.elf true.elf false.elf search.elf hexedit.elf login.elf ping.elf date.elf uptime.elf nslookup.elf ntpdate.elf -C userland wallpaper.bmp logo.bmp test_data.txt icons

iso: os.bin initrd.tar ahci_disk.img
	mkdir -p isodir/boot/grub
	mkdir -p isodir/icons
	cp os.bin isodir/boot/os.bin
	cp initrd.tar isodir/boot/initrd.tar
	cp icons/*.bmp isodir/icons/
	{ \
		echo 'set timeout=0'; \
		echo 'set default=0'; \
		echo 'set gfxpayload=1024x768x32'; \
		echo 'menuentry "OS" {'; \
		echo ' multiboot2 /boot/os.bin'; \
		echo ' module2 /boot/initrd.tar'; \
		echo ' boot'; \
		echo '}'; \
	} > isodir/boot/grub/grub.cfg
	grub-mkrescue -o os.iso isodir 2>/dev/null

run-nox: iso
	qemu-system-x86_64 -cdrom os.iso -drive id=disk,file=ahci_disk.img,if=none,format=raw -device ahci,id=ahci -device ide-hd,drive=disk,bus=ahci.0 -device qemu-xhci,id=xhci -device intel-hda,debug=4 -device hda-output -netdev user,id=net0 -device e1000,netdev=net0 -serial stdio -display none -no-reboot

run: iso
	qemu-system-x86_64 -cdrom os.iso -drive id=disk,file=ahci_disk.img,if=none,format=raw -device ahci,id=ahci -device ide-hd,drive=disk,bus=ahci.0 -device qemu-xhci,id=xhci -device intel-hda -device hda-output -netdev user,id=net0 -device e1000,netdev=net0 -serial stdio -no-reboot

debug: iso
	qemu-system-x86_64 -cdrom os.iso -drive id=disk,file=ahci_disk.img,if=none,format=raw -device ahci,id=ahci -device ide-hd,drive=disk,bus=ahci.0 -serial stdio -s -S -no-reboot

disasm: os.bin
	x86_64-linux-gnu-objdump -d os.bin > objdump.txt
	@echo "Disassembly written to objdump.txt"

size: os.bin
	x86_64-linux-gnu-size $(OBJS)
	@echo "---"
	x86_64-linux-gnu-size os.bin

clean:
	rm -f $(OBJS) $(DEPS) os.bin os.iso shell.elf hello.elf desktop.elf terminal.elf edit.elf calc.elf netstat.elf sysfetch.elf strings.elf colors.elf guess.elf more.elf factor.elf matrix.elf seq.elf rev.elf wc.elf uname.elf fortune.elf rot13.elf cmp.elf clear.elf sleep.elf chmod.elf chown.elf sort.elf tee.elf which.elf yes.elf true.elf false.elf search.elf hexedit.elf login.elf userland/*.o initrd.tar ahci_disk.img
	rm -rf isodir
	rm -f objdump.txt
	rm -f icons/*.bmp

format:
	@echo "Format target not yet configured"

help:
	@echo "ShadowBox OS v$(VERSION)"
	@echo ""
	@echo "Targets:"
	@echo " all Build the OS ISO (default)"
	@echo " iso Build ISO image"
	@echo " run Run in QEMU"
	@echo " run-nox Run in QEMU (no display)"
	@echo " debug Run in QEMU with GDB stub (-s -S)"
	@echo " disasm Disassemble os.bin"
	@echo " size Show kernel size breakdown"
	@echo " clean Remove build artifacts"
	@echo " format Format source files (not yet configured)"
	@echo ""
	@echo "Variables:"
	@echo " VERSION $(VERSION)"

-include $(DEPS)
