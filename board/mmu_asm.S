/*
 * OpenFeatureOS - ARMv7-A Cortex-A7 MMU & Cache Assembly Primitives
 * Target: Unisoc T107 (UMS9117) / Nokia 2660 Flip (TA-1480)
 *
 * Sourced from:
 *   - Nokia-2660-RE tools/ram_payload/vendor/fpdoom/ums9117/asmcode.s
 *   - RT-Thread third_party/rt-thread/libcpu/arm/cortex-a/cp15_gcc.S
 */

    .syntax unified
    .arch armv7-a
    .arm

    .section .text, "ax", %progbits

    .global tlb_invalidate_all
    .type tlb_invalidate_all, %function
tlb_invalidate_all:
    mov     r0, #0
    mcr     p15, 0, r0, c8, c7, 0       /* TLBIALL */
    dsb     sy
    isb
    bx      lr

    .global icache_invalidate_all
    .type icache_invalidate_all, %function
icache_invalidate_all:
    mov     r0, #0
    mcr     p15, 0, r0, c7, c5, 0       /* ICIALLU */
    dsb     sy
    isb
    bx      lr

    .global dcache_clean_invalidate_all
    .type dcache_clean_invalidate_all, %function
dcache_clean_invalidate_all:
    push    {r4-r8, lr}
    dmb     sy
    mrc     p15, 1, r0, c0, c0, 1       /* CLIDR */
    mov     r3, r0, lsr #23
    ands    r3, r3, #7 << 1             /* LoC * 2 */
    beq     9f
    mov     r8, #0
0:
    tst     r0, #6
    beq     8f
    lsr     r0, r0, #3
    mcr     p15, 2, r8, c0, c0, 0       /* CSSELR (select cache level) */
    isb
    mrc     p15, 1, r1, c0, c0, 0       /* CCSIDR */
    and     r2, r1, #7                  /* Line size */
    ubfx    r4, r1, #2, #10             /* Ways - 1 */
    ubfx    r7, r1, #13, #15            /* Sets - 1 */
    clz     r5, r4
1:
    lsl     r6, r7, #4
2:
    orr     r1, r8, r4, lsl r5          /* Level + way */
    orr     r1, r1, r6, lsl r2          /* Set */
    mcr     p15, 0, r1, c7, c14, 2      /* DCCISW: Clean and invalidate line by set/way */
    subs    r6, r6, #1 << 4
    bpl     2b
    subs    r4, r4, #1
    bpl     1b
8:
    add     r8, r8, #2
    cmp     r8, r3
    blo     0b
9:
    mov     r8, #0
    mcr     p15, 2, r8, c0, c0, 0
    dsb     sy
    isb
    pop     {r4-r8, pc}

    .global dcache_clean_range
    .type dcache_clean_range, %function
dcache_clean_range:
    mrc     p15, 0, r3, c0, c0, 1       /* CTR */
    mov     r2, #4
    ubfx    r3, r3, #16, #4
    lsl     r2, r2, r3
    sub     r3, r2, #1
    bic     r0, r0, r3
1:
    mcr     p15, 0, r0, c7, c10, 1      /* DCCMVAC: Clean data cache line by MVA */
    add     r0, r0, r2
    cmp     r0, r1
    blo     1b
    dsb     sy
    bx      lr

    .global dcache_invalidate_range
    .type dcache_invalidate_range, %function
dcache_invalidate_range:
    mrc     p15, 0, r3, c0, c0, 1       /* CTR */
    mov     r2, #4
    ubfx    r3, r3, #16, #4
    lsl     r2, r2, r3
    sub     r3, r2, #1
    bic     r0, r0, r3
1:
    mcr     p15, 0, r0, c7, c6, 1       /* DCIMVAC: Invalidate data cache line by MVA */
    add     r0, r0, r2
    cmp     r0, r1
    blo     1b
    dsb     sy
    bx      lr

    .global dcache_clean_invalidate_range
    .type dcache_clean_invalidate_range, %function
dcache_clean_invalidate_range:
    mrc     p15, 0, r3, c0, c0, 1       /* CTR */
    mov     r2, #4
    ubfx    r3, r3, #16, #4
    lsl     r2, r2, r3
    sub     r3, r2, #1
    bic     r0, r0, r3
1:
    mcr     p15, 0, r0, c7, c14, 1      /* DCCIMVAC: Clean and invalidate line by MVA */
    add     r0, r0, r2
    cmp     r0, r1
    blo     1b
    dsb     sy
    bx      lr

    .global mmu_enable_hw
    .type mmu_enable_hw, %function
mmu_enable_hw:
    /* r0 = TTBR0 address, r1 = DACR permissions */
    push    {r4, lr}

    /* 1. Invalidate TLB and caches before enabling MMU */
    mov     r2, #0
    mcr     p15, 0, r2, c8, c7, 0       /* TLBIALL */
    mcr     p15, 0, r2, c7, c5, 0       /* ICIALLU */
    dsb     sy
    isb

    /* 2. Program Translation Table Base Register 0 (TTBR0) */
    mcr     p15, 0, r0, c2, c0, 0       /* TTBR0 */

    /* 3. Program Domain Access Control Register (DACR) */
    mcr     p15, 0, r1, c3, c0, 0       /* DACR */

    /* 4. Configure TTBCR (N = 0: TTBR0 maps entire 4 GB address space) */
    mrc     p15, 0, r2, c2, c0, 2       /* TTBCR */
    bic     r2, r2, #0x07               /* N = 0 */
    mcr     p15, 0, r2, c2, c0, 2

    /* 5. Enable MMU (bit 0), D-Cache (bit 2), and I-Cache (bit 12) in SCTLR */
    mrc     p15, 0, r2, c1, c0, 0       /* SCTLR */
    orr     r2, r2, #(1 << 0)           /* M: MMU Enable */
    orr     r2, r2, #(1 << 2)           /* C: Data Cache Enable */
    orr     r2, r2, #(1 << 12)          /* I: Instruction Cache Enable */
    bic     r2, r2, #(1 << 28)          /* Clear TRE (TEX Remap disabled) */
    mcr     p15, 0, r2, c1, c0, 0       /* SCTLR */

    dsb     sy
    isb

    pop     {r4, pc}

    .global mmu_disable_hw
    .type mmu_disable_hw, %function
mmu_disable_hw:
    mrc     p15, 0, r0, c1, c0, 0
    bic     r0, r0, #(1 << 0)           /* Disable MMU */
    bic     r0, r0, #(1 << 2)           /* Disable D-Cache */
    mcr     p15, 0, r0, c1, c0, 0
    dsb     sy
    isb
    bx      lr
