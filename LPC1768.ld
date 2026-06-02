/* LPC1768 — 512 KB Flash @ 0x00000000
 *            32 KB SRAM  @ 0x10000000
 *
 * Compatible avec startup/gcc/startup_LPC17xx.s (CodeSourcery CS3, NXP original)
 * Symboles CS3 requis : __cs3_stack, __cs3_reset_cortex_m,
 *                       __cs3_heap_start, __bss_start__, __bss_end__
 */

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 512K
    SRAM  (rwx) : ORIGIN = 0x10000000, LENGTH = 32K
}

/* Top of stack — premier mot de la table des vecteurs CS3 */
__cs3_stack = ORIGIN(SRAM) + LENGTH(SRAM);
_estack     = __cs3_stack;
__stack     = __cs3_stack;

/* Alias CS3 pour le Reset_Handler */
__cs3_reset = __cs3_reset_cortex_m;

ENTRY(__cs3_reset_cortex_m)

SECTIONS
{
    /* Table des vecteurs CS3 — doit être en tout premier en Flash */
    .cs3.interrupt_vector : ALIGN(4)
    {
        KEEP(*(.cs3.interrupt_vector))
    } > FLASH

    /* Handler de reset CS3 */
    .cs3.reset : ALIGN(4)
    {
        KEEP(*(.cs3.reset))
    } > FLASH

    /* Code et données en lecture seule */
    .text : ALIGN(4)
    {
        *(.text*)
        *(.rodata*)
        *(.glue_7)
        *(.glue_7t)
        KEEP(*(.init))
        KEEP(*(.fini))
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    .ARM.exidx : ALIGN(4)
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } > FLASH

    /* Données initialisées (copiées Flash → SRAM par _start newlib) */
    _sidata = LOADADDR(.data);
    .data : ALIGN(4)
    {
        _sdata = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > SRAM AT > FLASH

    /* BSS — symboles requis par le startup CS3 et newlib */
    .bss : ALIGN(4)
    {
        __bss_start__ = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end__ = .;
    } > SRAM

    /* Stack CS3 — réservé dans le startup (.stack, 0x100 par défaut)
     * On le place explicitement en haut de SRAM pour qu'il ne chevauche pas le heap */
    .stack (NOLOAD) : ALIGN(8)
    {
        __cs3_stack_mem = .;
        . = ALIGN(8);
    } > SRAM

    /* Heap CS3 — newlib _sbrk commence ici et monte vers le stack */
    .heap (NOLOAD) : ALIGN(8)
    {
        __cs3_heap_start = .;
        _end = .;
        end  = .;
    } > SRAM
}