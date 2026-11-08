/*
 * OpenFeatureOS - Unisoc T107 (UMS9117) Hardware Register Map
 * Target: Nokia 2660 Flip (TA-1480 / HW3.0)
 *
 * All hardware constants are sourced directly from Nokia-2660-RE documentation.
 * Undocumented peripherals contain clear placeholders tagged with TODO.
 */

#ifndef T107_REGS_H_
#define T107_REGS_H_

#include <stdint.h>

#define REG32(addr)                 (*(volatile uint32_t *)(uintptr_t)(addr))
#define REG16(addr)                 (*(volatile uint16_t *)(uintptr_t)(addr))
#define REG8(addr)                  (*(volatile uint8_t *)(uintptr_t)(addr))

/* ============================================================================
 * 1. Physical Memory Map
 * ============================================================================
 */

/* src: Nokia-2660-RE docs/memory-map.md */
#define T107_BOOTROM_BASE           0x00000000U /* On-chip BootROM & Vector window (64 KB) */
#define T107_FDL1_DOWNLOAD_ADDR     0x00006200U /* BootROM download address in internal SRAM */

/* src: Nokia-2660-RE docs/memory-map.md */
#define T107_IRAM_BASE              0x40000000U /* Internal SRAM / IRAM window (~128 KB) */
#define T107_IRAM_SIZE              (128U * 1024U)

/* src: Nokia-2660-RE docs/hardware.md */
#define T107_DRAM_BASE              0x80000000U /* 48 MB SIP LPDDR DRAM Base */
#define T107_DRAM_SIZE              (48U * 1024U * 1024U)

/* src: Nokia-2660-RE docs/custom-code-execution.md */
#define T107_PAYLOAD_LOAD_ADDR      0x80100000U /* DRAM Payload Base (FDL1 USB injection) */
#define T107_PAYLOAD_ENTRY_ADDR     0x80100200U /* Execution Entry Point (after 512-byte DHTB) */

/* src: Nokia-2660-RE docs/memory-map.md */
#define T107_KERNEL_LOAD_ADDR       0x80A05E00U /* Stock MOCOR Kernel Load Address */
#define T107_USER_IMG_ADDR          0x88000000U /* Userland MMI Application Address */

/* src: Nokia-2660-RE docs/hardware.md */
#define T107_CHIP_ID_VAL            0x98180001U /* Unisoc T107 / UMS9117 Hardware Chip ID */

/* Telemetry Scratchpad in DRAM */
#define T107_DRAM_MAILBOX           0x80700100U /* DRAM Status Mailbox (non-destructive) */

/* ============================================================================
 * 2. UART Serial Interface (Debug Console)
 * ============================================================================
 */

/* src: Nokia-2660-RE docs/uart-serial-interface.md */
#define UART1_BASE                  0x70100000U /* Primary Debug Serial Console (1.8V TTL) */
#define UART1_TXD                   REG32(UART1_BASE + 0x00U) /* Transmit Data FIFO (WO) */
#define UART1_RXD                   REG32(UART1_BASE + 0x04U) /* Receive Data FIFO (RO) */
#define UART1_STS1                  REG32(UART1_BASE + 0x0CU) /* FIFO Status: [15:8] TX, [7:0] RX */
#define UART1_IEN                   REG32(UART1_BASE + 0x10U) /* Interrupt Enable Mask */
#define UART1_ICLR                  REG32(UART1_BASE + 0x14U) /* Interrupt Clear */
#define UART1_LCR                   REG32(UART1_BASE + 0x18U) /* Line Control (0x1C = 8N1) */
#define UART1_STS0                  REG32(UART1_BASE + 0x1CU) /* Line Status */
#define UART1_FCR                   REG32(UART1_BASE + 0x20U) /* FIFO Control */
#define UART1_CLKD0                 REG32(UART1_BASE + 0x24U) /* Baud Divisor Low 16 bits */
#define UART1_CLKD1                 REG32(UART1_BASE + 0x28U) /* Baud Divisor High 16 bits */

/* src: Nokia-2660-RE docs/uart-serial-interface.md */
#define UART_CLOCK_FREQ             26000000U   /* 26 MHz primary crystal (26M_XTL) */
#define UART_BAUD_115200_DIV        226U        /* 0x00E2 divisor for 115200 bps */
#define UART_LCR_8N1                0x001CU     /* 8 data bits, no parity, 1 stop bit */

