/*
 * OpenFeatureOS - Main Entry Point & RT-Thread Multithreading Demo
 * Target: Unisoc T107 (UMS9117) / Nokia 2660 Flip (TA-1480)
 *
 * Execution Model: Strictly Volatile DRAM (Zero NAND / eFuse writes).
 * Handed off by USB BSL/FDL1 at 0x80100000 (entry 0x80100200).
 */

#include <rtthread.h>
#include <rthw.h>
#include "t107_regs.h"
#include "printk.h"
#include "torch.h"
#include "watchdog.h"
#include "timer.h"
#include "intc.h"
#include "mmu.h"
#include "lcdc.h"
#include "ui_app.h"

extern uint32_t __heap_start;
extern uint32_t __heap_end;
extern uint32_t __mmu_table_start;

/*
 * Thread 1: Optical Heartbeat & Watchdog Sentinel
 * Runs as an independent RTOS thread with priority 15.
 */
static void torch_beacon_entry(void *parameter)
{
    (void)parameter;
    uint32_t beat_count = 0;

    rt_kprintf("[THREAD:torch] Optical heartbeat beacon task started (prio 15)\n");

    /* 3 greeting pulses to signal thread launch */
    for (int i = 0; i < 3; i++) {
        torch_set(1, 0x0F);
        rt_thread_mdelay(80);
        torch_set(0, 0);
        rt_thread_mdelay(80);
    }
    rt_thread_mdelay(300);

    while (1) {
        beat_count++;
        watchdog_pet();

        /* Double optical pulse: thump-thump */
        torch_set(1, 0x0F);
        rt_thread_mdelay(60);
        torch_set(0, 0);
        rt_thread_mdelay(120);
        torch_set(1, 0x0F);
        rt_thread_mdelay(80);
        torch_set(0, 0);

        rt_kprintf("[HEARTBEAT] Thread pulse #%u | OS Tick: %u | HW Timer: %u ms\n",
                   beat_count, (uint32_t)rt_tick_get(), timer_get_ms());

        /* Sleep for 1000 ms, yielding CPU to other tasks and idle thread */
        rt_thread_mdelay(1000);
    }
}

/*
 * Thread 2: System Telemetry & Status Reporter
 * Runs as a background telemetry task with priority 20.
 */
static void sys_reporter_entry(void *parameter)
{
    (void)parameter;
    uint32_t report_count = 0;

    rt_kprintf("[THREAD:reporter] Telemetry reporter task started (prio 20)\n");

    while (1) {
        /* Sleep for 3000 ms between periodic reports */
        rt_thread_mdelay(3000);
        report_count++;

        rt_kprintf("\n--- [System Telemetry Report #%u] ---\n", report_count);
        rt_kprintf("  Kernel OS      : RT-Thread %d.%d.%d (ARM Cortex-A7 UP)\n",
                   RT_VERSION_MAJOR, RT_VERSION_MINOR, RT_VERSION_PATCH);
        rt_kprintf("  Current Tick   : %u (at %u Hz)\n",
                   (uint32_t)rt_tick_get(), RT_TICK_PER_SECOND);
        rt_kprintf("  Hardware Uptime: %u ms\n", timer_get_ms());
        rt_kprintf("  System Status  : SCHEDULER HEALTHY, PREEMPTION ACTIVE\n");
        rt_kprintf("---------------------------------------\n\n");
    }
}

/*
 * Main Thread Entry Point (called automatically by RT-Thread application init)
 */
int main(void)
{
    /* 1. Optical Sign of Life: 3 flashes confirm RT-Thread OS booted & main thread active */
    torch_blink(3, 80, 80, 0x0F);

    /* 2. Run OpenFeatureOS Animated Boot Splash Screen for exactly 5.0 seconds (5000 ms) */
    lcdc_run_boot_splash(5000);

    rt_kprintf("\n");
    rt_kprintf("==============================================================\n");
    rt_kprintf("       ____                   ______            __            \n");
    rt_kprintf("      / __ \\____  ___  ____  / ____/__  ____ _/ /___  _______ \n");
    rt_kprintf("     / / / / __ \\/ _ \\/ __ \\/ /_  / _ \\/ __ `/ __/ / / / ___/ \n");
    rt_kprintf("    / /_/ / /_/ /  __/ / / / __/ /  __/ /_/ / /_/ /_/ / /     \n");
    rt_kprintf("    \\____/ .___/\\___/_/ /_/_/    \\___/\\__,_/\\__/\\__,_/_/      \n");
    rt_kprintf("        /_/                                                   \n");
    rt_kprintf("==============================================================\n");
    rt_kprintf(" Target Platform : Unisoc T107 (UMS9117) Cortex-A7 @ 1.0 GHz\n");
    rt_kprintf(" Target Device   : Nokia 2660 Flip (Model TA-1480, HW3.0)\n");
    rt_kprintf(" Kernel Core     : RT-Thread RTOS (libcpu Cortex-A7 UP port)\n");
    rt_kprintf(" Execution Model : Strictly Volatile DRAM (Zero NAND Writes)\n");
    rt_kprintf(" Base Load Addr  : 0x%08x | Entry: 0x%08x\n", (uint32_t)T107_PAYLOAD_LOAD_ADDR, (uint32_t)T107_PAYLOAD_ENTRY_ADDR);
    rt_kprintf(" DRAM Heap Range : 0x%08x - 0x%08x\n", (uint32_t)(uintptr_t)&__heap_start, (uint32_t)(uintptr_t)&__heap_end);
    rt_kprintf(" L1 MMU Table    : 0x%08x (16 KB section mapped)\n", (uint32_t)(uintptr_t)&__mmu_table_start);
    rt_kprintf(" Console Port    : UART1 (1.8V TTL @ 115200 8N1)\n");
    rt_kprintf("==============================================================\n");

    /* Mark successful RTOS boot stage in DRAM mailbox */
    REG32(T107_DRAM_MAILBOX + 0x0CU) = 0x52545430U; /* 'RTT0' */
    REG32(T107_DRAM_MAILBOX + 0x10U) = 0x00000001U; /* Status: Success */

    /* Spawn Thread 1: Optical Heartbeat & Watchdog Sentinel */
    rt_thread_t torch_thread = rt_thread_create("torch",
                                               torch_beacon_entry,
                                               RT_NULL,
                                               2048,
                                               15,
                                               10);
    if (torch_thread != RT_NULL) {
        rt_thread_startup(torch_thread);
        rt_kprintf("[MAIN] Created and started thread: torch\n");
    } else {
        rt_kprintf("[MAIN] ERROR: Failed to create thread: torch\n");
    }

    /* Spawn Thread 2: Periodic Telemetry Reporter */
    rt_thread_t reporter_thread = rt_thread_create("reporter",
                                                  sys_reporter_entry,
                                                  RT_NULL,
                                                  2048,
                                                  20,
                                                  10);
    if (reporter_thread != RT_NULL) {
        rt_thread_startup(reporter_thread);
        rt_kprintf("[MAIN] Created and started thread: reporter\n");
    } else {
        rt_kprintf("[MAIN] ERROR: Failed to create thread: reporter\n");
    }

    /* Spawn Thread 3: LVGL GUI Feature Phone UI Task */
    ui_app_init();

    rt_kprintf("[MAIN] All kernel threads spawned. Main thread entering supervisor loop.\n");

    while (1) {
        /* Main supervisor thread sleeps and yields */
        rt_thread_mdelay(10000);
    }

    return 0;
}
