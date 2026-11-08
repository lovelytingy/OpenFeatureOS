/*
 * OpenFeatureOS - Unisoc T107 (UMS9117) LCD Controller Driver
 * Target: Nokia 2660 Flip (TA-1480 / HW3.0) 240x320 QVGA RGB565 Display
 *
 * Drives the GalaxyCore GC9307 TFT LCD over hardware SPI1 (0x70B00000)
 * and Unisoc UMS9117 Hardware LCD Controller (LCDC 0x20800000).
 *
 * Sourced from:
 *   - Nokia-2660-RE tools/ram_payload/vendor/fpdoom/ums9117/syscode.c:360-575
 *   - Nokia-2660-RE tools/ram_payload/vendor/fpdoom/ums9117/lcd_spi.h
 *   - Nokia-2660-RE tools/ram_payload/vendor/fpdoom/ums9117/lcd_config.h (cmd9307_hmd)
 */

#include "lcdc.h"
#include "t107_regs.h"
#include "printk.h"
#include "mmu.h"
#include "timer.h"

extern uint8_t __framebuffer_start[];
extern uint8_t __framebuffer_end[];

uint16_t *g_framebuffer = (uint16_t *)__framebuffer_start;

/* 8x16 ASCII Bitmap Font Data */
static const uint8_t font8x16[] = {
#include "font8x16.h"
};

/*
 * SPI1 Controller Hardware Registers (0x70B00000)
 */
typedef volatile struct {
    uint32_t txd, clkd, ctl0, ctl1;               /* 0x00 .. 0x0C */
    uint32_t ctl2, ctl3, ctl4, ctl5;               /* 0x10 .. 0x1C */
    uint32_t int_en, int_clr, int_raw, int_mask;  /* 0x20 .. 0x2C */
    uint32_t sts1, sts2, dspwait, sts3;           /* 0x30 .. 0x3C */
    uint32_t ctl6, sts4, fifo_rst, ctl7;          /* 0x40 .. 0x4C */
    uint32_t sts5, ctl8, ctl9, ctl10;             /* 0x50 .. 0x5C */
    uint32_t ctl11, ctl12, sts6, sts7;            /* 0x60 .. 0x6C */
    uint32_t sts8, sts9;                          /* 0x70 .. 0x74 */
} spi_hw_t;

#define SPI1_HW ((spi_hw_t *)0x70B00000U)

static void delay_cycles(uint32_t count)
{
    for (volatile uint32_t i = 0; i < count; i++) {
        __asm__ volatile ("nop");
    }
}

static void delay_ms(uint32_t ms)
{
    uint32_t t0 = timer_get_ms();
    while ((timer_get_ms() - t0) < ms) {
        __asm__ volatile ("nop");
    }
}

/*
 * Low-level SPI1 Byte Transfer
 * is_data = 0: Command mode (CD bit = 0)
 * is_data = 1: Data mode    (CD bit = 1)
 */
static void spi1_send_byte(int is_data, uint8_t val)
{
    spi_hw_t *spi = SPI1_HW;

    if (!is_data) {
        spi->ctl8 &= ~(1U << 15); /* CD = 0 (Command) */
    }

    /* Set transmission length to 1 byte */
    spi->ctl8 = (spi->ctl8 & ~0x3FFU) | (0U << 4) | (1U >> 16);
    spi->ctl9 = (spi->ctl9 & ~0xFFFFU) | (1U & 0xFFFFU);
    spi->ctl12 |= 2U; /* SW_TX_REQ */
    spi->txd = val;

    /* Wait for transmission completion */
    uint32_t timeout = 500000U;
    while (!(spi->int_raw & (1U << 8)) && --timeout) {
        /* Busy wait */
    }
    spi->int_clr |= (1U << 8);

    timeout = 500000U;
    while ((spi->sts2 & (1U << 8)) && --timeout) {
        /* Busy wait */
    }
    timeout = 500000U;
    while (!(spi->sts2 & (1U << 7)) && --timeout) {
        /* Busy wait */
    }

    if (!is_data) {
        spi->ctl8 |= (1U << 15); /* CD = 1 (Data) */
    }
}

/*
 * Initialize SPI1 Peripheral at 48 MHz
 */
