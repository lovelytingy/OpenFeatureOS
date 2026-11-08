/*
 * OpenFeatureOS - Unisoc T107 (UMS9117) LCD Controller Driver
 * Target: Nokia 2660 Flip (TA-1480 / HW3.0) 240x320 QVGA RGB565 Display
 *
 * Drives the GalaxyCore GC9307 TFT LCD over hardware SPI1 (0x70B00000)
 * and Unisoc UMS9117 Hardware LCD Controller (LCDC 0x20800000).
 *
 * Sourced directly from verified hardware definitions:
 *   - Nokia-2660-RE tools/ram_payload/vendor/fpdoom/ums9117/syscode.c
 *   - Nokia-2660-RE tools/ram_payload/vendor/fpdoom/ums9117/lcd_spi.h
 *   - Nokia-2660-RE tools/ram_payload/vendor/fpdoom/ums9117/lcd_config.h
 *   - Nokia-2660-RE tools/ram_payload/vendor/fpdoom/fpdoom/syscode.h (lcdc_base_t)
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
 * Unisoc UMS9117 Hardware LCD Controller (LCDC) Register Structure (0x20800000)
 * Sourced from fpdoom/syscode.h:34-56 (Exact hardware byte layout verified)
 */
typedef volatile struct {
    uint32_t ctrl;           /* 0x00: Control: bit 0=enable, bit 1=fmark_mode, bit 3=start_refresh */
    uint32_t disp_size;      /* 0x04: Display dimensions: width | (height << 16) */
    uint32_t lcm_start;      /* 0x08: LCM window start position */
    uint32_t lcm_size;       /* 0x0C: LCM window size */
    uint32_t bg_color;       /* 0x10: Background color */
    uint32_t fifo_status;    /* 0x14: FIFO status (RO) */
    uint32_t sync_delay;     /* 0x18: Sync delay */
    uint32_t dummy_1c[1];    /* 0x1C: Reserved */
    struct {
        uint32_t ctrl;          /* 0x20: Image layer ctrl: fmt, endian, enable */
        uint32_t y_base_addr;   /* 0x24: Framebuffer address in words (addr >> 2) */
        uint32_t uv_base_addr;  /* 0x28: UV plane address (for YUV video) */
        uint32_t size_xy;       /* 0x2C: Image layer size: width | (height << 16) */
        uint32_t pitch;         /* 0x30: Framebuffer row pitch (in pixels) */
        uint32_t disp_xy;       /* 0x34: Display offset: x | (y << 16) */
        uint32_t dummy[6];      /* 0x38 .. 0x4C */
    } img;
    struct {
        uint32_t ctrl;
        uint32_t base_addr;
        uint32_t alpha_base_addr;
        uint32_t size_xy;
        uint32_t pitch;
        uint32_t disp_xy;
        uint32_t alpha;
        uint32_t grey_rgb;
        uint32_t ck;
        uint32_t dummy[3];
    } ocd1, ocd2, ocd3;       /* 0x50, 0x80, 0xB0 */
    struct {
        uint32_t ctrl;          /* 0xE0: Capture ctrl: bit 5=enable, bits 7:6=mode */
        uint32_t base_addr;     /* 0xE4: Destination FIFO address in words (addr >> 2) */
        uint32_t start_xy;      /* 0xE8 */
        uint32_t size_xy;       /* 0xEC */
        uint32_t pitch;         /* 0xF0 */
        uint32_t dummy[3];      /* 0xF4 .. 0xFC */
    } cap;
    struct {
        uint32_t ctrl;
        uint32_t contrast;
        uint32_t saturation;
        uint32_t brightness;
    } y2r;                   /* 0x100 */
    struct {
        uint32_t en;            /* 0x110: IRQ enable */
        uint32_t clr;           /* 0x114: IRQ clear */
        uint32_t status;        /* 0x118: Masked IRQ status */
        uint32_t raw;           /* 0x11C: Raw IRQ status: bit 0 = refresh_done */
    } irq;
} lcdc_hw_t;

#define LCDC_DEV ((lcdc_hw_t *)0x20800000U)
#define LCM_REGS ((volatile uint32_t *)0x20A00000U)

