/*
 * OpenFeatureOS - LVGL UI Application & Feature Phone Launcher
 * Target: Unisoc T107 (UMS9117) / Nokia 2660 Flip (TA-1480)
 */

#ifndef UI_APP_H_
#define UI_APP_H_

#include <stdint.h>

void ui_app_init(void);
void ui_app_run(void);
void ui_app_handle_key(uint32_t key);

#endif /* UI_APP_H_ */
