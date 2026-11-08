/*
 * OpenFeatureOS - SC2720 PMIC LCD Backlight Controller
 * Target: Nokia 2660 Flip (TA-1480 / HW3.0)
 *
 * Controls RGB and White LED current sinks via SC2720 PMIC ADI bus.
 */

#ifndef BOARD_BACKLIGHT_H_
#define BOARD_BACKLIGHT_H_

#include <stdint.h>

void backlight_init(void);
void backlight_set_brightness(uint32_t percent);

#endif /* BOARD_BACKLIGHT_H_ */