/* src: Nokia-2660-RE docs/uart-serial-interface.md */
#define REG_APB_CLK_EN              REG32(0x71300000U) /* APB Clock Gating Register */
#define APB_CLK_UART1_EN            (0x3U << 13)       /* Bits 13 & 14: UART1 clock ungate */

/* src: Nokia-2660-RE docs/uart-serial-interface.md */
#define REG_PINMUX_UART1            REG32(0x402A0018U) /* Pin Controller / IOMUX Register */

/* ============================================================================
 * 3. Analog Die Interface (ADI) & SC2720 PMIC
 * ============================================================================
 */

/* src: Nokia-2660-RE docs/hardware.md */
#define ADI_CTRL_BASE               0x40600000U /* ADI Master Bus Controller */
#define ADI_RD_CMD                  REG32(ADI_CTRL_BASE + 0x28U) /* Read Address Trigger */
#define ADI_RD_DATA                 REG32(ADI_CTRL_BASE + 0x2CU) /* Read Data Register */
#define ADI_ARM_STS                 REG32(ADI_CTRL_BASE + 0x30U) /* Bus Status (Bit 11 = Busy) */

/* src: Nokia-2660-RE docs/flashlight.md */
#define SC2720_PMIC_TORCH_REG       0x40608DF4U /* White LED Torch / Flashlight Driver */
#define SC2720_TORCH_EN             (1U << 15)  /* Bit 15: LED Master Power Gate */
#define SC2720_TORCH_LVL_MASK       0x000FU     /* Bits [3:0]: Brightness Current (0..15) */

/* src: Nokia-2660-RE docs/haptics.md */
#define SC2720_PMIC_LDO_VIBR_REG    0x40608E00U /* Haptic Vibration Motor LDO Regulator */
#define SC2720_VIBR_EN_MASK         (0x3U << 8) /* Bits [9:8]: LDO Enable (write 0x3) */

/* src: Nokia-2660-RE docs/hardware.md */
#define SC2720_PMIC_CLK_GATE        0x40608C08U /* PMIC Clock Gate: Bit 4 Audio, Bit 2 Watchdog */

/* src: Nokia-2660-RE docs/keypad-matrix.md */
#define SC2720_PMIC_EIC_BASE        0x40608000U /* SC2720 PMIC External Interrupt Controller */
#define SC2720_EIC_PWRKEY_REG       REG32(0x40608280U) /* Pin 1: Hardware Power/End Key */
#define SC2720_EIC_PWRKEY_BIT       (1U << 1)          /* Bit 1: Active state of Power Key */

/* ============================================================================
 * 4. APB Hardware Keypad Scanner
 * ============================================================================
 */

/* src: Nokia-2660-RE docs/keypad-matrix.md */
#define KPD_BASE                    0x40250000U /* 5x5 APB Keypad Matrix Controller */
#define KPD_CTRL                    REG32(KPD_BASE + 0x00U) /* Control Register */
#define KPD_INT_EN                  REG32(KPD_BASE + 0x04U) /* Interrupt Enable Mask */
#define KPD_INT_RAW                 REG32(KPD_BASE + 0x08U) /* Raw Event Status */
#define KPD_INT_MASK                REG32(KPD_BASE + 0x0CU) /* Masked Interrupt Status */
#define KPD_INT_CLR                 REG32(KPD_BASE + 0x10U) /* Interrupt Clear Register */
#define KPD_POLARITY                REG32(KPD_BASE + 0x18U) /* Line Polarity (0xFFFF default) */
#define KPD_DEBOUNCE                REG32(KPD_BASE + 0x1CU) /* Debounce Filter Duration */
#define KPD_LONG_KEY                REG32(KPD_BASE + 0x20U) /* Long Press Threshold */
#define KPD_SLEEP_CNT               REG32(KPD_BASE + 0x24U) /* Sleep Timer Counter */
#define KPD_CLK_DIV                 REG32(KPD_BASE + 0x28U) /* Scanner Clock Divider */
#define KPD_KEY_STATUS              REG32(KPD_BASE + 0x2CU) /* Real-Time Packed Coordinates */

/* src: Nokia-2660-RE docs/keypad-matrix.md */
#define KPD_CTRL_ENABLE             (1U << 0)   /* Scanner Enable */
#define KPD_COL_MASK_TA1480         (0x1FU << 8)  /* 5 Active Columns (Cols 0..4) */
#define KPD_ROW_MASK_TA1480         (0x1FU << 16) /* 5 Active Rows (Rows 0..4) */

