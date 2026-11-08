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
        *(.text .text.*)
        *(.rodata .rodata.* .gnu.linkonce.r.*)
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
