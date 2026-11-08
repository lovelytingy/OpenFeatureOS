/*
 * OpenFeatureOS - Board Support Package Initialization & RT-Thread Glue
 * Target: Unisoc T107 (UMS9117) / Nokia 2660 Flip (TA-1480)
 *
 * Sourced from:
 *   - RT-Thread Architecture Guidelines (bsp/qemu-vexpress-a9/drivers/board.c)
 *   - Nokia-2660-RE docs/kernel-architecture.md
 *   - Nokia-2660-Emulator crates/n2660-core/src/vm.rs
 */

#include "board.h"
#include "t107_regs.h"
#include "printk.h"
#include "watchdog.h"
#include "timer.h"
#include "intc.h"
#include "mmu.h"
#include "torch.h"
#include "lcdc.h"
#include "pinmap.h"
#include "gpio.h"
#include "backlight.h"

extern uint32_t __heap_start;
extern uint32_t __heap_end;

static uint32_t last_tick_ms = 0;

/*
 * Console character output hook called by rt_kprintf()
 */
void rt_hw_console_output(const char *str)
{
    while (*str) {
        if (*str == '\n') {
            kputc('\r');
        }
        kputc(*str++);
    }
}

/*
 * Interrupt Service Routine for System Timer Tick (SPRD_IRQ_TIMER #4)
 */
static void timer_tick_isr(uint32_t irq, void *data)
{
    (void)irq;
    (void)data;
    last_tick_ms = timer_get_ms();
    rt_tick_increase();
}

/*
 * RT-Thread Idle Thread Hook
 * Pets the hardware watchdog and services monotonic tick progression.
 */
static void idle_hook(void)
{
    /* 1. Pet the hardware PMIC watchdog to prevent SoC reset */
    watchdog_pet();

    /* 2. Check monotonic elapsed time on 1ms hardware counter */
    uint32_t now_ms = timer_get_ms();
    uint32_t delta_ms = now_ms - last_tick_ms;

    /* RT_TICK_PER_SECOND = 100 -> 1 tick = 10 ms */
    const uint32_t ms_per_tick = 1000U / RT_TICK_PER_SECOND;

    if (delta_ms >= ms_per_tick) {
        uint32_t ticks_to_add = delta_ms / ms_per_tick;
        last_tick_ms += ticks_to_add * ms_per_tick;

        /* Trigger software interrupt on timer line to advance RT-Thread ticks in ISR context */
        for (uint32_t i = 0; i < ticks_to_add; i++) {
            intc_trigger_soft_irq(SPRD_IRQ_TIMER);
        }
    }
}

/*
 * Board Hardware Initialization called from rtthread_startup()
 */
void rt_hw_board_init(void)
{
    kprintf("[BOARD] Initializing Unisoc T107 Board Support Package...\n");

    /* 1. Disable hardware PMIC watchdog during initial bringup */
    watchdog_init();

    /* 2. Initialize 1ms hardware system timer */
    timer_init();
    last_tick_ms = timer_get_ms();

    /* 3. Initialize SPRD 32-Line Interrupt Controller */
    intc_init();

    /* 4. Register timer tick handler on SPRD_IRQ_TIMER */
    intc_register_handler(SPRD_IRQ_TIMER, timer_tick_isr, 0);
    intc_enable_irq(SPRD_IRQ_TIMER);

    /* 5. Initialize ARMv7-A MMU & L1 Caches */
    mmu_init();

    /* 6. Initialize RT-Thread Kernel Dynamic Memory Heap */
    rt_system_heap_init((void *)&__heap_start, (void *)&__heap_end);
    kprintf("[BOARD] System Heap initialized: 0x%08x - 0x%08x (%u KB)\n",
            (uint32_t)(uintptr_t)&__heap_start,
            (uint32_t)(uintptr_t)&__heap_end,
            (uint32_t)((uintptr_t)&__heap_end - (uintptr_t)&__heap_start) / 1024U);

    /* 7. Initialize SoC Pin Multiplexing & Pad Controllers (SPI1, LCM, Keypad) */
    pinmap_init();

    /* 8. Initialize GPIOs and select Main (Inner 2.8" QVGA) Screen */
    gpio_init();

    /* 9. Power up SC2720 PMIC Backlight Controller and set brightness to 80% */
    backlight_init();
    backlight_set_brightness(80);

    /* 10. Initialize Display Pipeline (SPI1, GC9307 Panel, LCDC DMA Controller) */
    lcdc_init();

    /* 11. Run OpenFeatureOS v1.0 animated boot splash for exactly 5.0 seconds */
    lcdc_run_boot_splash(5000);

    /* 12. Register idle hook */
    rt_thread_idle_sethook(idle_hook);

    /* 13. Call RT-Thread auto-initialization table for board components */
#ifdef RT_USING_COMPONENTS_INIT
    rt_components_board_init();
#endif

    /* NOTE: CPU IRQs are unmasked when the first thread starts via rt_hw_context_switch_to() */
    kprintf("[BOARD] Board initialization completed successfully.\n");
}
