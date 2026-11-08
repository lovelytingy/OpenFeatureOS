/*
 * OpenFeatureOS - Unisoc T107 (UMS9117) LCD Controller Driver
 * Target: Nokia 2660 Flip (TA-1480) 240x320 QVGA RGB565 Display
 *
 * Sourced from:
 *   - Nokia-2660-RE tools/ram_payload/src/baremetal_lcd/payload.c
 *   - Nokia-2660-Emulator crates/n2660-core/src/vm.rs:1676-1750
 */

#ifndef LCDC_H_
#define LCDC_H_

#include <stdint.h>
#include "t107_regs.h"

/* Standard RGB565 Color Definitions */
#define COLOR_BLACK         0x0000U
#define COLOR_WHITE         0xFFFFU
#define COLOR_DARK_BG       0x0842U  /* Deep charcoal background */
#define COLOR_CARD_BG       0x18E3U  /* Elevated panel card background */
#define COLOR_CYAN_ACCENT   0x07FFU  /* OpenFeatureOS Cyan glow */
#define COLOR_NOKIA_BLUE    0x016CU  /* Classic Nokia deep blue */
#define COLOR_RED           0xF800U
#define COLOR_GREEN         0x07E0U
#define COLOR_BLUE          0x001FU
#define COLOR_YELLOW        0xFFE0U
#define COLOR_MAGENTA       0xF81FU
#define COLOR_ORANGE        0xFD20U
#define COLOR_GRAY          0x8410U
#define COLOR_LIGHT_GRAY    0xC618U

/* Framebuffer pointer exported for direct drawing or LVGL display driver */
extern uint16_t *g_framebuffer;

/* Driver functions */
void lcdc_init(void);
void lcdc_flush(void);
void lcdc_clear(uint16_t color);
void lcdc_fill_rect(int x, int y, int w, int h, uint16_t color);
void lcdc_draw_char(int x, int y, char c, uint16_t fg, uint16_t bg);
void lcdc_draw_char_scale(int x, int y, char c, uint16_t fg, uint16_t bg, int scale);
void lcdc_draw_string(int x, int y, const char *str, uint16_t fg, uint16_t bg);
void lcdc_draw_string_scale(int x, int y, const char *str, uint16_t fg, uint16_t bg, int scale);
void lcdc_draw_string_centered(int y, const char *str, uint16_t fg, uint16_t bg);
void lcdc_draw_string_centered_scale(int y, const char *str, uint16_t fg, uint16_t bg, int scale);
void lcdc_draw_boot_splash(void);
void lcdc_draw_boot_splash_static(void);
void lcdc_update_boot_progress(int progress_pct, const char *status_msg);
void lcdc_run_boot_splash(uint32_t duration_ms);
void lcdc_update_heartbeat(uint32_t beat_count, uint32_t uptime_s);

#endif /* LCDC_H_ */
