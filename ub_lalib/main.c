//--------------------------------------------------------------
// File     : main.c
// Datum    : 23.08.2014
// Version  : 1.0
// Autor    : UB
// EMail    : mc-4u(@)t-online.de
// Web      : www.mikrocontroller-4u.de
// CPU      : STM32F429
// IDE      : CooCox CoIDE 1.7.4
// GCC      : 4.7 2012q4
// Module   : CMSIS_BOOT, M4_CMSIS_CORE
// Funktion : Logic-Analyzer
// Hinweis  : Diese zwei Files muessen auf 8MHz stehen
//              "cmsis_boot/stm32f4xx.h"
//              "cmsis_boot/system_stm32f4xx.c"
// In Configuration diese Define hinzufügen :
// "STM32F429_439xx" , "__ASSEMBLY__" , "USE_STDPERIPH_DRIVER"
//
// PC SUMP-Client : http://www.lxtreme.nl/ols/
//--------------------------------------------------------------

#include "main.h"
#include "la.h"

DIN_DMA_t DIN_DMA;
LA_t LA;
volatile uint32_t COM_Counter;
volatile uint32_t BLINK_Counter;

//--------------------------------------------------------------
// SysTick User Handler
//--------------------------------------------------------------
void SysTick_UserHandler(void)
{
	if (COM_Counter != 0)
		COM_Counter--;

	if (BLINK_Counter != 0)
		BLINK_Counter--;
}

int main(void)
{
	LA_COM_t com_status;

	SystemInit();
	DBGMCU_Config(DBGMCU_SLEEP | DBGMCU_STOP | DBGMCU_STANDBY, ENABLE);

	la_init();
	com_init();

	while (1)
	{
		if (DIN_DMA.Status == DIN_DMA_WAITING)
		{
			// Wait for command
		}
		else if (DIN_DMA.Status == DIN_DMA_ARM_TRIGGER)
		{
			// Wait for trigger condition
			la_check_trigger();
		}
		else if (DIN_DMA.Status == DIN_DMA_RUNNING)
		{
			// Measurement is running (red LED flashes)
			if (BLINK_Counter == 0)
			{
				BLINK_Counter = BLINK_DELAY_MS;
				UB_Led_Toggle(LED_RED);
			}
		}
		else if (DIN_DMA.Status == DIN_DMA_READY)
		{
			// Measurement completed
			UB_Led_Off(LED_GREEN);
			DIN_DMA.Status = DIN_DMA_WAITING;
			// Send all data to the PC
			com_send_all_data();
		}

		//----------------------------------
		// COM-Functions
		//----------------------------------
		if (DIN_DMA.Status == DIN_DMA_WAITING || COM_Counter == 0)
		{
			COM_Counter = COM_DELAY_MS;
			com_status = com_do();

			if (com_status != LA_COM_NONE)
			{
				// Command has been received
				if (com_status == LA_COM_ARM_TRIGGER)
				{
					if (DIN_DMA.Status == DIN_DMA_WAITING)
					{
						UB_Led_On(LED_GREEN);
						DIN_DMA.Status = DIN_DMA_ARM_TRIGGER;
					}
				}
				else if (com_status == LA_COM_RESET)
				{
					UB_Led_Off(LED_GREEN);
					if (DIN_DMA.Status == DIN_DMA_RUNNING)
						UB_DIN_DMA_Stop();
					else if (DIN_DMA.Status == DIN_DMA_ARM_TRIGGER)
						DIN_DMA.Status = DIN_DMA_WAITING;
				}
				else
				{
					if ((com_status == LA_COM_SET_FRQ)
						|| (com_status == LA_COM_SET_SIZE)
						|| (com_status == LA_COM_SET_MASK)
						|| (com_status == LA_COM_SET_VALUE)
						)
					{
						la_update_settings(com_status);
					}
				}
			}
		}
	}
}
