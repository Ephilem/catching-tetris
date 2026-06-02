#include <stdint.h>

/* Variables globales pour inspection depuis le debugger */
volatile uint32_t hf_r0, hf_r1, hf_r2, hf_r3;
volatile uint32_t hf_r12, hf_lr, hf_pc, hf_psr;
volatile uint32_t hf_cfsr, hf_hfsr, hf_bfar, hf_mmfar;
volatile uint32_t hf_exc_return;

void HardFault_HandlerC(uint32_t *stack, uint32_t exc_return)
{
    /* Stack frame empilée par le CPU */
    hf_r0  = stack[0];
    hf_r1  = stack[1];
    hf_r2  = stack[2];
    hf_r3  = stack[3];
    hf_r12 = stack[4];
    hf_lr  = stack[5];    /* adresse de retour de la fonction appelante  */
    hf_pc  = stack[6];    /* PC au moment du fault — LA clé              */
    hf_psr = stack[7];

    hf_exc_return = exc_return;

    /* Registres de status des fautes (System Control Block) */
    hf_cfsr  = *(volatile uint32_t*)0xE000ED28;  /* Configurable Fault Status */
    hf_hfsr  = *(volatile uint32_t*)0xE000ED2C;  /* HardFault Status          */
    hf_mmfar = *(volatile uint32_t*)0xE000ED34;  /* MemManage Fault Address   */
    hf_bfar  = *(volatile uint32_t*)0xE000ED38;  /* BusFault Address          */

    /* Boucle ici pour que le debugger ait le temps de tout lire */
    while (1) {
        __asm volatile ("nop");
    }
}

/* Trampoline en asm : récupère le bon SP (MSP ou PSP) selon EXC_RETURN */
__attribute__((naked)) void HardFault_Handler(void)
{
    __asm volatile (
        "tst lr, #4              \n"  /* test bit 2 de EXC_RETURN          */
        "ite eq                  \n"
        "mrseq r0, msp           \n"  /* si =0, frame sur Main Stack       */
        "mrsne r0, psp           \n"  /* si =1, frame sur Process Stack    */
        "mov r1, lr              \n"  /* passe EXC_RETURN en 2e arg        */
        "b HardFault_HandlerC    \n"
    );
}