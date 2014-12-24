//--------------------------------------------------------------
// File     : stm32_ub_button.h
//--------------------------------------------------------------

//--------------------------------------------------------------
#ifndef __STM32F4_UB_BUTTON_H
#define __STM32F4_UB_BUTTON_H

#include <stdbool.h>
#include "stm32f4xx.h"

//--------------------------------------------------------------
// Enabling, disabling the functions debounced with timer
// 1 = active, 0 = inactive
//--------------------------------------------------------------
#define  BUTTON_USE_TIMER	1	// Functions by timer

//--------------------------------------------------------------
// Buttons
//--------------------------------------------------------------
typedef enum
{
	BUTTON_FIRST= 0,
	BTN_USER	= 0,    // BTN1 on STM32F429-Discovery
	BUTTON_LAST
} BUTTON_NAME_t;

//--------------------------------------------------------------
// Button Status
//--------------------------------------------------------------
typedef enum {
	BTN_RELEASED = 0,	// Button is released
	BTN_PRESSED,		// Button pressed
} BUTTON_STATUS_t;



//--------------------------------------------------------------
// Timer for Button => TIM7
// Frequency = 2 * APB1 (APB1 = 45 MHz) => TIM_CLK = 90 MHz
// TIM_Frq = TIM_CLK / (periode + 1) / (vorteiler + 1)
// TIM_Frq = 20Hz => 50ms (set not less than 1 ms)
//--------------------------------------------------------------
#if BUTTON_USE_TIMER == 1
	#define   UB_BUTTON_TIM              TIM7
	#define   UB_BUTTON_TIM_CLK          RCC_APB1Periph_TIM7
	#define   UB_BUTTON_TIM_PERIODE      4999
	#define   UB_BUTTON_TIM_PRESCALE     899
	#define   UB_BUTTON_TIM_IRQ          TIM7_IRQn
	#define   UB_BUTTON_TIM_ISR_HANDLER  TIM7_IRQHandler
#endif

//--------------------------------------------------------------
// Button structure
//--------------------------------------------------------------
typedef struct {
	BUTTON_NAME_t BUTTON_NAME;  // Name
	GPIO_TypeDef* BUTTON_PORT;  // Port
	const uint16_t BUTTON_PIN;  // Pin
	const uint32_t BUTTON_CLK;  // Clock
	GPIOPuPd_TypeDef BUTTON_R;  // Widerstand
} BUTTON_t;


//--------------------------------------------------------------
// Global functions
//--------------------------------------------------------------
void UB_Button_Init(void);
BUTTON_STATUS_t UB_Button_Read(BUTTON_NAME_t btn_name);
bool UB_Button_OnClick(BUTTON_NAME_t btn_name);

#if BUTTON_USE_TIMER == 1
	bool UB_Button_OnPressed(BUTTON_NAME_t btn_name);
	bool UB_Button_OnRelease(BUTTON_NAME_t btn_name);
#endif

//--------------------------------------------------------------
#endif // __STM32F4_UB_BUTTON_H
