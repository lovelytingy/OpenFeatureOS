/*
 * OpenFeatureOS - Main Entry Point
 * Target: Unisoc T107 (UMS9117) / Nokia 2660 Flip (TA-1480)
 */

#include "t107_regs.h"

extern void early_uart_puts(const char *s);

int main(void)
{
    early_uart_puts("======================================================\n");
    early_uart_puts(" OpenFeatureOS - Unisoc T107 (ARM Cortex-A7)\n");
    early_uart_puts(" Target: Nokia 2660 Flip (TA-1480)\n");
    early_uart_puts(" Volatile DRAM Execution @ 0x80100000 (Safe Mode)\n");
    early_uart_puts("======================================================\n");
    early_uart_puts("[BOOT] Vectors, Stacks, BSS & CPU modes initialized OK\n");
    early_uart_puts("[BOOT] Hardware registers linked via board/t107_regs.h\n");
    early_uart_puts("[BOOT] Kernel handoff successful!\n");

    /* Mark execution completed successfully in DRAM mailbox */
    REG32(T107_DRAM_MAILBOX + 0x0CU) = 0xDEADBEEFU;
    REG32(T107_DRAM_MAILBOX + 0x10U) = 0x00000001U; /* Status: Success */

    while (1) {
        /* Low-power wait for interrupt */
        __asm__ volatile ("wfi");
    }

    return 0;
}
