//--------------------------------------------------------------
// File     : stm32_ub_din_dma.h
//--------------------------------------------------------------

//--------------------------------------------------------------
#ifndef __STM32F4_UB_DIN_DMA_H
#define __STM32F4_UB_DIN_DMA_H


//--------------------------------------------------------------
// Includes
//--------------------------------------------------------------
#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_tim.h"
#include "stm32f4xx_dma.h"
#include "misc.h"
#include "stm32_ub_sdram.h"






//--------------------------------------------------------------
// Anzahl der Sample-Werte (im SDRAM)
// wegen doubleBuffer-Mode ist die Anzahl der SampleWerte
// 2mal so gross wie die Buffer_len
//
// jeder Samplewert ist 16bit gross, also werden
// zwei Byte pro Samplewert im SDRAM benoetigt
//
// die Buffer-Anzahl gibt an, wieviele Buffer-Blöcke
// hintereinander gesampelt werden sollen
// max passen 7MByte-Daten ins SDRAM
//
// Hinweis : die beiden Werte (LEN+ANZ) werden spaeter per
//           PC-Programm ueber die "Capturesize" veraendert
//--------------------------------------------------------------
#define  DIN_DMA_DEFAULT_BUFFER_LEN       4096
#define  DIN_DMA_DEFAULT_BUFFER_ANZ          1


//--------------------------------------------------------------
// DMA
// DMA=2, Channel=6, Stream=5
//--------------------------------------------------------------
#define  DIN_DMA_CLOCK          RCC_AHB1Periph_DMA2
#define  DIN_DMA_STREAM         DMA2_Stream5
#define  DIN_DMA_CHANNEL        DMA_Channel_6
#define  DIN_DMA_TCFLAG         DMA_IT_TCIF5
#define  DIN_DMA_HTFLAG         DMA_IT_HTIF5
#define  DIN_DMA_IRQ            DMA2_Stream5_IRQn
#define  DIN_DMA_IRQ_HANDLER    DMA2_Stream5_IRQHandler


//--------------------------------------------------------------
// start Adresse der Buffer-Daten im externen RAM
//--------------------------------------------------------------
#define SDRAM_BUF_START_ADR    ((uint32_t)0xD0100000)




//--------------------------------------------------------------
// GPIO-Port der gesampelt wird : PORT-C
//
// Adressen vom benutzten Port-C (Register IDR)
// um per DMA darauf zuzugreifen
// (siehe Seite 65+285 vom Referenz Manual)
//--------------------------------------------------------------
#define DIN_DMA_GPIOC_BASE_ADR     ((uint32_t)0x40020800) // ADR von Port-C
#define DIN_DMA_GPIO_IDR_OFFSET    ((uint32_t)0x00000010) // ADR von Register IDR

#define DIN_DMA_GPIO_IDR_ADDRESS   (DIN_DMA_GPIOC_BASE_ADR | DIN_DMA_GPIO_IDR_OFFSET) // bit 0-15



//--------------------------------------------------------------
// GPIO-PINs die gesampelt werden
// (muessen alle auf dem gleichen Port liegen)
//--------------------------------------------------------------
#define  DIN_DMA_GPIO_PIN0     GPIO_Pin_1  // CH0 = PC1 
#define  DIN_DMA_GPIO_PIN1     GPIO_Pin_3  // CH1 = PC3
#define  DIN_DMA_GPIO_PIN2     GPIO_Pin_8  // CH2 = PC8 
#define  DIN_DMA_GPIO_PIN3     GPIO_Pin_11 // CH3 = PC11
#define  DIN_DMA_GPIO_PIN4     GPIO_Pin_12 // CH4 = PC12
#define  DIN_DMA_GPIO_PIN5     GPIO_Pin_13 // CH5 = PC13
#define  DIN_DMA_GPIO_PIN6     GPIO_Pin_14 // CH6 = PC14
#define  DIN_DMA_GPIO_PIN7     GPIO_Pin_15 // CH7 = PC15

#define  DIN_DMA_GPIO_ALL   (DIN_DMA_GPIO_PIN0 | DIN_DMA_GPIO_PIN1 | DIN_DMA_GPIO_PIN2 | DIN_DMA_GPIO_PIN3 | DIN_DMA_GPIO_PIN4 | DIN_DMA_GPIO_PIN5 | DIN_DMA_GPIO_PIN6 | DIN_DMA_GPIO_PIN7)




//--------------------------------------------------------------
// Timer-1
// Funktion  = GPIOs einlesen (Takt für den DMA Transver)
//
// Grundfreq = 2*APB2 (APB2=84MHz) => TIM_CLK=168MHz
// FRQ       = CLK / (Prescale+1) / (Periode+1)
//
// Hinweis : die beiden Werte (PERIODE+PRESCALE) werden spaeter per
//           PC-Programm ueber die "Samplerate" veraendert
//--------------------------------------------------------------
#define DIN_DMA_TIM                   TIM1
#define DIM_DMA_TIM_CLOCK             RCC_APB2Periph_TIM1


#define DIN_DMA_DEFAULT_TIM_PERIODE          39
#define DIN_DMA_DEFAULT_TIM_PRESCALE        167



//--------------------------------------------------------------
// Status
//--------------------------------------------------------------
typedef enum {
	DIN_DMA_WAITING = 0,   // wait   (warte auf Start-Kommando)
	DIN_DMA_ARM_TRIGGER,  // arm    (warte auf Trigger-Bedingung)
	DIN_DMA_RUNNING,      // busy   (Messung laeuft)
	DIN_DMA_READY         // ready  (Messung fertig)
}DIN_DMA_STATUS_t;


//--------------------------------------------------------------
// Struktur
//--------------------------------------------------------------
typedef struct {
	DIN_DMA_STATUS_t Status; // status
	uint8_t dma_blocked;         // merker
	uint16_t tim_prescale;       // setting fuer "samplerate"
	uint16_t tim_periode;        // setting fuer "samplerate"
	uint16_t buf_len;            // setting fuer "capturesize"
	uint16_t buf_anz;            // setting fuer "capturesize"
	uint32_t akt_buf_nr;         // aktuelle buffer nr
	uint32_t buf_start_adr_a;
	uint32_t buf_start_adr_b;
	uint32_t buf_adr_delta;
} DIN_DMA_t;
extern DIN_DMA_t DIN_DMA;




//--------------------------------------------------------------
// Globale Funktionen
//--------------------------------------------------------------
void UB_DIN_DMA_Init(void);
void UB_DIN_DMA_ReInitDMA(void);
void UB_DIN_DMA_ReInitTimer(void);
void UB_DIN_DMA_Capture(void);
void UB_DIN_DMA_Stop(void);
uint16_t UB_DIN_DMA_Read_PORT(uint32_t position);


//--------------------------------------------------------------
#endif // __STM32F4_UB_DIN_DMA_H
