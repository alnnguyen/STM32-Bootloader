/* Bootloader image: sectors 0-1 of the STM32F401RE (first 32 KB of flash) */

ENTRY(Reset_Handler)

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 32K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 96K
}

_estack = ORIGIN(RAM) + LENGTH(RAM);   /* stack grows down from top of RAM */

SECTIONS
{
    /* Vector table must be first in FLASH */
    .isr_vector :
    {
        KEEP(*(.isr_vector))
    } > FLASH

    /* Code and read-only data */
    .text :
    {
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    /* Initialised data: lives in RAM, initial values stored in FLASH */
    _sidata = LOADADDR(.data);
    .data :
    {
        _sdata = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH

    /* Zero-initialised data */
    .bss :
    {
        _sbss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM
}
