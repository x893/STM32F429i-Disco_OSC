//--------------------------------------------------------------
// File     : stm32_ub_systick.c
// Datum    : 12.11.2013
// Version  : 1.0
// Autor    : UB
// EMail    : mc-4u(@)t-online.de
// Web      : www.mikrocontroller-4u.de
// CPU      : STM32F429
// IDE      : CooCox CoIDE 1.7.4
// GCC      : 4.7 2012q4
// Module   : keine
// Funktion : Pausen- Timer- und Counter-Funktionen
//            Zeiten im [us,ms,s] Raster
//--------------------------------------------------------------

//--------------------------------------------------------------
// Includes
//--------------------------------------------------------------
#include "stm32_ub_systick.h"

#if ((SYSTICK_RESOLUTION != 1) && (SYSTICK_RESOLUTION != 1000))
	#error "WRONG SYSTICK RESOLUTION !"
#endif

//--------------------------------------------------------------
// Globale Pausen-Variabeln
//--------------------------------------------------------------
static volatile uint32_t Systick_Delay;

//--------------------------------------------------------------
// Initialize System-Counter
// either in 1us or 1ms clock
//--------------------------------------------------------------
void UB_Systick_Init(void)
{
	RCC_ClocksTypeDef RCC_Clocks;

	// Reset all variables
	Systick_Delay = 0;
	RCC_GetClocksFreq(&RCC_Clocks);

#if SYSTICK_RESOLUTION == 1
	// Setting the timer to 1us
	SysTick_Config(RCC_Clocks.HCLK_Frequency / 1000000);
#else
	// Setting the timer to 1 ms
	SysTick_Config(RCC_Clocks.HCLK_Frequency / 1000);
#endif
}

#if SYSTICK_RESOLUTION == 1
//--------------------------------------------------------------
// Pause function (in us)
// the CPU waits until the time has expired
//--------------------------------------------------------------
void UB_Systick_Pause_us(volatile uint32_t pause)
{
	Systick_Delay = pause;
	while (Systick_Delay != 0)
		;
}
#endif

//--------------------------------------------------------------
// Pause function (in ms)
// the CPU waits until the time has expired
//--------------------------------------------------------------
void UB_Systick_Pause_ms(volatile uint32_t pause)
{
#if SYSTICK_RESOLUTION == 1
	while (pause != 0)
	{
		UB_Systick_Pause_us(1000);
		--pause;
	}
#else
	Systick_Delay = pause;
	while (Systick_Delay != 0)
		__WFI();
#endif
}

//--------------------------------------------------------------
// Pause function (in s)
// the CPU waits until the time has expired
//--------------------------------------------------------------
void UB_Systick_Pause_s(volatile uint32_t pause)
{
	while (pause != 0)
	{
		UB_Systick_Pause_ms(1000);
		--pause;
	}
}

//--------------------------------------------------------------
// SysTick User Handler
//--------------------------------------------------------------
void SysTick_UserHandler(void) __attribute__((weak));
void SysTick_UserHandler(void)
{
}

//--------------------------------------------------------------
// SysTick IRQ Handler
//--------------------------------------------------------------
void SysTick_Handler(void)
{
	// Tick for Pause
	if (Systick_Delay != 0)
		Systick_Delay--;
	SysTick_UserHandler();
}
