/*
 * OpenFeatureOS - LVGL Keypad Input Driver Port for Unisoc T107 (UMS9117)
 * Target: Nokia 2660 Flip (TA-1480 / HW3.0) 5x5 Keypad Matrix & SC2720 PMIC EIC
 */

#ifndef LV_PORT_INDEV_H_
#define LV_PORT_INDEV_H_

#include <stdint.h>
#include "lvgl/lvgl.h"

extern lv_group_t *g_lvgl_keypad_group;

typedef void (*keypad_event_cb_t)(int key, int is_down);

void keypad_hw_init(void);
void lv_port_indev_init(void);
int keypad_poll_event(int *out_code, int *out_is_down);
void keypad_drain_events(void);
void keypad_set_event_handler(keypad_event_cb_t cb);
uint32_t keypad_poll_raw(uint32_t *out_is_down);
uint32_t hw_code_to_lv_key(int code);

#endif /* LV_PORT_INDEV_H_ */
