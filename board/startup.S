/*
 * OpenFeatureOS - Startup Bootstrap & ARMv7-A Exception Vectors
 * Target: Unisoc T107 (UMS9117) Cortex-A7 (Nokia 2660 Flip TA-1480)
 *
 * Execution begins at 0x80100200 (handed off by FDL1 via USB download).
 * All code runs strictly in volatile DRAM.
 */

    .syntax unified
    .arch armv7-a
    .arm

/* ARM Processor Modes (CPSR Bits [4:0]) */
#define MODE_USR        0x10
#define MODE_FIQ        0x11
#define MODE_IRQ        0x12
#define MODE_SVC        0x13
#define MODE_ABT        0x17
#define MODE_UND        0x1B
#define MODE_SYS        0x1F

/* CPSR Interrupt Disable Masks */
#define I_BIT           0x80    /* Disable IRQ */
#define F_BIT           0x40    /* Disable FIQ */
#define NO_INT          (I_BIT | F_BIT)

/* Telemetry Mailbox in DRAM for early headless diagnostics */
#define DRAM_MAILBOX    0x80700100

/* ============================================================================
 * Section 1: ARM Exception Vectors Table (Mapped at 0x80100200)
 * ============================================================================
 */
    .section .vectors, "ax", %progbits
    .global _start
    .global _vectors

_start:
_vectors:
    b   reset_handler           /* 0x00: Reset / Entry point from FDL1 */
    b   undef_handler           /* 0x04: Undefined Instruction */
    b   svc_handler             /* 0x08: Software Interrupt (SVC) */
    b   pabt_handler            /* 0x0C: Prefetch Abort */
    b   dabt_handler            /* 0x10: Data Abort */
    b   hyp_handler             /* 0x14: Reserved / Hypervisor Trap */
    b   irq_handler             /* 0x18: Hardware IRQ Interrupt */
    b   fiq_handler             /* 0x1C: Fast Interrupt (FIQ) */

/* ============================================================================
 * Section 2: Reset Handler & CPU State Initialization
 * ============================================================================
 */
    .section .text.entry, "ax", %progbits
    .global reset_handler
    .type reset_handler, %function

