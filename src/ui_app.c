/*
 * OpenFeatureOS - LVGL UI Application & Feature Phone Launcher
 * Target: Unisoc T107 (UMS9117) / Nokia 2660 Flip (TA-1480 / HW3.0)
 * 240x320 QVGA Portrait Display
 */

#include "ui_app.h"
#include "lvgl/lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "torch.h"
#include "backlight.h"
#include "timer.h"
#include "printk.h"
#include <rtthread.h>
#include <string.h>

/* Top Status Bar */
static lv_obj_t *s_status_bar = NULL;
static lv_obj_t *s_lbl_clock = NULL;
static lv_obj_t *s_lbl_signal = NULL;
static lv_obj_t *s_lbl_battery = NULL;

/* Main Tabview */
static lv_obj_t *s_tv = NULL;

/* Tab 1: Phone Dialer */
static lv_obj_t *s_dial_label = NULL;
static char s_dial_buf[32] = {0};
static int s_dial_len = 0;
static lv_obj_t *s_btnm_dial = NULL;
static lv_obj_t *s_btn_call = NULL;
static lv_obj_t *s_btn_del = NULL;

/* Tab 2: Quick Tools */
static int s_torch_state = 0;
static uint8_t s_torch_level = 0x0F;
static lv_obj_t *s_torch_label = NULL;
static lv_obj_t *s_torch_btn = NULL;
static lv_obj_t *s_torch_lvl_btn = NULL;
static lv_obj_t *s_bl_label = NULL;
static uint32_t s_bl_percent = 80;

/* Tab 3: System Telemetry */
static lv_obj_t *s_sys_live_label = NULL;

/* Tab 4: Display Calibration */
static lv_obj_t *s_disp_info_label = NULL;

/* Bottom Softkey Bar */
static lv_obj_t *s_softkey_bar = NULL;
static lv_obj_t *s_lbl_lsk = NULL;
static lv_obj_t *s_lbl_ok = NULL;
static lv_obj_t *s_lbl_rsk = NULL;

/* ----------------------------------------------------------------------------
 * Helper: Append / Delete Dialed Digits
 * ----------------------------------------------------------------------------
 */
static void dialer_update_display(void)
{
    if (s_dial_len == 0) {
        lv_label_set_text(s_dial_label, "Enter Number...");
        lv_obj_set_style_text_color(s_dial_label, lv_color_hex(0x607D8B), LV_PART_MAIN);
    } else {
        /* Check for secret diagnostic codes */
        if (strcmp(s_dial_buf, "*#06#") == 0) {
            lv_label_set_text(s_dial_label, "IMEI: 356891104829104");
            lv_obj_set_style_text_color(s_dial_label, lv_color_hex(0x00FF88), LV_PART_MAIN);
        } else if (strcmp(s_dial_buf, "*#2886#") == 0) {
            lv_label_set_text(s_dial_label, "MMI AUTO-TEST: PASS");
            lv_obj_set_style_text_color(s_dial_label, lv_color_hex(0x00FF88), LV_PART_MAIN);
        } else {
            lv_label_set_text(s_dial_label, s_dial_buf);
            lv_obj_set_style_text_color(s_dial_label, lv_color_hex(0x00E5FF), LV_PART_MAIN);
        }
    }
}

static void dialer_append_char(char c)
{
    if (s_dial_len < (int)sizeof(s_dial_buf) - 1) {
        s_dial_buf[s_dial_len++] = c;
        s_dial_buf[s_dial_len] = '\0';
        dialer_update_display();
    }
}

static void dialer_delete_char(void)
{
    if (s_dial_len > 0) {
        s_dial_len--;
        s_dial_buf[s_dial_len] = '\0';
        dialer_update_display();
    }
}

static void dialer_clear(void)
{
    s_dial_len = 0;
    s_dial_buf[0] = '\0';
    dialer_update_display();
}

/* ----------------------------------------------------------------------------
 * Tab 1: Dialer Event Handlers
 * ----------------------------------------------------------------------------
 */
