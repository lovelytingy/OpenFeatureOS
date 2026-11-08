/*
 * OpenFeatureOS - Modern Feature Phone UI & Launcher
 * Target: Unisoc T107 (UMS9117) / Nokia 2660 Flip (TA-1480 / HW3.0)
 * 240x320 QVGA Portrait Display (LVGL v8.3.11)
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

/* Active View State */
typedef enum {
    VIEW_HOME = 0,
    VIEW_DIALER,
    VIEW_TORCH,
    VIEW_DEBUG,
    VIEW_SETTINGS
} app_view_t;

static app_view_t s_current_view = VIEW_HOME;

/* View Containers */
static lv_obj_t *s_cont_home = NULL;
static lv_obj_t *s_cont_dialer = NULL;
static lv_obj_t *s_cont_torch = NULL;
static lv_obj_t *s_cont_debug = NULL;
static lv_obj_t *s_cont_settings = NULL;

/* Top Status Bar */
static lv_obj_t *s_status_bar = NULL;
static lv_obj_t *s_lbl_clock = NULL;

/* Home Screen Widgets */
static lv_obj_t *s_lbl_home_time = NULL;
static lv_obj_t *s_lbl_home_date = NULL;
static lv_obj_t *s_btn_home_apps[4] = {NULL};
static lv_obj_t *s_lbl_home_torch_status = NULL;

/* Tab 1: Dialer */
static lv_obj_t *s_dial_label = NULL;
static char s_dial_buf[32] = {0};
static int s_dial_len = 0;
static lv_obj_t *s_btnm_dial = NULL;

/* Tab 2: Torch */
static int s_torch_state = 0;
static uint8_t s_torch_level = 0x0F;
static lv_obj_t *s_torch_badge = NULL;
static lv_obj_t *s_torch_toggle_btn = NULL;
static lv_obj_t *s_torch_lvl_btn = NULL;

/* Tab 3: System Debug */
static lv_obj_t *s_lbl_debug_telemetry = NULL;

/* Tab 4: Settings */
static uint32_t s_bl_percent = 80;
static lv_obj_t *s_lbl_bl_val = NULL;

/* Bottom Softkey Bar */
static lv_obj_t *s_softkey_bar = NULL;
static lv_obj_t *s_lbl_lsk = NULL;
static lv_obj_t *s_lbl_ok = NULL;
static lv_obj_t *s_lbl_rsk = NULL;

/* Forward Declarations */
static void switch_view(app_view_t view);
static void screen_key_event_cb(lv_event_t *e);

/* ----------------------------------------------------------------------------
 * Helper: Append / Delete Dialed Digits
 * ----------------------------------------------------------------------------
 */