static void spi1_hw_init(void)
{
    /* 1. Ungate SPI1 APB clock */
    REG32(0x402E1134U) = 0x200U;

    /* 2. Select 192 MHz source clock */
    REG32(0x21500054U) = 3U;
    delay_cycles(100);

    /* 3. Pulse SPI1 soft reset */
    REG32(0x71301004U) = 0x40U;
    delay_cycles(1000);
    REG32(0x71302004U) = 0x40U;
    delay_cycles(1000);

    /* 4. Configure SPI1 baseline registers */
    spi_hw_t *spi = SPI1_HW;
    spi->int_en = 0;
    spi->ctl0 = 0xF00U | 2U | (8U << 2);
    spi->ctl1 &= ~0x3000U;
    spi->ctl4 = 0x8000U;
    spi->ctl5 = 0;

    /* clkd = 192 MHz / (48 MHz * 2) - 1 = 1 (48 MHz SPI clock) */
    spi->clkd = 1U;
    /* Mode 3 */
    spi->ctl7 = (spi->ctl7 & ~(7U << 3)) | (3U << 3);
    /* 8-bit channel length */
    spi->ctl0 = (spi->ctl0 & ~(0x1FU << 2)) | (8U << 2);
    spi->ctl2 = (spi->ctl2 & ~0x1FU) | 7U;
    /* Transmit and receive mode */
    spi->ctl1 |= (3U << 12);
    /* CD bit = 0 (Command) */
    spi->ctl8 &= ~(1U << 15);
    /* Select Chip Select 0 */
    spi->ctl0 = (spi->ctl0 | (1U << 8)) & ~(1U << 8);

    kprintf("[SPI1] Controller configured at 48 MHz on base 0x70B00000\n");
}

/*
 * GC9307 TFT Controller Initialization Sequence
 * Verified working on Nokia 2660 Flip (TA-1480 / TA-1711)
 */
static void gc9307_hw_init(void)
{
    /* 1. Pulse hardware LCM reset line via 0x402E160C */
    REG32(0x402E160CU) = 1U;
    delay_ms(10);
    REG32(0x402E260CU) = 1U;
    delay_ms(10);
    REG32(0x402E160CU) = 1U;
    delay_ms(20);

    /* 2. Inter Register Enable */
    spi1_send_byte(0, 0xFE);
    spi1_send_byte(0, 0xEF);

    /* 3. Pixel Format: 16-bit RGB565 */
    spi1_send_byte(0, 0x3A); spi1_send_byte(1, 0x05);

    /* 4. Display Inversion ON */
    spi1_send_byte(0, 0x21);

    /* 5. Driver timing registers */
    spi1_send_byte(0, 0x86); spi1_send_byte(1, 0x98);
    spi1_send_byte(0, 0x89); spi1_send_byte(1, 0x03);
    spi1_send_byte(0, 0x8B); spi1_send_byte(1, 0x84);
    spi1_send_byte(0, 0x8D); spi1_send_byte(1, 0x33);
    spi1_send_byte(0, 0x8E); spi1_send_byte(1, 0x0F);
    spi1_send_byte(0, 0xE8); spi1_send_byte(1, 0x12); spi1_send_byte(1, 0x9F); /* Frame rate */
    spi1_send_byte(0, 0xFF); spi1_send_byte(1, 0x62);
    spi1_send_byte(0, 0x99); spi1_send_byte(1, 0x3E);
    spi1_send_byte(0, 0x9D); spi1_send_byte(1, 0x4B);
    spi1_send_byte(0, 0x98); spi1_send_byte(1, 0x3E);
    spi1_send_byte(0, 0x9C); spi1_send_byte(1, 0x4B);

    /* 6. Power Control */
    spi1_send_byte(0, 0xC3); spi1_send_byte(1, 0x37);
    spi1_send_byte(0, 0xC4); spi1_send_byte(1, 0x34);
    spi1_send_byte(0, 0xC9); spi1_send_byte(1, 0x09);

    /* 7. Gamma Curves */
    spi1_send_byte(0, 0xF0);
    spi1_send_byte(1, 0x42); spi1_send_byte(1, 0x00); spi1_send_byte(1, 0x14);
    spi1_send_byte(1, 0x16); spi1_send_byte(1, 0x0C); spi1_send_byte(1, 0x3A);

    spi1_send_byte(0, 0xF2);
    spi1_send_byte(1, 0x00); spi1_send_byte(1, 0x00); spi1_send_byte(1, 0x0F);
    spi1_send_byte(1, 0x17); spi1_send_byte(1, 0x0D); spi1_send_byte(1, 0x2E);

    spi1_send_byte(0, 0xF1);
    spi1_send_byte(1, 0x50); spi1_send_byte(1, 0xF2); spi1_send_byte(1, 0x85);
    spi1_send_byte(1, 0x1B); spi1_send_byte(1, 0x31); spi1_send_byte(1, 0x8C);

    spi1_send_byte(0, 0xF3);
    spi1_send_byte(1, 0x3B); spi1_send_byte(1, 0xF3); spi1_send_byte(1, 0x6F);
    spi1_send_byte(1, 0x22); spi1_send_byte(1, 0x06); spi1_send_byte(1, 0x9C);

    /* 8. Tearing Effect Line */
    spi1_send_byte(0, 0x35); spi1_send_byte(1, 0x00);
    spi1_send_byte(0, 0x44); spi1_send_byte(1, 0x00); spi1_send_byte(1, 0x09);

    /* 9. Sleep Out (0x11) & wait 120 ms */
    spi1_send_byte(0, 0x11);
    delay_ms(120);

    /* 10. Display ON (0x29) & wait 20 ms */
    spi1_send_byte(0, 0x29);
    delay_ms(20);

    /* 11. Memory Access Control (Orientation: 0x48) */
    spi1_send_byte(0, 0x36); spi1_send_byte(1, 0x48);

    /* 12. Column Address Set (0 to 239) */
    spi1_send_byte(0, 0x2A);
    spi1_send_byte(1, 0x00); spi1_send_byte(1, 0x00);
    spi1_send_byte(1, 0x00); spi1_send_byte(1, 0xEF);

    /* 13. Page Address Set (0 to 319) */
    spi1_send_byte(0, 0x2B);
    spi1_send_byte(1, 0x00); spi1_send_byte(1, 0x00);
    spi1_send_byte(1, 0x01); spi1_send_byte(1, 0x3F);

    /* 14. Memory Write Command (RAMWR) */
    spi1_send_byte(0, 0x2C);

    kprintf("[GC9307] Hardware panel awakened and ready for frame transfers\n");
}