/*
 * Unisoc UMS9117 SPI1 Controller Hardware Registers (0x70B00000)
 * Sourced from fpdoom/ums9117/lcd_spi.h:5-14
 */
typedef volatile struct {
    uint32_t txd;            /* 0x00: Transmit data FIFO */
    uint32_t clkd;           /* 0x04: Clock divider */
    uint32_t ctl0;           /* 0x08: Control 0: CS, channel length, mode */
    uint32_t ctl1;           /* 0x0C: Control 1: RTX mode */
    uint32_t ctl2;           /* 0x10: Control 2 */
    uint32_t ctl3;           /* 0x14 */
    uint32_t ctl4;           /* 0x18 */
    uint32_t ctl5;           /* 0x1C */
    uint32_t int_en;         /* 0x20: Interrupt enable */
    uint32_t int_clr;        /* 0x24: Interrupt clear */
    uint32_t int_raw;        /* 0x28: Interrupt raw status (bit 8 = TX_END_IRQ) */
    uint32_t int_mask;       /* 0x2C */
    uint32_t sts1;           /* 0x30 */
    uint32_t sts2;           /* 0x34: Status 2: bit 7 = TXF_REAL_EMPTY, bit 8 = BUSY */
    uint32_t dspwait;        /* 0x38 */
    uint32_t sts3;           /* 0x3C */
    uint32_t ctl6;           /* 0x40 */
    uint32_t sts4;           /* 0x44 */
    uint32_t fifo_rst;       /* 0x48: FIFO reset */
    uint32_t ctl7;           /* 0x4C: Control 7: bit 3..5=mode, bit 7=tx_hld, bit 14=rgb565 */
    uint32_t sts5;           /* 0x50 */
    uint32_t ctl8;           /* 0x54: Control 8: bit 15 = CD (0=cmd, 1=data), tx_len hi */
    uint32_t ctl9;           /* 0x58: Control 9: tx_len lo */
    uint32_t ctl10;          /* 0x5C */
    uint32_t ctl11;          /* 0x60 */
    uint32_t ctl12;          /* 0x64: Control 12: bit 1 = SW_TX_REQ */
    uint32_t sts6;           /* 0x68 */
    uint32_t sts7;           /* 0x6C */
    uint32_t sts8;           /* 0x70 */
    uint32_t sts9;           /* 0x74 */
} spi_hw_t;

#define SPI1_DEV ((spi_hw_t *)0x70B00000U)

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

static inline void spi_set_clkd(spi_hw_t *spi, int val) {
    spi->clkd = val;
}

static inline void spi_set_mode(spi_hw_t *spi, int val) {
    spi->ctl7 = (spi->ctl7 & ~(7U << 3)) | ((uint32_t)val << 3);
}

static inline void spi_set_chnl_len(spi_hw_t *spi, int val) {
    val &= 0x1F;
    spi->ctl0 = (spi->ctl0 & ~(0x1FU << 2)) | ((uint32_t)val << 2);
}

static inline void spi_set_s3w_rd_strt(spi_hw_t *spi, int val) {
    val &= 0x1F;
    spi->ctl2 = (spi->ctl2 & ~0x1FU) | (uint32_t)val;
}

static inline void spi_set_rtx_mode(spi_hw_t *spi, int mode) {
    spi->ctl1 |= (uint32_t)mode << 12;
}

static inline void spi_set_cd_bit(spi_hw_t *spi, int val) {
    /* 0 = cmd, 1 = data */
    spi->ctl8 = (spi->ctl8 & ~(1U << 15)) | ((uint32_t)val << 15);
}

static inline void spi_select_cs(spi_hw_t *spi, unsigned cs, unsigned state) {
    uint32_t bit = 1U << (cs + 8);
    spi->ctl0 = (spi->ctl0 | bit) & ~(state ? bit : 0);
}

static inline void spi_set_tx_len(spi_hw_t *spi, int data_len, int dummy_len) {
    uint32_t a = spi->ctl8 & ~0x3FFU;
    uint32_t b = spi->ctl9 & ~0xFFFFU;
    a |= ((uint32_t)dummy_len << 4) | ((uint32_t)data_len >> 16);
    spi->ctl8 = a;
    spi->ctl9 = b | ((uint32_t)data_len & 0xFFFFU);
}