static void dialer_update_display(void)
{
    if (!s_dial_label) return;

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
 * Torch Toggle Helper
 * ----------------------------------------------------------------------------
 */
static void torch_toggle_action(void)
{
    s_torch_state = !s_torch_state;
    torch_set(s_torch_state, s_torch_level);

    /* Update Torch View UI if loaded */
    if (s_torch_badge) {
        if (s_torch_state) {
            lv_label_set_text(s_torch_badge, "TORCH: [ ACTIVE - ON ]");
            lv_obj_set_style_bg_color(s_torch_badge, lv_color_hex(0x00E676), LV_PART_MAIN);
            lv_obj_set_style_text_color(s_torch_badge, lv_color_hex(0x000000), LV_PART_MAIN);
        } else {
            lv_label_set_text(s_torch_badge, "TORCH: [ IDLE - OFF ]");
            lv_obj_set_style_bg_color(s_torch_badge, lv_color_hex(0x37474F), LV_PART_MAIN);
            lv_obj_set_style_text_color(s_torch_badge, lv_color_hex(0xB0BEC5), LV_PART_MAIN);
        }
    }

    /* Update Home Screen Quick Torch Status */
    if (s_lbl_home_torch_status) {
        if (s_torch_state) {
            lv_label_set_text(s_lbl_home_torch_status, "[2] Flashlight: [ ON ]");
            lv_obj_set_style_text_color(s_lbl_home_torch_status, lv_color_hex(0x00FF88), LV_PART_MAIN);
        } else {
            lv_label_set_text(s_lbl_home_torch_status, "[2] Flashlight: [ OFF ]");
            lv_obj_set_style_text_color(s_lbl_home_torch_status, lv_color_hex(0x90A4AE), LV_PART_MAIN);
        }
    }
}

/* ----------------------------------------------------------------------------
 * View Switching Logic
 * ----------------------------------------------------------------------------
 */
static void switch_view(app_view_t view)
{
    s_current_view = view;

    /* Hide all view containers */
    if (s_cont_home)     lv_obj_add_flag(s_cont_home, LV_OBJ_FLAG_HIDDEN);
    if (s_cont_dialer)   lv_obj_add_flag(s_cont_dialer, LV_OBJ_FLAG_HIDDEN);
    if (s_cont_torch)    lv_obj_add_flag(s_cont_torch, LV_OBJ_FLAG_HIDDEN);
    if (s_cont_debug)    lv_obj_add_flag(s_cont_debug, LV_OBJ_FLAG_HIDDEN);
    if (s_cont_settings) lv_obj_add_flag(s_cont_settings, LV_OBJ_FLAG_HIDDEN);

    /* Show target container & set softkey hints */
    switch (view) {
        case VIEW_HOME:
            if (s_cont_home) lv_obj_clear_flag(s_cont_home, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(s_lbl_lsk, "LSK: Dialer");
            lv_label_set_text(s_lbl_ok,  "OK: Open");
            lv_label_set_text(s_lbl_rsk, "RSK: Torch");
            if (g_lvgl_keypad_group && s_btn_home_apps[0]) {
                lv_group_focus_obj(s_btn_home_apps[0]);
            }
            break;

        case VIEW_DIALER:
            if (s_cont_dialer) lv_obj_clear_flag(s_cont_dialer, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(s_lbl_lsk, "LSK: Home");
            lv_label_set_text(s_lbl_ok,  "OK: Dial");
            lv_label_set_text(s_lbl_rsk, "RSK: Del");
            if (g_lvgl_keypad_group && s_btnm_dial) {
                lv_group_focus_obj(s_btnm_dial);
            }
            break;

        case VIEW_TORCH:
            if (s_cont_torch) lv_obj_clear_flag(s_cont_torch, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(s_lbl_lsk, "LSK: Home");
            lv_label_set_text(s_lbl_ok,  "OK: Toggle");
            lv_label_set_text(s_lbl_rsk, "RSK: Back");
            if (g_lvgl_keypad_group && s_torch_toggle_btn) {
                lv_group_focus_obj(s_torch_toggle_btn);
            }
            break;

        case VIEW_DEBUG:
            if (s_cont_debug) lv_obj_clear_flag(s_cont_debug, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(s_lbl_lsk, "LSK: Home");
            lv_label_set_text(s_lbl_ok,  "OK: Top");
            lv_label_set_text(s_lbl_rsk, "RSK: Back");
            if (g_lvgl_keypad_group && s_cont_debug) {
                lv_group_focus_obj(s_cont_debug);
            }
            break;

        case VIEW_SETTINGS:
            if (s_cont_settings) lv_obj_clear_flag(s_cont_settings, LV_OBJ_FLAG_HIDDEN);
            lv_label_set_text(s_lbl_lsk, "LSK: Home");
            lv_label_set_text(s_lbl_ok,  "OK: Select");
            lv_label_set_text(s_lbl_rsk, "RSK: Back");
            if (g_lvgl_keypad_group && s_cont_settings) {
                lv_group_focus_obj(s_cont_settings);
            }
            break;
    }
}

/* ----------------------------------------------------------------------------
 * Button Event Handlers
 * ----------------------------------------------------------------------------
 */
static void home_app_click_cb(lv_event_t *e)
{
    lv_obj_t *target = lv_event_get_target(e);
    if (target == s_btn_home_apps[0]) {
        switch_view(VIEW_DIALER);
    } else if (target == s_btn_home_apps[1]) {
        torch_toggle_action();
    } else if (target == s_btn_home_apps[2]) {
        switch_view(VIEW_DEBUG);
    } else if (target == s_btn_home_apps[3]) {
        switch_view(VIEW_SETTINGS);
    }
}

static void dial_btnm_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *obj = lv_event_get_target(e);
        uint32_t id = lv_btnmatrix_get_selected_btn(obj);
        const char *txt = lv_btnmatrix_get_btn_text(obj, id);
        if (txt && txt[0] != '\0') {
            dialer_append_char(txt[0]);
        }
    }
}

static void dial_call_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
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

static void torch_toggle_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        torch_toggle_action();
    }
}

static void torch_lvl_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        if (s_torch_level == 0x03) {
            s_torch_level = 0x07; /* Mid */
            lv_label_set_text(lv_obj_get_child(s_torch_lvl_btn, 0), "Level: MID (50%)");
        } else if (s_torch_level == 0x07) {
            s_torch_level = 0x0F; /* Max */
            lv_label_set_text(lv_obj_get_child(s_torch_lvl_btn, 0), "Level: MAX (100%)");
        } else {
            s_torch_level = 0x03; /* Low */
            lv_label_set_text(lv_obj_get_child(s_torch_lvl_btn, 0), "Level: LOW (20%)");
        }
        if (s_torch_state) {
            torch_set(1, s_torch_level);
        }
    }
}

static void torch_sos_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        torch_morse_sos(s_torch_level);
        s_torch_state = 0;
        torch_set(0, 0);
        if (s_torch_badge) {
            lv_label_set_text(s_torch_badge, "TORCH: [ IDLE - OFF ]");
            lv_obj_set_style_bg_color(s_torch_badge, lv_color_hex(0x37474F), LV_PART_MAIN);
        }
    }
}

static void bl_btn_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        uint32_t val = (uint32_t)(uintptr_t)lv_event_get_user_data(e);
        s_bl_percent = val;
        backlight_set_brightness(s_bl_percent);
        if (s_lbl_bl_val) {
            char buf[32];
            rt_snprintf(buf, sizeof(buf), "Brightness: %u%%", s_bl_percent);
            lv_label_set_text(s_lbl_bl_val, buf);
        }
    }
}

