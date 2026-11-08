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
#include "timer.h"
#include "printk.h"
#include "ui_app.h"
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
 * Hardware initialization for the Unisoc T107 Keypad APB Scanner
 * and the SC2720 PMIC External Interrupt Controller (EIC).
 */
void keypad_hw_init(void)
{
    /* 1. Ungate APB Power Domain for Keypad Matrix Controller (Bit 8) */
    REG_AON_APB_PWR |= APB_PWR_KPD_ON;

    /* 2. Ungate Keypad Peripheral Clock (Bit 1) */
    REG_AON_CLK_EN |= CLK_KPD_EN;

    /* 3. Soft reset Keypad Controller */
    REG32(0x402E1008U) = 0x100U;
    timer_delay_ms(2);
    REG32(0x402E2008U) = 0x100U;
    timer_delay_ms(2);

    /* 4. Configure Keypad Scanner Parameters */
    KPD_INT_CLR = 0x0FFFU;
    KPD_CLK_DIV = 1U;
    KPD_DEBOUNCE = 16U;
    KPD_INT_EN = 0x0FFFU;
    KPD_POLARITY = 0xFFFFU;

    /* 5. Enable Matrix Grid: Enable all rows (0..7) and columns (0..7), scanner active + long key */
    KPD_CTRL = (0xFFU << 16) | (0xFFU << 8) | 5U;

    /* 6. Configure SC2720 PMIC EIC for Power/End (Pin 1) AND Key 8 (Pin 9) */
    pmic_adi_write(0x406080C8U, pmic_adi_read(0x406080C8U) | 0x10U);  /* Enable EIC clock gate */
    pmic_adi_write(0x40608298U, 0U);
    pmic_adi_write(0x40608C08U, pmic_adi_read(0x40608C08U) | 8U);     /* Power domain ungate */
    pmic_adi_write(0x40608E40U, pmic_adi_read(0x40608E40U) | 1U);     /* Unmask EIC power int */
    pmic_adi_write(0x40608C10U, pmic_adi_read(0x40608C10U) | 8U);     /* Set debouncer filter enable */
    /* Enable debounce mask for Pin 1 (Power) AND Pin 9 (Key 8) */
    pmic_adi_write(0x40608284U, pmic_adi_read(0x40608284U) | (1U << 1) | (1U << 9));

    kprintf("[KEYPAD] APB Scanner & SC2720 PMIC EIC initialized (TA-1480 keymap)\n");
}

static uint32_t s_active_key = 0;
static uint32_t s_active_down = 0;
static uint32_t s_pwr_poll_div = 0;

/*
 * Direct raw keypad polling helper for non-LVGL tasks or diagnostics.
 * Polls APB matrix scanner (0x40250000) and SC2720 PMIC EIC (0x40608280).
 */
uint32_t keypad_poll_raw(uint32_t *out_is_down)
{
    /* 1. Matrix scanning: high-speed APB register access */
    uint32_t raw = KPD_INT_RAW & 0xFFU;

    if (raw & 0x0FU) {
        /* KEY DOWN event: extract packed coordinates from slice 0 */
        uint32_t status = KPD_KEY_STATUS;
        uint32_t col = (status >> 4) & 0x07U;
        uint32_t row = status & 0x07U;
        uint32_t mapped = map_matrix_coords_to_lv_key(col, row);
        if (mapped != 0) {
            s_active_key = mapped;
            s_active_down = 1;
        }
        /* Clear all interrupt flags so subsequent events can register cleanly */
        KPD_INT_CLR = 0x0FFFU;
    } else if (raw & 0xF0U) {
        /* KEY UP event */
        s_active_down = 0;
        KPD_INT_CLR = 0x0FFFU;
    }

    /* 2. Check PMIC EIC: Pin 1 (Power/End) and Pin 9 (Key 8) */
    if (s_active_down == 0) {
        if (++s_pwr_poll_div >= 2) {
            s_pwr_poll_div = 0;
            uint32_t eic_data = pmic_adi_read(0x40608280U);
            if (!((eic_data >> 1) & 1)) {
                /* Active-low: Power / End key pressed */
                s_active_key = LV_KEY_ESC;
                s_active_down = 1;
            } else if ((eic_data >> 9) & 1) {
                /* Active-high: Key 8 pressed */
                s_active_key = '8';
                s_active_down = 1;
            }
        }
    } else if (s_active_key == LV_KEY_ESC || s_active_key == '8') {
        /* Release check for EIC keys */
        if (++s_pwr_poll_div >= 2) {
            s_pwr_poll_div = 0;
            uint32_t eic_data = pmic_adi_read(0x40608280U);
            if (s_active_key == LV_KEY_ESC && ((eic_data >> 1) & 1)) {
                s_active_down = 0;
            } else if (s_active_key == '8' && !((eic_data >> 9) & 1)) {
                s_active_down = 0;
            }
        }
    }

    if (out_is_down) *out_is_down = s_active_down;
    return s_active_key;
}

/*
 * LVGL Input Device Read Callback.
 * Periodically called by lv_timer_handler() to synchronize LVGL indev state.
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