static void spi_wait_tx(spi_hw_t *spi) {
    uint32_t timeout = 500000U;
    while (!(spi->int_raw & (1U << 8)) && --timeout);
    spi->int_clr |= (1U << 8);

    timeout = 500000U;
    while ((spi->sts2 & (1U << 8)) && --timeout);
    timeout = 500000U;
    while (!(spi->sts2 & (1U << 7)) && --timeout);
}

static void spi1_send(unsigned idx, unsigned data) {
    spi_hw_t *spi = SPI1_DEV;
    if (!idx) spi_set_cd_bit(spi, 0);
    spi_set_tx_len(spi, 1, 0);
    spi->ctl12 |= 2U; /* SW_TX_REQ */
    spi->txd = data;
    spi_wait_tx(spi);
    if (!idx) spi_set_cd_bit(spi, 1);
}

static void spi_refresh_init(spi_hw_t *spi) {
    spi->ctl7 |= 0x80U; /* SPI_TX_HLD_EN */
    spi_set_cd_bit(spi, 1);
    spi_set_chnl_len(spi, 16);
}

static void spi_refresh_next(spi_hw_t *spi) {
    unsigned len = LCD_WIDTH * LCD_HEIGHT;
    uint32_t timeout = 500000U;
    while (!(spi->sts2 & (1U << 7)) && --timeout);
    timeout = 500000U;
    while ((spi->sts2 & (1U << 8)) && --timeout);
    spi_set_tx_len(spi, len, 0);
    spi->ctl12 |= 2U;
}

static void lcm_reset(uint32_t delay_val) {
    uint32_t addr = 0x402E160CU;
    int method = 0;
    for (int i = 0; i < 3; i++) {
        REG32(addr + ((uint32_t)method << 12)) = 1U;
        method ^= 1;
        delay_ms(delay_val);
    }
}

static void spi1_hw_init(void)
{
    /* 1. Ungate SPI1 APB clock */
    REG32(0x402E1134U) = 0x200U;

    /* 2. Select 192 MHz source clock */
    REG32(0x21500054U) = 3U;
    delay_cycles(100);

    /* 3. Soft reset SPI1 */
    REG32(0x71301004U) = 0x40U;
    delay_cycles(1000);
    REG32(0x71302004U) = 0x40U;
    delay_cycles(1000);

    /* 4. Configure SPI1 base registers */
    spi_hw_t *spi = SPI1_DEV;
    spi->int_en = 0;
    spi->ctl0 = 0xF00U | 2U | (8U << 2);
    spi->ctl1 &= ~0x3000U;
    spi->ctl4 = 0x8000U;
    spi->ctl5 = 0;

    /* clkd: 192 MHz / (48 MHz * 2) - 1 = 1 */
    spi_set_clkd(spi, 1);
    /* SPI mode 3 */
    spi_set_mode(spi, 3);
    /* 8-bit channel length */
    spi_set_chnl_len(spi, 8);
    spi_set_s3w_rd_strt(spi, 7);
    spi_set_rtx_mode(spi, 3);
    spi_set_cd_bit(spi, 0);
    spi_select_cs(spi, 0, 1);

    kprintf("[SPI1] Controller configured at 48 MHz on base 0x70B00000\n");
}