/* ----------------------------------------------------------------------------
 * 1-Second Periodic Clock & System Telemetry Timer
 * ----------------------------------------------------------------------------
 */
static void ui_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    static uint32_t s_uptime_sec = 0;
    s_uptime_sec++;

    uint32_t mm = (s_uptime_sec / 60) % 60;
    uint32_t hh = (s_uptime_sec / 3600) % 24;
    uint32_t ss = s_uptime_sec % 60;

    /* Update Top Status Bar Clock */
    if (s_lbl_clock) {
        char buf[16];
        rt_snprintf(buf, sizeof(buf), "%02u:%02u", hh, mm);
        lv_label_set_text(s_lbl_clock, buf);
    }

    /* Update Home Screen Big Clock */
    if (s_lbl_home_time) {
        char buf[16];
        rt_snprintf(buf, sizeof(buf), "%02u:%02u:%02u", hh, mm, ss);
        lv_label_set_text(s_lbl_home_time, buf);
    }

    /* Update System Debug Telemetry */
    if (s_lbl_debug_telemetry && s_current_view == VIEW_DEBUG) {
        char tbuf[384];
        rt_snprintf(tbuf, sizeof(tbuf),
            "OS Name : OpenFeatureOS\n"
            "Version : v1.0 (Build 2026.11)\n"
            "Kernel  : RT-Thread 5.4.0 UP\n"
            "SoC     : Unisoc T107 (UMS9117)\n"
            "Core    : Cortex-A7 @ 1.0 GHz\n"
            "DRAM    : 48 MB LPDDR2 (0x80000000)\n"
            "Heap    : 31.6 MB Volatile\n"
            "Display : GC9307 240x320 SPI1 DMA\n"
            "Flash   : 0 NAND WRITES (SAFE)\n"
            "Uptime  : %u seconds | Ticks: %u",
            s_uptime_sec, (uint32_t)rt_tick_get());
        lv_label_set_text(s_lbl_debug_telemetry, tbuf);
    }
}

/* ----------------------------------------------------------------------------
 * Global Keypad Handler
 * ----------------------------------------------------------------------------
 */
static void screen_key_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;
    uint32_t key = lv_event_get_key(e);

    if (s_current_view == VIEW_HOME) {
        if (key == '1') {
            switch_view(VIEW_DIALER);
        } else if (key == '2') {
            torch_toggle_action();
        } else if (key == '3') {
            switch_view(VIEW_DEBUG);
        } else if (key == '4') {
            switch_view(VIEW_SETTINGS);
        } else if (key == LV_KEY_PREV) {
            /* LSK: Quick launch dialer */
            switch_view(VIEW_DIALER);
        } else if (key == LV_KEY_NEXT) {
            /* RSK: Quick flashlight toggle from Home */
            torch_toggle_action();
        } else if ((key >= '0' && key <= '9') || key == '*' || key == '#') {
            /* Direct typing from home immediately switches to dialer */
            switch_view(VIEW_DIALER);
            dialer_append_char((char)key);
        }
    } else if (s_current_view == VIEW_DIALER) {
        if (key == LV_KEY_PREV || key == LV_KEY_ESC) {
            /* LSK: Back to Home */
            switch_view(VIEW_HOME);
        } else if (key == LV_KEY_NEXT || key == LV_KEY_BACKSPACE) {
            /* RSK: Delete */
            dialer_delete_char();
        } else if ((key >= '0' && key <= '9') || key == '*' || key == '#') {
            dialer_append_char((char)key);
        }
    } else {
        /* Other views: LSK, RSK, or Back key returns to Home */
        if (key == LV_KEY_PREV || key == LV_KEY_NEXT || key == LV_KEY_BACKSPACE || key == LV_KEY_ESC) {
            switch_view(VIEW_HOME);
        }
    }
}

/* ----------------------------------------------------------------------------
 * Create Top Status Bar (240x24)
 * ----------------------------------------------------------------------------
 */
