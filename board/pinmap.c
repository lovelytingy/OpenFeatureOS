/*
 * OpenFeatureOS - Unisoc T107 Pinmux & Pad Controller Setup
 * Target: Nokia 2660 Flip (TA-1480 / HW3.0)
 *
 * Sourced from:
 *   - Nokia 2660 Flip hardware dump pinmap.bin (288 entries)
 *   - Nokia-2660-RE tools/ram_payload/vendor/fpdoom/ums9117/syscode.c:105-123
 */

#include "pinmap.h"
#include "pinmap_data.h"
#include "pmic_adi.h"
#include "t107_regs.h"
#include "printk.h"

void pinmap_init(void)
{
    /* 1. Ungate PIN APB clock (bit 20 = 0x100000) and pad controller ungate (0x1000) */
    REG32(0x402E1000U) |= 0x100000U;
    REG32(0x402E10B0U) |= 0x1000U;

    /* 2. Apply hardware pin configuration table (288 entries) */
    for (uint32_t i = 0; i < PINMAP_ENTRY_COUNT; i++) {
        uint32_t addr = g_pinmap_data[i].addr;
        uint32_t val  = g_pinmap_data[i].val;

        if (addr - 0x40608000U < 0x1000U) {
            /* ADI PMIC pin configuration */
            pmic_adi_write(addr, val & 0xFFFFU);
        } else if (addr - 0x402A0000U < 0x1000U) {
            /* SoC Pad multiplexer register */
            REG32(addr) = val;
        }
    }

    /* 3. Mask IOMUX status register */
    REG32(0x402A0018U) &= 0xE0000000U;

    kprintf("[PINMAP] Configured %u pinmux pads (SPI1, LCM, Keypad routed)\n",
            PINMAP_ENTRY_COUNT);
}
