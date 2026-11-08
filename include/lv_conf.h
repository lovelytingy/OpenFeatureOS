/**
 * @file lv_conf.h
 * Configuration file for LVGL v8.3 on OpenFeatureOS (Unisoc T107 / Nokia 2660 Flip)
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>
#include <rtthread.h>
#include "timer.h"

/*====================
   COLOR SETTINGS
 *====================*/

/* Color depth: 16 (RGB565) */
#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 0
#define LV_COLOR_SCREEN_TRANSP 0
#define LV_COLOR_CHROMA_KEY lv_color_hex(0x00ff00)

/*=========================
   MEMORY SETTINGS
 *=========================*/

/* Use RT-Thread dynamic heap allocator (31.6 MB available) */
#define LV_MEM_CUSTOM 1
#if LV_MEM_CUSTOM
    #define LV_MEM_CUSTOM_INCLUDE <rtthread.h>
    #define LV_MEM_CUSTOM_ALLOC   rt_malloc
    #define LV_MEM_CUSTOM_FREE    rt_free
    #define LV_MEM_CUSTOM_REALLOC rt_realloc
#endif

/*====================
   HAL SETTINGS
 *====================*/

/* Monotonic hardware system timer tick expression */
#define LV_TICK_CUSTOM 1
#if LV_TICK_CUSTOM
    #define LV_TICK_CUSTOM_INCLUDE "timer.h"
    #define LV_TICK_CUSTOM_SYS_TIME_EXPR (timer_get_ms())
#endif

/* Default display refresh period */
#define LV_DISP_DEF_REFR_PERIOD 30      /* [ms] (~33 FPS) */

/* Input device read period in milliseconds */
#define LV_INDEV_DEF_READ_PERIOD 30     /* [ms] */

/*=======================
 * FEATURE CONFIGURATION
 *=======================*/

#define LV_USE_ANIMATION        1
#define LV_USE_SHADOW           1
#define LV_USE_BLEND_MODES      1
#define LV_USE_OPA_SCALE        1
#define LV_USE_IMG_TRANSFORM    1
#define LV_USE_GROUP            1

/* Drawing engine */
#define LV_DRAW_COMPLEX         1
#if LV_DRAW_COMPLEX
    #define LV_SHADOW_CACHE_SIZE 0
    #define LV_CIRCLE_CACHE_SIZE 4
#endif

/* Logging */
#define LV_USE_LOG              0

/* Asserts */
#define LV_USE_ASSERT_NULL          1
#define LV_USE_ASSERT_MALLOC        1
#define LV_USE_ASSERT_STYLE         0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ           0

/* Built-in Fonts */
#define LV_FONT_MONTSERRAT_12   1
#define LV_FONT_MONTSERRAT_14   1
#define LV_FONT_MONTSERRAT_16   1
#define LV_FONT_MONTSERRAT_20   1
#define LV_FONT_MONTSERRAT_24   1
#define LV_FONT_DEFAULT         &lv_font_montserrat_14

/* Widgets */
#define LV_USE_ARC              1
#define LV_USE_BAR              1
#define LV_USE_BTN              1
#define LV_USE_BTNMATRIX        1
#define LV_USE_CANVAS           1
#define LV_USE_CHECKBOX         1
#define LV_USE_DROPDOWN         1
#define LV_USE_IMG              1
#define LV_USE_LABEL            1
#define LV_LABEL_TEXT_SELECTION 1
#define LV_LABEL_LONG_TXT_HINT  1
#define LV_USE_LINE             1
#define LV_USE_ROLLER           1
#define LV_USE_SLIDER           1
#define LV_USE_SWITCH           1
#define LV_USE_TEXTAREA         1
#define LV_USE_TABLE            1

/* Extra Widgets */
#define LV_USE_ANIMIMG          1
#define LV_USE_CALENDAR         1
#define LV_USE_CHART            1
#define LV_USE_COLORWHEEL       1
#define LV_USE_IMGBTN           1
#define LV_USE_KEYBOARD         1
#define LV_USE_LED              1
#define LV_USE_LIST             1
#define LV_USE_MENU             1
#define LV_USE_METER            1
#define LV_USE_MSGBOX           1
#define LV_USE_SPAN             1
#define LV_USE_SPINBOX          1
#define LV_USE_SPINNER          1
#define LV_USE_TABVIEW          1
#define LV_USE_TILEVIEW         1
#define LV_USE_WIN              1

/* Third-party library integrations */
#define LV_USE_FS_STDIO         0
#define LV_USE_FS_POSIX         0
#define LV_USE_FS_WIN32         0
#define LV_USE_FS_FATFS         0
#define LV_USE_FS_LITTLEFS      0
#define LV_USE_PNG              0
#define LV_USE_BMP              0
#define LV_USE_SJPG             0
#define LV_USE_GIF              0
#define LV_USE_QRCODE           0
#define LV_USE_FREETYPE         0
#define LV_USE_TINY_TTF         0
#define LV_USE_RLOTTIE          0
#define LV_USE_FFMPEG           0

/* Layouts */
#define LV_USE_FLEX             1
#define LV_USE_GRID             1

/* Performance Monitor */
#define LV_USE_PERF_MONITOR     0

#endif /* LV_CONF_H */
