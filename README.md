# OpenFeatureOS

OpenFeatureOS is an open-source, lightweight operating system for 4G feature phones based on RT-Thread, targeting the Unisoc T107 (UMS9117) SoC on the Nokia 2660 Flip (Model TA-1480).

---

## Hardware Target Specifications

| Component | Specification |
|:---|:---|
| **Device Model** | Nokia 2660 Flip (TA-1480, HW3.0) |
| **SoC / Chipset** | Unisoc T107 / UMS9117 (Single ARM Cortex-A7 @ up to 1.0 GHz) |
| **Volatile RAM** | 48 MB LPDDR (PoP package on SoC) |
| **Internal Flash** | 128 MB SPI NAND |
| **Primary Display** | 2.8" QVGA 240×320 TFT LCD (GalaxyCore GC9307 via SPI1) |
| **Sub Display** | 1.77" QQVGA 128×160 TFT LCD (Sitronix ST7735 via SPI) |
| **Keypad Controller**| APB 5×5 matrix scanner (`0x40250000`) + PMIC EIC Power Key (`0x40608280`) |
| **PMIC Companion** | Unisoc SC2720 (via ADI bus) |
| **Debug UART** | UART1 at `0x70100000` (1.8V TTL, 115,200 baud 8N1) |
| **Power MCU** | Dedicated auxiliary ARM Cortex-M4 core (`pm_cm4.bin`) |

---

## Execution Model & Safety Guarantee

OpenFeatureOS runs **strictly in volatile DRAM** loaded via USB download mode (BSL/FDL1) with `spd_dump`.
- **Zero NAND writes**: No modifications are made to SPI NAND flash, boot partitions, or eFuses.
- **Factory-Safe**: Unplugging the USB cable and reseating the battery returns the phone to 100% factory-stock condition.

---

## Repository Structure

```
OpenFeatureOS/
├── board/          # Board support package (linker script, startup, register map)
├── docs/           # Architecture, references, and toolchain setup
│   ├── REFERENCES.md  # Reference submodules, pinned SHAs, and emulator report
│   └── TOOLCHAIN.md   # Cross-compiler and host environment guide
├── include/        # Common headers and OS definitions
├── scripts/        # Packaging utilities (mkdhtb.py for FDL1 DHTB container)
├── src/            # Core kernel, scheduler, and driver implementations
├── third_party/    # Pinned submodules
│   ├── nokia-2660-re        # RE documentation & working payload references
│   ├── nokia-2660-emulator  # Unisoc T107 hardware emulator (n2660emu)
│   └── rt-thread            # Upstream RT-Thread RTOS repository
├── Makefile        # Top-level build system
└── LICENSE         # GNU General Public License v3.0
```

---

## Quick Start

### 1. Requirements
- `arm-none-eabi-gcc` toolchain (Cortex-A7 support)
- GNU Make 3.81+
- Python 3

See [`docs/TOOLCHAIN.md`](docs/TOOLCHAIN.md) for platform-specific installation instructions.

### 2. Building
```bash
make
```

Artifacts will be generated in `build/`:
- `openfeatureos.elf`: Unstripped ELF binary
- `openfeatureos.raw`: Flat binary payload
- `openfeatureos.bin`: DHTB-encapsulated binary ready for DRAM loading at `0x80100000`

### 3. Emulation Testing
```bash
make emu
```

Launches `n2660emu` with the built payload loaded directly into DRAM.

---

## License

OpenFeatureOS is released under the **GNU General Public License v3.0 (GPL-3.0)**.
Third-party reference components are licensed under their respective licenses:
- `third_party/nokia-2660-re`: GPL-3.0
- `third_party/nokia-2660-emulator`: MIT License
- `third_party/rt-thread`: Apache License 2.0
