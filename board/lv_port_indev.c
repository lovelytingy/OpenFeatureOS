/*
 * OpenFeatureOS - LVGL Keypad Input Driver Port for Unisoc T107 (UMS9117)
 * Target: Nokia 2660 Flip (TA-1480 / HW3.0) 5x5 Keypad Matrix & SC2720 PMIC EIC
 *
 * Sourced directly from proven, device-tested reference code:
 *   - Nokia-2660-RE tools/ram_payload/src/keypad_test/keypad_test_main.c
 *   - Nokia-2660-RE tools/ram_payload/vendor/fpdoom/ums9117/syscode.c
 *   - Nokia-2660-RE docs/keypad-matrix.md
 */

#include "lv_port_indev.h"
#include "t107_regs.h"
#include "pmic_adi.h"
#include "timer.h"
#include "printk.h"
#include <rtthread.h>

lv_group_t *g_lvgl_keypad_group = NULL;
static lv_indev_drv_t indev_drv;

static keypad_event_cb_t s_event_cb = NULL;
static uint32_t s_active_key = 0;
static uint32_t s_active_down = 0;
static int s_active_code = 0;

/*
 * Verified Nokia 2660 Flip TA-1480 64-entry Keymap Table.
 * Extracted directly from kernel.bin at 0x814f5ac4 (tools/ram_payload/data/keymap.bin).
 * Index formula: k = (row << 3) | col = row * 8 + col.
 */
static const uint8_t s_keymap_t1480[64] = {
    /* Row 0 */
    /* Col 0: '*' */       0x2A,
    /* Col 1: '0' */       0x30,
    /* Col 2: SHORTCUT */  0x1F,
    /* Col 3: '#' */       0x23,
    /* Col 4: LSK */       0x08,
    /* Col 5..7 */         0xFF, 0xFF, 0xFF,

    /* Row 1 */
    /* Col 0: '7' */       0x37,
    /* Col 1: '3' */       0x33,
    /* Col 2: LEFT */      0x06,
    /* Col 3: '9' */       0x39,
    /* Col 4: RSK */       0x09,
    /* Col 5..7 */         0xFF, 0xFF, 0xFF,

    /* Row 2 */
    /* Col 0: '1' */       0x31,
    /* Col 1: '2' */       0x32,
    /* Col 2: (EIC9) */    0xFF,
    /* Col 3: '6' */       0x36,
    /* Col 4: SOS */       0x29,
    /* Col 5..7 */         0xFF, 0xFF, 0xFF,

    /* Row 3 */
    /* Col 0: CALL */      0x01,
    /* Col 1: DOWN */      0x05,
    /* Col 2: CENTER OK */ 0x0D,
    /* Col 3: RIGHT */     0x07,
    /* Col 4: VOL+ */      0x24,
    /* Col 5..7 */         0xFF, 0xFF, 0xFF,

    /* Row 4 */
    /* Col 0: '4' */       0x34,
    /* Col 1: '5' */       0x35,
    /* Col 2: UP */        0x04,
    /* Col 3: BACK */      0x0E,
    /* Col 4: VOL- */      0x25,
    /* Col 5..7 */         0xFF, 0xFF, 0xFF,

    /* Rows 5..6 (unused) */
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

    /* Row 7: Index 63 (0x3F) is dedicated EIC Pin 9 = Numeric Key '8' */
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x38
};

/*
 * Maps hardware keycodes to LVGL navigation and character keys.
 */
uint32_t hw_code_to_lv_key(int code)
{
    switch (code) {
    case 0x01: return 'C';                /* CALL / DIAL */
    case 0x02: return LV_KEY_ESC;         /* POWER / END */
    case 0x04: return LV_KEY_UP;          /* D-PAD UP */
    case 0x05: return LV_KEY_DOWN;        /* D-PAD DOWN */
    case 0x06: return LV_KEY_LEFT;        /* D-PAD LEFT */
    case 0x07: return LV_KEY_RIGHT;       /* D-PAD RIGHT */
    case 0x08: return LV_KEY_PREV;        /* LSK */
    case 0x09: return LV_KEY_NEXT;        /* RSK */
    case 0x0D: return LV_KEY_ENTER;       /* CENTER / OK */
    case 0x0E: return LV_KEY_BACKSPACE;   /* BACK KEY */
    case 0x1F: return LV_KEY_HOME;        /* SHORTCUT KEY */
    case 0x23: return '#';                /* HASH */
    case 0x24: return LV_KEY_UP;          /* VOLUME UP */
    case 0x25: return LV_KEY_DOWN;        /* VOLUME DOWN */
    case 0x29: return 'S';                /* SOS */
    case 0x2A: return '*';                /* STAR */
    case 0x30: return '0';
    case 0x31: return '1';
    case 0x32: return '2';
    case 0x33: return '3';
    case 0x34: return '4';
    case 0x35: return '5';
    case 0x36: return '6';
    case 0x37: return '7';
    case 0x38: return '8';
    case 0x39: return '9';
    default:   return 0;
    }
}

/*
 * Hardware initialization for Unisoc T107 Keypad APB Scanner & SC2720 PMIC EIC.
 * Matches Nokia-2660-RE tools/ram_payload/vendor/fpdoom/ums9117/syscode.c:keypad_init
 */
