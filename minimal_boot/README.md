# STM32MP2 Bare-Metal Minimal Boot

Bare-metal test payload for the STM32MP2, designed for timing analysis, glitching experiments, and low‑level behavioral studies. 
Provides **UART control**, a **GPIO trigger pin**, and several selectable runtime modes.

---

## 1. Requirements

### **Host System**
- AArch64 bare‑metal toolchain (`aarch64-none-elf-gcc`, `aarch64-none-elf-objcopy`, etc.)
- `make`, `bash`
- `mkimage` (U‑Boot tools)
- `mkfs.ext4`

### **Hardware**
- STM32MP2 development board
- UART connected to `USART_BASE = 0x40220000`
- Trigger output on **GPIOJ pin PJ1**

---

## 2. Project Structure

```
minimal_boot/
  main.c
  startup.s
  linkscript.ld
  Makefile
shared/
  makefile-common.mk
ext/
  userfs_stage/
  st-image-userfs-openstlinux-weston-stm32mp2.userfs.ext4
build_userfs_image.sh
```

---

## 3. Setup

1. Install ARM AArch64 bare‑metal toolchain.
2. Confirm installation:

   ```bash
   aarch64-none-elf-gcc --version
   ```

3. Ensure required tools exist:

   ```bash
   mkimage -V
   mkfs.ext4 -V
   ```

4. Default project directory assumption:

```
$HOME/stm32mp2-baremetal/minimal_boot
```

---

## 4. Building the Bare-Metal Binary

Inside the project folder:

```bash
cd minimal_boot
make
```

Results:

- `build/main.elf` – Linked ELF
- `build/main.bin` – Raw binary

Optional disassembly listing:

```bash
make list
```

Produces: `build/main.lst`

---

## 5. Building a U-Boot Standalone Image + UserFS

Use the helper script:

```bash
./build_userfs_image.sh
```

Or specify a custom directory:

```bash
DIR=/path/to/minimal_boot ./build_userfs_image.sh
```

Outputs:

- `build/main-standalone.uimg`
- `ext/st-image-userfs-openstlinux-weston-stm32mp2.userfs.ext4`

---

## 6. Booting via U-Boot

Example boot commands:

```bash
mmc dev 1
ext4load mmc 1:6 0x88000000 /main-standalone.uimg
bootm 0x88000000
```

**Run Address:** `0x88000000` (matches the linker script)

---

## 7. Runtime Behavior

### **UART Protocol**

On power‑up, device outputs:

```
s
```

For each run:

1. Host sends:

   ```
   g
   ```

2. Device:
   - Executes the active mode
   - Pulls trigger low
   - Returns:

     ```
     r<number>
     ```

No newline is sent.

### **Trigger Pin**

- PJ1 toggles high for the duration of sensitive execution windows.
- Use for glitching, timing capture, or measurement alignment.

---

## 8. Selecting a Mode

Edit `main.c`:

```c
#define MODE_COMPARISON
```

### **Available Modes**

---

### **1. MODE_COUNTING_LOOP**
Simple nested loop; deterministic execution time and return value.

---

### **2. MODE_WHILE**
Timeout loop:

- Normally returns `r9999`
- Glitch causing early exit produces smaller numbers

---

### **3. MODE_WHILE_NO_TIMEOUT**
Infinite loop unless memory/register is modified externally.

---

### **4. MODE_COMPARISON** *(recommended for glitching)*  
- 1000 repeated probe blocks  
- Any glitch setting `ok == 2` returns `r0`
- Normal execution returns `r9999`

Designed for precise glitch timing alignment.

---

### **5. MODE_STRp(untested)**
Memory write/verify routine:

- Inline store block surrounded by NOPs
- Injects deliberate corruption
- Prints mismatches
- Returns mismatch count

---



## 🛠️ 10. Troubleshooting

### No `'s'` at startup?
- Baud mismatch (must match U‑Boot baud)
- UART clocks/pinmux not configured by boot stages

### Always returns `r9999`?
- Expected behavior unless a fault/glitch hits the controlled window

### No trigger pulse?
- PJ1 pin not mapped or overridden by U‑Boot
- Verify physical board pinout


