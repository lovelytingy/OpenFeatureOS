/*
 * OpenFeatureOS - Main Entry Point
 * Target: Unisoc T107 (UMS9117) / Nokia 2660 Flip (TA-1480)
 */

#include "t107_regs.h"
#include "printk.h"
#include "torch.h"
#include "watchdog.h"
#include "timer.h"
#include "intc.h"
#include "mmu.h"

extern uint32_t __heap_start;
extern uint32_t __heap_end;
extern uint32_t __mmu_table_start;

static volatile uint32_t test_irq_count = 0;

static void test_irq_callback(uint32_t irq, void *data)
{
    (void)data;
    test_irq_count++;
    kprintf("[INTC] >> Successfully dispatched IRQ line #%u in interrupt context (count=%u)\n",
            irq, test_irq_count);
}

int main(void)
{
    kprintf("\n");
    kprintf("==============================================================\n");
    kprintf("       ____                   ______            __            \n");
    kprintf("      / __ \\____  ___  ____  / ____/__  ____ _/ /___  _______ \n");
    kprintf("     / / / / __ \\/ _ \\/ __ \\/ /_  / _ \\/ __ `/ __/ / / / ___/ \n");
    kprintf("    / /_/ / /_/ /  __/ / / / __/ /  __/ /_/ / /_/ /_/ / /     \n");
    kprintf("    \\____/ .___/\\___/_/ /_/_/    \\___/\\__,_/\\__/\\__,_/_/      \n");
    kprintf("        /_/                                                   \n");
    kprintf("==============================================================\n");
    kprintf(" Target Platform : Unisoc T107 (UMS9117) Cortex-A7 @ 1.0 GHz\n");
    kprintf(" Target Device   : Nokia 2660 Flip (Model TA-1480, HW3.0)\n");
    kprintf(" Execution Model : Strictly Volatile DRAM (Zero NAND Writes)\n");
    kprintf(" Base Load Addr  : 0x%08x | Entry: 0x%08x\n", (uint32_t)T107_PAYLOAD_LOAD_ADDR, (uint32_t)T107_PAYLOAD_ENTRY_ADDR);
    kprintf(" DRAM Heap Range : 0x%08x - 0x%08x\n", (uint32_t)(uintptr_t)&__heap_start, (uint32_t)(uintptr_t)&__heap_end);
    kprintf(" L1 MMU Table    : 0x%08x (16 KB aligned)\n", (uint32_t)(uintptr_t)&__mmu_table_start);
    kprintf(" Console Port    : UART1 (1.8V TTL @ 115200 8N1)\n");
    kprintf("==============================================================\n");
    kprintf("[BOOT] CPU Vectors, Stacks, BSS & CPU modes initialized OK\n");
    kprintf("[BOOT] Banked stacks allocated: 52 KB (FIQ, IRQ, ABT, UND, SYS, SVC)\n");

    /* Mark execution stage in DRAM mailbox */
    REG32(T107_DRAM_MAILBOX + 0x0CU) = 0xDEADBEEFU;
    REG32(T107_DRAM_MAILBOX + 0x10U) = 0x00000001U; /* Status: Success */

    /* 1. Watchdog Subsystem: Disable PMIC watchdog for safe uninterrupted execution */
    watchdog_init();

    /* 2. System Timer: Free-running 1ms hardware counter */
    timer_init();
    kprintf("[TIMER] System free-running uptime: %u ms\n", timer_get_ms());

    /* 3. Interrupt Controller (INTC): Initialize SPRD INTC and test IRQ dispatch */
    intc_init();
    intc_register_handler(SPRD_IRQ_TIMER, test_irq_callback, 0);
    intc_enable_irq(SPRD_IRQ_TIMER);
    interrupts_enable();

    kprintf("[INTC] Testing software interrupt trigger on line #%u...\n", SPRD_IRQ_TIMER);
    intc_trigger_soft_irq(SPRD_IRQ_TIMER);

    if (test_irq_count > 0) {
        kprintf("[INTC] Hardware IRQ dispatch loop VERIFIED (total: %u events)\n", test_irq_count);
    } else {
        kprintf("[INTC] Interrupt enabled on line #%u, awaiting asynchronous assertion\n", SPRD_IRQ_TIMER);
    }

    /* 4. ARMv7 MMU & Caches: Identity map DRAM & MMIO, enable L1 D-Cache & I-Cache */
    mmu_init();

    /* 5. Visual Optical Heartbeat Confirmation on SC2720 PMIC Torch */
    kprintf("[TORCH] Blinking 3x greeting pulses (on device)...\n");
    torch_blink(3, 80, 80, 0x0F);
    delay_ms(300);

    /* Continuous optical heartbeat loop */
    kprintf("[TORCH] Running continuous optical heartbeat loop...\n");
    uint32_t beat = 0;
    while (1) {
        kprintf("[BEAT] Heartbeat pulse #%u | Uptime: %u ms\n", ++beat, timer_get_ms());
        watchdog_pet();
        torch_heartbeat();
    }

    return 0;
}
