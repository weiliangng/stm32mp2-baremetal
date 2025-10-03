#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
MINIMAL_BOOT_DIR="$REPO_ROOT/minimal_boot"
STAGE_DIR="$REPO_ROOT/ext/userfs_stage"
OUTPUT_IMAGE="$REPO_ROOT/ext/st-image-userfs-openstlinux-weston-stm32mp2.userfs.ext4"
BLOCK_SIZE=${USERFS_BLOCK_SIZE:-4096}
BLOCK_COUNT=${USERFS_BLOCK_COUNT:-4096}
MKIMAGE_BIN="${MKIMAGE_BIN:-mkimage}"
EXPECTED_PREFIX="${EXPECTED_TOOL_PREFIX:-aarch64-none-elf-}"
SOURCE_PREFIX="${TOOLCHAIN_PREFIX:-${CROSS_COMPILE:-$EXPECTED_PREFIX}}"

cleanup_wrap_dir() {
  [[ -n "${WRAP_DIR:-}" && -d "${WRAP_DIR:-}" ]] && rm -rf "$WRAP_DIR"
}
trap cleanup_wrap_dir EXIT

require_cmd() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "Error: required command '$1' is not available." >&2
    exit 1
  fi
}

ensure_toolchain() {
  local tools=(gcc g++ objcopy objdump size)
  local missing=()
  for tool in "${tools[@]}"; do
    if ! command -v "${EXPECTED_PREFIX}${tool}" >/dev/null 2>&1; then
      missing+=("$tool")
    fi
  done

  if [[ ${#missing[@]} -eq 0 ]]; then
    return
  fi

  if [[ -z "${SOURCE_PREFIX}" ]]; then
    echo "Error: missing tools for prefix ${EXPECTED_PREFIX} and TOOLCHAIN_PREFIX/CROSS_COMPILE not provided." >&2
    exit 1
  fi

  WRAP_DIR="$(mktemp -d)"
  for tool in "${missing[@]}"; do
    local target="${SOURCE_PREFIX}${tool}"
    if ! command -v "$target" >/dev/null 2>&1; then
      echo "Error: cannot wrap '${EXPECTED_PREFIX}${tool}' because '$target' is unavailable." >&2
      exit 1
    fi
    ln -s "$(command -v "$target")" "$WRAP_DIR/${EXPECTED_PREFIX}${tool}"
  done

  export PATH="$WRAP_DIR:$PATH"
}

require_cmd "$MKIMAGE_BIN"
require_cmd mkfs.ext4

if [[ ! -d "$MINIMAL_BOOT_DIR" ]]; then
  echo "Error: expected minimal_boot directory at $MINIMAL_BOOT_DIR" >&2
  exit 1
fi

ensure_toolchain

mkdir -p "$STAGE_DIR"

print_step() {
  echo "[$1] $2"
}

print_step "1/4" "Building minimal_boot (tool prefix=${EXPECTED_PREFIX})..."
(
  cd "$MINIMAL_BOOT_DIR"
  make
)

BIN_FILE="$MINIMAL_BOOT_DIR/build/main.bin"
UIMG_FILE="$MINIMAL_BOOT_DIR/build/main-standalone.uimg"

if [[ ! -f "$BIN_FILE" ]]; then
  echo "Error: expected binary at $BIN_FILE" >&2
  exit 1
fi

print_step "2/4" "Creating U-Boot standalone image..."
"$MKIMAGE_BIN" -A arm64 -O u-boot -T standalone -C none \
  -a 0x88000000 -e 0x88000000 \
  -n "stm32mp2-baremetal" \
  -d "$BIN_FILE" "$UIMG_FILE"

print_step "3/4" "Staging payload into ext/userfs_stage..."
install -m 0644 "$UIMG_FILE" "$STAGE_DIR/main-standalone.uimg"

print_step "4/4" "Packing ext4 image..."
rm -f "$OUTPUT_IMAGE"
mkfs.ext4 -q -d "$STAGE_DIR" -b "$BLOCK_SIZE" -L userfs "$OUTPUT_IMAGE" "$BLOCK_COUNT"

print_step "done" "Image available at $OUTPUT_IMAGE"
