//===========================================================//
// Projet Micro - INFO1 - ENSSAT - S2 2018							 //
//===========================================================//
// File                : Programme de d�part
// Hardware Environment: Open1768
// Build Environment   : Keil �Vision
//===========================================================//

#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_libcfg_default.h"
#include "lpc17xx_timer.h"
#include "touch/ili_lcd_general.h"
#include "touch/lcd_api.h"
#include "affichagelcd.h"
#include "touch/touch_panel.h"


#include "globaldec.h" // fichier contenant toutes les d�clarations de variables globales
#include <stdio.h>

#include "game.h"
#include "memory.h"
#include "timer.h"
#include "joystick.h"
#include "render.h"


//===========================================================//
// Function: Main
//===========================================================//

int Init() {
    SystemCoreClockUpdate(); // met a jour les var de la clock, s'assure qeu tout est bien config.
    Lcd_Initializtion();
    Memory_Init();
    Timer_InitMainTimer();
    Joystick_Init();
    Game_Init();
}



int main(void) {
    Init();

    Lcd_Clear(Black);
    Timer_StartMainTimer();

    char chaine[32];

    while (1) {
        if (gameState.irqFlags.flag_update) {
            gameState.irqFlags.flag_update = 0;

            Joystick_State js = gameState.joystickState;

            if (js.dir.x != 0 || js.dir.y != 0) {
                ivec2 nextPos = {
                    gameState.massGrid.pos.x + js.dir.x,
                    gameState.massGrid.pos.y + js.dir.y
                };
                Game_SetMassPosition(nextPos);
            }

            Render_Render();
        }

    }
}

void TIMER0_IRQHandler() {
    Joystick_Read(&gameState.joystickState);

    // flags updates

    // Render flag : up each 10ms*5 = 50ms
    gameState.irqFlags.cpt_update++;
    if (gameState.irqFlags.cpt_update >= 5) {
        gameState.irqFlags.cpt_update = 0;
        gameState.irqFlags.flag_update = 1;
    }

    TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
}

//---------------------------------------------------------------------------------------------
#ifdef  DEBUG
void check_failed(uint8_t *file, uint32_t line) { while (1); }
#endif