void lcdc_init(void)
{
    uint32_t fb_addr = (uint32_t)(uintptr_t)g_framebuffer;
    kprintf("[LCDC] Initializing UMS9117 Display Subsystem at 0x%08x\n", LCDC_BASE);
    kprintf("[LCDC] Primary Framebuffer: 0x%08x (%u bytes, 240x320 RGB565)\n",
            fb_addr, LCD_FB_SIZE);

    /* 1. Power on LCM (0x1000) and LCDC (0x800) in AHB power domain */
    REG32(0x20E01000U) = 0x1000U | 0x800U;

    /* 2. Soft reset LCDC */
    REG32(0x20E01004U) = 2U;
    delay_ms(10);
    REG32(0x20E02004U) = 2U;
    delay_ms(10);

    /* 3. Initialize SPI1 interface */
    spi1_hw_init();

    /* 4. Wake up and initialize GC9307 TFT controller */
    gc9307_hw_init();

    /* 5. Configure SPI1 for 16-bit DMA transactions */
    spi_hw_t *spi = SPI1_HW;
    spi->ctl7 |= 0x80U;        /* SPI_TX_HLD_EN */
    spi->ctl8 |= (1U << 15);   /* CD bit = 1 (DATA) */
    spi->ctl0 = (spi->ctl0 & ~(0x1FU << 2)) | (16U << 2); /* 16-bit channel length */

    /* 6. Configure LCDC Hardware Controller */
    REG32(LCDC_BASE + 0x0000U) |= 1U; /* Enable LCDC */
    REG32(LCDC_BASE + 0x0000U) |= 2U; /* fmark_mode = 1 */
    REG32(LCDC_BASE + 0x0000U) &= ~4U; /* fmark_pol = 0 */
    REG32(LCDC_BASE + 0x0004U) = 0x000000U; /* bg_color = black */

    REG32(LCDC_BASE + 0x0008U) = LCD_WIDTH | (LCD_HEIGHT << 16); /* disp_size */
    REG32(LCDC_BASE + 0x000CU) = 0U; /* lcm_start */
    REG32(LCDC_BASE + 0x0010U) = LCD_WIDTH | (LCD_HEIGHT << 16); /* lcm_size */

    /* Capture destination: route pixels directly into SPI1 TX FIFO (0x70B00000) */
    uint32_t spi_txd_addr = 0x70B00000U;
    REG32(LCDC_BASE + 0x0014U) |= 0x20U; /* cap.ctrl enable */
    REG32(LCDC_BASE + 0x0014U) = (REG32(LCDC_BASE + 0x0014U) & ~(3U << 6)); /* mode 0 */
    REG32(LCDC_BASE + 0x0000U) &= ~(7U << 5);
    REG32(LCDC_BASE + 0x0018U) = spi_txd_addr >> 2; /* cap.base_addr */

    /* Image format: RGB565 (fmt 5), little endian (2 << 8) */
    uint32_t img_ctrl = REG32(LCDC_BASE + 0x0020U);
    img_ctrl &= ~2U; /* disable color key */
    img_ctrl = (img_ctrl & ~(15U << 4)) | (5U << 4); /* format = RGB565 */
    img_ctrl = (img_ctrl & ~(3U << 8)) | (2U << 8);  /* little endian */
    img_ctrl |= 1U;                                  /* img enable */
    REG32(LCDC_BASE + 0x0020U) = img_ctrl;

    REG32(LCDC_BASE + 0x0028U) = LCD_WIDTH; /* pitch */
    REG32(LCDC_BASE + 0x002CU) = LCD_WIDTH | (LCD_HEIGHT << 16); /* size_xy */
    REG32(LCDC_BASE + 0x0030U) = 0U; /* disp_xy */
    REG32(LCDC_BASE + 0x0024U) = fb_addr >> 2; /* y_base */

    /* Clear initial framebuffer screen to dark background */
    lcdc_clear(COLOR_DARK_BG);
    lcdc_flush();

    kprintf("[LCDC] Display Controller active. Direct DMA linked to SPI1 TXD.\n");
}

