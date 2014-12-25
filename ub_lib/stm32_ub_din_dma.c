//--------------------------------------------------------------
// File     : stm32_ub_din_dma.c
// Datum    : 23.08.2014
// Version  : 1.1 (angepasst fuer Logic-Analyzer)
// Autor    : UB
// EMail    : mc-4u(@)t-online.de
// Web      : www.mikrocontroller-4u.de
// CPU      : STM32F4
// IDE      : CooCox CoIDE 1.7.4
// GCC      : 4.7 2012q4
// Module   : GPIO, TIM, DMA, MISC, STM32_UB_SDRAM
// Funktion : Multi GPIO-Input per DMA2 und Timer1
//            Speicherort = externes SDRAM
//
// Hinweis  : Fuer Logic-Analyzer nur freie Pins vom
//            STM32F429-Disco-Board
// Eingang  : PC1, PC3, PC8, PC11, PC12, PC13, PC14, PC15
//--------------------------------------------------------------


//--------------------------------------------------------------
// Includes
//--------------------------------------------------------------
#include "stm32_ub_din_dma.h"




//--------------------------------------------------------------
// interne Funktionen
//--------------------------------------------------------------
void P_DIN_DMA_InitIO(void);
void P_DIN_DMA_InitTIM(void);
void P_DIN_DMA_InitDMA(void);
void P_DIN_DMA_InitNVIC(void);
void P_DIN_DMA_start(void);

//--------------------------------------------------------------
// Init aller Input Kanaele
//--------------------------------------------------------------
void UB_DIN_DMA_Init(void)
{
	DIN_DMA.Status = DIN_DMA_WAITING;
	DIN_DMA.dma_blocked = 0;
	DIN_DMA.tim_prescale = DIN_DMA_DEFAULT_TIM_PRESCALE;
	DIN_DMA.tim_periode = DIN_DMA_DEFAULT_TIM_PERIODE;
	DIN_DMA.buf_len = DIN_DMA_DEFAULT_BUFFER_LEN;
	DIN_DMA.buf_anz = DIN_DMA_DEFAULT_BUFFER_ANZ;

	DIN_DMA.akt_buf_nr = 0;
	DIN_DMA.buf_start_adr_a = SDRAM_BUF_START_ADR;
	// 2 byte per sample value
	DIN_DMA.buf_start_adr_b = SDRAM_BUF_START_ADR + (2 * DIN_DMA.buf_len);
	// 2 sample values for double buffer mode
	DIN_DMA.buf_adr_delta = (4 * DIN_DMA.buf_len);

	UB_SDRAM_Init();
	P_DIN_DMA_InitIO();
	P_DIN_DMA_InitTIM();
	P_DIN_DMA_InitDMA();
	P_DIN_DMA_InitNVIC();
}


//--------------------------------------------------------------
// Reinit DMA
//--------------------------------------------------------------
void UB_DIN_DMA_ReInitDMA(void)
{
	DIN_DMA.buf_start_adr_a = SDRAM_BUF_START_ADR;
	DIN_DMA.buf_start_adr_b = SDRAM_BUF_START_ADR + (2 * DIN_DMA.buf_len);
	DIN_DMA.buf_adr_delta = (4 * DIN_DMA.buf_len);

	P_DIN_DMA_InitDMA();
	P_DIN_DMA_InitNVIC();
}


//--------------------------------------------------------------
// Reinit Timer
//--------------------------------------------------------------
void UB_DIN_DMA_ReInitTimer(void)
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;

	TIM_Cmd(DIN_DMA_TIM, DISABLE);
	TIM_DeInit(DIN_DMA_TIM);

	TIM_TimeBaseStructure.TIM_Period = DIN_DMA.tim_periode;
	TIM_TimeBaseStructure.TIM_Prescaler = DIN_DMA.tim_prescale;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(DIN_DMA_TIM, &TIM_TimeBaseStructure);
}


