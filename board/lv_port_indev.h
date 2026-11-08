/*
 * OpenFeatureOS - LVGL Keypad Input Driver Port for Unisoc T107 (UMS9117)
 * Target: Nokia 2660 Flip (TA-1480) 5x5 Keypad Matrix
 */

#ifndef LV_PORT_INDEV_H_
#define LV_PORT_INDEV_H_

#include "lvgl/lvgl.h"

extern lv_group_t *g_lvgl_keypad_group;

void lv_port_indev_init(void);

#endif /* LV_PORT_INDEV_H_ */
