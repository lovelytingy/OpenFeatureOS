/*
 * OpenFeatureOS - ARMv7-A Cortex-A7 MMU & Page Table Initializer
 * Target: Unisoc T107 (UMS9117) / Nokia 2660 Flip (TA-1480)
 *
 * Sourced from:
 *   - RT-Thread third_party/rt-thread/libcpu/arm/cortex-a/mmu.c
 *   - Nokia-2660-RE tools/ram_payload/vendor/fpdoom/ums9117/entry.c:145-174
 */

#include "mmu.h"
#include "printk.h"
#include "t107_regs.h"

extern uint32_t __mmu_table_start[];
extern uint32_t __mmu_table_end[];

void mmu_init(void)
{
    volatile uint32_t *page_table = (volatile uint32_t *)__mmu_table_start;
    uint32_t ttb_addr = (uint32_t)(uintptr_t)__mmu_table_start;

    kprintf("[MMU] Building 4096-entry ARMv7 Level-1 Translation Table @ 0x%08x\n", ttb_addr);

    /* 1. Identity map the entire 4 GB physical address space in 1 MB sections */
    for (uint32_t i = 0; i < MMU_TABLE_ENTRIES; i++) {
        uint32_t paddr = i << 20;

        if (paddr >= 0x80000000U && paddr < 0x83000000U) {
            /* 48 MB LPDDR2 Physical DRAM: Normal memory, Inner/Outer Cacheable, WB-WA */
            page_table[i] = paddr | SECTION_ATTR_NORMAL;
        } else {
            /* Peripherals (0x40000000-0x7FFFFFFF) & Device Memory: Shared, Non-cacheable, XN */
            page_table[i] = paddr | SECTION_ATTR_DEVICE;
        }
    }

    /* 2. Flush data cache before programming MMU translation table */
    dcache_clean_invalidate_all();

    /* 3. Program hardware registers and activate MMU + L1 D-Cache + I-Cache */
    /* DACR = 0x3 (Domain 0 set to Manager mode: full access, no fault traps) */
    mmu_enable_hw(ttb_addr, 0x00000003U);

    kprintf("[MMU] ARMv7 MMU Active | L1 Data Cache: ENABLED | Instruction Cache: ENABLED\n");
    kprintf("[MMU] DRAM 0x80000000-0x83000000 mapped as Normal Cacheable Memory (WB-WA)\n");
    kprintf("[MMU] MMIO Peripherals mapped as Shared Strongly-Ordered Device Memory\n");
}