//--------------------------------------------------------------
// Start the measurement by timer and DMA
//--------------------------------------------------------------
void UB_DIN_DMA_Capture(void)
{
	DIN_DMA.Status = DIN_DMA_RUNNING;
	DIN_DMA.akt_buf_nr = 0;

	P_DIN_DMA_start();
}


//--------------------------------------------------------------
// Stop the measurement
//--------------------------------------------------------------
void UB_DIN_DMA_Stop(void)
{
	if (DIN_DMA.Status == DIN_DMA_RUNNING)
	{
		DIN_DMA.Status = DIN_DMA_READY;

		TIM_Cmd(DIN_DMA_TIM, DISABLE);
		UB_DIN_DMA_ReInitDMA();
	}
}


//--------------------------------------------------------------
// Reads the value of port, at a position of
//
// position :	[0 to DIN DMA.buf len * 2 * DIN DMA.buf_anz]
// return   :	16bit
//--------------------------------------------------------------
uint16_t UB_DIN_DMA_Read_PORT(uint32_t position)
{
	uint32_t maxpos;
	uint32_t addr;

	maxpos = (DIN_DMA.buf_len * 2 * DIN_DMA.buf_anz) - 1;
	if (position > maxpos)
		position = maxpos;

	addr = DIN_DMA.buf_start_adr_a + (position * 2); // * 2 for 16bit
	return (*(uint16_t *)(addr) & DIN_DMA_GPIO_ALL);
}

//--------------------------------------------------------------
// Init all IO pins
//--------------------------------------------------------------
void P_DIN_DMA_InitIO(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

	GPIO_InitStructure.GPIO_Pin = DIN_DMA_GPIO_ALL;

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
}

//--------------------------------------------------------------
// Init vom Timer
//--------------------------------------------------------------
void P_DIN_DMA_InitTIM(void)
{
	RCC_APB2PeriphClockCmd(DIM_DMA_TIM_CLOCK, ENABLE);
	UB_DIN_DMA_ReInitTimer();
}

//--------------------------------------------------------------
// Init DMA
//--------------------------------------------------------------
void P_DIN_DMA_InitDMA(void)
{
	DMA_InitTypeDef DMA_InitStructure;

	DIN_DMA.dma_blocked = 1;

	//---------------------------------------------
	// DMA per Timer-Update
	// (siehe Seite 305 vom Referenz Manual)
	// DMA=2, Channel=6, Stream=5
	//---------------------------------------------

	RCC_AHB1PeriphClockCmd(DIN_DMA_CLOCK, ENABLE);

	// Disable DMA, DMA wait to stream disable
	DMA_Cmd(DIN_DMA_STREAM, DISABLE);
	while (DMA_GetCmdStatus(DIN_DMA_STREAM) == ENABLE)
		;
	DMA_DeInit(DIN_DMA_STREAM);

	// DMA init (DMA2, Channel6, Stream5)
	DMA_InitStructure.DMA_Channel = DIN_DMA_CHANNEL;
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)DIN_DMA_GPIO_IDR_ADDRESS;
	DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)DIN_DMA.buf_start_adr_a;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
	DMA_InitStructure.DMA_BufferSize = DIN_DMA.buf_len;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord; // 16bit
	DMA_InitStructure.DMA_MemoryDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
	DMA_Init(DIN_DMA_STREAM, &DMA_InitStructure);

	// Double-Buffer-Mode
	DIN_DMA_STREAM->CR |= (uint32_t)DMA_SxCR_DBM;
	DIN_DMA_STREAM->M1AR = (uint32_t)DIN_DMA.buf_start_adr_b;

	// Flags loeschen
	DMA_ClearITPendingBit(DIN_DMA_STREAM, DIN_DMA_TCFLAG);
	DMA_ClearITPendingBit(DIN_DMA_STREAM, DIN_DMA_HTFLAG);

	// Enable DMA, wait until DMA stream enable
	DMA_Cmd(DIN_DMA_STREAM, ENABLE);
	while (DMA_GetCmdStatus(DIN_DMA_STREAM) == DISABLE)
		;

	DIN_DMA.dma_blocked = 0;
}


