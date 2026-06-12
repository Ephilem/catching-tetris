#include "timer.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_pinsel.h"

/**
 * Initialize TIMER0 to interrupt each 10ms
 */
void Timer_InitMainTimer() {
	TIM_TIMERCFG_Type setup;
	TIM_MATCHCFG_Type matchSetup;
	
	setup.PrescaleOption = TIM_PRESCALE_USVAL;
	setup.PrescaleValue = 1000; // increment each 1ms
	
	TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &setup);
	
	matchSetup.MatchChannel = 0;
	matchSetup.MatchValue = 10;
	matchSetup.ExtMatchOutputType = TIM_EXTMATCH_TOGGLE;
	matchSetup.ResetOnMatch = ENABLE;
	matchSetup.StopOnMatch = DISABLE;
	matchSetup.IntOnMatch = ENABLE;
	
	TIM_ConfigMatch(LPC_TIM0, &matchSetup);

	NVIC_EnableIRQ(TIMER0_IRQn);
}

void Timer_StartMainTimer() {
	TIM_Cmd(LPC_TIM0, ENABLE);
}