reset_handler:
    /* 1. Force Supervisor (SVC) mode and disable all asynchronous interrupts */
    msr     cpsr_c, #(MODE_SVC | NO_INT)

    /* 2. Record Magic Markers into DRAM Mailbox (Sign of Life for USB/Emulator) */
    ldr     r0, =DRAM_MAILBOX
    ldr     r1, =0x14802660     /* Marker 1: Nokia 2660 TA-1480 */
    str     r1, [r0, #0x00]
    ldr     r1, =0xCAFEBABE     /* Marker 2: Execution Confirmed */
    str     r1, [r0, #0x04]
    ldr     r1, =0x00000001     /* Stage: Reset Handler Active */
    str     r1, [r0, #0x08]

    /* 3. Defensive CP15 Sanitization: Invalidate I-Cache & TLBs */
    mov     r0, #0
    mcr     p15, 0, r0, c7, c5, 0   /* ICIALLU: Invalidate all Instruction Caches */
    mcr     p15, 0, r0, c8, c7, 0   /* TLBIALL: Invalidate Unified TLB */
    dsb
    isb

    /* 4. Disable MMU & D-Cache, Enable I-Cache & Branch Prediction (SCTLR) */
    mrc     p15, 0, r0, c1, c0, 0   /* Read SCTLR (System Control Register) */
    bic     r0, r0, #(1 << 0)       /* Bit 0: Disable MMU */
    bic     r0, r0, #(1 << 1)       /* Bit 1: Disable strict alignment checks */
    bic     r0, r0, #(1 << 2)       /* Bit 2: Disable Data Cache */
    bic     r0, r0, #(1 << 13)      /* Bit 13: Normal exception vectors (use VBAR) */
    orr     r0, r0, #(1 << 12)      /* Bit 12: Enable Instruction Cache */
    orr     r0, r0, #(1 << 11)      /* Bit 11: Enable Branch Prediction */
    mcr     p15, 0, r0, c1, c0, 0   /* Write back to SCTLR */
    isb

    /* 5. Set Vector Base Address Register (VBAR) to _vectors (0x80100200) */
    ldr     r0, =_vectors
    mcr     p15, 0, r0, c12, c0, 0  /* VBAR */
    isb

    /* 6. Enable VFP and NEON Coprocessors (CP10 and CP11) in CPACR */
    mrc     p15, 0, r0, c1, c0, 2   /* Read CPACR */
    orr     r0, r0, #(0xF << 20)    /* Full access for CP10 and CP11 */
    mcr     p15, 0, r0, c1, c0, 2   /* Write CPACR */
    isb
    mov     r0, #(1 << 30)          /* Bit 30: VFP Enable bit */
    vmsr    fpexc, r0               /* Enable VFP / Advanced SIMD */

    /* 7. Initialize Banked Stack Pointers for All ARM Modes */
    /* FIQ Mode */
    msr     cpsr_c, #(MODE_FIQ | NO_INT)
    ldr     sp, =__stack_fiq_top

    /* Abort Mode */
    msr     cpsr_c, #(MODE_ABT | NO_INT)
    ldr     sp, =__stack_abt_top

    /* Undefined Mode */
    msr     cpsr_c, #(MODE_UND | NO_INT)
    ldr     sp, =__stack_und_top

    /* IRQ Mode */
    msr     cpsr_c, #(MODE_IRQ | NO_INT)
    ldr     sp, =__stack_irq_top

    /* System / User Mode */
    msr     cpsr_c, #(MODE_SYS | NO_INT)
    ldr     sp, =__stack_sys_top

    /* Supervisor (SVC) Mode - OS Core Execution Mode */
    msr     cpsr_c, #(MODE_SVC | NO_INT)
    ldr     sp, =__stack_svc_top

    /* 8. Zero out the BSS section */
    ldr     r0, =__bss_start
    ldr     r1, =__bss_end
    mov     r2, #0
1:
    cmp     r0, r1
    strlt   r2, [r0], #4
    blt     1b

    /* 9. Relocate initialized data from load-address to execution-address (if different) */
    ldr     r0, =__data_start
    ldr     r1, =__data_load_start
    ldr     r2, =__data_end
2:
    cmp     r0, r2
    ldrlt   r3, [r1], #4
    strlt   r3, [r0], #4
    blt     2b

    /* 10. Update Mailbox Status: Handoff to C */
    ldr     r0, =DRAM_MAILBOX
    ldr     r1, =0x00000002     /* Stage: C Runtime Ready */
    str     r1, [r0, #0x08]

    /* 11. Early hardware bringup & RT-Thread entry */
    bl      board_early_init
    bl      rtthread_startup

    /* 12. Safe hang loop if rtthread_startup() returns */
hang_loop:
    wfi
    b       hang_loop

    .size reset_handler, . - reset_handler

/* ============================================================================
 * Section 3: Default Exception Traps & RT-Thread IRQ Context Switch
 * ============================================================================
 */
    .section .text, "ax", %progbits

    .global undef_handler
    .type undef_handler, %function
undef_handler:
    ldr     r0, =DRAM_MAILBOX
    ldr     r1, =0xDEAD0001     /* Crash Code: Undefined Instruction */
    str     r1, [r0, #0x10]
1:  wfi
    b       1b

    .global svc_handler
    .type svc_handler, %function
svc_handler:
    movs    pc, lr

    .global pabt_handler
    .type pabt_handler, %function
pabt_handler:
    ldr     r0, =DRAM_MAILBOX
    ldr     r1, =0xDEAD0003     /* Crash Code: Prefetch Abort */
    str     r1, [r0, #0x10]
1:  wfi
    b       1b

    .global dabt_handler
    .type dabt_handler, %function
dabt_handler:
    ldr     r0, =DRAM_MAILBOX
    ldr     r1, =0xDEAD0004     /* Crash Code: Data Abort */
    str     r1, [r0, #0x10]
1:  wfi
    b       1b

    .global hyp_handler
    .type hyp_handler, %function
hyp_handler:
    ldr     r0, =DRAM_MAILBOX
    ldr     r1, =0xDEAD0005     /* Crash Code: Hypervisor Trap */
    str     r1, [r0, #0x10]
1:  wfi
    b       1b

    .global irq_handler
    .type irq_handler, %function
irq_handler:
    /* Save full register context on IRQ mode stack */
    stmfd   sp!, {r0-r12, lr}

    /* Notify RT-Thread OS of interrupt entry */
    bl      rt_interrupt_enter

    /* Dispatch peripheral interrupts */
    bl      irq_dispatch

    /* Notify RT-Thread OS of interrupt departure */
    bl      rt_interrupt_leave

    /* Check if the scheduler requested a thread switch inside ISR */
    ldr     r0, =rt_thread_switch_interrupt_flag
    ldr     r1, [r0]
    cmp     r1, #1
    beq     rt_hw_context_switch_interrupt_do

    /* Fast path: No context switch. Restore from IRQ stack and return */
    ldmfd   sp!, {r0-r12, lr}
    subs    pc, lr, #4

rt_hw_context_switch_interrupt_do:
    mov     r1, #0
    str     r1, [r0]            /* Clear rt_thread_switch_interrupt_flag */

    /* Transfer saved context from IRQ stack to preempted thread's SVC stack */
    mov     r1, sp              /* r1 = pointer to saved r0-r3 on IRQ stack */
    add     sp, sp, #(4 * 4)    /* Advance IRQ sp past r0-r3 */
    ldmfd   sp!, {r4-r12, lr}   /* Restore r4-r12, lr_irq from IRQ stack */
    mrs     r0, spsr            /* Save interrupted thread's CPSR into r0 */
    sub     r2, lr, #4          /* Interrupted thread's PC into r2 */

    /* Switch CPU to Supervisor (SVC) mode with interrupts masked */
    msr     cpsr_c, #(MODE_SVC | NO_INT)

    /* Push preempted thread's full register frame onto its SVC stack */
    stmfd   sp!, {r2}           /* PC */
    stmfd   sp!, {r4-r12, lr}   /* LR, R12-R4 */
    ldmfd   r1, {r5-r8}         /* Load original task r0-r3 from r1 into scratch r5-r8 */
    stmfd   sp!, {r5-r8}        /* R3-R0 onto SVC stack */
    stmfd   sp!, {r0}           /* CPSR */

    /* Store updated SVC SP into preempted thread's TCB: *rt_interrupt_from_thread = sp */
    ldr     r4, =rt_interrupt_from_thread
    ldr     r5, [r4]
    str     sp, [r5]

    /* Load new thread's SVC SP: sp = *rt_interrupt_to_thread */
    ldr     r6, =rt_interrupt_to_thread
    ldr     r6, [r6]
    ldr     sp, [r6]

    /* Resume new thread via standard context switch exit */
    b       rt_hw_context_switch_exit

    .global fiq_handler
    .type fiq_handler, %function
fiq_handler:
    subs    pc, lr, #4
