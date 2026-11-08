/*
 * OpenFeatureOS - Hardware System Timer Driver
 * Target: Unisoc T107 (UMS9117) / Nokia 2660 Flip (TA-1480)
 *
 * Sourced from:
 *   - Nokia-2660-RE tools/ram_payload/src/baremetal_lcd/payload.c:24
 *   - Nokia-2660-RE tools/ram_payload/vendor/fpdoom/ums9117/syscomm.c:138
 *   - Nokia-2660-RE tools/emulator/ums9117_emu.py:294
 */

#include "timer.h"
#include "t107_regs.h"
#include "printk.h"

static uint32_t timer_boot_offset = 0;

void timer_init(void)
{
    /* Capture boot timestamp from 1ms free-running hardware counter */
    timer_boot_offset = TIMER_1MS_COUNT;
    kprintf("[TIMER] Initialized 1ms hardware system timer @ 0x%08x (base tick: %u ms)\n",
            TIMER_1MS_BASE + 0x0CU, timer_boot_offset);
}

uint32_t timer_get_ms(void)
{
    return TIMER_1MS_COUNT - timer_boot_offset;
}

void timer_delay_ms(uint32_t ms)
{
    uint32_t start = TIMER_1MS_COUNT;
    while ((TIMER_1MS_COUNT - start) < ms) {
        /* Busy wait on free-running hardware 1ms counter */
    }
}
