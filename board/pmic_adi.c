/*
 * OpenFeatureOS - Unisoc T107 ADI (Analog Die Interface) Bus Driver
 * Target: Unisoc T107 (UMS9117) / SC2720 PMIC (Nokia 2660 Flip TA-1480)
 *
 * Sourced from:
 *   - Nokia-2660-RE docs/hardware.md
 */

#include "pmic_adi.h"
#include "t107_regs.h"

uint32_t pmic_adi_read(uint32_t addr)
{
    int32_t val;
    uint32_t timeout = 100000U;
    ADI_RD_CMD = addr;
    while ((val = (int32_t)ADI_RD_DATA) < 0 && --timeout) {
        /* Poll until bit 31 clears (data ready) */
    }
    return (uint16_t)val;
}

void pmic_adi_write(uint32_t addr, uint32_t val)
{
    uint32_t timeout = 100000U;
    while ((ADI_ARM_STS & 0x800U) && --timeout) {
        /* Wait if write FIFO is busy (bit 11) */
    }
    REG32(addr) = val;
}
