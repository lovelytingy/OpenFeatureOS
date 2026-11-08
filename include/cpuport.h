/*
 * OpenFeatureOS - ARM Cortex-A7 Architecture Port Definitions
 * Target: Unisoc T107 (UMS9117) ARM Cortex-A7 (Nokia 2660 Flip TA-1480)
 *
 * Sourced from:
 *   - RT-Thread libcpu/arm/cortex-a/cpuport.h
 */

#ifndef CPUPORT_H__
#define CPUPORT_H__

#include <rtdef.h>

#define USERMODE    0x10
#define FIQMODE     0x11
#define IRQMODE     0x12
#define SVCMODE     0x13
#define ABORTMODE   0x17
#define UNDEFMODE   0x1B
#define MODEMASK    0x1F

#define T_Bit       (1 << 5)
#define F_Bit       (1 << 6)
#define I_Bit       (1 << 7)

void _thread_start(void);

#endif /* CPUPORT_H__ */
