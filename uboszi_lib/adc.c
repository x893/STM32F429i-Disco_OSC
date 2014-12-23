//--------------------------------------------------------------
// File     : adc.c
// Datum    : 05.01.2014
// Version  : 1.1
// Autor    : UB
// EMail    : mc-4u(@)t-online.de
// Web      : www.mikrocontroller-4u.de
// CPU      : STM32F429
// IDE      : CooCox CoIDE 1.7.4
// GCC      : 4.7 2012q4
// Module   : ADC, DMA, TIM
// Funktion : ADC
//--------------------------------------------------------------

#include "adc.h"

uint16_t ADC_DMA_Buffer_A[ADC1d_LAST * ADC_ARRAY_LEN];  //  (A) Roh-Daten per DMA
uint16_t ADC_DMA_Buffer_B[ADC1d_LAST * ADC_ARRAY_LEN];  //  (B) Roh-Daten per DMA
uint16_t ADC_DMA_Buffer_C[ADC1d_LAST * ADC_ARRAY_LEN];  //  (C) sortierte Daten

ADC_t ADC_UB;

//--------------------------------------------------------------
// Definition of the ADC pins used (max = 16)
//--------------------------------------------------------------
const ADC1d_t ADC1d[] =
{
	//	NAME	PORT	PIN			CLOCK					Channel
	{ ADC_PA5, GPIOA, GPIO_Pin_5, RCC_AHB1Periph_GPIOA, ADC_Channel_5 }, // ADC an PA5 = ADC12_IN5
	{ ADC_PA7, GPIOA, GPIO_Pin_7, RCC_AHB1Periph_GPIOA, ADC_Channel_7 }, // ADC an PA7 = ADC12_IN7
};

void P_ADC_InitIO(void);
void P_ADC_InitDMA_DoubleBuffer(void);
void P_ADC_InitNVIC(void);
void P_ADC_InitADC(void);
void P_ADC_Start(void);
void P_ADC_InitTimer2(void);
void P_ADC_Clear(void);

//--------------------------------------------------------------
// Init ADC1 and ADC2 in DMA Mode and start the cyclic conversion
//--------------------------------------------------------------
void ADC_Init_ALL(void)
{
	register ADC_t *adc = &ADC_UB;

	// Init all variables
	adc->Status = ADC_VORLAUF;
	adc->TriggerPos = 0;
	adc->TriggerQuarter = 0;
	adc->DmaStatus = 0;

	P_ADC_Clear();
	P_ADC_InitTimer2();
	P_ADC_InitIO();
	P_ADC_InitDMA_DoubleBuffer();
	P_ADC_InitNVIC();
	P_ADC_InitADC();
	P_ADC_Start();
}

//--------------------------------------------------------------
// ändern der Frequenz vom Timer2
// (Timebase der Abtastrate)
//
// n : [0...16]
//--------------------------------------------------------------
const uint16_t ADC_Freq_Table[] = {
	5375,	3124,	// 5s => 5Hz	= 200ms
	2687,	2499,	// 2s => 12,5Hz	= 80ms
	1343,	2499,	// 1s => 25Hz	= 40ms
	671,	2499,	// 500ms => 50Hz	= 20ms
	335,	1999,	// 200ms => 125Hz	= 8ms
	167,	1999,	// 100ms => 250Hz	= 4ms
	83,		1999,	// 50ms  => 500Hz	= 2ms
	41,		1599,	// 20ms  => 1,25kHz = 800us
	20,		1599,	// 10ms  => 2,5kHz	= 400us
	20,		799,	// 5ms   => 5kHz	= 200us
	20,		319,	// 2ms   => 12,5kHz	= 80us
	20,		159,	// 1ms   => 25kHz	= 40us
	20,		79,		// 500us => 50kHz	= 20us
	20,		31,		// 200us => 125kHz	= 8us
	20,		15,		// 100us => 250kHz	= 4us
	20,		7,		// 50us  => 500kHz	= 2us
	20,		3,		// 25us  => 1MHz	= 1us
};
void ADC_change_Frq(uint16_t freq)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	register uint32_t prescaler = OSZI_TIM2_PRESCALE;
	register uint32_t period = OSZI_TIM2_PERIODE;

	// Timer anhalten
	TIM_Cmd(TIM2, DISABLE);
	if (freq <= 16)
	{
		prescaler = ADC_Freq_Table[freq * 2];
		period = ADC_Freq_Table[freq * 2 + 1];
	}
	
	// Set the new values
	TIM_TimeBaseStructure.TIM_Period = period;
	TIM_TimeBaseStructure.TIM_Prescaler = prescaler;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

	// Start timer again, if necessary
	if (ADC_UB.Status != ADC_READY)
	{
		// Timer2 enable
		TIM_ARRPreloadConfig(TIM2, ENABLE);
		TIM_Cmd(TIM2, ENABLE);
	}
}

