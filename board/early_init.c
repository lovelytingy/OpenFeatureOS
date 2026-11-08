/*
 * OpenFeatureOS - Early Board Hardware Bring-up
 * Target: Unisoc T107 (UMS9117) / Nokia 2660 Flip (TA-1480)
 */

#include "t107_regs.h"

void early_uart_putc(char c)
{
    /* Wait if TX FIFO is full (bits [15:8] of STS1) */
    while ((UART1_STS1 & 0xFF00U) >= 0x1000U) {
        /* Poll */
    }
    UART1_TXD = (uint32_t)c;
}

void early_uart_puts(const char *s)
{
    while (*s) {
        if (*s == '\n') {
            early_uart_putc('\r');
        }
        early_uart_putc(*s++);
    }
}

/* SC2720 PMIC ADI bus access routines */
uint32_t pmic_adi_read(uint32_t addr)
{
    int32_t val;
    ADI_RD_CMD = addr;
    while ((val = (int32_t)ADI_RD_DATA) < 0) {
        /* Poll until bit 31 clears (data ready) */
    }
    return (uint16_t)val;
}

void pmic_adi_write(uint32_t addr, uint32_t val)
{
    while (ADI_ARM_STS & 0x800U) {
        /* Wait if write FIFO is busy */
    }
    REG32(addr) = val;
}

void pmic_torch_enable(int enable, uint32_t level)
{
    uint32_t cur = pmic_adi_read(SC2720_PMIC_TORCH_REG);
    if (enable) {
        level &= SC2720_TORCH_LVL_MASK;
        /* Keep reserved bits, set bit 15 (enable), apply current level in [3:0] */
        pmic_adi_write(SC2720_PMIC_TORCH_REG, (cur & ~0x800FU) | SC2720_TORCH_EN | level);
    } else {
        pmic_adi_write(SC2720_PMIC_TORCH_REG, cur & ~SC2720_TORCH_EN);
    }
}

/* Send BSL_REP_ACK (0x80) back to host via FDL1 in SRAM (0x00006c23) */
static void send_fdl1_ack(void)
{
    typedef void (*fdl_ack_fn)(uint32_t code);
    volatile uint16_t *sram_probe = (volatile uint16_t *)0x00006C22U;
    if (*sram_probe != 0 && *sram_probe != 0xFFFF) {
        fdl_ack_fn send_ack = (fdl_ack_fn)0x00006C23U;
        send_ack(0x80U);
    }
}

void board_early_init(void)
{
    /* Ungate UART1 APB clock (bits 13 & 14) */
    REG_APB_CLK_EN |= APB_CLK_UART1_EN;

    /* Configure 115200 8N1 at 26 MHz: divisor = 226 (0x00E2) */
    UART1_CLKD0 = UART_BAUD_115200_DIV;
    UART1_LCR   = UART_LCR_8N1;

    /* Update Mailbox Stage */
    REG32(T107_DRAM_MAILBOX + 0x08U) = 0x00000003U; /* Stage: early_init completed */

    early_uart_puts("\n[OpenFeatureOS] board_early_init: UART1 initialized (115200 8N1)\n");

    /* Turn ON PMIC rear torch at maximum brightness (instant physical sign of life) */
    early_uart_puts("[OpenFeatureOS] Enabling SC2720 PMIC Torch LED (Physical Sign of Life)...\n");
    pmic_torch_enable(1, 0x0F);

    /* Acknowledge USB FDL1 dispatcher */
    send_fdl1_ack();
}
