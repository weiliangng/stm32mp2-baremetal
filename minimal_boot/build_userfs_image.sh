#!/usr/bin/env bash
set -euo pipefail

MINIMAL_BOOT_DIR="${DIR:-$HOME/stm32mp2-baremetal/minimal_boot}"
STAGE_DIR="${MINIMAL_BOOT_DIR%/}/../ext/userfs_stage"
OUTPUT_IMAGE="${MINIMAL_BOOT_DIR%/}/../ext/st-image-userfs-openstlinux-weston-stm32mp2.userfs.ext4"

cd "$MINIMAL_BOOT_DIR"
make
mkimage -A arm64 -O u-boot -T standalone -C none \
  -a 0x88000000 -e 0x88000000 \
  -n "stm32mp2-baremetal" \
  -d build/main.bin build/main-standalone.uimg

mkdir -p "$STAGE_DIR"
install -m 0644 build/main-standalone.uimg "$STAGE_DIR/main-standalone.uimg"
rm -f "$OUTPUT_IMAGE"
mkfs.ext4 -q -d "$STAGE_DIR" -b 4096 -L userfs "$OUTPUT_IMAGE" 4096

cp /home/user/stm32mp2-baremetal/ext/st-image-userfs-openstlinux-weston-stm32mp2.userfs.ext4 /home/user/mputest/Starter-Package/stm32mp2-openstlinux-6.6-yocto-scarthgap-mpu-v25.06.11/images/stm32mp2
