/*
 * OpenFeatureOS Linker Script
 * Target: Unisoc T107 (UMS9117) ARM Cortex-A7 (Nokia 2660 Flip TA-1480)
 *
 * Memory layout:
 *   - DRAM Physical Base: 0x80000000 (48 MB total LPDDR)
 *   - Download Image Base: 0x80100000 (prefixed with 512-byte DHTB header)
 *   - Execution Entry Point: 0x80100200 (_start / _vectors)
 */

OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)

MEMORY
{
    /* DRAM execution window starting at offset +0x200 after DHTB header */
    DRAM (rwx) : ORIGIN = 0x80100200, LENGTH = 32M - 0x200
}

SECTIONS
{
    /* Code and constant data */
    .text : {
        __text_start = .;
        KEEP(*(.vectors))
        KEEP(*(.text.entry))
        /*
         * Align general C code to next 8 KB boundary (0x80102000) to avoid collisions
         * with fixed vendor boot1 hook addresses (0x8010065C, 0x80100C24, 0x80101C82)
         * in the emulator.
         */
        . = ALIGN(0x2000);
        *(.text .text.*)
        *(.rodata .rodata.* .gnu.linkonce.r.*)

        /* RT-Thread Finsh shell symbol tables */
        . = ALIGN(4);
        __fsymtab_start = .;
        KEEP(*(FSymTab))
        __fsymtab_end = .;
        . = ALIGN(4);
        __vsymtab_start = .;
        KEEP(*(VSymTab))
        __vsymtab_end = .;

        /* RT-Thread module symbol tables */
        . = ALIGN(4);
        __rtmsymtab_start = .;
        KEEP(*(RTMSymTab))
        __rtmsymtab_end = .;

        /* RT-Thread automatic component initialization section */
        . = ALIGN(4);
        __rt_init_start = .;
        KEEP(*(SORT(.rti_fn*)))
        __rt_init_end = .;

        . = ALIGN(4);
        __text_end = .;
    } > DRAM

    /* Initialized data */
    .data : {
        . = ALIGN(4);
        __data_load_start = LOADADDR(.data);
        __data_start = .;
        *(.data .data.* .gnu.linkonce.d.*)
        . = ALIGN(4);
        __data_end = .;
    } > DRAM

    /* Zero-initialized data (BSS) */
    .bss (NOLOAD) : {
        . = ALIGN(4);
        __bss_start = .;
        *(.bss .bss.* .gnu.linkonce.b.*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end = .;
    } > DRAM

    /* Banked stacks for ARMv7-A processor modes */
    .stack (NOLOAD) : {
        . = ALIGN(8);
        __stack_bottom = .;

        . += 0x1000; /* 4 KB FIQ mode stack */
        __stack_fiq_top = .;

        . += 0x1000; /* 4 KB ABT mode stack */
        __stack_abt_top = .;

        . += 0x1000; /* 4 KB UND mode stack */
        __stack_und_top = .;

        . += 0x2000; /* 8 KB IRQ mode stack */
        __stack_irq_top = .;

        . += 0x4000; /* 16 KB SYS/USR mode stack */
        __stack_sys_top = .;

        . += 0x4000; /* 16 KB SVC mode stack (primary OS stack) */
        __stack_svc_top = .;

        __stack_top = .;
    } > DRAM

    /* ARMv7-A 16 KB First-Level Translation Table (L1 Page Table) */
    .mmu_table (NOLOAD) : {
        . = ALIGN(16384);
        __mmu_table_start = .;
        . += 0x4000; /* 16 KB = 4096 entries * 4 bytes */
        __mmu_table_end = .;
    } > DRAM

    /* Primary QVGA (240x320 RGB565) LCD Framebuffer: 153,600 bytes */
    .framebuffer (NOLOAD) : {
        . = ALIGN(1024);
        __framebuffer_start = .;
        . += 240 * 320 * 2;
        __framebuffer_end = .;
    } > DRAM

    /* Dynamic memory / kernel heap boundary definitions */
    . = ALIGN(8);
    __heap_start = .;
    __heap_end   = 0x82000000; /* 32 MB heap ceiling in DRAM */

    /* Discard exception frame unwinding tables not needed for baremetal */
    /DISCARD/ : {
        *(.ARM.exidx .ARM.exidx.*)
        *(.ARM.extab .ARM.extab.*)
        *(.comment)
        *(.note.*)
    }
}
