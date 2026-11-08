/*
 * OpenFeatureOS - Interrupt Controller Driver (SPRD INTC & ARM GIC-400)
 * Target: Unisoc T107 (UMS9117) / Nokia 2660 Flip (TA-1480)
 *
 * Sourced from:
 *   - Nokia-2660-Emulator crates/n2660-core/src/peripherals/intc.rs
 *   - Nokia-2660-Emulator crates/n2660-core/src/types.rs
 */

#include "intc.h"
#include "t107_regs.h"
#include "printk.h"

extern void reset_handler(void);
extern void undef_handler(void);
extern void svc_handler(void);
extern void pabt_handler(void);
extern void dabt_handler(void);
extern void irq_handler(void);
extern void fiq_handler(void);

struct irq_desc {
    irq_handler_t handler;
    void         *data;
};

static struct irq_desc irq_table[MAX_IRQS];
static uint32_t irq_count[MAX_IRQS];

static void install_vector_table(uint32_t base)
{
    volatile uint32_t *vec = (volatile uint32_t *)base;

    /* 8x 'ldr pc, [pc, #24]' (0xE59FF018) -> fetches target from &vec[i + 8] */
    for (int i = 0; i < 8; i++) {
        vec[i] = 0xE59FF018U;
    }
    vec[8]  = (uint32_t)(uintptr_t)reset_handler;
    vec[9]  = (uint32_t)(uintptr_t)undef_handler;
    vec[10] = (uint32_t)(uintptr_t)svc_handler;
    vec[11] = (uint32_t)(uintptr_t)pabt_handler;
    vec[12] = (uint32_t)(uintptr_t)dabt_handler;
    vec[13] = 0U; /* Reserved */
    vec[14] = (uint32_t)(uintptr_t)irq_handler;
    vec[15] = (uint32_t)(uintptr_t)fiq_handler;
}

void intc_init(void)
{
    kprintf("[INTC] Initializing Unisoc SPRD 32-Line Interrupt Controller at 0x%08x\n", SPRD_INTC_BASE);

    /* 1. Install vector tables at DRAM base (0x80000000) and payload entry (0x80100200) */
    install_vector_table(0x80000000U);
    install_vector_table(0x80100200U);

    /* 2. Disable all IRQ lines initially */
    SPRD_INTC_IRQ_DIS = 0xFFFFFFFFU;
    SPRD_INTC_FIQ_DIS = 0xFFFFFFFFU;

    /* 3. Route all interrupt lines to IRQ (bit = 0), not FIQ (bit = 1) */
    SPRD_INTC_TYPE = 0x00000000U;

    /* 4. Clear all raw and software interrupts */
    SPRD_INTC_IRQ_RAW_CLR  = 0xFFFFFFFFU;
    SPRD_INTC_IRQ_SOFT_CLR = 0xFFFFFFFFU;
    SPRD_INTC_FIQ_SOFT_CLR = 0xFFFFFFFFU;

    /* 5. Reset handler table and statistics */
    for (uint32_t i = 0; i < MAX_IRQS; i++) {
        irq_table[i].handler = 0;
        irq_table[i].data    = 0;
        irq_count[i]         = 0;
    }

    kprintf("[INTC] SPRD INTC successfully initialized. Vector trampolines installed.\n");
}

int intc_register_handler(uint32_t irq, irq_handler_t handler, void *data)
{
    if (irq >= MAX_IRQS) {
        return -1;
    }
    irq_table[irq].handler = handler;
    irq_table[irq].data    = data;
    return 0;
}

void intc_enable_irq(uint32_t irq)
{
    if (irq < MAX_IRQS) {
        SPRD_INTC_IRQ_EN = (1U << irq);
    }
}

void intc_disable_irq(uint32_t irq)
{
    if (irq < MAX_IRQS) {
        SPRD_INTC_IRQ_DIS = (1U << irq);
    }
}

void intc_trigger_soft_irq(uint32_t irq)
{
    if (irq < MAX_IRQS) {
        SPRD_INTC_IRQ_SOFT = (1U << irq);
    }
}

void intc_clear_soft_irq(uint32_t irq)
{
    if (irq < MAX_IRQS) {
        SPRD_INTC_IRQ_SOFT_CLR = (1U << irq);
    }
}

void interrupts_enable(void)
{
    __asm__ volatile ("cpsie i" ::: "memory");
}

void interrupts_disable(void)
{
    __asm__ volatile ("cpsid i" ::: "memory");
}

void irq_dispatch(void)
{
    /* Read active masked IRQ pending bitmap from INTC */
    uint32_t mask_sts = SPRD_INTC_IRQ_MASK_STS;

    while (mask_sts != 0U) {
        /* Find index of lowest set bit */
        uint32_t irq = (uint32_t)__builtin_ctz(mask_sts);

        irq_count[irq]++;

        if (irq_table[irq].handler) {
            irq_table[irq].handler(irq, irq_table[irq].data);
        } else {
            kprintf("[INTC] Warning: Unhandled spurious IRQ #%u\n", irq);
        }

        /* Clear soft IRQ and raw IRQ flags */
        SPRD_INTC_IRQ_SOFT_CLR = (1U << irq);
        SPRD_INTC_IRQ_RAW_CLR  = (1U << irq);

        /* Refresh pending status */
        mask_sts = SPRD_INTC_IRQ_MASK_STS;
    }
}
