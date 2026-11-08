/*
 * OpenFeatureOS - Unisoc T107 (UMS9117) GPIO Controller
 * Target: Nokia 2660 Flip (TA-1480 / HW3.0)
 *
 * Sourced from:
 *   - Nokia-2660-RE tools/ram_payload/vendor/fpdoom/ums9117/syscode.c:53-101
 *   - Nokia-2660-RE tools/ram_payload/run_hello_world.sh:32
 */

#include "gpio.h"
#include "t107_regs.h"
#include "printk.h"

int gpio_set(unsigned int id, unsigned int off, int state)
{
    if (id >= 0x80U) return -1;
    uint32_t bank = id >> 4;
    uint32_t shl  = id & 0xFU;
    uint32_t addr = GPIO_BASE + (bank << 7) + off;
    uint32_t tmp  = REG32(addr);

    if (state < 0) {
        return (tmp >> shl) & 1U;
    }
    tmp &= ~(1U << shl);
    tmp |= ((uint32_t)state & 1U) << shl;
    REG32(addr) = tmp;
    return 0;
}

void gpio_init(void)
{
    /* 1. Ungate GPIO APB clock */
    REG32(0x402E1000U) |= 8U;

    /* 2. Configure baseline GPIOs from phone firmware table */
    static const struct {
        int16_t id;
        int16_t val;
        uint32_t dir;
        uint32_t mode;
    } s_gpios[] = {
        { 101, 1, 0, 5 }, /* 0x65 */
        { 90,  0, 0, 5 }, /* 0x5A */
        { 95,  1, 0, 5 }, /* 0x5F */
        { 94,  1, 0, 5 }, /* 0x5E */
        { 40,  1, 0, 5 }, /* 0x28 */
        { 45,  0, 1, 0 }, /* 0x2D */
        { 114, 0, 0, 5 }, /* 0x72 */
        { -1,  0, 0, 0 }
    };

    for (int i = 0; s_gpios[i].id != -1; i++) {
        unsigned int id = (unsigned int)s_gpios[i].id;
        gpio_set(id, GPIO_OFF_DMSK, 1);
        if (!s_gpios[i].dir) {
            gpio_set(id, GPIO_OFF_DIR, 1);  /* Output */
            gpio_set(id, GPIO_OFF_INEN, 0); /* Input buffer off */
            int val = s_gpios[i].val != -1 && (s_gpios[i].val & 0xFF);
            gpio_set(id, GPIO_OFF_DATA, val);
        } else {
            gpio_set(id, GPIO_OFF_DIR, 0);  /* Input */
            gpio_set(id, GPIO_OFF_INEN, 1); /* Input buffer on */
        }
    }

    /* 3. Explicitly select Main (inner 2.8" QVGA 240x320 GC9307) Screen */
    gpio_select_screen(1);
    kprintf("[GPIO] Initialized. Inner Main Screen selected (GPIO40=0, GPIO95=1)\n");
}

void gpio_select_screen(int main_screen)
{
    if (main_screen) {
        /* Main screen: GPIO 40 = 0, GPIO 95 = 1 */
        gpio_set(GPIO_PIN_SCREEN_SEL1, GPIO_OFF_DMSK, 1);
        gpio_set(GPIO_PIN_SCREEN_SEL1, GPIO_OFF_DIR, 1);
        gpio_set(GPIO_PIN_SCREEN_SEL1, GPIO_OFF_INEN, 0);
        gpio_set(GPIO_PIN_SCREEN_SEL1, GPIO_OFF_DATA, 0);

        gpio_set(GPIO_PIN_SCREEN_SEL2, GPIO_OFF_DMSK, 1);
        gpio_set(GPIO_PIN_SCREEN_SEL2, GPIO_OFF_DIR, 1);
        gpio_set(GPIO_PIN_SCREEN_SEL2, GPIO_OFF_INEN, 0);
        gpio_set(GPIO_PIN_SCREEN_SEL2, GPIO_OFF_DATA, 1);
    } else {
        /* Cover screen: GPIO 40 = 1, GPIO 95 = 0 */
        gpio_set(GPIO_PIN_SCREEN_SEL1, GPIO_OFF_DMSK, 1);
        gpio_set(GPIO_PIN_SCREEN_SEL1, GPIO_OFF_DIR, 1);
        gpio_set(GPIO_PIN_SCREEN_SEL1, GPIO_OFF_INEN, 0);
        gpio_set(GPIO_PIN_SCREEN_SEL1, GPIO_OFF_DATA, 1);

        gpio_set(GPIO_PIN_SCREEN_SEL2, GPIO_OFF_DMSK, 1);
        gpio_set(GPIO_PIN_SCREEN_SEL2, GPIO_OFF_DIR, 1);
        gpio_set(GPIO_PIN_SCREEN_SEL2, GPIO_OFF_INEN, 0);
        gpio_set(GPIO_PIN_SCREEN_SEL2, GPIO_OFF_DATA, 0);
    }
}