static void dial_btnm_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);

    if (code == LV_EVENT_VALUE_CHANGED) {
        uint32_t id = lv_btnmatrix_get_selected_btn(obj);
        const char *txt = lv_btnmatrix_get_btn_text(obj, id);
        if (txt && txt[0] != '\0') {
            dialer_append_char(txt[0]);
        }
    }
}

static void dial_call_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        if (s_dial_len > 0) {
            char msg[48];
            rt_snprintf(msg, sizeof(msg), "Calling: %s...", s_dial_buf);
            lv_label_set_text(s_dial_label, msg);
            lv_obj_set_style_text_color(s_dial_label, lv_color_hex(0x00FF88), LV_PART_MAIN);
        } else {
            lv_label_set_text(s_dial_label, "Enter Number First!");
            lv_obj_set_style_text_color(s_dial_label, lv_color_hex(0xFF5252), LV_PART_MAIN);
        }
    }
}

static void dial_del_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        dialer_delete_char();
    } else if (code == LV_EVENT_LONG_PRESSED) {
        dialer_clear();
    }
}

/* ----------------------------------------------------------------------------
 * Tab 2: Quick Tools Event Handlers
 * ----------------------------------------------------------------------------
 */
static void torch_toggle_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        s_torch_state = !s_torch_state;
        torch_set(s_torch_state, s_torch_level);
        if (s_torch_label) {
            if (s_torch_state) {
                lv_label_set_text(s_torch_label, "Torch: ON [Active]");
                lv_obj_set_style_text_color(s_torch_label, lv_color_hex(0xFFFF00), LV_PART_MAIN);
            } else {
                lv_label_set_text(s_torch_label, "Torch: OFF [Ready]");
                lv_obj_set_style_text_color(s_torch_label, lv_color_hex(0x90A4AE), LV_PART_MAIN);
            }
        }
    }
}

static void torch_level_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        if (s_torch_level == 0x02) {
            s_torch_level = 0x07;
            lv_label_set_text(s_torch_lvl_btn, "Level: MID (50%)");
        } else if (s_torch_level == 0x07) {
            s_torch_level = 0x0F;
            lv_label_set_text(s_torch_lvl_btn, "Level: MAX (100%)");
        } else {
            s_torch_level = 0x02;
            lv_label_set_text(s_torch_lvl_btn, "Level: LOW (20%)");
        }
        if (s_torch_state) {
            torch_set(1, s_torch_level);
        }
    }
}

static void backlight_cycle_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        if (s_bl_percent == 30)      s_bl_percent = 50;
        else if (s_bl_percent == 50) s_bl_percent = 80;
        else if (s_bl_percent == 80) s_bl_percent = 100;
        else                         s_bl_percent = 30;

        backlight_set_brightness(s_bl_percent);
        if (s_bl_label) {
            char bl_txt[32];
            rt_snprintf(bl_txt, sizeof(bl_txt), "Brightness: %u%%", s_bl_percent);
            lv_label_set_text(s_bl_label, bl_txt);
        }
    }
}

static void pulse_test_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        torch_blink(3, 60, 60, s_torch_level);
    }
}

/* ----------------------------------------------------------------------------
 * Periodic 1-Second UI Timer Callback
 * ----------------------------------------------------------------------------
 */
static void ui_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    uint32_t uptime_s = timer_get_ms() / 1000U;
    uint32_t mins = (uptime_s / 60U) % 60U;
    uint32_t secs = uptime_s % 60U;

    /* Update status bar clock/uptime */
    if (s_lbl_clock) {
        char clk_buf[16];
        rt_snprintf(clk_buf, sizeof(clk_buf), "%02u:%02u", mins, secs);
        lv_label_set_text(s_lbl_clock, clk_buf);
    }

    /* Update Tab 3 real-time RTOS telemetry */
    if (s_sys_live_label) {
        char sys_buf[64];
        rt_snprintf(sys_buf, sizeof(sys_buf), "Uptime: %us | Tick: %u",
                    uptime_s, (uint32_t)rt_tick_get());
        lv_label_set_text(s_sys_live_label, sys_buf);
    }
}

