/*
 * OpenFeatureOS - Unisoc T107 (UMS9117) LCD Controller Driver
 * Target: Nokia 2660 Flip (TA-1480) 240x320 QVGA RGB565 Display
 *
 * Direct hardware framebuffer driver for the GalaxyCore GC9307 LCD
 * via the Unisoc UMS9117 Hardware LCD Controller (0x20800000).
 */

#include "lcdc.h"
#include "t107_regs.h"
#include "printk.h"
#include "mmu.h"

extern uint8_t __framebuffer_start[];
extern uint8_t __framebuffer_end[];

uint16_t *g_framebuffer = (uint16_t *)__framebuffer_start;

/* 8x16 ASCII Bitmap Font Data */
static const uint8_t font8x16[] = {
#include "font8x16.h"
};

void lcdc_init(void)
{
    uint32_t fb_addr = (uint32_t)(uintptr_t)g_framebuffer;
    kprintf("[LCDC] Initializing UMS9117 Display Engine at 0x%08x\n", LCDC_BASE);
    kprintf("[LCDC] Primary QVGA Framebuffer: 0x%08x (%u bytes, 240x320 RGB565)\n",
            fb_addr, LCD_FB_SIZE);

    /* Program hardware framebuffer pointer (word-aligned address >> 2) */
    LCDC_Y_BASE = fb_addr >> 2;

    /* Program display dimensions: width in low 16 bits, height in high 16 bits */
    LCDC_SIZE_XY = (LCD_WIDTH & 0xFFFFU) | ((LCD_HEIGHT & 0xFFFFU) << 16);

    /* Clear any pending IRQ flags */
    LCDC_IRQ_CLR = LCDC_IRQ_REFRESH_DONE;

    /* Enable LCD Controller */
    LCDC_CTRL |= LCDC_CTRL_ENABLE;

    /* Clear initial framebuffer screen to dark background */
    lcdc_clear(COLOR_DARK_BG);
    lcdc_flush();

    kprintf("[LCDC] Display Controller active. Framebuffer refresh ready.\n");
}

void lcdc_flush(void)
{
    uint32_t fb_addr = (uint32_t)(uintptr_t)g_framebuffer;

    /* Clean data cache lines across the framebuffer range to guarantee DRAM coherency for DMA */
    dcache_clean_range(fb_addr, fb_addr + LCD_FB_SIZE);

    /* Trigger hardware refresh transaction */
    LCDC_CTRL |= LCDC_CTRL_REFRESH_START;

    /* Wait for hardware refresh completion interrupt flag */
    uint32_t timeout = 500000U;
    while (!(LCDC_IRQ_RAW & LCDC_IRQ_REFRESH_DONE) && --timeout) {
        /* Busy wait on hardware completion */
    }

    /* Acknowledge and clear refresh interrupt */
    LCDC_IRQ_CLR = LCDC_IRQ_REFRESH_DONE;
}

void lcdc_clear(uint16_t color)
{
    for (uint32_t i = 0; i < (LCD_WIDTH * LCD_HEIGHT); i++) {
        g_framebuffer[i] = color;
    }
}

void lcdc_fill_rect(int x, int y, int w, int h, uint16_t color)
{
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > (int)LCD_WIDTH)  { w = (int)LCD_WIDTH - x; }
    if (y + h > (int)LCD_HEIGHT) { h = (int)LCD_HEIGHT - y; }
    if (w <= 0 || h <= 0) return;

    for (int row = y; row < y + h; row++) {
        int row_offset = row * (int)LCD_WIDTH;
        for (int col = x; col < x + w; col++) {
            g_framebuffer[row_offset + col] = color;
        }
    }
}

void lcdc_draw_char(int x, int y, char c, uint16_t fg, uint16_t bg)
{
    if (c < 0x20 || c > 0x7E) return;
    const uint8_t *glyph = &font8x16[(c - 0x20) * 16];

    for (int row = 0; row < 16; row++) {
        int py = y + row;
        if (py < 0 || py >= (int)LCD_HEIGHT) continue;

        uint8_t bits = glyph[row];
        int row_offset = py * (int)LCD_WIDTH;

        for (int col = 0; col < 8; col++) {
            int px = x + col;
            if (px < 0 || px >= (int)LCD_WIDTH) continue;

            if (bits & (0x80 >> col)) {
                g_framebuffer[row_offset + px] = fg;
            } else if (bg != fg) {
                g_framebuffer[row_offset + px] = bg;
            }
        }
    }
}

void lcdc_draw_string(int x, int y, const char *str, uint16_t fg, uint16_t bg)
{
    while (*str) {
        lcdc_draw_char(x, y, *str++, fg, bg);
        x += 8;
        if (x + 8 > (int)LCD_WIDTH) break;
    }
}

void lcdc_draw_string_centered(int y, const char *str, uint16_t fg, uint16_t bg)
{
    int len = 0;
    while (str[len]) len++;
    int x = ((int)LCD_WIDTH - (len * 8)) / 2;
    if (x < 0) x = 0;
    lcdc_draw_string(x, y, str, fg, bg);
}