static void gc9307_hw_init(void)
{
    /* 1. Reset LCM hardware lines */
    lcm_reset(10);
    delay_ms(20);

    /* 2. Inter Register Enable */
    spi1_send(0, 0xFE);
    spi1_send(0, 0xEF);

    /* 3. Pixel Format: 16-bit RGB565 */
    spi1_send(0, 0x3A); spi1_send(1, 0x05);

    /* 4. Display Inversion ON */
    spi1_send(0, 0x21);

    /* 5. Driver timing registers */
    spi1_send(0, 0x86); spi1_send(1, 0x98);
    spi1_send(0, 0x89); spi1_send(1, 0x03);
    spi1_send(0, 0x8B); spi1_send(1, 0x84);
    spi1_send(0, 0x8D); spi1_send(1, 0x33);
    spi1_send(0, 0x8E); spi1_send(1, 0x0F);
    spi1_send(0, 0xE8); spi1_send(1, 0x12); spi1_send(1, 0x9F); /* Frame rate */
    spi1_send(0, 0xFF); spi1_send(1, 0x62);
    spi1_send(0, 0x99); spi1_send(1, 0x3E);
    spi1_send(0, 0x9D); spi1_send(1, 0x4B);
    spi1_send(0, 0x98); spi1_send(1, 0x3E);
    spi1_send(0, 0x9C); spi1_send(1, 0x4B);

    /* 6. Power Control */
    spi1_send(0, 0xC3); spi1_send(1, 0x37);
    spi1_send(0, 0xC4); spi1_send(1, 0x34);
    spi1_send(0, 0xC9); spi1_send(1, 0x09);

    /* 7. Gamma Curves */
    spi1_send(0, 0xF0);
    spi1_send(1, 0x42); spi1_send(1, 0x00); spi1_send(1, 0x14);
    spi1_send(1, 0x16); spi1_send(1, 0x0C); spi1_send(1, 0x3A);

    spi1_send(0, 0xF2);
    spi1_send(1, 0x00); spi1_send(1, 0x00); spi1_send(1, 0x0F);
    spi1_send(1, 0x17); spi1_send(1, 0x0D); spi1_send(1, 0x2E);

    spi1_send(0, 0xF1);
    spi1_send(1, 0x50); spi1_send(1, 0xF2); spi1_send(1, 0x85);
    spi1_send(1, 0x1B); spi1_send(1, 0x31); spi1_send(1, 0x8C);

    spi1_send(0, 0xF3);
    spi1_send(1, 0x3B); spi1_send(1, 0xF3); spi1_send(1, 0x6F);
    spi1_send(1, 0x22); spi1_send(1, 0x06); spi1_send(1, 0x9C);

    /* 8. Tearing Effect Line */
    spi1_send(0, 0x35); spi1_send(1, 0x00);
    spi1_send(0, 0x44); spi1_send(1, 0x00); spi1_send(1, 0x09);

    /* 9. Sleep Out (0x11) & wait 120 ms */
    spi1_send(0, 0x11);
    delay_ms(120);

    /* 10. Display ON (0x29) & wait 20 ms */
    spi1_send(0, 0x29);
    delay_ms(20);

    /* 11. Memory Access Control (Orientation: 0x48) */
    spi1_send(0, 0x36); spi1_send(1, 0x48);

    /* 12. Column Address Set (0 to 239) */
    spi1_send(0, 0x2A);
    spi1_send(1, 0x00); spi1_send(1, 0x00);
    spi1_send(1, 0x00); spi1_send(1, 0xEF);

    /* 13. Page Address Set (0 to 319) */
    spi1_send(0, 0x2B);
    spi1_send(1, 0x00); spi1_send(1, 0x00);
    spi1_send(1, 0x01); spi1_send(1, 0x3F);

    /* 14. Initial Memory Write Command (RAMWR) */
    spi1_send(0, 0x2C);

    kprintf("[GC9307] Hardware panel awakened and configured (240x320 QVGA)\n");
}

