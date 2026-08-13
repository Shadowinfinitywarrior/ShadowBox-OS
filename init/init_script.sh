#!/bin/sh
# init_script.sh  —  ShadowBox ramfs launch script
#
# This script is stored in the initrd for documentation and future use.
# It is NOT executed by the current boot sequence (kernel/main.c loads
# desktop.elf directly).  It serves as a blueprint for a future pivot to
# a proper PID-1 init that exec()s a shell which sources this script.
#
# Usage (once /bin/sh is available):
#   execve("/init_script.sh", argv, envp)  or
#   source it from a real /init binary.

set -e

echo "[init_script] ShadowBox userland starting"

# Mount virtual filesystems if not already done by the kernel
# (These are already mounted by kernel/main.c; listed here for reference.)
# mount -t devfs  devfs  /dev
# mount -t procfs procfs /proc
# mount -t tmpfs  tmpfs  /tmp
# mount -t sysfs  sysfs  /sys

echo "[init_script] Launching desktop.elf"

# Launch the desktop application in the background so init can wait on it
/desktop.elf &
DESKTOP_PID=$!

echo "[init_script] desktop.elf launched (pid=$DESKTOP_PID)"

# Wait for the desktop to exit, then reboot/halt
wait $DESKTOP_PID
echo "[init_script] desktop exited, system halting"
