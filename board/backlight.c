/*
 * OpenFeatureOS - SC2720 PMIC LCD Backlight Controller
 * Target: Nokia 2660 Flip (TA-1480 / HW3.0)
 *
 * Sourced from:
 *   - Nokia-2660-RE tools/ram_payload/vendor/fpdoom/ums9117/syscode.c:127-186
 *   - Nokia-2660-RE tools/ram_payload/run_hello_world.sh:27-33
 */

#include "backlight.h"
#include "pmic_adi.h"
#include "printk.h"

#define PMIC_BL_PWR1     0x40608C08U
#define PMIC_BL_CLK      0x40608C10U
#define PMIC_BL_LP       0x40608DECU
#define PMIC_BL_OUT      0x406081C8U
#define PMIC_BL_MASK     0x40608180U
#define PMIC_BL_CTRL     0x406081D8U

#define PMIC_BL_SINK_R   0x406081B8U
#define PMIC_BL_SINK_G   0x406081BCU
#define PMIC_BL_SINK_B   0x406081C0U
#define PMIC_BL_SINK_W   0x406081C4U

void backlight_init(void)
{
    /* 1. Enable backlight power rails (bit 9 = 0x200) */
    pmic_adi_write(PMIC_BL_PWR1, pmic_adi_read(PMIC_BL_PWR1) | 0x200U);

    /* 2. Enable backlight clock (bit 7 = 0x80) */
    pmic_adi_write(PMIC_BL_CLK, pmic_adi_read(PMIC_BL_CLK) | 0x80U);

    /* 3. Clear low power mode bits (~5) */
    pmic_adi_write(PMIC_BL_LP, pmic_adi_read(PMIC_BL_LP) & ~5U);

    /* 4. Clear output control override (~0xFF) */
    pmic_adi_write(PMIC_BL_OUT, pmic_adi_read(PMIC_BL_OUT) & ~0xFFU);

    /* 5. Clear all 4 current sink channels */
    for (uint32_t i = 0; i < 4; i++) {
        pmic_adi_write(PMIC_BL_SINK_R + (i * 4U),
                       pmic_adi_read(PMIC_BL_SINK_R + (i * 4U)) & ~0x3FU);
    }
    kprintf("[BACKLIGHT] SC2720 PMIC Backlight Controller powered up\n");
}

void backlight_set_brightness(uint32_t percent)
{
    if (percent > 100U) percent = 100U;

    /* Disable PWM flickering (new method: bit 0 = 0 when on) */
    uint32_t ctrl = pmic_adi_read(PMIC_BL_CTRL) | 1U;
    if (percent > 0) {
        ctrl &= ~1U;
    }
    pmic_adi_write(PMIC_BL_CTRL, ctrl);

    if (percent == 0) {
        pmic_adi_write(PMIC_BL_MASK, 0);
        pmic_adi_write(PMIC_BL_SINK_R, pmic_adi_read(PMIC_BL_SINK_R) & ~0x3FU);
        pmic_adi_write(PMIC_BL_SINK_G, pmic_adi_read(PMIC_BL_SINK_G) & ~0x3FU);
        pmic_adi_write(PMIC_BL_SINK_B, pmic_adi_read(PMIC_BL_SINK_B) & ~0x3FU);
        pmic_adi_write(PMIC_BL_SINK_W, pmic_adi_read(PMIC_BL_SINK_W) & ~0x3FU);
        return;
    }

    /*
     * Enable both RGB sinks (Main 2.8" panel: bits 2..3, 6..7, 10..11 = 0x0CCC)
     * AND White sink (Cover 1.77" panel: bits 12..13 = 0x3000).
     * Combined mask: 0x3CCC.
     */
    pmic_adi_write(PMIC_BL_MASK, 0x3CCCU);

    /* Main screen RGB base sink target: 0x19 (25 decimal) */
    uint32_t rgb_val = (percent * 0x19U + 50U) / 100U;
    if (rgb_val > 0x3FU) rgb_val = 0x3FU;

    /* Cover screen White base sink target: 0x2D (45 decimal) */
    uint32_t w_val = (percent * 0x2DU + 50U) / 100U;
    if (w_val > 0x3FU) w_val = 0x3FU;

    pmic_adi_write(PMIC_BL_SINK_R, (pmic_adi_read(PMIC_BL_SINK_R) & ~0x3FU) | rgb_val);
    pmic_adi_write(PMIC_BL_SINK_G, (pmic_adi_read(PMIC_BL_SINK_G) & ~0x3FU) | rgb_val);
    pmic_adi_write(PMIC_BL_SINK_B, (pmic_adi_read(PMIC_BL_SINK_B) & ~0x3FU) | rgb_val);
    pmic_adi_write(PMIC_BL_SINK_W, (pmic_adi_read(PMIC_BL_SINK_W) & ~0x3FU) | w_val);

    kprintf("[BACKLIGHT] Brightness set to %u%% (RGB=0x%02x, White=0x%02x)\n",
            percent, rgb_val, w_val);
}
