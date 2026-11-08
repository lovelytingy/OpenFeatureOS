# OpenFeatureOS Reference Ecosystem & Citations

This document catalogues all external reference repositories, hardware specifications,
submodule pins, licenses, and citation guidelines used in the OpenFeatureOS project.

---

## 1. Submodule Registry & Pinned Commits

| Submodule Path | Upstream Repository URL | Pinned Commit SHA | Upstream License | Status |
|:---|:---|:---|:---|:---:|
| `third_party/nokia-2660-re` | `https://github.com/lovelytingy/Nokia-2660-RE` | `ab55f8a8390a6f9d95d6a27f909a0dd17b104478` | GNU GPL v3.0 | Pinned |
| `third_party/nokia-2660-emulator` | `https://github.com/lovelytingy/Nokia-2660-Emulator` | `c67e7029cb6ba268ad26b01b3eb3d5c5f49b01bf` | MIT License | Pinned |
| `third_party/rt-thread` | `https://github.com/RT-Thread/rt-thread.git` | `37d8b8d1d0b8b39d9238cd86dac2a1ae79d190c1` | Apache License 2.0 | Pinned |

---

## 2. Reference Repository Reports

### 2.1 Nokia-2660-RE (Reverse-Engineering Dossier)
* **Repository**: [https://github.com/lovelytingy/Nokia-2660-RE](https://github.com/lovelytingy/Nokia-2660-RE)
* **Pinned Commit**: `ab55f8a8390a6f9d95d6a27f909a0dd17b104478`
* **License**: GNU General Public License v3.0 (`GPL-3.0`)
* **Primary Reference Sections**:
  - `docs/hardware.md`: Hardware component breakdown, dual screen specifications (GC9307 / ST7735), SC2720 PMIC, Cortex-M4 auxiliary power management core.
  - `docs/memory-map.md`: Physical memory architecture, internal SRAM (`0x00006200` / `0x40000000`), LPDDR DRAM (`0x80000000`–`0x84000000`), APB/AHB window (`0x50000000`–`0x7FFFFFFF`).
  - `docs/custom-code-execution.md`: Non-destructive volatile DRAM execution over USB download mode (BSL/FDL1 via `spd_dump`), DHTB 512-byte header convention, load address `0x80100000`, entry point `0x80100200`.
  - `docs/boot-process.md`: Multi-stage trust chain (BootROM → BOOT0/SPL → BOOT1/LK → Kernel → Userland).
  - `docs/uart-serial-interface.md`: UART1 hardware register map (`0x70100000`), 26 MHz crystal divisor formulas (`CLKD0 = 0xE2` for 115,200 baud), APB clock gating register (`0x71300000 |= 0x6000`).
  - `docs/keypad-matrix.md`: APB keypad scanner (`0x40250000`), 5×5 matrix coordinates, power/clock gating (`0x402e1000` / `0x402e1010`), and dedicated PMIC EIC Power/End key (`0x40608280` bit 1).
  - `docs/cortex-m4-power-management.md`: Auxiliary Cortex-M4 power management MCU (`pm_cm4.bin`), LPDDR self-refresh (`dmc_retention.c`), SC2720 ADI bus communication.
  - `tools/ram_payload/`: Working reference payload implementations (`baremetal_asm`, `baremetal_lcd`, `hello_world`) demonstrating stack initialization, DHTB header encapsulation, and register handling.

### 2.2 Nokia-2660-Emulator (N2660Emu)
* **Repository**: [https://github.com/lovelytingy/Nokia-2660-Emulator](https://github.com/lovelytingy/Nokia-2660-Emulator)
* **Pinned Commit**: `c67e7029cb6ba268ad26b01b3eb3d5c5f49b01bf`
* **License**: MIT License
* **Core Emulation Engine**: Pure Rust emulator (`crates/n2660-core`) using the Unicorn-engine FFI backend to simulate a single ARM Cortex-A7 (ARMv7-A) core up to 1.0 GHz with bit-exact MMIO hooks.
* **Modeled Peripherals & Subsystems**:
  1. **CPU & Memory Layout**:
     - ARM Cortex-A7 core (ARMv7-A little-endian, high vectors at `0xFFFF0000`, CP15 registers).
     - 256 KB On-chip SYSRAM / IRAM at `0x00000000`.
     - 48 MB LPDDR DRAM mapped at `0x80000000` (extended to 64 MB up to `0x84000000`).
     - 128 MB SPI NAND flash with direct XIP window at `0x60000000`.
  2. **UART Debug Console**:
     - Memory-mapped at `0x70100000` (`SPRD_DEBUG_UART_BASE`).
     - Offset `0x00`: TX FIFO writes (characters routed to host stdout and emulator TX buffer).
     - Offset `0x0C`: Status register STS1 (reports FIFO depth / TX empty).
     - Also models synthetic UART0 at `0x50000000`.
  3. **Keypad Matrix Controller**:
     - Memory-mapped at `0x40250000` (`SPRD_KEYPAD_BASE`).
     - Models `kpd_ctrl`, `kpd_int_en`, `kpd_int_raw`, and `kpd_key_status` (packed 8-bit slices with row/column coordinates).
     - Interrupt assertion to INTC IRQ line 5 (`SPRD_IRQ_KPD`).
  4. **Power & Reset / PMIC EIC**:
     - Models SC2720 PMIC EIC line at `0x40608280` / `0x40608000` for the dedicated Power/End button.
  5. **Timers & Clocks**:
     - System Timer at `0x40230000` (`SPRD_SYSTIMER_BASE`) and Kernel System Timer at `0x40040000` (`SPRD_KERNEL_SYSTIMER_BASE`): monotonic tick counter tied to instruction retirement.
     - Synthetic SPRD timer at `0x50030000` (down-counter generating IRQ 4).
  6. **Interrupt Controller**:
     - SPRD 32-line INTC model (`SprdIntc`) mapped at `0x50050000` and GIC window at `0x12000000`.
     - IRQ assertion and masking (`irq_raw_sts`, `irq_en`, `int_type`).
  7. **Display Subsystem**:
     - SPI1 Controller mapped at `0x70B00000` (`SPRD_APB_SPI1_BASE`).
     - Command / data mode toggling via register offset `0x44` (bit 15).
     - GalaxyCore GC9307 SPI driver emulation (command sequence, frame buffer rendering for 240×320 QVGA).
     - Sitronix ST7735 sub-display emulation (128×160 QQVGA).
  8. **Storage & BootROM / BSL**:
     - Serial Flash Controller (`SPRD_HW_SFC_BASE` at `0x21A00000`).
     - USB BSL protocol framing and libc emulation hooks.
* **How to Run the Emulator**:
  - **Headless binary execution**:
    ```bash
    ./third_party/nokia-2660-emulator/bin/n2660emu <payload.bin> --prefer-pac --entry 0x80100200
    ```
  - **Interactive Terminal UI (TUI)**:
    ```bash
    ./third_party/nokia-2660-emulator/bin/n2660emu <payload.bin> --prefer-pac --entry 0x80100200 -i
    ```
  - **Native Desktop GUI (macOS Cocoa / Linux X11/Wayland)**:
    ```bash
    ./third_party/nokia-2660-emulator/bin/n2660emu <payload.bin> --prefer-pac --entry 0x80100200 --gui
    ```

### 2.3 RT-Thread
* **Repository**: [https://github.com/RT-Thread/rt-thread.git](https://github.com/RT-Thread/rt-thread.git)
* **Pinned Commit**: `37d8b8d1d0b8b39d9238cd86dac2a1ae79d190c1`
* **License**: Apache License 2.0 (`Apache-2.0`)
* **Role in OpenFeatureOS**:
  - Real-Time Operating System kernel foundation providing thread scheduling, synchronization primitives (semaphores, mutexes, event flags, mailboxes, message queues), memory management (heap/slab), and device drivers framework.
  - Architecture adaptation: `libcpu/arm/cortex-a` provides ARMv7-A context switching, interrupt vector dispatching, and cache/MMU primitives.

---

## 3. Code Reuse & Citation Policy

1. **Hardware Register Tagging**:
   - Every hardware constant, base address, bitmask, and shift defined in `board/t107_regs.h` must include a citation tag pointing to its source document in `Nokia-2660-RE`, e.g.:
     ```c
     /* src: Nokia-2660-RE docs/uart-serial-interface.md */
     #define UART1_BASE           0x70100000U
     ```
2. **Undocumented Constants**:
   - For registers or IRQ numbers not explicitly documented in the reference files, placeholders with clear `TODO: Lookup verified address` tags must be placed in `board/t107_regs.h`. Register addresses must never be fabricated.
3. **Hardware Claims**:
   - All status summaries must strictly delineate:
     - `[BUILDS]`: Compiles cleanly with `arm-none-eabi-gcc`.
     - `[EMULATOR]`: Passes test execution in `n2660emu`.
     - `[VERIFIED ON DEVICE]`: Confirmed on physical Nokia 2660 Flip (TA-1480).
4. **Safety Enforcement**:
   - The OS payload runs exclusively in volatile DRAM at `0x80100000` via USB download mode (BSL/FDL1).
   - Under no circumstances will code write to SPI NAND flash, boot partitions, or eFuses.
