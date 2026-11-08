/*
 * OpenFeatureOS - Unisoc T107 (UMS9117) GPIO Controller
 * Target: Nokia 2660 Flip (TA-1480 / HW3.0)
 *
 * Controls dual-screen multiplexer GPIOs:
 *   - GPIO 40 (0x28): Main Screen = 0, Cover Screen = 1
 *   - GPIO 95 (0x5F): Main Screen = 1, Cover Screen = 0
 */

#ifndef BOARD_GPIO_H_
#define BOARD_GPIO_H_

#include <stdint.h>

#define GPIO_BASE               0x40280000U
#define GPIO_OFF_DATA           0x00U
#define GPIO_OFF_DMSK           0x04U
#define GPIO_OFF_DIR            0x08U
#define GPIO_OFF_INEN           0x28U

/* Screen selection multiplexer pins */
#define GPIO_PIN_SCREEN_SEL1    40U  /* 0x28: Main screen = 0, Cover = 1 */
#define GPIO_PIN_SCREEN_SEL2    95U  /* 0x5F: Main screen = 1, Cover = 0 */

int  gpio_set(unsigned int id, unsigned int off, int state);
void gpio_init(void);
void gpio_select_screen(int main_screen);

#endif /* BOARD_GPIO_H_ */
