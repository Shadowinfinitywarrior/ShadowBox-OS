CC = x86_64-linux-gnu-gcc
LD = x86_64-linux-gnu-ld
AS = x86_64-linux-gnu-gcc

VERSION := $(shell git describe --tags --always --dirty 2>/dev/null || echo "v0.2.0")

CFLAGS = -Wall -Wextra -ffreestanding -mcmodel=kernel -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -fno-pic -fno-pie -O2 -Iinclude -MMD -DSHADOWBOX_VERSION=\"$(VERSION)\" -fno-stack-protector
USER_CFLAGS = -Wall -Wextra -ffreestanding -mcmodel=small -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -fno-pic -fno-pie -O2 -Iinclude -Iuserland -fno-stack-protector -U_FORTIFY_SOURCE
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
	arch/x86_64/mm/aslr.o

ARCH_DRV_OBJS = \
	arch/x86_64/drivers/pic.o \
	arch/x86_64/drivers/serial.o \
	arch/x86_64/drivers/pit.o \
	arch/x86_64/drivers/keyboard.o \
	arch/x86_64/drivers/mouse.o \
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
	arch/x86_64/drivers/e1000.o

KERNEL_OBJS = \
	kernel/main.o \
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
	kernel/trackpad.o

FS_OBJS = \
	fs/block.o \
	fs/ext2.o \
	fs/devfs.o \
	fs/procfs.o \
	fs/sysfs.o \
	fs/tmpfs.o

NET_OBJS = \
	net/net.o \
	net/tcp.o \
	net/udp.o \
	net/socket.o

LIB_OBJS = \
	lib/kstring.o

OBJS = $(ARCH_BOOT_OBJS) $(ARCH_KERNEL_OBJS) $(ARCH_MM_OBJS) $(ARCH_DRV_OBJS) \
       $(KERNEL_OBJS) $(FS_OBJS) $(NET_OBJS) $(LIB_OBJS)

DEPS = $(OBJS:.o=.d)

.PHONY: all iso run run-nox clean debug disasm size help format

all: iso

kernel/main.o: kernel/main.c

shell.elf: userland/shell.c
	$(CC) $(USER_CFLAGS) -no-pie -nostdlib userland/shell.c -o shell.elf

hello.elf: userland/hello.c
	$(CC) $(USER_CFLAGS) -no-pie -nostdlib userland/hello.c -o hello.elf

desktop.elf: userland/desktop.c
	$(CC) $(USER_CFLAGS) -no-pie -nostdlib userland/desktop.c -o desktop.elf

edit.elf: userland/edit.c
	$(CC) $(USER_CFLAGS) -no-pie -nostdlib userland/edit.c -o edit.elf

calc.elf: userland/calc.c
	$(CC) $(USER_CFLAGS) -no-pie -nostdlib userland/calc.c -o calc.elf

netstat.elf: userland/netstat.c
	$(CC) $(USER_CFLAGS) -no-pie -nostdlib userland/netstat.c -o netstat.elf

sysfetch.elf: userland/sysfetch.c
	$(CC) $(USER_CFLAGS) -no-pie -nostdlib userland/sysfetch.c -o sysfetch.elf

strings.elf: userland/strings.c
	$(CC) $(USER_CFLAGS) -no-pie -nostdlib userland/strings.c -o strings.elf

colors.elf: userland/colors.c
	$(CC) $(USER_CFLAGS) -no-pie -nostdlib userland/colors.c -o colors.elf

guess.elf: userland/guess.c
	$(CC) $(USER_CFLAGS) -no-pie -nostdlib userland/guess.c -o guess.elf

more.elf: userland/more.c
	$(CC) $(USER_CFLAGS) -no-pie -nostdlib userland/more.c -o more.elf

factor.elf: userland/factor.c
	$(CC) $(USER_CFLAGS) -no-pie -nostdlib userland/factor.c -o factor.elf

matrix.elf: userland/matrix.c
	$(CC) $(USER_CFLAGS) -no-pie -nostdlib userland/matrix.c -o matrix.elf

seq.elf: userland/seq.c
	$(CC) $(USER_CFLAGS) -no-pie -nostdlib userland/seq.c -o seq.elf
terminal.elf: userland/terminal.c
	$(CC) $(USER_CFLAGS) -no-pie -nostdlib userland/terminal.c -o terminal.elf

rev.elf: userland/rev.c
	$(CC) $(USER_CFLAGS) -no-pie -nostdlib userland/rev.c -o rev.elf

wc.elf: userland/wc.c
	$(CC) $(USER_CFLAGS) -no-pie -nostdlib userland/wc.c -o wc.elf

uname.elf: userland/uname.c
	$(CC) $(USER_CFLAGS) -no-pie -nostdlib userland/uname.c -o uname.elf

fortune.elf: userland/fortune.c
	$(CC) $(USER_CFLAGS) -no-pie -nostdlib userland/fortune.c -o fortune.elf

rot13.elf: userland/rot13.c
	$(CC) $(USER_CFLAGS) -no-pie -nostdlib userland/rot13.c -o rot13.elf

