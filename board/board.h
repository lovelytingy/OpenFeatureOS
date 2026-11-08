/*
 * OpenFeatureOS - Board Support Package Header
 * Target: Unisoc T107 (UMS9117) / Nokia 2660 Flip (TA-1480)
 *
 * Sourced from:
 *   - RT-Thread Architecture Guidelines (bsp/qemu-vexpress-a9/drivers/board.h)
 *   - Nokia-2660-RE docs/kernel-architecture.md
 */

#ifndef BOARD_H__
#define BOARD_H__

#include <rtthread.h>
#include <rthw.h>
#include "t107_regs.h"

void rt_hw_board_init(void);
void rt_hw_console_output(const char *str);

#endif /* BOARD_H__ */
