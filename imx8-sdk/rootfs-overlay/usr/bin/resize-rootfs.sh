#!/bin/bash
set -e

# Log to console
echo "[resize-rootfs] Starting root partition resize..."

ROOT_PART=$(findmnt / -o SOURCE -n)
DISK=$(echo "$ROOT_PART" | sed -E 's/p?[0-9]+$//')
PART_NUM=$(echo "$ROOT_PART" | grep -o '[0-9]*$')
echo "[resize-rootfs] Resizing partition $ROOT_PART on $DISK..."
growpart "$DISK" "$PART_NUM"
resize2fs "$ROOT_PART"
echo "[resize-rootfs] Resize complete."
systemctl disable resize-rootfs.service

# Remove script (optional)
rm -- "$0"