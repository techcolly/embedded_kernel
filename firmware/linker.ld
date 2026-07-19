ENTRY(Reset_Handler)

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}

/* top of RAM — loaded into SP by hardware on reset */
_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
    /* must be first in flash: hardware reads SP and reset vector from 0x08000000 */
    .isr_vector :
    {
        KEEP(*(.isr_vector))  /* KEEP prevents --gc-sections from stripping it */
    } > FLASH

    .text :
    {
        *(.text*)
        *(.rodata*)

        /* C++ global constructor pointer table — walked by Reset_Handler before main() */
        . = ALIGN(4);
        __init_array_start = .;
        KEEP(*(.init_array*))
        __init_array_end = .;

        . = ALIGN(4);
        _etext = .;
    } > FLASH

    /* VMA in RAM, LMA in flash (stored after .text) — Reset_Handler copies it across */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;

        *(.data*)

        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH

    _sidata = LOADADDR(.data);  /* flash address where .data initial values live */

    /* zero-initialized globals — no content in flash, Reset_Handler zeroes it at runtime */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;

        *(.bss*)
        *(COMMON)  /* tentative definitions (uninitialized globals across translation units) */

        . = ALIGN(4);
        _ebss = .;
    } > RAM
}
