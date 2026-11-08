/*
 * OpenFeatureOS - LVGL Display Driver Port for Unisoc T107 (UMS9117)
 * Target: Nokia 2660 Flip (TA-1480) 240x320 RGB565 GC9307 Display
 */

#include "lv_port_disp.h"
#include "lcdc.h"
#include "t107_regs.h"

#define DISP_BUF_LINES 40

static lv_disp_drv_t disp_drv;
static lv_disp_draw_buf_t disp_buf;
static lv_color_t buf_1[LCD_WIDTH * DISP_BUF_LINES];
static lv_color_t buf_2[LCD_WIDTH * DISP_BUF_LINES];

static void disp_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    int32_t x1 = area->x1;
    int32_t y1 = area->y1;
    int32_t x2 = area->x2;
    int32_t y2 = area->y2;

    for (int32_t y = y1; y <= y2; y++) {
        uint32_t row_idx = y * LCD_WIDTH;
        for (int32_t x = x1; x <= x2; x++) {
            g_framebuffer[row_idx + x] = color_p->full;
            color_p++;
        }
    }

    /* Flush primary hardware framebuffer to GC9307 display panel */
    lcdc_flush();

    /* Inform LVGL that flushing is complete */
    lv_disp_flush_ready(drv);
}

void lv_port_disp_init(void)
{
    lv_disp_draw_buf_init(&disp_buf, buf_1, buf_2, LCD_WIDTH * DISP_BUF_LINES);

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LCD_WIDTH;
    disp_drv.ver_res = LCD_HEIGHT;
    disp_drv.flush_cb = disp_flush_cb;
    disp_drv.draw_buf = &disp_buf;

    lv_disp_drv_register(&disp_drv);
}
