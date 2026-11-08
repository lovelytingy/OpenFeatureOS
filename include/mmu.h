/*
 * OpenFeatureOS - ARMv7-A MMU & L1/L2 Cache Header
 * Target: Unisoc T107 (UMS9117) Cortex-A7 (Nokia 2660 Flip TA-1480)
 *
 * Sourced from:
 *   - RT-Thread third_party/rt-thread/libcpu/arm/cortex-a/mmu.h
 *   - Nokia-2660-RE tools/ram_payload/vendor/fpdoom/ums9117/entry.c:145-174
 */

#ifndef MMU_H_
#define MMU_H_

#include <stdint.h>

#define MMU_TABLE_ENTRIES           4096U       /* 4096 1 MB sections = 4 GB address space */
#define MMU_SECTION_SIZE            0x00100000U /* 1 MB per section */

/* ARMv7-A Short-Descriptor Section Format */
#define DESC_SEC                    (0x2U)              /* Bit[1:0] = 0b10 (1 MB Section) */
#define XN                          (1U << 4)           /* Bit 4: Execute Never */
#define DOMAIN0                     (0U << 5)           /* Bits [8:5]: Domain 0 */
#define AP_RW                       (3U << 10)          /* Bits [11:10]: AP = 0b11 (Full R/W) */
#define AP_RO                       (2U << 10)          /* Bits [11:10]: AP = 0b10 (Read Only) */
#define MEMWBWA                     ((1U << 12) | (3U << 2)) /* TEX=1, C=1, B=1: Normal WB-WA Cacheable */
#define SHAREDEVICE                 (1U << 2)           /* C=0, B=1: Shared Device Memory */
#define SHARED                      (1U << 16)          /* Bit 16: Shareable memory attribute */

/* Combined section attributes */
#define SECTION_ATTR_DEVICE         (SHARED | AP_RW | DOMAIN0 | SHAREDEVICE | DESC_SEC | XN)
#define SECTION_ATTR_NORMAL         (SHARED | AP_RW | DOMAIN0 | MEMWBWA | DESC_SEC)

/* MMU C and Assembly Primitives */
void mmu_init(void);

/* Low-level assembly primitives in board/mmu_asm.S */
void mmu_enable_hw(uint32_t ttb_base, uint32_t dacr);
void mmu_disable_hw(void);
void tlb_invalidate_all(void);
void icache_invalidate_all(void);
void dcache_clean_all(void);
void dcache_clean_invalidate_all(void);
void dcache_clean_range(uint32_t start, uint32_t end);
void dcache_invalidate_range(uint32_t start, uint32_t end);
void dcache_clean_invalidate_range(uint32_t start, uint32_t end);

#endif /* MMU_H_ */
