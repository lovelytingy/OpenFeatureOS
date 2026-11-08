/*
 * OpenFeatureOS - Early Board Hardware Bring-up
 * Target: Unisoc T107 (UMS9117) / Nokia 2660 Flip (TA-1480)
 */

#include <stdint.h>

/* src: Nokia-2660-RE docs/uart-serial-interface.md */
#define REG_APB_CLK_EN      (*(volatile uint32_t *)0x71300000U)
#define UART1_TXD           (*(volatile uint32_t *)0x70100000U)
#define UART1_STS1          (*(volatile uint32_t *)0x7010000CU)
#define UART1_LCR           (*(volatile uint32_t *)0x70100018U)
#define UART1_CLKD0         (*(volatile uint32_t *)0x70100024U)

/* src: Nokia-2660-RE docs/flashlight.md */
#define ADI_BASE            0x40600000U
#define PMIC_TORCH_REG      0x40608DF4U

/* DRAM Mailbox Status Scratchpad */
#define MAILBOX_BASE        ((volatile uint32_t *)0x80700100U)

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
static uint32_t pmic_adi_read(uint32_t addr)
{
    int32_t val;
    *(volatile uint32_t *)(ADI_BASE + 0x28) = addr;
    while ((val = *(volatile int32_t *)(ADI_BASE + 0x2C)) < 0) {
        /* Poll */
    }
    return (uint16_t)val;
}

static void pmic_adi_write(uint32_t addr, uint32_t val)
{
    while (*(volatile uint32_t *)(ADI_BASE + 0x30) & 0x800U) {
        /* Wait if write FIFO is busy */
    }
    *(volatile uint32_t *)addr = val;
}

void pmic_torch_enable(int enable, uint32_t level)
{
    uint32_t cur = pmic_adi_read(PMIC_TORCH_REG);
    if (enable) {
        level &= 0x0FU;
        /* Keep reserved bits, set bit 15 (enable), apply current level in [3:0] */
        pmic_adi_write(PMIC_TORCH_REG, (cur & ~0x800FU) | 0x8000U | level);
    } else {
        pmic_adi_write(PMIC_TORCH_REG, cur & ~0x8000U);
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
    REG_APB_CLK_EN |= 0x6000U;

    /* Configure 115200 8N1 at 26 MHz: divisor = 226 (0x00E2) */
    UART1_CLKD0 = 0x00E2U;
    UART1_LCR   = 0x001CU; /* 8 data bits, no parity, 1 stop bit */

    /* Update Mailbox Stage */
    MAILBOX_BASE[2] = 0x00000003U; /* Stage: early_init completed */

    early_uart_puts("\n[OpenFeatureOS] board_early_init: UART1 initialized (115200 8N1)\n");

    /* Turn ON PMIC rear torch at maximum brightness (instant physical sign of life) */
    early_uart_puts("[OpenFeatureOS] Enabling SC2720 PMIC Torch LED (Physical Sign of Life)...\n");
    pmic_torch_enable(1, 0x0F);

    /* Acknowledge USB FDL1 dispatcher */
    send_fdl1_ack();
}