//--------------------------------------------------------------
// Init NVIC
//--------------------------------------------------------------
void P_DIN_DMA_InitNVIC(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;

	//---------------------------------------------
	// init vom DMA Timer-Update
	// für TransferComplete Interrupt
	// und HalfTransferComplete Interrupt
	// DMA2, Stream5, Channel6
	//---------------------------------------------

	DMA_ITConfig(DIN_DMA_STREAM, DMA_IT_TC | DMA_IT_HT, ENABLE);

	NVIC_InitStructure.NVIC_IRQChannel = DIN_DMA_IRQ;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}


//--------------------------------------------------------------
// interne Funktionen
// start vom Timer
//--------------------------------------------------------------
void P_DIN_DMA_start(void)
{
	// DMA-Timer-Update enable
	TIM_DMACmd(DIN_DMA_TIM, TIM_DMA_Update, ENABLE);

	// Timer enable
	TIM_ARRPreloadConfig(DIN_DMA_TIM, ENABLE);
	TIM_Cmd(DIN_DMA_TIM, ENABLE);
}


//--------------------------------------------------------------
// Interrupt (ISR-Funktion)
// wird bei DMA Interrupt aufgerufen
//   (Bei HalfTransferComplete und TransferCompleteInterrupt)
//
// (DMA2, Stream5, Channel6)
//--------------------------------------------------------------
void DIN_DMA_IRQ_HANDLER(void)
{

	if (DMA_GetITStatus(DIN_DMA_STREAM, DIN_DMA_HTFLAG) == SET)
	{
		// Half Transfer Complete Interrupt interrupt from DMA2 occurred
		DMA_ClearITPendingBit(DIN_DMA_STREAM, DIN_DMA_HTFLAG);

		if (DIN_DMA.dma_blocked == 0)
		{
			if ((DIN_DMA_STREAM->CR & DMA_SxCR_CT) == 0)
			{
				// BUF-A1
				if (DIN_DMA.akt_buf_nr > 0) {
					// Adresse von Buffer-B hochsetzen
					DIN_DMA.buf_start_adr_b += DIN_DMA.buf_adr_delta;
					DIN_DMA_STREAM->M1AR = (uint32_t)DIN_DMA.buf_start_adr_b;
				}
			}
			else
			{
				// BUF-B1
				// Adresse von Buffer-A hochsetzen
				DIN_DMA.buf_start_adr_a += DIN_DMA.buf_adr_delta;
				DIN_DMA_STREAM->M0AR = (uint32_t)DIN_DMA.buf_start_adr_a;
			}
		}
	}
	else if (DMA_GetITStatus(DIN_DMA_STREAM, DIN_DMA_TCFLAG) == SET)
	{
		// Transfer Complete Interrupt interrupt from DMA2 occurred
		DMA_ClearITPendingBit(DIN_DMA_STREAM, DIN_DMA_TCFLAG);

		if (DIN_DMA.dma_blocked == 0) {
			if ((DIN_DMA_STREAM->CR & DMA_SxCR_CT) != 0)
			{
				// BUF-A2
			}
			else
			{
				// BUF-B2
				// (Ende von einem Block)
				DIN_DMA.akt_buf_nr++;
				// check ob letzter Block errreicht
				if (DIN_DMA.akt_buf_nr >= DIN_DMA.buf_anz)
				{
					// timer stoppen
					TIM_Cmd(DIN_DMA_TIM, DISABLE);
					// adressen reset
					UB_DIN_DMA_ReInitDMA();
					DIN_DMA.Status = DIN_DMA_READY;
				}
			}
		}
	}
}