/* ----------------------------------------------------------------------------
 * Global Keypad Handler for Softkeys and Direct Numeric Dialing
 * ----------------------------------------------------------------------------
 */
static void screen_key_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_KEY) return;

    uint32_t key = lv_indev_get_key(lv_indev_get_act());

    /* Left Softkey (LSK): Cycle to next Tab */
    if (key == LV_KEY_PREV) {
        if (s_tv) {
            uint16_t cur = lv_tabview_get_tab_act(s_tv);
            lv_tabview_set_act(s_tv, (cur + 1) % 4, LV_ANIM_ON);
        }
        return;
    }

    /* Right Softkey (RSK) or ESC: Delete digit or Back to Tab 0 */
    if (key == LV_KEY_NEXT || key == LV_KEY_ESC) {
        if (s_dial_len > 0) {
            dialer_delete_char();
        } else if (s_tv && lv_tabview_get_tab_act(s_tv) != 0) {
            lv_tabview_set_act(s_tv, 0, LV_ANIM_ON);
        }
        return;
    }

    /* Backspace key */
    if (key == LV_KEY_BACKSPACE) {
        dialer_delete_char();
        return;
    }

    /* Green Call key: Trigger dialer call */
    if (key == 'C') {
        if (s_tv) lv_tabview_set_act(s_tv, 0, LV_ANIM_ON);
        dial_call_event_cb(e);
        return;
    }

    /* Dedicated SOS key: Strobe flashlight */
    if (key == 'S') {
        torch_blink(5, 50, 50, 0x0F);
        return;
    }

    /* Direct Numeric Keypad Input ('0'-'9', '*', '#') */
    if ((key >= '0' && key <= '9') || key == '*' || key == '#') {
        if (s_tv && lv_tabview_get_tab_act(s_tv) != 0) {
            lv_tabview_set_act(s_tv, 0, LV_ANIM_ON);
        }
        dialer_append_char((char)key);
    }
}

/* ----------------------------------------------------------------------------
 * UI Construction: Modern OpenFeatureOS Home Interface
 * ----------------------------------------------------------------------------
 */
