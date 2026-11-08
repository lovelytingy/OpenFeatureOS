/*
 * OpenFeatureOS - LVGL Keypad Input Driver Port for Unisoc T107 (UMS9117)
 * Target: Nokia 2660 Flip (TA-1480) 5x5 Keypad Matrix
 *
 * Sourced from:
 *   - Nokia-2660-RE tools/ram_payload/src/baremetal_lcd/payload.c:190-220
 *   - Nokia-2660-RE docs/hardware.md (5x5 Matrix APB Scanner)
 */

#include "lv_port_indev.h"
#include "t107_regs.h"

lv_group_t *g_lvgl_keypad_group = NULL;
static lv_indev_drv_t indev_drv;

static uint32_t map_scancode_to_lv_key(uint32_t code)
{
    switch (code) {
        /* D-Pad Navigation Keys */
        case 0x0CU: return LV_KEY_UP;
        case 0x14U: return LV_KEY_DOWN;
        case 0x0DU: return LV_KEY_LEFT;
        case 0x15U: return LV_KEY_RIGHT;
        case 0x13U: return LV_KEY_ENTER; /* Center OK / Select Key */

        /* Soft Keys */
        case 0x03U: return LV_KEY_PREV;  /* Left Soft Key (LSK) */
        case 0x04U: return LV_KEY_ESC;   /* Right Soft Key (RSK / Back) */

        /* Numeric Keypad */
        case 0x00U: return '1';
        case 0x01U: return '2';
        case 0x02U: return '3';
        case 0x08U: return '4';
        case 0x09U: return '5';
        case 0x0AU: return '6';
        case 0x10U: return '7';
        case 0x11U: return '8';
        case 0x12U: return '9';
        case 0x19U: return '0';
        case 0x18U: return '*';
        case 0x1AU: return '#';

        default:    return 0;
    }
}

static void keypad_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    (void)drv;
    static uint32_t last_key = 0;

    /* Check if any key is currently pressed on 5x5 APB scanner */
    uint32_t is_pressed = KPD_INT_RAW & 0x01U;

    if (is_pressed) {
        uint32_t code = KPD_KEY_STATUS & 0x7FU;
        uint32_t key = map_scancode_to_lv_key(code);
        if (key != 0) {
            last_key = key;
            data->state = LV_INDEV_STATE_PRESSED;
            data->key = key;
        } else {
            data->state = LV_INDEV_STATE_RELEASED;
            data->key = last_key;
        }
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
        data->key = last_key;
    }
}

void lv_port_indev_init(void)
{
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_KEYPAD;
    indev_drv.read_cb = keypad_read_cb;

    lv_indev_t *keypad_indev = lv_indev_drv_register(&indev_drv);

    /* Create the default focus group for keypad-driven UI navigation */
    g_lvgl_keypad_group = lv_group_create();
    lv_group_set_default(g_lvgl_keypad_group);
    lv_indev_set_group(keypad_indev, g_lvgl_keypad_group);
}
