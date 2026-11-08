/*
 * OpenFeatureOS - Main Entry Point
 * Target: Unisoc T107 (UMS9117) / Nokia 2660 Flip (TA-1480)
 */

#include "t107_regs.h"
#include "printk.h"
#include "torch.h"

extern uint32_t __heap_start;
extern uint32_t __heap_end;

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
    kprintf(" Console Port    : UART1 (1.8V TTL @ 115200 8N1)\n");
    kprintf("==============================================================\n");
    kprintf("[BOOT] CPU Vectors, Stacks, BSS & CPU modes initialized OK\n");
    kprintf("[BOOT] Banked stacks allocated: 52 KB (FIQ, IRQ, ABT, UND, SYS, SVC)\n");
    kprintf("[BOOT] SC2720 PMIC Torch Active -> Starting Visual Heartbeat Beacon\n");

    /* Mark execution completed successfully in DRAM mailbox */
    REG32(T107_DRAM_MAILBOX + 0x0CU) = 0xDEADBEEFU;
    REG32(T107_DRAM_MAILBOX + 0x10U) = 0x00000001U; /* Status: Success */

    /* Visual Boot Confirmation: 3 rapid greeting flashes on rear torch */
    kprintf("[TORCH] Blinking 3x greeting pulses (on device)...\n");
    torch_blink(3, 80, 80, 0x0F);
    delay_ms(300);

    /* Continuous optical heartbeat loop */
    kprintf("[TORCH] Running continuous optical heartbeat loop...\n");
    uint32_t beat = 0;
    while (1) {
        kprintf("[BEAT] Heartbeat pulse #%u (Uptime tick)\n", ++beat);
        torch_heartbeat();
    }

    return 0;
}
