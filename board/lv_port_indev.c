/*
 * OpenFeatureOS - LVGL Keypad Input Driver Port for Unisoc T107 (UMS9117)
 * Target: Nokia 2660 Flip (TA-1480 / HW3.0) 5x5 Keypad Matrix & SC2720 PMIC EIC
 *
 * Physical Matrix Layout: 5 Columns x 5 Rows (23 keys + 2 EIC lines)
 * Sourced directly from verified hardware documentation:
 *   - Nokia-2660-RE docs/keypad-matrix.md
 *   - Nokia-2660-RE tools/ram_payload/src/keypad_test/keypad_test_main.c
 *   - Nokia-2660-RE tools/ram_payload/vendor/fpdoom/ums9117/syscode.c
 */

#include "lv_port_indev.h"
#include "t107_regs.h"
#include "pmic_adi.h"
#include "printk.h"
#include <rtthread.h>

lv_group_t *g_lvgl_keypad_group = NULL;
static lv_indev_drv_t indev_drv;

/*
 * Maps Nokia 2660 Flip 5x5 matrix physical coordinates (col, row)
 * to LVGL navigation and character keys.
 */
static uint32_t map_matrix_coords_to_lv_key(uint32_t col, uint32_t row)
{
    if (col == 0) {
        if (row == 0) return '*';
        if (row == 1) return '7';
        if (row == 2) return '1';
        if (row == 3) return 'C';             /* Green Call / Dial Key */
        if (row == 4) return '4';
    } else if (col == 1) {
        if (row == 0) return '0';
        if (row == 1) return '3';
        if (row == 2) return '2';
        if (row == 3) return LV_KEY_DOWN;     /* D-Pad Down */
        if (row == 4) return '5';
    } else if (col == 2) {
        if (row == 0) return LV_KEY_HOME;     /* Shortcut Key */
        if (row == 1) return LV_KEY_LEFT;     /* D-Pad Left */
        if (row == 2) return '8';             /* Key 8 matrix crosspoint */
        if (row == 3) return LV_KEY_ENTER;    /* Center OK / Select Key */
        if (row == 4) return LV_KEY_UP;       /* D-Pad Up */
    } else if (col == 3) {
        if (row == 0) return '#';
        if (row == 1) return '9';
        if (row == 2) return '6';
        if (row == 3) return LV_KEY_RIGHT;    /* D-Pad Right */
        if (row == 4) return LV_KEY_BACKSPACE;/* Back Key */
    } else if (col == 4) {
        if (row == 0) return LV_KEY_PREV;     /* Left Selection Key (LSK) */
        if (row == 1) return LV_KEY_NEXT;     /* Right Selection Key (RSK) */
        if (row == 2) return 'S';             /* Dedicated SOS / Emergency Key */
        if (row == 3) return LV_KEY_UP;       /* Side Volume Up */
        if (row == 4) return LV_KEY_DOWN;     /* Side Volume Down */
    }
    return 0;
}

/*
 * Hardware initialization for the Unisoc T107 5x5 Keypad APB Scanner
 * and the SC2720 PMIC External Interrupt Controller (EIC).
 */
void keypad_hw_init(void)
{
    /* 1. Ungate APB Power Domain for Keypad Matrix Controller (Bit 8) */
    REG32(0x402E1000U) |= 0x100U;

    /* 2. Ungate Keypad Peripheral Clock (Bit 1) */
    REG32(0x402E1010U) |= 0x002U;

    /* 3. Soft reset Keypad Controller */
    REG32(0x402E1008U) = 0x100U;
    rt_thread_mdelay(2);
    REG32(0x402E2008U) = 0x100U;
    rt_thread_mdelay(2);

    /* 4. Configure Keypad Scanner Parameters */
    KPD_INT_CLR = 0x0FFFU;
    KPD_CLK_DIV = 1U;
    KPD_DEBOUNCE = 16U;
    KPD_INT_EN = 0x0FFFU;
    KPD_POLARITY = 0xFFFFU;

    /* 5. Enable 5x5 Matrix Grid (Cols 0..4, Rows 0..4 enabled for TA-1480) */
    KPD_CTRL = (1U << 0) | (4U << 0) | (0x1FU << 16) | (0x1FU << 8);

    /* 6. Configure SC2720 PMIC EIC for Power/End button (Pin 1) & Key 8 (Pin 9) */
    pmic_adi_write(0x406080C8U, pmic_adi_read(0x406080C8U) | 0x10U);
    pmic_adi_write(0x40608298U, 0U);
    pmic_adi_write(0x40608C08U, pmic_adi_read(0x40608C08U) | 8U);
    pmic_adi_write(0x40608E40U, pmic_adi_read(0x40608E40U) | 1U);
    pmic_adi_write(0x40608C10U, pmic_adi_read(0x40608C10U) | 8U);
    pmic_adi_write(0x40608284U, pmic_adi_read(0x40608284U) | (1U << 1) | (1U << 9));

    kprintf("[KEYPAD] 5x5 APB Scanner & SC2720 PMIC EIC initialized (TA-1480 keymap)\n");
}

/*
 * Direct raw keypad polling helper for non-LVGL tasks or diagnostics.
 */
uint32_t keypad_poll_raw(uint32_t *out_is_down)
{
    uint32_t key = 0;
    int is_down = 0;

    /* SC2720 PMIC Power Button (EIC Pin 1) */
    uint32_t eic_data = pmic_adi_read(0x40608280U);
    if (!((eic_data >> 1) & 1)) {
        key = LV_KEY_ESC;
        is_down = 1;
    } else if ((eic_data >> 9) & 1) {
        key = '8';
        is_down = 1;
    } else {
        uint32_t raw = KPD_INT_RAW & 0xFFU;
        if (raw & 0x0FU) {
            uint32_t status = KPD_KEY_STATUS;
            uint32_t col = status & 0x07U;
            uint32_t row = (status >> 4) & 0x07U;
            key = map_matrix_coords_to_lv_key(col, row);
            is_down = 1;
        }
        if (raw) {
            KPD_INT_CLR = 0x0FFFU;
        }
    }

    if (out_is_down) *out_is_down = is_down;
    return key;
}

/*
 * LVGL Input Device Read Callback.
 * Periodically called by lv_timer_handler() to poll hardware keypad state.
 */
static void keypad_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    (void)drv;
    static uint32_t s_last_key = 0;
    uint32_t is_down = 0;
    uint32_t key = keypad_poll_raw(&is_down);

    if (is_down && key != 0) {
        s_last_key = key;
        data->state = LV_INDEV_STATE_PRESSED;
        data->key = key;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
        data->key = s_last_key;
    }
}

void lv_port_indev_init(void)
{
    /* Initialize physical keypad matrix hardware */
    keypad_hw_init();

    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_KEYPAD;
    indev_drv.read_cb = keypad_read_cb;

    lv_indev_t *keypad_indev = lv_indev_drv_register(&indev_drv);

    /* Create the default focus group for keypad-driven UI navigation */
    g_lvgl_keypad_group = lv_group_create();
    lv_group_set_default(g_lvgl_keypad_group);
    lv_indev_set_group(keypad_indev, g_lvgl_keypad_group);

    kprintf("[LVGL:INDEV] Keypad indev registered with default navigation group.\n");
}