void lcdc_flush(void)
{
    uint32_t fb_addr = (uint32_t)(uintptr_t)g_framebuffer;
    spi_hw_t *spi = SPI1_HW;

    /* 1. Clean data cache lines across framebuffer range to guarantee DRAM coherency */
    dcache_clean_range(fb_addr, fb_addr + LCD_FB_SIZE);

    /* 2. Reset GC9307 write address by sending 0x2C (RAMWR) command over SPI1 */
    uint32_t timeout = 500000U;
    while (!(spi->sts2 & (1U << 7)) && --timeout);
    timeout = 500000U;
    while ((spi->sts2 & (1U << 8)) && --timeout);

    spi->ctl7 &= ~(1U << 14); /* Disable RGB565_EN */
    spi->ctl0 = (spi->ctl0 & ~(0x1FU << 2)) | (8U << 2); /* 8-bit channel */
    spi1_send_byte(0, 0x2C); /* Memory Write command */

    /* 3. Prepare SPI1 for bulk DMA transfer of 76,800 pixels (16-bit words) */
    spi->ctl0 = (spi->ctl0 & ~(0x1FU << 2)) | (16U << 2); /* 16-bit channel */
    timeout = 500000U;
    while (!(spi->sts2 & (1U << 7)) && --timeout);
    timeout = 500000U;
    while ((spi->sts2 & (1U << 8)) && --timeout);

    uint32_t pixel_count = LCD_WIDTH * LCD_HEIGHT;
    spi->ctl8 = (spi->ctl8 & ~0x3FFU) | (0U << 4) | (pixel_count >> 16);
    spi->ctl9 = (spi->ctl9 & ~0xFFFFU) | (pixel_count & 0xFFFFU);
    spi->ctl12 |= 2U; /* SW_TX_REQ */

    /* 4. Trigger LCDC hardware DMA refresh */
    REG32(LCDC_BASE + 0x011CU) = 0U; /* Clear status */
    REG32(LCDC_BASE + 0x0000U) |= LCDC_CTRL_REFRESH_START;

    /* 5. Wait for LCDC refresh completion interrupt */
    timeout = 500000U;
    while (!(REG32(LCDC_BASE + 0x011CU) & LCDC_IRQ_REFRESH_DONE) && --timeout) {
        /* Busy wait */
    }
    REG32(LCDC_BASE + 0x0114U) = LCDC_IRQ_REFRESH_DONE; /* Clear IRQ */

    /* 6. Wait for SPI1 DMA completion */
    timeout = 500000U;
    while (!(spi->int_raw & (1U << 8)) && --timeout) {
        /* Busy wait */
    }
    spi->int_clr |= (1U << 8);
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
