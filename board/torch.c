/*
 * OpenFeatureOS - SC2720 PMIC Flashlight / Torch Driver
 * Target: Unisoc T107 (UMS9117) / Nokia 2660 Flip (TA-1480)
 *
 * Sourced from:
 *   - Nokia-2660-RE docs/flashlight.md
 *   - Nokia-2660-RE docs/hardware.md
 */

#include "torch.h"
#include "t107_regs.h"
#include "printk.h"
#include "pmic_adi.h"

void delay_cycles(uint32_t cycles)
{
    __asm__ volatile (
        "1: subs %0, %0, #1 \n\t"
        "   bne  1b         \n\t"
        : "+r" (cycles)
        :
        : "cc"
    );
}

void delay_us(uint32_t us)
{
    /* Scaled for early bringup before hardware timer interrupts */
    delay_cycles(us * 20U);
}

void delay_ms(uint32_t ms)
{
    while (ms--) {
        delay_us(1000U);
    }
}

void torch_init(void)
{
    kprintf("[PMIC] Initializing SC2720 White LED Torch Driver at 0x%08x\n", SC2720_PMIC_TORCH_REG);
    /* Ensure torch is cleanly turned off initially */
    torch_set(0, 0);
}

void torch_set(int on, uint8_t level)
{
    uint32_t cur = pmic_adi_read(SC2720_PMIC_TORCH_REG);
    if (on) {
        level &= SC2720_TORCH_LVL_MASK;
        /* Keep reserved bits, set bit 15 (TORCH_EN), apply intensity level */
        pmic_adi_write(SC2720_PMIC_TORCH_REG, (cur & ~0x800FU) | SC2720_TORCH_EN | level);
    } else {
        /* Clear bit 15 to disable current sink */
        pmic_adi_write(SC2720_PMIC_TORCH_REG, cur & ~SC2720_TORCH_EN);
    }
}

void torch_pulse(uint32_t duration_ms, uint8_t level)
{
    torch_set(1, level);
    delay_ms(duration_ms);
    torch_set(0, 0);
}

void torch_blink(int count, uint32_t on_ms, uint32_t off_ms, uint8_t level)
{
    for (int i = 0; i < count; i++) {
        torch_set(1, level);
        delay_ms(on_ms);
        torch_set(0, 0);
        delay_ms(off_ms);
    }
}

void torch_heartbeat(void)
{
    /* Double optical pulse: thump-thump ... pause */
    torch_pulse(60, 0x0F);
    delay_ms(120);
    torch_pulse(80, 0x0F);
    delay_ms(800);
}

void torch_morse_sos(uint8_t level)
{
    /* S: 3 short pulses */
    torch_blink(3, 100, 100, level);
    delay_ms(200);

    /* O: 3 long pulses */
    torch_blink(3, 300, 150, level);
    delay_ms(200);

    /* S: 3 short pulses */
    torch_blink(3, 100, 100, level);
    delay_ms(600);
}
