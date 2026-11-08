/*
 * OpenFeatureOS - Watchdog Handling Header
 * Target: Unisoc T107 (UMS9117) / SC2720 PMIC (Nokia 2660 Flip TA-1480)
 */

#ifndef WATCHDOG_H_
#define WATCHDOG_H_

#include <stdint.h>

void watchdog_init(void);
void watchdog_disable(void);
void watchdog_pet(void);

#endif /* WATCHDOG_H_ */
