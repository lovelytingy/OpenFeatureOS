/*
 * OpenFeatureOS - Unisoc T107 ADI (Analog Die Interface) Bus Header
 * Target: Unisoc T107 (UMS9117) / SC2720 PMIC (Nokia 2660 Flip TA-1480)
 */

#ifndef PMIC_ADI_H_
#define PMIC_ADI_H_

#include <stdint.h>

uint32_t pmic_adi_read(uint32_t addr);
void     pmic_adi_write(uint32_t addr, uint32_t val);

#endif /* PMIC_ADI_H_ */
