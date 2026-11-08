/*
 * OpenFeatureOS - Unisoc T107 Pinmux & Pad Controller Setup
 * Target: Nokia 2660 Flip (TA-1480 / HW3.0)
 *
 * Configures all pin multiplexing registers (0x402A0000) and PMIC pins (0x40608000)
 * to route SPI1, LCD control lines, and keypad scanner lines.
 */

#ifndef BOARD_PINMAP_H_
#define BOARD_PINMAP_H_

#include <stdint.h>

void pinmap_init(void);

#endif /* BOARD_PINMAP_H_ */