cmp.elf: userland/cmp.c
	$(CC) $(USER_CFLAGS) -no-pie -nostdlib userland/cmp.c -o cmp.elf

clear.elf: userland/clear.c
	$(CC) $(USER_CFLAGS) -no-pie -nostdlib userland/clear.c -o clear.elf

sleep.elf: userland/sleep.c
	$(CC) $(USER_CFLAGS) -no-pie -nostdlib userland/sleep.c -o sleep.elf

initrd.tar: shell.elf hello.elf desktop.elf terminal.elf edit.elf calc.elf netstat.elf sysfetch.elf strings.elf colors.elf guess.elf more.elf factor.elf matrix.elf seq.elf rev.elf wc.elf uname.elf fortune.elf rot13.elf cmp.elf clear.elf sleep.elf chmod.elf chown.elf userland/wallpaper.bmp userland/logo.bmp
	tar -cf initrd.tar shell.elf hello.elf desktop.elf terminal.elf edit.elf calc.elf netstat.elf sysfetch.elf strings.elf colors.elf guess.elf more.elf factor.elf matrix.elf seq.elf rev.elf wc.elf uname.elf fortune.elf rot13.elf cmp.elf clear.elf sleep.elf chmod.elf chown.elf -C userland wallpaper.bmp logo.bmp

chmod.elf: userland/chmod.c
	$(CC) $(USER_CFLAGS) -no-pie -nostdlib userland/chmod.c -o chmod.elf

chown.elf: userland/chown.c
	$(CC) $(USER_CFLAGS) -no-pie -nostdlib userland/chown.c -o chown.elf

os.bin: $(OBJS) link.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(AS) $(ASFLAGS) -c $< -o $@

ahci_disk.img:
	dd if=/dev/zero of=ahci_disk.img bs=1M count=10
	mke2fs -t ext2 -F ahci_disk.img

iso: os.bin initrd.tar ahci_disk.img
	mkdir -p isodir/boot/grub
	cp os.bin isodir/boot/os.bin
	cp initrd.tar isodir/boot/initrd.tar
	{ \
		echo 'set timeout=0'; \
		echo 'set default=0'; \
		echo 'set gfxpayload=1024x768x32'; \
		echo 'menuentry "OS" {'; \
		echo '  multiboot2 /boot/os.bin'; \
		echo '  module2 /boot/initrd.tar'; \
		echo '  boot'; \
		echo '}'; \
	} > isodir/boot/grub/grub.cfg
	grub-mkrescue -o os.iso isodir 2>/dev/null

run-nox: iso
	qemu-system-x86_64 -cdrom os.iso -drive id=disk,file=ahci_disk.img,if=none,format=raw -device ahci,id=ahci -device ide-hd,drive=disk,bus=ahci.0 -device qemu-xhci,id=xhci -device intel-hda,debug=4 -device hda-output -serial stdio -display none -no-reboot

run: iso
	qemu-system-x86_64 -cdrom os.iso -drive id=disk,file=ahci_disk.img,if=none,format=raw -device ahci,id=ahci -device ide-hd,drive=disk,bus=ahci.0 -device qemu-xhci,id=xhci -device intel-hda,debug=4 -device hda-output -serial stdio -no-reboot

debug: iso
	qemu-system-x86_64 -cdrom os.iso -drive id=disk,file=ahci_disk.img,if=none,format=raw -device ahci,id=ahci -device ide-hd,drive=disk,bus=ahci.0 -device qemu-xhci,id=xhci -serial stdio -s -S -no-reboot

disasm: os.bin
	x86_64-linux-gnu-objdump -d os.bin > objdump.txt
	@echo "Disassembly written to objdump.txt"

size: os.bin
	x86_64-linux-gnu-size $(OBJS)
	@echo "---"
	x86_64-linux-gnu-size os.bin

clean:
	rm -f $(OBJS) $(DEPS) os.bin os.iso shell.elf hello.elf edit.elf calc.elf netstat.elf sysfetch.elf strings.elf colors.elf guess.elf more.elf factor.elf matrix.elf seq.elf rev.elf wc.elf uname.elf fortune.elf rot13.elf cmp.elf clear.elf sleep.elf userland/*.o initrd.tar ahci_disk.img
	rm -rf isodir
	rm -f objdump.txt

format:
	@echo "Format target not yet configured"

help:
	@echo "ShadowBox OS v$(VERSION)"
	@echo ""
	@echo "Targets:"
	@echo "  all       Build the OS ISO (default)"
	@echo "  iso       Build ISO image"
	@echo "  run       Run in QEMU"
	@echo "  run-nox   Run in QEMU (no display)"
	@echo "  debug     Run in QEMU with GDB stub (-s -S)"
	@echo "  disasm    Disassemble os.bin"
	@echo "  size      Show kernel size breakdown"
	@echo "  clean     Remove build artifacts"
	@echo "  format    Format source files (not yet configured)"
	@echo ""
	@echo "Variables:"
	@echo "  VERSION   $(VERSION)"

-include $(DEPS)