/* src: Nokia-2660-RE docs/keypad-matrix.md */
#define REG_AON_APB_PWR             REG32(0x402E1000U) /* AON APB Power Domain Control */
#define APB_PWR_KPD_ON              (1U << 8)          /* Bit 8: Keypad Power Domain On */
#define REG_AON_CLK_EN              REG32(0x402E1010U) /* AON Clock Enable Control */
#define CLK_KPD_EN                  (1U << 1)          /* Bit 1: Keypad Clock Enable */

/* ============================================================================
 * 5. Display Subsystem (SPI1 & GalaxyCore GC9307)
 * ============================================================================
 */

/* src: Nokia-2660-RE docs/hardware.md */
#define SPI1_BASE                   0x70B00000U /* SPI1 Bus Controller (Main LCD) */
#define SPI1_TXD                    REG32(SPI1_BASE + 0x00U) /* Transmit FIFO (WO) */
#define SPI1_INT_RAW                REG32(SPI1_BASE + 0x28U) /* Interrupt Raw Status */
#define SPI1_STATUS                 REG32(SPI1_BASE + 0x34U) /* Bus Status */
#define SPI1_CTRL                   REG32(SPI1_BASE + 0x44U) /* Control (Bit 15 = D/C select) */
#define SPI1_CTRL_DC_DATA           (1U << 15)  /* Bit 15: 1 = Data, 0 = Command */

/* src: Nokia-2660-RE docs/hardware.md */
#define GC9307_CHIP_ID              0x009307U   /* GalaxyCore GC9307 ReadID (240x320 QVGA) */
#define GC9307_WIDTH                240U
#define GC9307_HEIGHT               320U

/* src: Nokia-2660-RE docs/hardware.md */
#define GPIO_SPEAKER_PA_EN          31U         /* GPIO 31: Speaker Class-D PA Enable */

/* ============================================================================
 * 6. Auxiliary Cortex-M4 Power Management Core
 * ============================================================================
 */

/* src: Nokia-2660-RE docs/cortex-m4-power-management.md */
#define CM4_IRAM_BASE               0x00000000U /* Cortex-M4 Code IRAM (128 KB) */
#define CM4_SRAM_BASE               0x20000000U /* Cortex-M4 Static Data SRAM (64 KB) */
#define CM4_SHARED_DDR_BASE         0x80700000U /* Inter-Core IPC Ringbuffer Window */

/* ============================================================================
 * 7. System Timers & Watchdog Subsystems
 * ============================================================================
 */

/* src: Nokia-2660-RE tools/ram_payload/src/baremetal_lcd/payload.c:24 */
/* src: Nokia-2660-RE tools/ram_payload/vendor/fpdoom/ums9117/syscomm.c:138 */
/* src: Nokia-2660-RE tools/emulator/ums9117_emu.py:294 */
#define TIMER_1MS_BASE              0x40230000U /* 1 ms Free-Running System Timer Base */
#define TIMER_1MS_COUNT             REG32(TIMER_1MS_BASE + 0x0CU) /* 1 ms Free-Running Counter (RO) */

/* src: Nokia-2660-RE tools/ram_payload/vendor/fpdoom/ums9117/syscode.c:781-796 */
#define SC2720_PMIC_WDG_BASE        0x40608040U /* SC2720 Watchdog Base via ADI Bus */
#define SC2720_WDG_LOAD_LOW         (SC2720_PMIC_WDG_BASE + 0x00U) /* Counter Reload Low Word */
#define SC2720_WDG_LOAD_HIGH        (SC2720_PMIC_WDG_BASE + 0x04U) /* Counter Reload High Word */
#define SC2720_WDG_CTRL             (SC2720_PMIC_WDG_BASE + 0x08U) /* Watchdog Control Register */
#define SC2720_WDG_LOCK             (SC2720_PMIC_WDG_BASE + 0x20U) /* Lock Register (Key: 0xE551) */
#define SC2720_WDG_UNLOCK_KEY       0xE551U                        /* Unlock magic sequence */
#define SC2720_WDG_CTRL_EN          (1U << 0)                      /* Bit 0: Watchdog Timer Enable */
#define SC2720_WDG_CTRL_RST         (1U << 1)                      /* Bit 1: Reload / Pet Counter */

/* TODO: Look up hardware Watchdog registers on the Cortex-A7 APB bus if independent of PMIC */
#define T107_APB_WDG_BASE_PLACEHOLDER 0x40220000U /* TODO: Verify APB SoC Watchdog Base */

/* ============================================================================
 * 8. Interrupt Controller (SPRD INTC & ARM GIC-400)
 * ============================================================================
 */

