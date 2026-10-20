/*
 * Minimal startup for STM32F401 (Cortex-M4), written in C.
 *
 * On reset the CPU:
 *   1. loads the stack pointer from address 0x08000000 (vector table entry 0)
 *   2. jumps to the address at 0x08000004 (entry 1 = Reset_Handler)
 *
 * Reset_Handler then prepares RAM for C code and calls main().
 */
#include <stdint.h>

/* Symbols defined in the linker script (linker.ld) */
extern uint32_t _estack;   /* top of RAM — initial stack pointer       */
extern uint32_t _sidata;   /* start of .data initial values in FLASH   */
extern uint32_t _sdata;    /* start of .data in RAM                    */
extern uint32_t _edata;    /* end of .data in RAM                      */
extern uint32_t _sbss;     /* start of .bss in RAM                     */
extern uint32_t _ebss;     /* end of .bss in RAM                       */

int main(void);

void Reset_Handler(void)
{
    /* Copy initialised globals (.data) from FLASH to RAM */
    uint32_t *src = &_sidata;
    for (uint32_t *dst = &_sdata; dst < &_edata; )
        *dst++ = *src++;

    /* Zero uninitialised globals (.bss) */
    for (uint32_t *dst = &_sbss; dst < &_ebss; )
        *dst++ = 0;

    main();

    while (1) { }   /* main should never return */
}

void Default_Handler(void)
{
    while (1) { }   /* unexpected exception: halt here, inspect in debugger */
}

/* Core exception handlers; weak so you can override them in your own code */
void NMI_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void)     __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void)    __attribute__((weak, alias("Default_Handler")));

/*
 * The vector table. Placed in section .isr_vector, which the linker script
 * puts at the very start of FLASH (0x08000000).
 * Peripheral IRQs (entries 16+) are omitted; add them when you use interrupts.
 */
__attribute__((section(".isr_vector"), used))
void (* const vector_table[16])(void) = {
    (void (*)(void))&_estack,   /*  0: initial stack pointer */
    Reset_Handler,              /*  1 */
    NMI_Handler,                /*  2 */
    HardFault_Handler,          /*  3 */
    MemManage_Handler,          /*  4 */
    BusFault_Handler,           /*  5 */
    UsageFault_Handler,         /*  6 */
    0, 0, 0, 0,                 /*  7-10: reserved */
    SVC_Handler,                /* 11 */
    DebugMon_Handler,           /* 12 */
    0,                          /* 13: reserved */
    PendSV_Handler,             /* 14 */
    SysTick_Handler,            /* 15 */
};