//--------------------------------------------------------------
// ändern vom Mode des DMA
// n != 1 => Double-Buffer-Mode
// n = 1  => Single-Buffer-Mode
//--------------------------------------------------------------
void ADC_change_Mode(uint16_t trigger_mode)
{
	DMA_InitTypeDef DMA_InitStructure;

	// Set flag
	ADC_UB.DmaStatus = 1;

	TIM_Cmd(TIM2, DISABLE);

	DMA_Cmd(ADC1_DMA_STREAM, DISABLE);
	// Wait until DMA stream disable
	while (DMA_GetCmdStatus(ADC1_DMA_STREAM) == ENABLE)
		;
	DMA_DeInit(ADC1_DMA_STREAM);

	// DMA-Config
	DMA_InitStructure.DMA_Channel = ADC1_DMA_CHANNEL;
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)ADC1_CDR_ADDRESS;
	DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t) & ADC_DMA_Buffer_A;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
	DMA_InitStructure.DMA_BufferSize = ADC1d_LAST * ADC_ARRAY_LEN;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
	DMA_Init(ADC1_DMA_STREAM, &DMA_InitStructure);

	if (trigger_mode != MENU_TRIGGER_MODE_AUTO)
	{
		// Double-Buffer-Mode
		ADC1_DMA_STREAM->CR |= DMA_SxCR_DBM;
		ADC1_DMA_STREAM->M1AR = (uint32_t) & ADC_DMA_Buffer_B;
	}
	else
	{
		// Normal Mode
		ADC1_DMA_STREAM->CR &= ~DMA_SxCR_DBM;
	}

	// Clear flags
	DMA_ClearITPendingBit(DMA2_Stream0, DMA_IT_TCIF0);
	DMA_ClearITPendingBit(DMA2_Stream0, DMA_IT_HTIF0);

	// DMA-enable
	DMA_Cmd(ADC1_DMA_STREAM, ENABLE);

	// Wait until DMA enable Stream
	while (DMA_GetCmdStatus(ADC1_DMA_STREAM) == DISABLE)
		;

	// Initialize NVIC
	P_ADC_InitNVIC();

	// Start timer again, if necessary
	if (ADC_UB.Status != ADC_READY || trigger_mode == MENU_TRIGGER_MODE_AUTO)
	{
		// Timer2 enable
		TIM_ARRPreloadConfig(TIM2, ENABLE);
		TIM_Cmd(TIM2, ENABLE);
	}

	// Reset flag
	ADC_UB.DmaStatus = 0;
}

//--------------------------------------------------------------
// Clear all ADC arrays
//--------------------------------------------------------------
void P_ADC_Clear(void)
{
	register uint32_t n = ADC1d_LAST * ADC_ARRAY_LEN;
	register volatile uint16_t *dstA = ADC_DMA_Buffer_A;
	register volatile uint16_t *dstB = ADC_DMA_Buffer_B;
	register volatile uint16_t *dstC = ADC_DMA_Buffer_C;

	while (n != 0)
	{
		n--;
		*dstA++ = 0;
		*dstB++ = 0;
		*dstC++ = 0;
	}
}

//--------------------------------------------------------------
// Init all IO-Pins
//--------------------------------------------------------------
void P_ADC_InitIO(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	ADC1d_NAME_t adc_name;

	for (adc_name = ADC_FIRST; adc_name < ADC1d_LAST; adc_name++)
	{
		// Clock Enable
		RCC_AHB1PeriphClockCmd(ADC1d[adc_name].ADC_CLK, ENABLE);

		// Config des Pins als Analog-Eingang
		GPIO_InitStructure.GPIO_Pin = ADC1d[adc_name].ADC_PIN;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
		GPIO_Init(ADC1d[adc_name].ADC_PORT, &GPIO_InitStructure);
	}
}