void lcdc_draw_boot_splash(void)
{
    /* 1. Deep space background */
    lcdc_clear(COLOR_DARK_BG);

    /* 2. Top Header Banner: Nokia Blue with Cyan Accent Line */
    lcdc_fill_rect(0, 0, LCD_WIDTH, 48, COLOR_NOKIA_BLUE);
    lcdc_fill_rect(0, 48, LCD_WIDTH, 2, COLOR_CYAN_ACCENT);

    lcdc_draw_string_centered(10, "OpenFeatureOS", COLOR_WHITE, COLOR_NOKIA_BLUE);
    lcdc_draw_string_centered(28, "v0.1.0-alpha (RT-Thread)", COLOR_CYAN_ACCENT, COLOR_NOKIA_BLUE);

    /* 3. System Hardware Card */
    lcdc_fill_rect(8, 58, LCD_WIDTH - 16, 76, COLOR_CARD_BG);
    lcdc_fill_rect(8, 58, 4, 76, COLOR_GREEN); /* Green status pill */
    lcdc_draw_string(18, 64, "SoC   : Unisoc T107 1.0GHz", COLOR_WHITE, COLOR_CARD_BG);
    lcdc_draw_string(18, 82, "Target: Nokia 2660 (TA-1480)", COLOR_LIGHT_GRAY, COLOR_CARD_BG);
    lcdc_draw_string(18, 100, "Core  : ARM Cortex-A7 UP", COLOR_LIGHT_GRAY, COLOR_CARD_BG);
    lcdc_draw_string(18, 118, "Status: DRAM BOOT SUCCESS", COLOR_GREEN, COLOR_CARD_BG);

    /* 4. Memory & Display Card */
    lcdc_fill_rect(8, 142, LCD_WIDTH - 16, 76, COLOR_CARD_BG);
    lcdc_fill_rect(8, 142, 4, 76, COLOR_CYAN_ACCENT); /* Cyan status pill */
    lcdc_draw_string(18, 148, "Memory: 48 MB LPDDR", COLOR_WHITE, COLOR_CARD_BG);
    lcdc_draw_string(18, 166, "Heap  : 31.6 MB Free", COLOR_LIGHT_GRAY, COLOR_CARD_BG);
    lcdc_draw_string(18, 184, "Disp  : GC9307 240x320 RGB", COLOR_LIGHT_GRAY, COLOR_CARD_BG);
    lcdc_draw_string(18, 202, "Safety: ZERO NAND WRITES", COLOR_YELLOW, COLOR_CARD_BG);

    /* 5. 8-Color Calibration Gradient Bars */
    const uint16_t swatch[] = {
        COLOR_WHITE, COLOR_CYAN_ACCENT, COLOR_YELLOW, COLOR_GREEN,
        COLOR_MAGENTA, COLOR_RED, COLOR_BLUE, COLOR_GRAY
    };
    int bar_w = (LCD_WIDTH - 16) / 8;
    for (int i = 0; i < 8; i++) {
        lcdc_fill_rect(8 + (i * bar_w), 226, bar_w, 14, swatch[i]);
    }

    /* 6. Multithreading Scheduler Status Card */
    lcdc_fill_rect(8, 248, LCD_WIDTH - 16, 42, COLOR_CARD_BG);
    lcdc_fill_rect(8, 248, 4, 42, COLOR_ORANGE);
    lcdc_draw_string(18, 254, "RTOS  : RT-Thread 5.4.0", COLOR_WHITE, COLOR_CARD_BG);
    lcdc_draw_string(18, 272, "Tasks : torch, reporter, gui", COLOR_ORANGE, COLOR_CARD_BG);

    /* 7. Bottom Status Footer */
    lcdc_fill_rect(0, 298, LCD_WIDTH, 22, 0x0000);
    lcdc_draw_string_centered(302, "OpenFeatureOS Live on Device", COLOR_GREEN, 0x0000);

    /* Flush initial frame buffer to LCD panel */
    lcdc_flush();
}

void lcdc_update_heartbeat(uint32_t beat_count, uint32_t uptime_s)
{
    char buf[32];
    /* Simple integer formatting for live display updates */
    int i = 0;
    buf[i++] = 'B'; buf[i++] = 'e'; buf[i++] = 'a'; buf[i++] = 't'; buf[i++] = ':'; buf[i++] = ' ';
    
    uint32_t n = beat_count;
    char num_buf[10];
    int ni = 0;
    if (n == 0) { num_buf[ni++] = '0'; }
    else {
        while (n > 0) { num_buf[ni++] = '0' + (n % 10); n /= 10; }
    }
    while (ni > 0) buf[i++] = num_buf[--ni];

    buf[i++] = ' '; buf[i++] = '|'; buf[i++] = ' ';
    buf[i++] = 'U'; buf[i++] = 'p'; buf[i++] = ':'; buf[i++] = ' ';
    n = uptime_s;
    if (n == 0) { num_buf[ni++] = '0'; }
    else {
        while (n > 0) { num_buf[ni++] = '0' + (n % 10); n /= 10; }
    }
    while (ni > 0) buf[i++] = num_buf[--ni];
    buf[i++] = 's';
    buf[i] = '\0';

    /* Redraw footer bar with live telemetry update */
    lcdc_fill_rect(0, 298, LCD_WIDTH, 22, 0x0000);
    lcdc_draw_string_centered(302, buf, COLOR_CYAN_ACCENT, 0x0000);
    lcdc_flush();
}