void lcdc_init(void)
{
    uint32_t fb_addr = (uint32_t)(uintptr_t)g_framebuffer;
    kprintf("[LCDC] Initializing UMS9117 Display Subsystem at 0x%08x\n", 0x20800000U);
    kprintf("[LCDC] Primary Framebuffer: 0x%08x (%u bytes, 240x320 RGB565)\n",
            fb_addr, LCD_FB_SIZE);

    /* 1. Power on LCM (0x1000) and LCDC (0x800) in AHB power domain */
    REG32(0x20E01000U) = 0x1000U | 0x800U;

    /* 2. Configure LCM hardware module */
    LCM_REGS[0] = 0;
    LCM_REGS[4] = 1;         /* 0x20A00010 */
    LCM_REGS[5] = 0xA50100U; /* 0x20A00014 */

    /* 3. Soft reset LCDC */
    REG32(0x20E01004U) = 2U;
    delay_ms(10);
    REG32(0x20E02004U) = 2U;
    delay_ms(10);

    /* 4. Initialize SPI1 interface */
    spi1_hw_init();

    /* 5. Wake up and initialize GC9307 TFT controller */
    gc9307_hw_init();

    /* 6. Configure LCDC Hardware Controller */
    lcdc_hw_t *lcdc = LCDC_DEV;
    spi_hw_t *spi = SPI1_DEV;

    lcdc->ctrl |= 1U;  /* lcdc_enable = 1 */
    lcdc->ctrl |= 2U;  /* fmark_mode = 1 */
    lcdc->ctrl &= ~4U; /* fmark_pol = 0 */
    lcdc->bg_color = 0x000000U;

    lcdc->disp_size = LCD_WIDTH | (LCD_HEIGHT << 16);
    lcdc->lcm_start = 0;
    lcdc->lcm_size = LCD_WIDTH | (LCD_HEIGHT << 16);

    /* Configure Capture Unit: route pixels directly into SPI1 TX FIFO (0x70B00000) */
    spi_refresh_init(spi);
    uint32_t spi_txd_addr = 0x70B00000U;
    lcdc->cap.ctrl |= 0x20U; /* cap enable */
    lcdc->cap.ctrl = (lcdc->cap.ctrl & ~(3U << 6)) | (0U << 6); /* mode 0 */
    lcdc->ctrl &= ~(7U << 5);
    lcdc->cap.base_addr = spi_txd_addr >> 2;

    /* Configure Image Layer (format = RGB565, little endian, 240x320) */
    uint32_t a = lcdc->img.ctrl;
    a &= ~2U; /* disable color key */
    a = (a & ~(15U << 4)) | (5U << 4); /* fmt 5 = RGB565 */
    a = (a & ~(3U << 8)) | (2U << 8);  /* 2 << 8 = little endian */
    lcdc->img.ctrl = a;

    lcdc->img.pitch = LCD_WIDTH;
    lcdc->img.size_xy = LCD_WIDTH | (LCD_HEIGHT << 16);
    lcdc->img.disp_xy = 0;
    lcdc->img.y_base_addr = fb_addr >> 2;
    lcdc->img.ctrl |= 1U; /* enable image layer */

    /* Prime display with initial frame */
    lcdc_clear(COLOR_DARK_BG);
    lcdc_flush();

    kprintf("[LCDC] Display Controller active. DMA linked to SPI1 TXD (0x70B00000)\n");
}

void lcdc_flush(void)
{
    uint32_t fb_addr = (uint32_t)(uintptr_t)g_framebuffer;
    spi_hw_t *spi = SPI1_DEV;
    lcdc_hw_t *lcdc = LCDC_DEV;

    /* 1. Reset GC9307 page write address to 0x2C (RAMWR) */
    uint32_t timeout = 500000U;
    while (!(spi->sts2 & (1U << 7)) && --timeout);
    timeout = 500000U;
    while ((spi->sts2 & (1U << 8)) && --timeout);

    spi->ctl7 &= ~(1U << 14); /* RGB565_EN */
    spi_set_chnl_len(spi, 8);
    spi1_send(0, 0x2C);
    spi_set_chnl_len(spi, 16);

    /* 2. Flush data cache so DMA sees updated framebuffer in DRAM */
    dcache_clean_range(fb_addr, fb_addr + LCD_FB_SIZE);

    /* 3. Prepare SPI1 for bulk DMA transfer of 76,800 pixels */
    spi_refresh_next(spi);

    /* 4. Trigger LCDC refresh */
    lcdc->irq.en |= 1U;
    lcdc->ctrl |= 8U; /* Start refresh */

    /* 5. Wait for LCDC refresh completion */
    timeout = 500000U;
    while (!(lcdc->irq.raw & 1U) && --timeout);
    lcdc->irq.clr |= 1U;

    /* 6. Wait for SPI1 TX FIFO empty and not busy */
    timeout = 500000U;
    while (!(spi->sts2 & (1U << 7)) && --timeout);
    timeout = 500000U;
    while ((spi->sts2 & (1U << 8)) && --timeout);
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
