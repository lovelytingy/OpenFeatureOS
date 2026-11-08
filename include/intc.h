/*
 * OpenFeatureOS - Interrupt Controller (INTC/GIC) Header
 * Target: Unisoc T107 (UMS9117) / Nokia 2660 Flip (TA-1480)
 */

#ifndef INTC_H_
#define INTC_H_

#include <stdint.h>

#define MAX_IRQS                32U

typedef void (*irq_handler_t)(uint32_t irq, void *data);

void intc_init(void);
int  intc_register_handler(uint32_t irq, irq_handler_t handler, void *data);
void intc_enable_irq(uint32_t irq);
void intc_disable_irq(uint32_t irq);
void intc_trigger_soft_irq(uint32_t irq);
void intc_clear_soft_irq(uint32_t irq);

/* Low-level CPU interrupt enable/disable primitives */
void interrupts_enable(void);
void interrupts_disable(void);

/* Low-level dispatcher invoked by assembly IRQ vector */
void irq_dispatch(void);

#endif /* INTC_H_ */
