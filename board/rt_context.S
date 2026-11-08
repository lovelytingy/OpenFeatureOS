/*
 * OpenFeatureOS - RT-Thread Context Switch Primitives for ARM Cortex-A7
 * Target: Unisoc T107 (UMS9117) / Nokia 2660 Flip (TA-1480)
 *
 * Sourced from:
 *   - RT-Thread libcpu/arm/cortex-a/context_gcc.S
 *   - RT-Thread libcpu/arm/cortex-a/start_gcc.S
 */

    .syntax unified
    .arch armv7-a
    .arm

    .section .text, "ax"

/*
 * rt_base_t rt_hw_interrupt_disable(void);
 * Returns previous CPSR in r0 with IRQ disabled.
 */
    .global rt_hw_interrupt_disable
    .type rt_hw_interrupt_disable, %function
rt_hw_interrupt_disable:
    mrs     r0, cpsr
    cpsid   i
    bx      lr

/*
 * void rt_hw_interrupt_enable(rt_base_t level);
 * Restores CPSR from r0.
 */
    .global rt_hw_interrupt_enable
    .type rt_hw_interrupt_enable, %function
rt_hw_interrupt_enable:
    msr     cpsr_c, r0
    bx      lr

/*
 * rt_bool_t rt_hw_interrupt_is_disabled(void);
 */
    .global rt_hw_interrupt_is_disabled
    .type rt_hw_interrupt_is_disabled, %function
rt_hw_interrupt_is_disabled:
    mrs     r0, cpsr
    and     r0, r0, #0x80
    lsr     r0, r0, #7
    bx      lr

/*
 * void rt_hw_context_switch_to(rt_ubase_t to);
 * r0 --> pointer to to_thread->sp
 */
    .global rt_hw_context_switch_to
    .type rt_hw_context_switch_to, %function
rt_hw_context_switch_to:
    clrex
    ldr     sp, [r0]            /* Load new task SP */
    b       rt_hw_context_switch_exit

/*
 * void rt_hw_context_switch(rt_ubase_t from, rt_ubase_t to);
 * r0 --> pointer to from_thread->sp
 * r1 --> pointer to to_thread->sp
 */
    .global rt_hw_context_switch
    .type rt_hw_context_switch, %function
rt_hw_context_switch:
    clrex
    stmfd   sp!, {lr}           /* Push return PC */
    stmfd   sp!, {r0-r12, lr}   /* Push lr and r12-r0 */

    mrs     r4, cpsr
    tst     lr, #0x01
    orrne   r4, r4, #0x20       /* Thumb mode flag */
    stmfd   sp!, {r4}           /* Push CPSR */

    str     sp, [r0]            /* Save current SP into from_thread->sp */
    ldr     sp, [r1]            /* Load new SP from to_thread->sp */

    b       rt_hw_context_switch_exit

/*
 * Context switch exit trampoline
 * Restores context from SP and resumes execution
 */
    .global rt_hw_context_switch_exit
    .type rt_hw_context_switch_exit, %function
rt_hw_context_switch_exit:
    ldmfd   sp!, {r1}           /* Pop CPSR into r1 */
    msr     spsr_cxsf, r1       /* Load SPSR with target CPSR */
    ldmfd   sp!, {r0-r12, lr, pc}^ /* Restore registers, atomically copy SPSR->CPSR, jump to PC */

/*
 * Thread startup trampoline
 * r0: thread parameter
 * r1: thread entry function (tentry)
 * lr: thread exit function (texit)
 */
    .global _thread_start
    .type _thread_start, %function
_thread_start:
    mov     r10, lr             /* Save texit in r10 */
    blx     r1                  /* Call tentry(parameter) */
    blx     r10                 /* Call texit() on return */
    b       .                   /* Trap if texit returns */