//--------------------------------------------------------------
// Init DMA (im Double-Buffer-Mode)
//--------------------------------------------------------------
void P_ADC_InitDMA_DoubleBuffer(void)
{
	DMA_InitTypeDef DMA_InitStructure;

	// Clock Enable
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);

	// DMA-Disable
	DMA_Cmd(ADC1_DMA_STREAM, DISABLE);
	// Wait for DMA-Stream disable
	while (DMA_GetCmdStatus(ADC1_DMA_STREAM) == ENABLE)
		;
	DMA_DeInit(ADC1_DMA_STREAM);

	// DMA-Config
	DMA_InitStructure.DMA_Channel = ADC1_DMA_CHANNEL;
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)ADC1_CDR_ADDRESS;
	DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t) & ADC_DMA_Buffer_A;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
	DMA_InitStructure.DMA_BufferSize = ADC1d_LAST * ADC_ARRAY_LEN;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
	DMA_Init(ADC1_DMA_STREAM, &DMA_InitStructure);

	// Double-Buffer-Mode
	ADC1_DMA_STREAM->CR |= DMA_SxCR_DBM;
	ADC1_DMA_STREAM->M1AR = (uint32_t) & ADC_DMA_Buffer_B;

	// Clear flasg
	DMA_ClearITPendingBit(DMA2_Stream0, DMA_IT_TCIF0);
	DMA_ClearITPendingBit(DMA2_Stream0, DMA_IT_HTIF0);

	// DMA-enable
	DMA_Cmd(ADC1_DMA_STREAM, ENABLE);

	// Wait for DMA-Stream enable
	while (DMA_GetCmdStatus(ADC1_DMA_STREAM) == DISABLE)
		;
}

//--------------------------------------------------------------
// Init NVIC
//--------------------------------------------------------------
void P_ADC_InitNVIC(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;

	//---------------------------------------------
	// Init DMA interrupt for Transfer Complete Interrupt
	// and Half Transfer Complete Interrupt
	// DMA2, Stream0, Channel0
	//---------------------------------------------

	DMA_ITConfig(DMA2_Stream0, DMA_IT_TC | DMA_IT_HT, ENABLE);

	// NVIC config
	NVIC_InitStructure.NVIC_IRQChannel = DMA2_Stream0_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

//--------------------------------------------------------------
// Init ADC 1+2 (in Dual regular simultaneous mode)
//
// @ 12bit + ADC_TwoSamplingDelay_5Cycles + 21MHz ADC-Clock :
//
// ADC_SampleTime_3Cycles  => Sample_Time =  3+12+5=20 => 952ns
// ADC_SampleTime_15Cycles => Sample_Time = 15+12+5=32 => 1.52us
// ADC_SampleTime_28Cycles => Sample_Time = 28+12+5=45 => 2.14us
//--------------------------------------------------------------
void P_ADC_InitADC(void)
{
	ADC_CommonInitTypeDef ADC_CommonInitStructure;
	ADC_InitTypeDef ADC_InitStructure;

	// Clock Enable
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_ADC2, ENABLE);

	//-------------------------------------
	// ADC-Config (DualMode)
	//-------------------------------------

	ADC_CommonInitStructure.ADC_Mode = ADC_DualMode_RegSimult;
	ADC_CommonInitStructure.ADC_Prescaler = ADC1d_VORTEILER;
	ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_1;
	ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
	ADC_CommonInit(&ADC_CommonInitStructure);

	//-------------------------------------
	// ADC1 (Master)
	//-------------------------------------

	ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
	ADC_InitStructure.ADC_ScanConvMode = DISABLE; // only one entry in the Regular List -> disable
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T2_TRGO;
	ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_Rising; // Master is triggered
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfConversion = 1;
	ADC_Init(ADC1, &ADC_InitStructure);

	ADC_RegularChannelConfig(ADC1, ADC1d[0].ADC_CH, 1, ADC_SampleTime_3Cycles);

	//-------------------------------------
	// ADC2 (Slave)
	//-------------------------------------

	ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
	ADC_InitStructure.ADC_ScanConvMode = DISABLE; // only one entry in the Regular List -> disable
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T2_TRGO;
	ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None; // Slave may not be triggered
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfConversion = 1;
	ADC_Init(ADC2, &ADC_InitStructure);

	ADC_RegularChannelConfig(ADC2, ADC1d[1].ADC_CH, 1, ADC_SampleTime_3Cycles);
}

//--------------------------------------------------------------
// Enable and start from the ADC and DMA
//--------------------------------------------------------------
void P_ADC_Start(void)
{
	ADC_MultiModeDMARequestAfterLastTransferCmd(ENABLE);
	ADC_DMACmd(ADC1, ENABLE);
	ADC_Cmd(ADC1, ENABLE);
	ADC_Cmd(ADC2, ENABLE);
	// Timer2 enable
	TIM_ARRPreloadConfig(TIM2, ENABLE);
	TIM_Cmd(TIM2, ENABLE);
}

//--------------------------------------------------------------
// Init Timer
//--------------------------------------------------------------
void P_ADC_InitTimer2(void)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

	//---------------------------------------------
	// init Timer2
	// Clock-Source for ADC-Conversion
	//---------------------------------------------

	// Clock enable
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

	// Timer2 init
	TIM_TimeBaseStructure.TIM_Period = OSZI_TIM2_PERIODE;
	TIM_TimeBaseStructure.TIM_Prescaler = OSZI_TIM2_PRESCALE;
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

	TIM_SelectOutputTrigger(TIM2, TIM_TRGOSource_Update);
}

