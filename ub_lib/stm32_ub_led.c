//--------------------------------------------------------------
// File     : stm32_ub_led.c
// Datum    : 24.10.2013
// Version  : 1.0
// Autor    : UB
// EMail    : mc-4u(@)t-online.de
// Web      : www.mikrocontroller-4u.de
// CPU      : STM32F429
// IDE      : CooCox CoIDE 1.7.4
// GCC      : 4.7 2012q4
// Module   : GPIO
// Funktion : LED Funktionen
//--------------------------------------------------------------

//--------------------------------------------------------------
// Includes
//--------------------------------------------------------------
#include "stm32_ub_led.h"

//--------------------------------------------------------------
// Definition aller LEDs
// Reihenfolge wie bei LED_NAME_t
//
// Init : [LED_OFF,LED_ON]
//--------------------------------------------------------------
const LED_t LED[] = {
	// Name    ,PORT , PIN       , CLOCK              , Init
	// PG13 = Gruene LED auf dem Discovery-Board
	{ LED_GREEN, GPIOG, GPIO_Pin_13, RCC_AHB1Periph_GPIOG, LED_OFF },
	// PG14=Rote LED auf dem Discovery-Board
	{ LED_RED, GPIOG, GPIO_Pin_14, RCC_AHB1Periph_GPIOG, LED_OFF },
};

//--------------------------------------------------------------
// Init aller LEDs
//--------------------------------------------------------------
void UB_Led_Init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	LED_NAME_t led_name;

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;

	for (led_name = LED_FIRST; led_name < LED_LAST; led_name++)
	{
		// LED port clock Enable
		RCC_AHB1PeriphClockCmd(LED[led_name].LED_CLK, ENABLE);

		// Config as digital output
		GPIO_InitStructure.GPIO_Pin = LED[led_name].LED_PIN;
		GPIO_Init(LED[led_name].LED_PORT, &GPIO_InitStructure);

		// Set default value
		if (LED[led_name].LED_INIT == LED_OFF)
		{
			UB_Led_Off(led_name);
		}
		else
		{
			UB_Led_On(led_name);
		}
	}
}


//--------------------------------------------------------------
// LED ausschalten
//--------------------------------------------------------------
void UB_Led_Off(LED_NAME_t led_name)
{
	LED[led_name].LED_PORT->BSRRH = LED[led_name].LED_PIN;
}

//--------------------------------------------------------------
// LED einschalten
//--------------------------------------------------------------
void UB_Led_On(LED_NAME_t led_name)
{
	LED[led_name].LED_PORT->BSRRL = LED[led_name].LED_PIN;
}

//--------------------------------------------------------------
// LED toggeln
//--------------------------------------------------------------
void UB_Led_Toggle(LED_NAME_t led_name)
{
	LED[led_name].LED_PORT->ODR ^= LED[led_name].LED_PIN;
}

//--------------------------------------------------------------
// LED ein- oder ausschalten
//--------------------------------------------------------------
void UB_Led_Switch(LED_NAME_t led_name, LED_STATUS_t wert)
{
	if (wert == LED_OFF) {
		UB_Led_Off(led_name);
	}
	else {
		UB_Led_On(led_name);
	}
}
