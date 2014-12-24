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

volatile uint32_t  GUI_Timer_ms;

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
	GPIO_InitTypeDef  GPIO_InitStructure;

	// Reset all variables
	Systick_Delay = 0;
	GUI_Timer_ms = 0;

	RCC_GetClocksFreq(&RCC_Clocks);

#if SYSTICK_RESOLUTION == 1
	// Setting the timer to 1us
	SysTick_Config(RCC_Clocks.HCLK_Frequency / 1000000);
#else
	// Setting the timer to 1 ms
	SysTick_Config(RCC_Clocks.HCLK_Frequency / 1000);
#endif


	// Clock Enable
	RCC_AHB1PeriphClockCmd(GPIO_TST_1_CLOCK, ENABLE);

	// Config TST_1_PIN as digital output
	GPIO_InitStructure.GPIO_Pin = GPIO_TST_1_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIO_TST_1_PORT, &GPIO_InitStructure);
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
// SysTick IRQ Handler
//--------------------------------------------------------------
void SysTick_Handler(void)
{
	// Toggle TST_1_PIN
	GPIO_TST_1_PORT->ODR ^= GPIO_TST_1_PIN;

	// Tick for Pause
	if (Systick_Delay != 0)
		Systick_Delay--;

	if (GUI_Timer_ms != 0)
		GUI_Timer_ms--;
}