void keypad_hw_init(void)
{
    /* 1. Ungate APB Power Domain for Keypad Matrix Controller (Bit 8) */
    REG32(0x402E1000U) |= (1U << 8);

    /* 2. Ungate Keypad Peripheral Clock (Bit 1) */
    REG32(0x402E1010U) |= (1U << 1);

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

    /* 5. Enable Matrix Grid: Exactly as in tested ums9117 syscode.c */
    uint32_t ctrl = KPD_CTRL;
    ctrl |= 1U;         /* Enable scanner */
    ctrl &= ~2U;        /* Disable sleep mode */
    ctrl |= 4U;         /* Enable long key detection */
    ctrl &= ~0xFCFC00U; /* Clear row/col active masks */
    ctrl |= (0x1CU << 16) | (0x1CU << 8); /* Active masks for TA-1480 5x5 matrix */
    KPD_CTRL = ctrl;

    /* 6. Configure SC2720 PMIC EIC for Power/End (Pin 1) AND Key 8 (Pin 9) */
    pmic_adi_write(0x406080C8U, pmic_adi_read(0x406080C8U) | 0x10U);  /* Enable EIC clock gate */
    pmic_adi_write(0x40608298U, 0U);
    pmic_adi_write(0x40608C08U, pmic_adi_read(0x40608C08U) | 8U);     /* Power domain ungate */
    pmic_adi_write(0x40608E40U, pmic_adi_read(0x40608E40U) | 1U);     /* Unmask EIC power int */
    pmic_adi_write(0x40608C10U, pmic_adi_read(0x40608C10U) | 8U);     /* Set debouncer filter enable */
    /* Enable debounce mask for Pin 1 (Power) AND Pin 9 (Key 8) */
    pmic_adi_write(0x40608284U, pmic_adi_read(0x40608284U) | (1U << 1) | (1U << 9));

    kprintf("[KEYPAD] APB Scanner & SC2720 PMIC EIC initialized (ram_payload driver)\n");
}

/*
 * Reads hardware events from APB Keypad Matrix (0x40250000) and SC2720 PMIC EIC (0x40608280).
 * Matches Nokia-2660-RE syscode.c:sys_event and keypad_test_main.c:read_power_button.
 * Returns:
 *   1 if a new event was retrieved (*out_code and *out_is_down set)
 *   0 if no pending events
 */
int keypad_poll_event(int *out_code, int *out_is_down)
{
    static int s_i = 0;
    static uint32_t s_event = 0, s_status = 0;
    static uint8_t s_eic9_saved = 0;
    static int s_last_pb = -1;
    uint32_t event, status;
    int i = s_i;
    unsigned k, down;

    /* 1. Poll Dedicated SC2720 PMIC EIC Pin 1 (Power / End button) */
    uint32_t eic_raw = pmic_adi_read(0x40608280U);
    int cur_pb = ((eic_raw >> 1) & 1) ^ 1; /* Active-low: 1 = pressed, 0 = released */
    if (s_last_pb == -1) {
        s_last_pb = cur_pb;
    } else if (cur_pb != s_last_pb) {
        s_last_pb = cur_pb;
        *out_code = 0x02; /* Keycode 0x02: POWER / END */
        *out_is_down = cur_pb;
        return 1;
    }

    event = s_event;
    status = s_status;

    if (!i) {
        event = KPD_INT_RAW & 0xFFU;
        status = KPD_KEY_STATUS;

        if (!event) {
            /* Check EIC Pin 9 (Key '8') when APB matrix has no events */
            int x = (eic_raw >> 9) & 1;
            if ((s_eic9_saved ^ x) & 1) {
                s_eic9_saved = down = x;
                k = 0x3F; /* Matrix index 63 maps to '8' */
                i = 8;
                goto found_key;
            }
            return 0;
        }

        KPD_INT_CLR = 0x0FFFU;
        if (status & 8U) return 0; /* Incomplete scan line guard */

        /* Check EIC Pin 9 along with matrix event */
        int x = (eic_raw >> 9) & 1;
        s_eic9_saved = (s_eic9_saved ^ x) << 1 | x;

        s_event = event;
        s_status = status;
    }

    for (; i < 8; i++) {
        if ((event >> i) & 1) {
            k = status >> ((i & 3) * 8);
            down = (i < 4);
            k = ((k & 0x70) >> 1) | (k & 7);
found_key:
            if (k < 64) {
                uint8_t code = s_keymap_t1480[k];
                if (code != 0xFF) {
                    *out_code = (int)code;
                    *out_is_down = (int)down;
                    s_i = i + 1;
                    return 1;
                }
            }
        }
    }

    if ((s_eic9_saved & 2) && i == 8) {
        k = 0x3F;
        down = s_eic9_saved & 1;
        s_eic9_saved &= 1; /* Clear transition bit */
        goto found_key;
    }

    s_i = 0;
    return 0;
}

void keypad_set_event_handler(keypad_event_cb_t cb)
{
    s_event_cb = cb;
}

void keypad_drain_events(void)
{
    int code = 0, is_down = 0;
    while (keypad_poll_event(&code, &is_down)) {
        s_active_code = code;
        s_active_down = (uint32_t)is_down;
        s_active_key = hw_code_to_lv_key(code);

        if (s_event_cb) {
            s_event_cb(code, is_down);
        }
    }
}

uint32_t keypad_poll_raw(uint32_t *out_is_down)
{
    keypad_drain_events();
    if (out_is_down) *out_is_down = s_active_down;
    return s_active_key;
}

static void keypad_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    (void)drv;
    keypad_drain_events();

    if (s_active_down && s_active_key != 0) {
        data->state = LV_INDEV_STATE_PRESSED;
        data->key = s_active_key;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
        data->key = s_active_key;
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