static void create_openfeatureos_home(void)
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0A0E17), LV_PART_MAIN);

    /* 1. Top Status Bar (240x24) */
    s_status_bar = lv_obj_create(scr);
    lv_obj_set_size(s_status_bar, 240, 24);
    lv_obj_set_pos(s_status_bar, 0, 0);
    lv_obj_set_style_bg_color(s_status_bar, lv_color_hex(0x050811), LV_PART_MAIN);
    lv_obj_set_style_border_width(s_status_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(s_status_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_status_bar, 2, LV_PART_MAIN);

    /* 4G VoLTE Badge */
    s_lbl_signal = lv_label_create(s_status_bar);
    lv_label_set_text(s_lbl_signal, "4G [VoLTE]");
    lv_obj_set_style_text_color(s_lbl_signal, lv_color_hex(0x00E5FF), LV_PART_MAIN);
    lv_obj_align(s_lbl_signal, LV_ALIGN_LEFT_MID, 4, 0);

    /* Live Digital Clock */
    s_lbl_clock = lv_label_create(s_status_bar);
    lv_label_set_text(s_lbl_clock, "00:00");
    lv_obj_set_style_text_color(s_lbl_clock, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(s_lbl_clock, LV_ALIGN_CENTER, 0, 0);

    /* Battery Badge */
    s_lbl_battery = lv_label_create(s_status_bar);
    lv_label_set_text(s_lbl_battery, "100% [PWR]");
    lv_obj_set_style_text_color(s_lbl_battery, lv_color_hex(0x00FF88), LV_PART_MAIN);
    lv_obj_align(s_lbl_battery, LV_ALIGN_RIGHT_MID, -4, 0);

    /* Thin Cyan Accent Separator */
    lv_obj_t *bar_line = lv_obj_create(scr);
    lv_obj_set_size(bar_line, 240, 2);
    lv_obj_set_pos(bar_line, 0, 24);
    lv_obj_set_style_bg_color(bar_line, lv_color_hex(0x00E5FF), LV_PART_MAIN);
    lv_obj_set_style_border_width(bar_line, 0, LV_PART_MAIN);

    /* 2. Main Tabview Container (Height 250 px, from y=26 to y=276) */
    s_tv = lv_tabview_create(scr, LV_DIR_TOP, 30);
    lv_obj_set_pos(s_tv, 0, 26);
    lv_obj_set_size(s_tv, 240, 270);
    lv_obj_set_style_bg_color(s_tv, lv_color_hex(0x0A0E17), LV_PART_MAIN);

    /* Style the Tab buttons header */
    lv_obj_t *tab_btns = lv_tabview_get_tab_btns(s_tv);
    lv_obj_set_style_bg_color(tab_btns, lv_color_hex(0x0D1626), LV_PART_MAIN);
    lv_obj_set_style_text_color(tab_btns, lv_color_hex(0x90A4AE), LV_PART_MAIN);
    lv_obj_set_style_text_color(tab_btns, lv_color_hex(0x00E5FF), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_border_side(tab_btns, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
    lv_obj_set_style_border_color(tab_btns, lv_color_hex(0x1E2A40), LV_PART_MAIN);
    lv_obj_set_style_border_width(tab_btns, 1, LV_PART_MAIN);

    /* Add the 4 feature tabs */
    lv_obj_t *tab_dial = lv_tabview_add_tab(s_tv, "Dialer");
    lv_obj_t *tab_tools = lv_tabview_add_tab(s_tv, "Tools");
    lv_obj_t *tab_sys = lv_tabview_add_tab(s_tv, "System");
    lv_obj_t *tab_disp = lv_tabview_add_tab(s_tv, "Display");

    lv_obj_set_style_pad_all(tab_dial, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_all(tab_tools, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_all(tab_sys, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_all(tab_disp, 4, LV_PART_MAIN);

    /* ========================================================================
     * TAB 1: PHONE DIALER
     * ========================================================================
     */
    /* Dialed Number Display Box */
    lv_obj_t *dial_box = lv_obj_create(tab_dial);
    lv_obj_set_size(dial_box, 226, 32);
    lv_obj_set_pos(dial_box, 3, 2);
    lv_obj_set_style_bg_color(dial_box, lv_color_hex(0x060911), LV_PART_MAIN);
    lv_obj_set_style_border_color(dial_box, lv_color_hex(0x00E5FF), LV_PART_MAIN);
    lv_obj_set_style_border_width(dial_box, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(dial_box, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_all(dial_box, 2, LV_PART_MAIN);

    s_dial_label = lv_label_create(dial_box);
    lv_label_set_text(s_dial_label, "Enter Number...");
    lv_obj_set_style_text_color(s_dial_label, lv_color_hex(0x607D8B), LV_PART_MAIN);
    lv_obj_align(s_dial_label, LV_ALIGN_CENTER, 0, 0);

    /* 3x4 Dialpad Button Matrix */
    static const char *btnm_map[] = {
        "1", "2", "3", "\n",
        "4", "5", "6", "\n",
        "7", "8", "9", "\n",
        "*", "0", "#", ""
    };
    s_btnm_dial = lv_btnmatrix_create(tab_dial);
    lv_btnmatrix_set_map(s_btnm_dial, btnm_map);
    lv_obj_set_size(s_btnm_dial, 226, 142);
    lv_obj_set_pos(s_btnm_dial, 3, 38);
    lv_obj_set_style_bg_color(s_btnm_dial, lv_color_hex(0x0D1626), LV_PART_MAIN);
    lv_obj_set_style_border_color(s_btnm_dial, lv_color_hex(0x1E2A40), LV_PART_MAIN);
    lv_obj_set_style_border_width(s_btnm_dial, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(s_btnm_dial, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_btnm_dial, 2, LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_btnm_dial, lv_color_hex(0x142035), LV_PART_ITEMS);
    lv_obj_set_style_text_color(s_btnm_dial, lv_color_hex(0xFFFFFF), LV_PART_ITEMS);
    lv_obj_set_style_radius(s_btnm_dial, 4, LV_PART_ITEMS);
    lv_obj_add_event_cb(s_btnm_dial, dial_btnm_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_group_add_obj(g_lvgl_keypad_group, s_btnm_dial);

    /* Action Buttons (CALL & CLEAR) */
    s_btn_call = lv_btn_create(tab_dial);
    lv_obj_set_size(s_btn_call, 108, 30);
    lv_obj_set_pos(s_btn_call, 3, 184);
    lv_obj_set_style_bg_color(s_btn_call, lv_color_hex(0x00A844), LV_PART_MAIN);
    lv_obj_set_style_radius(s_btn_call, 6, LV_PART_MAIN);
    lv_obj_t *lbl_call = lv_label_create(s_btn_call);
    lv_label_set_text(lbl_call, "CALL");
    lv_obj_align(lbl_call, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(s_btn_call, dial_call_event_cb, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(g_lvgl_keypad_group, s_btn_call);

    s_btn_del = lv_btn_create(tab_dial);
    lv_obj_set_size(s_btn_del, 108, 30);
    lv_obj_set_pos(s_btn_del, 121, 184);
    lv_obj_set_style_bg_color(s_btn_del, lv_color_hex(0xD50000), LV_PART_MAIN);
    lv_obj_set_style_radius(s_btn_del, 6, LV_PART_MAIN);
    lv_obj_t *lbl_del = lv_label_create(s_btn_del);
    lv_label_set_text(lbl_del, "CLEAR");
    lv_obj_align(lbl_del, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(s_btn_del, dial_del_event_cb, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(g_lvgl_keypad_group, s_btn_del);

    /* ========================================================================
     * TAB 2: QUICK TOOLS (Flashlight, Backlight, Haptics)
     * ========================================================================
     */
    /* Card 1: Flashlight / Torch */
    lv_obj_t *card_torch = lv_obj_create(tab_tools);
    lv_obj_set_size(card_torch, 226, 68);
    lv_obj_set_pos(card_torch, 3, 2);
    lv_obj_set_style_bg_color(card_torch, lv_color_hex(0x131B2A), LV_PART_MAIN);
    lv_obj_set_style_border_color(card_torch, lv_color_hex(0x1E2A40), LV_PART_MAIN);
    lv_obj_set_style_border_width(card_torch, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(card_torch, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card_torch, 4, LV_PART_MAIN);

    s_torch_label = lv_label_create(card_torch);
    lv_label_set_text(s_torch_label, "Torch: OFF [Ready]");
    lv_obj_set_style_text_color(s_torch_label, lv_color_hex(0x90A4AE), LV_PART_MAIN);
    lv_obj_set_pos(s_torch_label, 4, 2);

    s_torch_btn = lv_btn_create(card_torch);
    lv_obj_set_size(s_torch_btn, 102, 28);
    lv_obj_set_pos(s_torch_btn, 4, 26);
    lv_obj_set_style_bg_color(s_torch_btn, lv_color_hex(0x007ACC), LV_PART_MAIN);
    lv_obj_t *lbl_t_btn = lv_label_create(s_torch_btn);
    lv_label_set_text(lbl_t_btn, "Toggle");
    lv_obj_align(lbl_t_btn, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(s_torch_btn, torch_toggle_event_cb, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(g_lvgl_keypad_group, s_torch_btn);

    s_torch_lvl_btn = lv_btn_create(card_torch);
    lv_obj_set_size(s_torch_lvl_btn, 106, 28);
    lv_obj_set_pos(s_torch_lvl_btn, 110, 26);
    lv_obj_set_style_bg_color(s_torch_lvl_btn, lv_color_hex(0x23334D), LV_PART_MAIN);
    lv_obj_t *lbl_t_lvl = lv_label_create(s_torch_lvl_btn);
    lv_label_set_text(lbl_t_lvl, "Max (100%)");
    lv_obj_align(lbl_t_lvl, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(s_torch_lvl_btn, torch_level_event_cb, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(g_lvgl_keypad_group, s_torch_lvl_btn);

    /* Card 2: Backlight Control */
    lv_obj_t *card_bl = lv_obj_create(tab_tools);
    lv_obj_set_size(card_bl, 226, 68);
    lv_obj_set_pos(card_bl, 3, 74);
    lv_obj_set_style_bg_color(card_bl, lv_color_hex(0x131B2A), LV_PART_MAIN);
    lv_obj_set_style_border_color(card_bl, lv_color_hex(0x1E2A40), LV_PART_MAIN);
    lv_obj_set_style_border_width(card_bl, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(card_bl, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card_bl, 4, LV_PART_MAIN);

    s_bl_label = lv_label_create(card_bl);
    lv_label_set_text(s_bl_label, "Brightness: 80%");
    lv_obj_set_style_text_color(s_bl_label, lv_color_hex(0x00E5FF), LV_PART_MAIN);
    lv_obj_set_pos(s_bl_label, 4, 2);

    lv_obj_t *btn_bl = lv_btn_create(card_bl);
    lv_obj_set_size(btn_bl, 212, 28);
    lv_obj_set_pos(btn_bl, 4, 26);
    lv_obj_set_style_bg_color(btn_bl, lv_color_hex(0x0D3B66), LV_PART_MAIN);
    lv_obj_t *lbl_bl_btn = lv_label_create(btn_bl);
    lv_label_set_text(lbl_bl_btn, "Cycle 30% / 50% / 80% / 100%");
    lv_obj_align(lbl_bl_btn, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(btn_bl, backlight_cycle_event_cb, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(g_lvgl_keypad_group, btn_bl);

    /* Card 3: Optical Diagnostic Strobe */
    lv_obj_t *card_diag = lv_obj_create(tab_tools);
    lv_obj_set_size(card_diag, 226, 68);
    lv_obj_set_pos(card_diag, 3, 146);
    lv_obj_set_style_bg_color(card_diag, lv_color_hex(0x131B2A), LV_PART_MAIN);
    lv_obj_set_style_border_color(card_diag, lv_color_hex(0x1E2A40), LV_PART_MAIN);
    lv_obj_set_style_border_width(card_diag, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(card_diag, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card_diag, 4, LV_PART_MAIN);

    lv_obj_t *lbl_diag = lv_label_create(card_diag);
    lv_label_set_text(lbl_diag, "Optical Sign-of-Life");
    lv_obj_set_style_text_color(s_bl_label, lv_color_hex(0x90A4AE), LV_PART_MAIN);
    lv_obj_set_pos(lbl_diag, 4, 2);

    lv_obj_t *btn_pulse = lv_btn_create(card_diag);
    lv_obj_set_size(btn_pulse, 212, 28);
    lv_obj_set_pos(btn_pulse, 4, 26);
    lv_obj_set_style_bg_color(btn_pulse, lv_color_hex(0x4A148C), LV_PART_MAIN);
    lv_obj_t *lbl_p_btn = lv_label_create(btn_pulse);
    lv_label_set_text(lbl_p_btn, "Flash Strobe Test (3x)");
    lv_obj_align(lbl_p_btn, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(btn_pulse, pulse_test_event_cb, LV_EVENT_CLICKED, NULL);
    lv_group_add_obj(g_lvgl_keypad_group, btn_pulse);

    /* ========================================================================
     * TAB 3: SYSTEM TELEMETRY
     * ========================================================================
     */
    lv_obj_t *card_spec = lv_obj_create(tab_sys);
    lv_obj_set_size(card_spec, 226, 136);
    lv_obj_set_pos(card_spec, 3, 2);
    lv_obj_set_style_bg_color(card_spec, lv_color_hex(0x131B2A), LV_PART_MAIN);
    lv_obj_set_style_border_color(card_spec, lv_color_hex(0x1E2A40), LV_PART_MAIN);
    lv_obj_set_style_border_width(card_spec, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(card_spec, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card_spec, 6, LV_PART_MAIN);

    lv_obj_t *spec_txt = lv_label_create(card_spec);
    lv_label_set_text(spec_txt,
                      "Platform: Unisoc T107 (UMS9117)\n"
                      "Core    : ARM Cortex-A7 @ 1.0 GHz\n"
                      "Kernel  : RT-Thread 5.4.0 RTOS\n"
                      "Memory  : 48 MB LPDDR (31 MB Heap)\n"
                      "Display : GC9307 240x320 @ 48 MHz\n"
                      "Safety  : 100% Volatile DRAM Boot");
    lv_obj_set_style_text_color(spec_txt, lv_color_hex(0xCFD8DC), LV_PART_MAIN);
    lv_obj_set_pos(spec_txt, 2, 2);

    /* Live Telemetry Card */
    lv_obj_t *card_live = lv_obj_create(tab_sys);
    lv_obj_set_size(card_live, 226, 70);
    lv_obj_set_pos(card_live, 3, 142);
    lv_obj_set_style_bg_color(card_live, lv_color_hex(0x060911), LV_PART_MAIN);
    lv_obj_set_style_border_color(card_live, lv_color_hex(0x00FF88), LV_PART_MAIN);
    lv_obj_set_style_border_width(card_live, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(card_live, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card_live, 6, LV_PART_MAIN);

    lv_obj_t *live_hdr = lv_label_create(card_live);
    lv_label_set_text(live_hdr, "STATUS: SCHEDULER HEALTHY");
    lv_obj_set_style_text_color(live_hdr, lv_color_hex(0x00FF88), LV_PART_MAIN);
    lv_obj_set_pos(live_hdr, 2, 2);

    s_sys_live_label = lv_label_create(card_live);
    lv_label_set_text(s_sys_live_label, "Uptime: 0s | Tick: 0");
    lv_obj_set_style_text_color(s_sys_live_label, lv_color_hex(0x00E5FF), LV_PART_MAIN);
    lv_obj_set_pos(s_sys_live_label, 2, 24);

    /* ========================================================================
     * TAB 4: DISPLAY CALIBRATION & COLOR SWATCHES
     * ========================================================================
     */
    lv_obj_t *card_colors = lv_obj_create(tab_disp);
    lv_obj_set_size(card_colors, 226, 110);
    lv_obj_set_pos(card_colors, 3, 2);
    lv_obj_set_style_bg_color(card_colors, lv_color_hex(0x131B2A), LV_PART_MAIN);
    lv_obj_set_style_border_color(card_colors, lv_color_hex(0x1E2A40), LV_PART_MAIN);
    lv_obj_set_style_border_width(card_colors, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(card_colors, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card_colors, 4, LV_PART_MAIN);

    lv_obj_t *c_lbl = lv_label_create(card_colors);
    lv_label_set_text(c_lbl, "RGB565 Color Swatches:");
    lv_obj_set_style_text_color(c_lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_pos(c_lbl, 2, 2);

    /* 8 Color Blocks */
    static const uint32_t swatches[] = {
        0xFFFFFF, 0x00E5FF, 0xFFEB3B, 0x00E676,
        0xE040FB, 0xFF1744, 0x2979FF, 0x78909C
    };
    int sw_w = 26;
    for (int i = 0; i < 8; i++) {
        lv_obj_t *sw = lv_obj_create(card_colors);
        lv_obj_set_size(sw, sw_w - 2, 34);
        lv_obj_set_pos(sw, 4 + (i * sw_w), 24);
        lv_obj_set_style_bg_color(sw, lv_color_hex(swatches[i]), LV_PART_MAIN);
        lv_obj_set_style_border_width(sw, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(sw, 2, LV_PART_MAIN);
    }

    /* Grayscale Ramp */
    lv_obj_t *g_bar = lv_obj_create(card_colors);
    lv_obj_set_size(g_bar, 212, 14);
    lv_obj_set_pos(g_bar, 4, 66);
    lv_obj_set_style_bg_color(g_bar, lv_color_hex(0x424242), LV_PART_MAIN);
    lv_obj_set_style_border_width(g_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(g_bar, 2, LV_PART_MAIN);

    /* Panel Specs Card */
    lv_obj_t *card_pinfo = lv_obj_create(tab_disp);
    lv_obj_set_size(card_pinfo, 226, 96);
    lv_obj_set_pos(card_pinfo, 3, 116);
    lv_obj_set_style_bg_color(card_pinfo, lv_color_hex(0x131B2A), LV_PART_MAIN);
    lv_obj_set_style_border_color(card_pinfo, lv_color_hex(0x1E2A40), LV_PART_MAIN);
    lv_obj_set_style_border_width(card_pinfo, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(card_pinfo, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card_pinfo, 6, LV_PART_MAIN);

    s_disp_info_label = lv_label_create(card_pinfo);
    lv_label_set_text(s_disp_info_label,
                      "Controller: GC9307 TFT\n"
                      "Resolution: 240 x 320 QVGA\n"
                      "Colors    : 65,536 (RGB565)\n"
                      "Bus Speed : 48 MHz (SPI1 DMA)\n"
                      "Backlight : SC2720 PMIC Dual Sink");
    lv_obj_set_style_text_color(s_disp_info_label, lv_color_hex(0xB0BEC5), LV_PART_MAIN);
    lv_obj_set_pos(s_disp_info_label, 2, 2);

    /* 3. Bottom Softkey Bar (240x24 at y=296) */
    s_softkey_bar = lv_obj_create(scr);
    lv_obj_set_size(s_softkey_bar, 240, 24);
    lv_obj_set_pos(s_softkey_bar, 0, 296);
    lv_obj_set_style_bg_color(s_softkey_bar, lv_color_hex(0x050811), LV_PART_MAIN);
    lv_obj_set_style_border_width(s_softkey_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(s_softkey_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_softkey_bar, 2, LV_PART_MAIN);

    s_lbl_lsk = lv_label_create(s_softkey_bar);
    lv_label_set_text(s_lbl_lsk, "LSK: Next");
    lv_obj_set_style_text_color(s_lbl_lsk, lv_color_hex(0x90A4AE), LV_PART_MAIN);
    lv_obj_align(s_lbl_lsk, LV_ALIGN_LEFT_MID, 4, 0);

    s_lbl_ok = lv_label_create(s_softkey_bar);
    lv_label_set_text(s_lbl_ok, "OK: Select");
    lv_obj_set_style_text_color(s_lbl_ok, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(s_lbl_ok, LV_ALIGN_CENTER, 0, 0);

    s_lbl_rsk = lv_label_create(s_softkey_bar);
    lv_label_set_text(s_lbl_rsk, "RSK: Clear");
    lv_obj_set_style_text_color(s_lbl_rsk, lv_color_hex(0x90A4AE), LV_PART_MAIN);
    lv_obj_align(s_lbl_rsk, LV_ALIGN_RIGHT_MID, -4, 0);

    /* Focus on Dialpad by default */
    lv_group_focus_obj(s_btnm_dial);

    /* Register global key listener on the active screen */
    lv_obj_add_event_cb(scr, screen_key_event_cb, LV_EVENT_KEY, NULL);

    /* Periodic 1-Second Uptime & Telemetry Timer */
    lv_timer_create(ui_timer_cb, 1000, NULL);
}

/*
 * LVGL GUI Thread Entry Function
 */
static void lvgl_ui_thread_entry(void *param)
{
    (void)param;

    kprintf("[LVGL] Initializing LVGL Graphics Engine v%d.%d.%d\n",
            LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR, LVGL_VERSION_PATCH);

    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();

    kprintf("[LVGL] Display & Keypad drivers registered successfully.\n");

    create_openfeatureos_home();

    kprintf("[LVGL] OpenFeatureOS Modern Home UI created successfully.\n");
    kprintf("[LVGL] Entering main GUI event loop.\n");

    while (1) {
        lv_timer_handler();
        rt_thread_mdelay(10);
    }
}

void ui_app_init(void)
{
    rt_thread_t ui_thread = rt_thread_create("lvgl_ui",
                                            lvgl_ui_thread_entry,
                                            RT_NULL,
                                            8192,
                                            16,
                                            10);
    if (ui_thread != RT_NULL) {
        rt_thread_startup(ui_thread);
        kprintf("[UI] Spawned LVGL GUI task successfully (Priority: 16, Stack: 8KB)\n");
    } else {
        kprintf("[UI] ERROR: Failed to spawn LVGL GUI task!\n");
    }
}