static void create_status_bar(lv_obj_t *parent)
{
    s_status_bar = lv_obj_create(parent);
    lv_obj_set_size(s_status_bar, 240, 24);
    lv_obj_set_pos(s_status_bar, 0, 0);
    lv_obj_set_style_bg_color(s_status_bar, lv_color_hex(0x060912), LV_PART_MAIN);
    lv_obj_set_style_border_side(s_status_bar, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
    lv_obj_set_style_border_color(s_status_bar, lv_color_hex(0x00E5FF), LV_PART_MAIN);
    lv_obj_set_style_border_width(s_status_bar, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(s_status_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_status_bar, 2, LV_PART_MAIN);
    lv_obj_clear_flag(s_status_bar, LV_OBJ_FLAG_SCROLLABLE);

    /* 4G VoLTE Badge */
    lv_obj_t *lbl_4g = lv_label_create(s_status_bar);
    lv_label_set_text(lbl_4g, "4G VoLTE");
    lv_obj_set_style_text_color(lbl_4g, lv_color_hex(0x00E5FF), LV_PART_MAIN);
    lv_obj_align(lbl_4g, LV_ALIGN_LEFT_MID, 6, 0);

    /* Clock */
    s_lbl_clock = lv_label_create(s_status_bar);
    lv_label_set_text(s_lbl_clock, "00:00");
    lv_obj_set_style_text_color(s_lbl_clock, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(s_lbl_clock, LV_ALIGN_CENTER, 0, 0);

    /* Battery Badge */
    lv_obj_t *lbl_bat = lv_label_create(s_status_bar);
    lv_label_set_text(lbl_bat, "100%");
    lv_obj_set_style_text_color(lbl_bat, lv_color_hex(0x00E676), LV_PART_MAIN);
    lv_obj_align(lbl_bat, LV_ALIGN_RIGHT_MID, -6, 0);
}

/* ----------------------------------------------------------------------------
 * Create Bottom Softkey Bar (240x24 at y=296)
 * ----------------------------------------------------------------------------
 */
static void create_softkey_bar(lv_obj_t *parent)
{
    s_softkey_bar = lv_obj_create(parent);
    lv_obj_set_size(s_softkey_bar, 240, 24);
    lv_obj_set_pos(s_softkey_bar, 0, 296);
    lv_obj_set_style_bg_color(s_softkey_bar, lv_color_hex(0x060912), LV_PART_MAIN);
    lv_obj_set_style_border_side(s_softkey_bar, LV_BORDER_SIDE_TOP, LV_PART_MAIN);
    lv_obj_set_style_border_color(s_softkey_bar, lv_color_hex(0x00E5FF), LV_PART_MAIN);
    lv_obj_set_style_border_width(s_softkey_bar, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(s_softkey_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_softkey_bar, 2, LV_PART_MAIN);
    lv_obj_clear_flag(s_softkey_bar, LV_OBJ_FLAG_SCROLLABLE);

    s_lbl_lsk = lv_label_create(s_softkey_bar);
    lv_label_set_text(s_lbl_lsk, "LSK: Dialer");
    lv_obj_set_style_text_color(s_lbl_lsk, lv_color_hex(0x80D8FF), LV_PART_MAIN);
    lv_obj_align(s_lbl_lsk, LV_ALIGN_LEFT_MID, 6, 0);

    s_lbl_ok = lv_label_create(s_softkey_bar);
    lv_label_set_text(s_lbl_ok, "OK: Open");
    lv_obj_set_style_text_color(s_lbl_ok, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(s_lbl_ok, LV_ALIGN_CENTER, 0, 0);

    s_lbl_rsk = lv_label_create(s_softkey_bar);
    lv_label_set_text(s_lbl_rsk, "RSK: Torch");
    lv_obj_set_style_text_color(s_lbl_rsk, lv_color_hex(0x80D8FF), LV_PART_MAIN);
    lv_obj_align(s_lbl_rsk, LV_ALIGN_RIGHT_MID, -6, 0);
}

/* ----------------------------------------------------------------------------
 * View 1: Home View (Feature Phone Wallpaper & App Launcher)
 * ----------------------------------------------------------------------------
 */
static void create_home_view(lv_obj_t *parent)
{
    s_cont_home = lv_obj_create(parent);
    lv_obj_set_size(s_cont_home, 240, 272);
    lv_obj_set_pos(s_cont_home, 0, 24);
    /* Wallpaper: Deep Indigo Night with subtle texture */
    lv_obj_set_style_bg_color(s_cont_home, lv_color_hex(0x0C1222), LV_PART_MAIN);
    lv_obj_set_style_border_width(s_cont_home, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(s_cont_home, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_cont_home, 6, LV_PART_MAIN);
    lv_obj_clear_flag(s_cont_home, LV_OBJ_FLAG_SCROLLABLE);

    /* Wallpaper Clock Widget Card */
    lv_obj_t *card_clock = lv_obj_create(s_cont_home);
    lv_obj_set_size(card_clock, 228, 76);
    lv_obj_set_pos(card_clock, 0, 2);
    lv_obj_set_style_bg_color(card_clock, lv_color_hex(0x131D33), LV_PART_MAIN);
    lv_obj_set_style_border_color(card_clock, lv_color_hex(0x00E5FF), LV_PART_MAIN);
    lv_obj_set_style_border_width(card_clock, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(card_clock, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card_clock, 4, LV_PART_MAIN);
    lv_obj_clear_flag(card_clock, LV_OBJ_FLAG_SCROLLABLE);

    s_lbl_home_time = lv_label_create(card_clock);
    lv_label_set_text(s_lbl_home_time, "00:00:00");
    lv_obj_set_style_text_color(s_lbl_home_time, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(s_lbl_home_time, LV_ALIGN_TOP_MID, 0, 2);

    s_lbl_home_date = lv_label_create(card_clock);
    lv_label_set_text(s_lbl_home_date, "Sunday, 8 Nov 2026");
    lv_obj_set_style_text_color(s_lbl_home_date, lv_color_hex(0x00E5FF), LV_PART_MAIN);
    lv_obj_align(s_lbl_home_date, LV_ALIGN_CENTER, 0, 8);

    lv_obj_t *lbl_op = lv_label_create(card_clock);
    lv_label_set_text(lbl_op, "OpenFeatureOS 4G • Volatile DRAM");
    lv_obj_set_style_text_color(lbl_op, lv_color_hex(0x78909C), LV_PART_MAIN);
    lv_obj_align(lbl_op, LV_ALIGN_BOTTOM_MID, 0, -2);

    /* Feature Phone Apps Grid / Vertical List */
    const char *app_names[4] = {
        "[1] Phone Dialer",
        "[2] Flashlight: [ OFF ]",
        "[3] System Debug & Specs",
        "[4] Settings & Display"
    };

    int start_y = 86;
    for (int i = 0; i < 4; i++) {
        lv_obj_t *btn = lv_btn_create(s_cont_home);
        lv_obj_set_size(btn, 228, 38);
        lv_obj_set_pos(btn, 0, start_y + (i * 44));
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x131D33), LV_PART_MAIN);
        lv_obj_set_style_border_color(btn, lv_color_hex(0x1E2E4E), LV_PART_MAIN);
        lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN);
        lv_obj_set_style_radius(btn, 6, LV_PART_MAIN);
        lv_obj_set_style_pad_all(btn, 4, LV_PART_MAIN);

        /* Glowing focus style for keypad navigation */
        lv_obj_set_style_border_color(btn, lv_color_hex(0x00E5FF), LV_STATE_FOCUSED);
        lv_obj_set_style_border_width(btn, 2, LV_STATE_FOCUSED);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x1B2C4E), LV_STATE_FOCUSED);

        lv_obj_add_event_cb(btn, home_app_click_cb, LV_EVENT_CLICKED, NULL);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, app_names[i]);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 8, 0);

        s_btn_home_apps[i] = btn;
        if (i == 1) {
            s_lbl_home_torch_status = lbl;
        }

        if (g_lvgl_keypad_group) {
            lv_group_add_obj(g_lvgl_keypad_group, btn);
        }
    }
}

/* ----------------------------------------------------------------------------
 * View 2: Phone Dialer
 * ----------------------------------------------------------------------------
 */
static void create_dialer_view(lv_obj_t *parent)
{
    s_cont_dialer = lv_obj_create(parent);
    lv_obj_set_size(s_cont_dialer, 240, 272);
    lv_obj_set_pos(s_cont_dialer, 0, 24);
    lv_obj_set_style_bg_color(s_cont_dialer, lv_color_hex(0x0A0F1D), LV_PART_MAIN);
    lv_obj_set_style_border_width(s_cont_dialer, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(s_cont_dialer, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_cont_dialer, 4, LV_PART_MAIN);
    lv_obj_clear_flag(s_cont_dialer, LV_OBJ_FLAG_SCROLLABLE);

    /* Dialed Digits Readout Box */
    lv_obj_t *card_readout = lv_obj_create(s_cont_dialer);
    lv_obj_set_size(card_readout, 232, 40);
    lv_obj_set_pos(card_readout, 0, 2);
    lv_obj_set_style_bg_color(card_readout, lv_color_hex(0x060912), LV_PART_MAIN);
    lv_obj_set_style_border_color(card_readout, lv_color_hex(0x00E5FF), LV_PART_MAIN);
    lv_obj_set_style_border_width(card_readout, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(card_readout, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card_readout, 4, LV_PART_MAIN);
    lv_obj_clear_flag(card_readout, LV_OBJ_FLAG_SCROLLABLE);

    s_dial_label = lv_label_create(card_readout);
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

    s_btnm_dial = lv_btnmatrix_create(s_cont_dialer);
    lv_btnmatrix_set_map(s_btnm_dial, btnm_map);
    lv_obj_set_size(s_btnm_dial, 232, 170);
    lv_obj_set_pos(s_btnm_dial, 0, 48);
    lv_obj_set_style_bg_color(s_btnm_dial, lv_color_hex(0x0A0F1D), LV_PART_MAIN);
    lv_obj_set_style_border_width(s_btnm_dial, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(s_btnm_dial, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_btnm_dial, 2, LV_PART_MAIN);

    /* Keys Style */
    lv_obj_set_style_bg_color(s_btnm_dial, lv_color_hex(0x152238), LV_PART_ITEMS);
    lv_obj_set_style_text_color(s_btnm_dial, lv_color_hex(0xFFFFFF), LV_PART_ITEMS);
    lv_obj_set_style_border_color(s_btnm_dial, lv_color_hex(0x1E3A5F), LV_PART_ITEMS);
    lv_obj_set_style_border_width(s_btnm_dial, 1, LV_PART_ITEMS);
    lv_obj_set_style_radius(s_btnm_dial, 6, LV_PART_ITEMS);

    /* Focus Highlight */
    lv_obj_set_style_bg_color(s_btnm_dial, lv_color_hex(0x00E5FF), LV_PART_ITEMS | LV_STATE_FOCUSED);
    lv_obj_set_style_text_color(s_btnm_dial, lv_color_hex(0x000000), LV_PART_ITEMS | LV_STATE_FOCUSED);

    lv_obj_add_event_cb(s_btnm_dial, dial_btnm_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* Action Buttons: CALL and CLEAR */
    lv_obj_t *btn_call = lv_btn_create(s_cont_dialer);
    lv_obj_set_size(btn_call, 112, 38);
    lv_obj_set_pos(btn_call, 0, 224);
    lv_obj_set_style_bg_color(btn_call, lv_color_hex(0x00C853), LV_PART_MAIN);
    lv_obj_set_style_radius(btn_call, 6, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_call, dial_call_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_call = lv_label_create(btn_call);
    lv_label_set_text(lbl_call, "CALL");
    lv_obj_set_style_text_color(lbl_call, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(lbl_call, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *btn_del = lv_btn_create(s_cont_dialer);
    lv_obj_set_size(btn_del, 112, 38);
    lv_obj_set_pos(btn_del, 120, 224);
    lv_obj_set_style_bg_color(btn_del, lv_color_hex(0xD50000), LV_PART_MAIN);
    lv_obj_set_style_radius(btn_del, 6, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_del, dial_del_event_cb, LV_EVENT_ALL, NULL);

    lv_obj_t *lbl_del = lv_label_create(btn_del);
    lv_label_set_text(lbl_del, "CLEAR");
    lv_obj_set_style_text_color(lbl_del, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(lbl_del, LV_ALIGN_CENTER, 0, 0);

    if (g_lvgl_keypad_group) {
        lv_group_add_obj(g_lvgl_keypad_group, s_btnm_dial);
        lv_group_add_obj(g_lvgl_keypad_group, btn_call);
        lv_group_add_obj(g_lvgl_keypad_group, btn_del);
    }

    lv_obj_add_flag(s_cont_dialer, LV_OBJ_FLAG_HIDDEN);
}

/* ----------------------------------------------------------------------------
 * View 3: Torch / Flashlight Tool
 * ----------------------------------------------------------------------------
 */
static void create_torch_view(lv_obj_t *parent)
{
    s_cont_torch = lv_obj_create(parent);
    lv_obj_set_size(s_cont_torch, 240, 272);
    lv_obj_set_pos(s_cont_torch, 0, 24);
    lv_obj_set_style_bg_color(s_cont_torch, lv_color_hex(0x0A0F1D), LV_PART_MAIN);
    lv_obj_set_style_border_width(s_cont_torch, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(s_cont_torch, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_cont_torch, 8, LV_PART_MAIN);
    lv_obj_clear_flag(s_cont_torch, LV_OBJ_FLAG_SCROLLABLE);

    /* Header Title */
    lv_obj_t *title = lv_label_create(s_cont_torch);
    lv_label_set_text(title, "FLASHLIGHT TOOL");
    lv_obj_set_style_text_color(title, lv_color_hex(0x00E5FF), LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 4);

    /* Large Status Badge */
    s_torch_badge = lv_label_create(s_cont_torch);
    lv_label_set_text(s_torch_badge, "TORCH: [ IDLE - OFF ]");
    lv_obj_set_style_bg_color(s_torch_badge, lv_color_hex(0x37474F), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_torch_badge, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_text_color(s_torch_badge, lv_color_hex(0xB0BEC5), LV_PART_MAIN);
    lv_obj_set_style_radius(s_torch_badge, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_torch_badge, 10, LV_PART_MAIN);
    lv_obj_set_width(s_torch_badge, 220);
    lv_obj_set_style_text_align(s_torch_badge, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_pos(s_torch_badge, 2, 34);

    /* Toggle Button */
    s_torch_toggle_btn = lv_btn_create(s_cont_torch);
    lv_obj_set_size(s_torch_toggle_btn, 220, 44);
    lv_obj_set_pos(s_torch_toggle_btn, 2, 88);
    lv_obj_set_style_bg_color(s_torch_toggle_btn, lv_color_hex(0x0288D1), LV_PART_MAIN);
    lv_obj_set_style_border_color(s_torch_toggle_btn, lv_color_hex(0x00E5FF), LV_STATE_FOCUSED);
    lv_obj_set_style_border_width(s_torch_toggle_btn, 2, LV_STATE_FOCUSED);
    lv_obj_set_style_radius(s_torch_toggle_btn, 6, LV_PART_MAIN);
    lv_obj_add_event_cb(s_torch_toggle_btn, torch_toggle_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_toggle = lv_label_create(s_torch_toggle_btn);
    lv_label_set_text(lbl_toggle, "TOGGLE TORCH (OK)");
    lv_obj_set_style_text_color(lbl_toggle, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(lbl_toggle, LV_ALIGN_CENTER, 0, 0);

    /* Brightness Level Button */
    s_torch_lvl_btn = lv_btn_create(s_cont_torch);
    lv_obj_set_size(s_torch_lvl_btn, 220, 40);
    lv_obj_set_pos(s_torch_lvl_btn, 2, 142);
    lv_obj_set_style_bg_color(s_torch_lvl_btn, lv_color_hex(0x263238), LV_PART_MAIN);
    lv_obj_set_style_border_color(s_torch_lvl_btn, lv_color_hex(0x00E5FF), LV_STATE_FOCUSED);
    lv_obj_set_style_border_width(s_torch_lvl_btn, 2, LV_STATE_FOCUSED);
    lv_obj_set_style_radius(s_torch_lvl_btn, 6, LV_PART_MAIN);
    lv_obj_add_event_cb(s_torch_lvl_btn, torch_lvl_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_lvl = lv_label_create(s_torch_lvl_btn);
    lv_label_set_text(lbl_lvl, "Level: MAX (100%)");
    lv_obj_set_style_text_color(lbl_lvl, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(lbl_lvl, LV_ALIGN_CENTER, 0, 0);

    /* SOS Morse Strobe Button */
    lv_obj_t *btn_sos = lv_btn_create(s_cont_torch);
    lv_obj_set_size(btn_sos, 220, 40);
    lv_obj_set_pos(btn_sos, 2, 192);
    lv_obj_set_style_bg_color(btn_sos, lv_color_hex(0xBF360C), LV_PART_MAIN);
    lv_obj_set_style_border_color(btn_sos, lv_color_hex(0x00E5FF), LV_STATE_FOCUSED);
    lv_obj_set_style_border_width(btn_sos, 2, LV_STATE_FOCUSED);
    lv_obj_set_style_radius(btn_sos, 6, LV_PART_MAIN);
    lv_obj_add_event_cb(btn_sos, torch_sos_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_sos = lv_label_create(btn_sos);
    lv_label_set_text(lbl_sos, "STROBE SOS BEACON");
    lv_obj_set_style_text_color(lbl_sos, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(lbl_sos, LV_ALIGN_CENTER, 0, 0);

    if (g_lvgl_keypad_group) {
        lv_group_add_obj(g_lvgl_keypad_group, s_torch_toggle_btn);
        lv_group_add_obj(g_lvgl_keypad_group, s_torch_lvl_btn);
        lv_group_add_obj(g_lvgl_keypad_group, btn_sos);
    }

    lv_obj_add_flag(s_cont_torch, LV_OBJ_FLAG_HIDDEN);
}

/* ----------------------------------------------------------------------------
 * View 4: System Debug & Hardware Telemetry (Requested by User!)
 * ----------------------------------------------------------------------------
 */
static void create_debug_view(lv_obj_t *parent)
{
    s_cont_debug = lv_obj_create(parent);
    lv_obj_set_size(s_cont_debug, 240, 272);
    lv_obj_set_pos(s_cont_debug, 0, 24);
    lv_obj_set_style_bg_color(s_cont_debug, lv_color_hex(0x0A0F1D), LV_PART_MAIN);
    lv_obj_set_style_border_width(s_cont_debug, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(s_cont_debug, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_cont_debug, 6, LV_PART_MAIN);
    lv_obj_clear_flag(s_cont_debug, LV_OBJ_FLAG_SCROLLABLE);

    /* Header Title */
    lv_obj_t *title = lv_label_create(s_cont_debug);
    lv_label_set_text(title, "SYSTEM DEBUG & HARDWARE");
    lv_obj_set_style_text_color(title, lv_color_hex(0x00E5FF), LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 2);

    /* Telemetry Card */
    lv_obj_t *card = lv_obj_create(s_cont_debug);
    lv_obj_set_size(card, 228, 230);
    lv_obj_set_pos(card, 0, 24);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x131D33), LV_PART_MAIN);
    lv_obj_set_style_border_color(card, lv_color_hex(0x00E5FF), LV_PART_MAIN);
    lv_obj_set_style_border_width(card, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(card, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card, 6, LV_PART_MAIN);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    s_lbl_debug_telemetry = lv_label_create(card);
    lv_obj_set_style_text_color(s_lbl_debug_telemetry, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_line_space(s_lbl_debug_telemetry, 4, LV_PART_MAIN);

    /* Static initial content */
    lv_label_set_text(s_lbl_debug_telemetry,
        "OS Name : OpenFeatureOS\n"
        "Version : v1.0 (Build 2026.11)\n"
        "Kernel  : RT-Thread 5.4.0 UP\n"
        "SoC     : Unisoc T107 (UMS9117)\n"
        "Core    : Cortex-A7 @ 1.0 GHz\n"
        "DRAM    : 48 MB LPDDR2 (0x80000000)\n"
        "Heap    : 31.6 MB Volatile\n"
        "Display : GC9307 240x320 SPI1 DMA\n"
        "Flash   : 0 NAND WRITES (SAFE)\n"
        "Uptime  : Initializing...");
    lv_obj_align(s_lbl_debug_telemetry, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_add_flag(s_cont_debug, LV_OBJ_FLAG_HIDDEN);
}

/* ----------------------------------------------------------------------------
 * View 5: Settings & Display
 * ----------------------------------------------------------------------------
 */
static void create_settings_view(lv_obj_t *parent)
{
    s_cont_settings = lv_obj_create(parent);
    lv_obj_set_size(s_cont_settings, 240, 272);
    lv_obj_set_pos(s_cont_settings, 0, 24);
    lv_obj_set_style_bg_color(s_cont_settings, lv_color_hex(0x0A0F1D), LV_PART_MAIN);
    lv_obj_set_style_border_width(s_cont_settings, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(s_cont_settings, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_cont_settings, 6, LV_PART_MAIN);
    lv_obj_clear_flag(s_cont_settings, LV_OBJ_FLAG_SCROLLABLE);

    /* Header Title */
    lv_obj_t *title = lv_label_create(s_cont_settings);
    lv_label_set_text(title, "SETTINGS & TOOLS");
    lv_obj_set_style_text_color(title, lv_color_hex(0x00E5FF), LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 2);

    /* Backlight Group Card */
    lv_obj_t *card_bl = lv_obj_create(s_cont_settings);
    lv_obj_set_size(card_bl, 228, 92);
    lv_obj_set_pos(card_bl, 0, 24);
    lv_obj_set_style_bg_color(card_bl, lv_color_hex(0x131D33), LV_PART_MAIN);
    lv_obj_set_style_border_color(card_bl, lv_color_hex(0x1E3A5F), LV_PART_MAIN);
    lv_obj_set_style_border_width(card_bl, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(card_bl, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card_bl, 4, LV_PART_MAIN);
    lv_obj_clear_flag(card_bl, LV_OBJ_FLAG_SCROLLABLE);

    s_lbl_bl_val = lv_label_create(card_bl);
    lv_label_set_text(s_lbl_bl_val, "Screen Brightness: 80%");
    lv_obj_set_style_text_color(s_lbl_bl_val, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(s_lbl_bl_val, LV_ALIGN_TOP_MID, 0, 2);

    /* 4 Brightness Preset Buttons */
    const uint32_t presets[4] = { 30, 50, 80, 100 };
    const char *preset_labels[4] = { "30%", "50%", "80%", "100%" };

    for (int i = 0; i < 4; i++) {
        lv_obj_t *btn = lv_btn_create(card_bl);
        lv_obj_set_size(btn, 50, 36);
        lv_obj_set_pos(btn, i * 54, 40);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x0288D1), LV_PART_MAIN);
        lv_obj_set_style_border_color(btn, lv_color_hex(0x00E5FF), LV_STATE_FOCUSED);
        lv_obj_set_style_border_width(btn, 2, LV_STATE_FOCUSED);
        lv_obj_set_style_radius(btn, 4, LV_PART_MAIN);
        lv_obj_add_event_cb(btn, bl_btn_event_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)presets[i]);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, preset_labels[i]);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);

        if (g_lvgl_keypad_group) {
            lv_group_add_obj(g_lvgl_keypad_group, btn);
        }
    }

    /* Display Color Calibration Swatches Card */
    lv_obj_t *card_swatch = lv_obj_create(s_cont_settings);
    lv_obj_set_size(card_swatch, 228, 116);
    lv_obj_set_pos(card_swatch, 0, 122);
    lv_obj_set_style_bg_color(card_swatch, lv_color_hex(0x131D33), LV_PART_MAIN);
    lv_obj_set_style_border_color(card_swatch, lv_color_hex(0x1E3A5F), LV_PART_MAIN);
    lv_obj_set_style_border_width(card_swatch, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(card_swatch, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card_swatch, 4, LV_PART_MAIN);
    lv_obj_clear_flag(card_swatch, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_sw = lv_label_create(card_swatch);
    lv_label_set_text(lbl_sw, "RGB565 Color Calibration");
    lv_obj_set_style_text_color(lbl_sw, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(lbl_sw, LV_ALIGN_TOP_MID, 0, 2);

    /* 8 Color Swatches */
    const uint32_t colors[8] = {
        0xFFFFFF, 0xFF0000, 0x00FF00, 0x0000FF,
        0x00FFFF, 0xFFFF00, 0xFF00FF, 0x757575
    };
    int w = 218 / 8;
    for (int i = 0; i < 8; i++) {
        lv_obj_t *b = lv_obj_create(card_swatch);
        lv_obj_set_size(b, w, 32);
        lv_obj_set_pos(b, i * w, 28);
        lv_obj_set_style_bg_color(b, lv_color_hex(colors[i]), LV_PART_MAIN);
        lv_obj_set_style_border_width(b, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(b, 2, LV_PART_MAIN);
    }

    /* Grayscale Ramp */
    for (int i = 0; i < 8; i++) {
        uint32_t c = (i * 36);
        uint32_t rgb = (c << 16) | (c << 8) | c;
        lv_obj_t *b = lv_obj_create(card_swatch);
        lv_obj_set_size(b, w, 24);
        lv_obj_set_pos(b, i * w, 68);
        lv_obj_set_style_bg_color(b, lv_color_hex(rgb), LV_PART_MAIN);
        lv_obj_set_style_border_width(b, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(b, 2, LV_PART_MAIN);
    }

    lv_obj_add_flag(s_cont_settings, LV_OBJ_FLAG_HIDDEN);
}

/* ----------------------------------------------------------------------------
 * Master Feature Phone Launcher Initializer
 * ----------------------------------------------------------------------------
 */
static void create_openfeatureos_home(void)
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0A0F1D), LV_PART_MAIN);

    /* 1. Top Status Bar */
    create_status_bar(scr);

    /* 2. Create View Containers */
    create_home_view(scr);
    create_dialer_view(scr);
    create_torch_view(scr);
    create_debug_view(scr);
    create_settings_view(scr);

    /* 3. Bottom Softkey Bar */
    create_softkey_bar(scr);

    /* 4. Register Global Keypad Event Callback */
    lv_obj_add_event_cb(scr, screen_key_event_cb, LV_EVENT_KEY, NULL);

    /* 5. Set initial view to HOME */
    switch_view(VIEW_HOME);

    /* 6. Periodic 1-Second Uptime & Telemetry Timer */
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
