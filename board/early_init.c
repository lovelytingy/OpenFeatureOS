/*
 * OpenFeatureOS - Early Board Hardware Bring-up
 * Target: Unisoc T107 (UMS9117) / Nokia 2660 Flip (TA-1480)
 */

#include "t107_regs.h"
#include "printk.h"
#include "torch.h"

/* Send BSL_REP_ACK (0x80) back to host via FDL1 in SRAM (0x00006c23) */
static void send_fdl1_ack(void)
{
    typedef void (*fdl_ack_fn)(uint32_t code);
    volatile uint32_t *fdl1_header = (volatile uint32_t *)0x00006200U;
    /* Verify FDL1 DHTB container magic in SRAM before branching */
    if (*fdl1_header == 0x42544844U) { /* 'DHTB' */
        fdl_ack_fn send_ack = (fdl_ack_fn)0x00006C23U;
        send_ack(0x80U);
    }
}

void board_early_init(void)
{
    /* 1. Ungate UART1 APB clock (bits 13 & 14) */
    REG_APB_CLK_EN |= APB_CLK_UART1_EN;

    /* 2. Configure 115200 8N1 at 26 MHz: divisor = 226 (0x00E2) */
    UART1_CLKD0 = UART_BAUD_115200_DIV;
    UART1_LCR   = UART_LCR_8N1;

    /* 3. Update Mailbox Stage */
    REG32(T107_DRAM_MAILBOX + 0x08U) = 0x00000003U; /* Stage: early_init completed */

    /* 4. Acknowledge USB FDL1 dispatcher so spd_dump confirms handoff */
    send_fdl1_ack();

    /* 5. Initialize SC2720 PMIC Torch subsystem */
    torch_init();
}
