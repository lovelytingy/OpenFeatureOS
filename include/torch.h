/*
 * OpenFeatureOS - SC2720 PMIC Flashlight / Torch Driver Header
 * Target: Unisoc T107 (UMS9117) / Nokia 2660 Flip (TA-1480)
 */

#ifndef TORCH_H_
#define TORCH_H_

#include <stdint.h>

void torch_init(void);
void torch_set(int on, uint8_t level);
void torch_pulse(uint32_t duration_ms, uint8_t level);
void torch_blink(int count, uint32_t on_ms, uint32_t off_ms, uint8_t level);
void torch_heartbeat(void);
void torch_morse_sos(uint8_t level);

void delay_cycles(uint32_t cycles);
void delay_us(uint32_t us);
void delay_ms(uint32_t ms);

#endif /* TORCH_H_ */
