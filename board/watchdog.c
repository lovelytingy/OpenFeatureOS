/*
 * OpenFeatureOS - Watchdog Driver & Power Management Safety
 * Target: Unisoc T107 (UMS9117) / SC2720 PMIC (Nokia 2660 Flip TA-1480)
 *
 * Sourced from:
 *   - Nokia-2660-RE tools/ram_payload/vendor/fpdoom/ums9117/syscode.c:781-796
 */

#include "watchdog.h"
#include "pmic_adi.h"
#include "t107_regs.h"
#include "printk.h"

void watchdog_init(void)
{
    kprintf("[WDG] Initializing hardware watchdog handler...\n");
    /* Ensure watchdog is disabled during OS bootstrap */
    watchdog_disable();
    kprintf("[WDG] SC2720 PMIC hardware watchdog disabled (safe DRAM execution)\n");
}

void watchdog_disable(void)
{
    /* src: Nokia-2660-RE tools/ram_payload/vendor/fpdoom/ums9117/syscode.c:788 */
    /* Unlock SC2720 PMIC watchdog write registers */
    pmic_adi_write(SC2720_WDG_LOCK, SC2720_WDG_UNLOCK_KEY);

    /* Disable watchdog timer in control register */
    pmic_adi_write(SC2720_WDG_CTRL, 0);

    /* Relock watchdog register with inverted unlock key */
    pmic_adi_write(SC2720_WDG_LOCK, (uint16_t)~SC2720_WDG_UNLOCK_KEY);
}

void watchdog_pet(void)
{
    /* Unlock SC2720 PMIC watchdog */
    pmic_adi_write(SC2720_WDG_LOCK, SC2720_WDG_UNLOCK_KEY);

    /* Reload / pet counter */
    uint32_t ctrl = pmic_adi_read(SC2720_WDG_CTRL);
    pmic_adi_write(SC2720_WDG_CTRL, ctrl | SC2720_WDG_CTRL_RST);

    /* Relock watchdog register */
    pmic_adi_write(SC2720_WDG_LOCK, (uint16_t)~SC2720_WDG_UNLOCK_KEY);
}
