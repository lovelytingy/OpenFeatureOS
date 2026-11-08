/*
 * OpenFeatureOS - Hardware System Timer Header
 * Target: Unisoc T107 (UMS9117) / Nokia 2660 Flip (TA-1480)
 */

#ifndef TIMER_H_
#define TIMER_H_

#include <stdint.h>

void     timer_init(void);
uint32_t timer_get_ms(void);
void     timer_delay_ms(uint32_t ms);

#endif /* TIMER_H_ */