//--------------------------------------------------------------
// Examined trigger point
//--------------------------------------------------------------
void ADC_searchTrigger(register uint16_t * src, uint16_t offset, uint16_t quadrant)
{
	register ADC_t *adc = &ADC_UB;
	register Menu_t *menu = &Menu;
	register uint16_t n;
	register uint16_t data;
	register uint16_t trigger;

	if (menu->Trigger.Mode == MENU_TRIGGER_MODE_AUTO)
		return;

	src += offset;
	if (menu->Trigger.Source == MENU_TRIGGER_SOURCE_CH1)
	{
		trigger = menu->Trigger.ValueCh1;
	}
	else
	{
		trigger = menu->Trigger.ValueCh2;
		src++;
	}

	for (n = 0; n < ADC_HALF_ARRAY_LEN; n++)
	{
		data = *src;
		src += 2;
		if (menu->Trigger.Edge == 0)
		{
			if (adc->Status == ADC_RUNNING)
			{
				if (data < trigger)
					adc->Status = ADC_PRE_TRIGGER;
			}
			else if (data >= trigger)
			{
				adc->Status = ADC_TRIGGER_OK;
				adc->TriggerPos = n + offset;
				adc->TriggerQuarter = quadrant;
				break;
			}
		}
		else
		{
			if (adc->Status == ADC_RUNNING)
			{
				if (data > trigger)
					adc->Status = ADC_PRE_TRIGGER;
			}
			else if (data <= trigger)
			{
				adc->Status = ADC_TRIGGER_OK;
				adc->TriggerPos = n + offset;
				adc->TriggerQuarter = quadrant;
				break;
			}
		}
	}
}

//--------------------------------------------------------------
// Examined trigger point in quadrant 1
//--------------------------------------------------------------
void ADC_searchTrigger_A1(void)
{
	ADC_searchTrigger(ADC_DMA_Buffer_A, 0, 1);
}

//--------------------------------------------------------------
// Examined trigger point in quadrant 2
//--------------------------------------------------------------
void ADC_searchTrigger_A2(void)
{
	ADC_searchTrigger(ADC_DMA_Buffer_A, ADC_HALF_ARRAY_LEN, 2);
}

//--------------------------------------------------------------
// Examined trigger point in quadrant 3
//--------------------------------------------------------------
void ADC_searchTrigger_B1(void)
{
	ADC_searchTrigger(ADC_DMA_Buffer_B, 0, 3);
}

//--------------------------------------------------------------
// Examined trigger point in quadrant 4
//--------------------------------------------------------------
void ADC_searchTrigger_B2(void)
{
	ADC_searchTrigger(ADC_DMA_Buffer_B, ADC_HALF_ARRAY_LEN, 4);
}

//--------------------------------------------------------------
// Interrupt (ISR-Function)
// is called when DMA interrupt
//   (In Half Transfer Complete and TransferCompleteInterrupt)
//
//--------------------------------------------------------------
void DMA2_Stream0_IRQHandler(void)
{
	register ADC_t *adc = &ADC_UB;

	if (DMA_GetITStatus(DMA2_Stream0, DMA_IT_HTIF0))
	{
		// HalfTransferInterruptComplete interrupt from DMA2 occurred
		DMA_ClearITPendingBit(DMA2_Stream0, DMA_IT_HTIF0);

		if (adc->DmaStatus == 0)
		{
			if (adc->Status == ADC_RUNNING
			||	adc->Status == ADC_PRE_TRIGGER
				)
			{
				if ((ADC1_DMA_STREAM->CR & DMA_SxCR_CT) == 0)
				{
					ADC_searchTrigger_A1();
				}
				else
				{
					ADC_searchTrigger_B1();
				}
			}
		}
	}
	else if (DMA_GetITStatus(DMA2_Stream0, DMA_IT_TCIF0))
	{
		// TransferInterruptComplete interrupt from DMA2 occurred
		DMA_ClearITPendingBit(DMA2_Stream0, DMA_IT_TCIF0);

		if (adc->DmaStatus == 0)
		{
			TIM_Cmd(TIM2, DISABLE);

			if (adc->Status != ADC_VORLAUF)
			{
				if (adc->Status == ADC_TRIGGER_OK)
				{
					adc->Status = ADC_READY;
				}
				else
				{
					TIM_Cmd(TIM2, ENABLE);
					if ((ADC1_DMA_STREAM->CR & DMA_SxCR_CT) != 0)
					{
						ADC_searchTrigger_A2();
					}
					else
					{
						ADC_searchTrigger_B2();
					}
				}
			}
			else
			{
				TIM_Cmd(TIM2, ENABLE);
				adc->Status = ADC_RUNNING;
			}
		}
		UB_Led_Toggle(LED_GREEN);
	}
}
