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
    // srand(SysTick->VAL);
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

    while (1) {
        if (gameState.irqFlags.flag_tick) {
            gameState.irqFlags.flag_tick = 0;

            gameState.gravityCpt++;
            if (gameState.gravityCpt >= GRAVITY_SPEED) {
                gameState.gravityCpt = 0;
                Game_ApplyGravity();
            }

            Game_UpdateUserInput(gameState.joystickState.dir);
            Game_PiecesSpawnSystem();
        }

        if (gameState.irqFlags.flag_render) {
            gameState.irqFlags.flag_render = 0;

            Render_Render();
        }

    }
}

void TIMER0_IRQHandler() {
    Joystick_Read(&gameState.joystickState);

    // flags updates
    gameState.irqFlags.flag_tick = 1;

    // Render flag : up each 10ms*5 = 50ms
    gameState.irqFlags.cpt_render++;
    if (gameState.irqFlags.cpt_render >= 4) {
        gameState.irqFlags.cpt_render = 0;
        gameState.irqFlags.flag_render = 1;
    }

    TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
}

//---------------------------------------------------------------------------------------------
#ifdef  DEBUG
void check_failed(uint8_t *file, uint32_t line) { while (1); }
#endif
