/*
 * OpenFeatureOS - LVGL UI Application & Feature Phone Launcher
 * Target: Unisoc T107 (UMS9117) / Nokia 2660 Flip (TA-1480)
 */

#include "ui_app.h"
#include "lvgl/lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "torch.h"
#include "printk.h"
#include <rtthread.h>

static int s_torch_state = 0;
static lv_obj_t *s_torch_btn = NULL;
static lv_obj_t *s_torch_label = NULL;

static void torch_btn_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        s_torch_state = !s_torch_state;
        torch_set(s_torch_state, 0x0F);
        if (s_torch_label) {
            lv_label_set_text_fmt(s_torch_label, "Torch: %s", s_torch_state ? "ON [Active]" : "OFF [Ready]");
        }
    }
}

static void create_openfeatureos_home(void)
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x10141D), LV_PART_MAIN);

    /* 1. Top Status Bar */
    lv_obj_t *status_bar = lv_obj_create(scr);
    lv_obj_set_size(status_bar, 240, 24);
    lv_obj_set_pos(status_bar, 0, 0);
    lv_obj_set_style_bg_color(status_bar, lv_color_hex(0x0A0D14), LV_PART_MAIN);
    lv_obj_set_style_border_width(status_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(status_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(status_bar, 2, LV_PART_MAIN);

    lv_obj_t *lbl_net = lv_label_create(status_bar);
    lv_label_set_text(lbl_net, "4G [VoLTE]");
    lv_obj_set_style_text_color(lbl_net, lv_color_hex(0x00D2FF), LV_PART_MAIN);
    lv_obj_align(lbl_net, LV_ALIGN_LEFT_MID, 4, 0);

    lv_obj_t *lbl_bat = lv_label_create(status_bar);
    lv_label_set_text(lbl_bat, "100% [PWR]");
    lv_obj_set_style_text_color(lbl_bat, lv_color_hex(0x00FF88), LV_PART_MAIN);
    lv_obj_align(lbl_bat, LV_ALIGN_RIGHT_MID, -4, 0);

    /* 2. Brand Header */
    lv_obj_t *hdr = lv_obj_create(scr);
    lv_obj_set_size(hdr, 240, 36);
    lv_obj_set_pos(hdr, 0, 24);
    lv_obj_set_style_bg_color(hdr, lv_color_hex(0x0D3B66), LV_PART_MAIN);
    lv_obj_set_style_border_width(hdr, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(hdr, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(hdr, 0, LV_PART_MAIN);

    lv_obj_t *title = lv_label_create(hdr);
    lv_label_set_text(title, "OpenFeatureOS");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    /* Accent line */
    lv_obj_t *line = lv_obj_create(scr);
    lv_obj_set_size(line, 240, 2);
    lv_obj_set_pos(line, 0, 60);
    lv_obj_set_style_bg_color(line, lv_color_hex(0x00D2FF), LV_PART_MAIN);
    lv_obj_set_style_border_width(line, 0, LV_PART_MAIN);

    /* 3. Main Menu List */
    lv_obj_t *list = lv_list_create(scr);
    lv_obj_set_size(list, 230, 206);
    lv_obj_set_pos(list, 5, 64);
    lv_obj_set_style_bg_color(list, lv_color_hex(0x151B26), LV_PART_MAIN);
    lv_obj_set_style_border_color(list, lv_color_hex(0x232E40), LV_PART_MAIN);
    lv_obj_set_style_border_width(list, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(list, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_all(list, 4, LV_PART_MAIN);

    /* Menu Items */
    lv_obj_t *btn_dialer = lv_list_add_btn(list, NULL, "1. Phone Dialer");
    lv_group_add_obj(g_lvgl_keypad_group, btn_dialer);

    lv_obj_t *btn_msg = lv_list_add_btn(list, NULL, "2. Messages");
    lv_group_add_obj(g_lvgl_keypad_group, btn_msg);

    s_torch_btn = lv_list_add_btn(list, NULL, "3. Torch: OFF [Ready]");
    s_torch_label = lv_obj_get_child(s_torch_btn, 0);
    lv_obj_add_event_cb(s_torch_btn, torch_btn_event_cb, LV_EVENT_ALL, NULL);
    lv_group_add_obj(g_lvgl_keypad_group, s_torch_btn);

    lv_obj_t *btn_info = lv_list_add_btn(list, NULL, "4. System Info");
    lv_group_add_obj(g_lvgl_keypad_group, btn_info);

    lv_obj_t *btn_sett = lv_list_add_btn(list, NULL, "5. Settings");
    lv_group_add_obj(g_lvgl_keypad_group, btn_sett);

    /* 4. Bottom Softkey Bar */
    lv_obj_t *softkey_bar = lv_obj_create(scr);
    lv_obj_set_size(softkey_bar, 240, 24);
    lv_obj_set_pos(softkey_bar, 0, 296);
    lv_obj_set_style_bg_color(softkey_bar, lv_color_hex(0x0A0D14), LV_PART_MAIN);
    lv_obj_set_style_border_width(softkey_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(softkey_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(softkey_bar, 2, LV_PART_MAIN);

    lv_obj_t *lbl_lsk = lv_label_create(softkey_bar);
    lv_label_set_text(lbl_lsk, "Options");
    lv_obj_set_style_text_color(lbl_lsk, lv_color_hex(0x8899AA), LV_PART_MAIN);
    lv_obj_align(lbl_lsk, LV_ALIGN_LEFT_MID, 6, 0);

    lv_obj_t *lbl_ok = lv_label_create(softkey_bar);
    lv_label_set_text(lbl_ok, "Select");
    lv_obj_set_style_text_color(lbl_ok, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(lbl_ok, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *lbl_rsk = lv_label_create(softkey_bar);
    lv_label_set_text(lbl_rsk, "Back");
    lv_obj_set_style_text_color(lbl_rsk, lv_color_hex(0x8899AA), LV_PART_MAIN);
    lv_obj_align(lbl_rsk, LV_ALIGN_RIGHT_MID, -6, 0);

    /* Focus first menu item */
    lv_group_focus_obj(btn_dialer);
}

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

    kprintf("[LVGL] OpenFeatureOS Home Screen created. Entering UI dispatch loop.\n");

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