/* src: Nokia-2660-Emulator crates/n2660-core/src/types.rs:112 */
/* src: Nokia-2660-Emulator crates/n2660-core/src/peripherals/intc.rs */
#define SPRD_INTC_BASE              0x50050000U /* Unisoc 32-Line Interrupt Controller Base */
#define SPRD_INTC_IRQ_RAW_STS       REG32(SPRD_INTC_BASE + 0x00U) /* IRQ Raw Status (RO) */
#define SPRD_INTC_IRQ_MASK_STS      REG32(SPRD_INTC_BASE + 0x04U) /* IRQ Masked Status (RO) */
#define SPRD_INTC_IRQ_EN            REG32(SPRD_INTC_BASE + 0x08U) /* IRQ Enable Set (WO: write 1 to enable) */
#define SPRD_INTC_IRQ_DIS           REG32(SPRD_INTC_BASE + 0x0CU) /* IRQ Enable Clear (WO: write 1 to disable) */
#define SPRD_INTC_IRQ_SOFT          REG32(SPRD_INTC_BASE + 0x10U) /* Software IRQ Set */
#define SPRD_INTC_IRQ_SOFT_CLR      REG32(SPRD_INTC_BASE + 0x14U) /* Software IRQ Clear */
#define SPRD_INTC_IRQ_RAW_CLR       REG32(SPRD_INTC_BASE + 0x18U) /* Raw IRQ Clear */
#define SPRD_INTC_FIQ_RAW_STS       REG32(SPRD_INTC_BASE + 0x1CU) /* FIQ Raw Status (RO) */
#define SPRD_INTC_FIQ_MASK_STS      REG32(SPRD_INTC_BASE + 0x20U) /* FIQ Masked Status (RO) */
#define SPRD_INTC_FIQ_EN            REG32(SPRD_INTC_BASE + 0x24U) /* FIQ Enable Set */
#define SPRD_INTC_FIQ_DIS           REG32(SPRD_INTC_BASE + 0x28U) /* FIQ Enable Clear */
#define SPRD_INTC_FIQ_SOFT          REG32(SPRD_INTC_BASE + 0x2CU) /* Software FIQ Set */
#define SPRD_INTC_FIQ_SOFT_CLR      REG32(SPRD_INTC_BASE + 0x30U) /* Software FIQ Clear */
#define SPRD_INTC_TYPE              REG32(SPRD_INTC_BASE + 0x34U) /* 0 = IRQ route, 1 = FIQ route */

/* src: Nokia-2660-Emulator crates/n2660-core/src/types.rs:131-136 */
#define SPRD_IRQ_UART0              0U  /* Hardware UART0 Interrupt */
#define SPRD_IRQ_TIMER              4U  /* System Timer Periodic Interrupt */
#define SPRD_IRQ_KEYPAD             5U  /* APB Keypad Matrix Scanner Interrupt */
#define SPRD_IRQ_GPIO               6U  /* GPIO Pin Change Interrupt */
#define SPRD_IRQ_SPI0               8U  /* SPI0 Controller Interrupt */
#define SPRD_IRQ_SPI1               9U  /* SPI1 Controller Interrupt (Main LCD) */

/* src: Nokia-2660-Emulator crates/n2660-core/src/types.rs:31 */
#define SPRD_GIC_BASE               0x12000000U /* ARM Cortex-A7 MPCore GIC Base */
#define GICD_BASE                   (SPRD_GIC_BASE + 0x1000U) /* GIC Distributor Base */
#define GICC_BASE                   (SPRD_GIC_BASE + 0x2000U) /* GIC CPU Interface Base */

#define GICD_CTLR                   REG32(GICD_BASE + 0x000U)
#define GICD_TYPER                  REG32(GICD_BASE + 0x004U)
#define GICC_CTLR                   REG32(GICC_BASE + 0x000U)
#define GICC_PMR                    REG32(GICC_BASE + 0x004U)
#define GICC_IAR                    REG32(GICC_BASE + 0x00CU)
#define GICC_EOIR                   REG32(GICC_BASE + 0x010U)

/* ============================================================================
 * 9. Undocumented Hardware Registers & TODO Placeholders
 * ============================================================================
 */

/* TODO: Look up verified hardware base address for AON timer if distinct from 0x40230000 */
#define T107_AON_SYSTIMER_PLACEHOLDER 0x40040000U /* TODO: Verify AON System Timer Base */
#define T107_IRQ_UART1_PLACEHOLDER  1U          /* TODO: Verify UART1 IRQ Line */

#endif /* T107_REGS_H_ */
