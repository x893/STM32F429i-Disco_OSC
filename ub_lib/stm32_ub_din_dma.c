//--------------------------------------------------------------
// File     : stm32_ub_din_dma.c
// Datum    : 08.06.2014
// Version  : 1.1 (angepasst fuer Logic-Analyzer)
// Autor    : UB
// EMail    : mc-4u(@)t-online.de
// Web      : www.mikrocontroller-4u.de
// CPU      : STM32F4
// IDE      : CooCox CoIDE 1.7.4
// GCC      : 4.7 2012q4
// Module   : GPIO, TIM, DMA, MISC
// Funktion : Multi GPIO-Input per DMA2 und Timer1
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
  DIN_DMA.akt_status=DIN_DMA_WAITING;
  DIN_DMA.dma_blocked=0;
  DIN_DMA.tim_prescale=DIN_DMA_DEFAULT_TIM_PRESCALE;
  DIN_DMA.tim_periode=DIN_DMA_DEFAULT_TIM_PERIODE;
  DIN_DMA.buf_len=DIN_DMA_DEFAULT_BUFFER_LEN;
  DIN_DMA.buf_anz=DIN_DMA_DEFAULT_BUFFER_ANZ;

  DIN_DMA.akt_buf_nr=0;
  DIN_DMA.max_buf_nr=1;
  DIN_DMA.buf_start_adr_a=SDRAM_BUF_START_ADR;
  DIN_DMA.buf_start_adr_b=SDRAM_BUF_START_ADR+(2*DIN_DMA.buf_len); // 2byte pro samplewert
  DIN_DMA.buf_adr_delta=(4*DIN_DMA.buf_len); // 2 samplewerte wegen Doublebuffer-Mode


  // init der GPIOs
  P_DIN_DMA_InitIO();
  // init vom Timer
  P_DIN_DMA_InitTIM();
  // init vom DMA
  P_DIN_DMA_InitDMA();
  // init vom NVIC
  P_DIN_DMA_InitNVIC();
}

//--------------------------------------------------------------
void UB_DIN_DMA_ReInitDMA(void)
{
  DIN_DMA.buf_start_adr_a=SDRAM_BUF_START_ADR;
  DIN_DMA.buf_start_adr_b=SDRAM_BUF_START_ADR+(2*DIN_DMA.buf_len); // 2byte pro samplewert
  DIN_DMA.buf_adr_delta=(4*DIN_DMA.buf_len); // 2 samplewerte wegen Doublebuffer-Mode

  // reinit DMA
  P_DIN_DMA_InitDMA();
  // reinit NVIC
  P_DIN_DMA_InitNVIC();
}

//--------------------------------------------------------------
void UB_DIN_DMA_ReInitTimer(void)
{
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;

  // timer stopen
  TIM_Cmd(DIN_DMA_TIM, DISABLE);

  // Timer deinit
  TIM_DeInit(DIN_DMA_TIM);

  // Timer init
  TIM_TimeBaseStructure.TIM_Period =  DIN_DMA.tim_periode;
  TIM_TimeBaseStructure.TIM_Prescaler = DIN_DMA.tim_prescale;
  TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseStructure.TIM_RepetitionCounter=0;
  TIM_TimeBaseInit(DIN_DMA_TIM, &TIM_TimeBaseStructure);
}

//--------------------------------------------------------------
// start vom einlesen per Timer und DMA
//--------------------------------------------------------------
void UB_DIN_DMA_Capture(void)
{
  DIN_DMA.akt_status=DIN_DMA_RUNNING;

  // start Adressen berechnen
  DIN_DMA.max_buf_nr=DIN_DMA.buf_anz;
  // check auf maxwerte
  if(DIN_DMA.max_buf_nr<1) DIN_DMA.max_buf_nr=1;
  if(DIN_DMA.max_buf_nr>20) DIN_DMA.max_buf_nr=20;
  // akt buffer nr
  DIN_DMA.akt_buf_nr=0;

  // rote LED einschalten
  //   UB_Led_On(LED_RED);
  // timer starten
  P_DIN_DMA_start();
}

//--------------------------------------------------------------
void UB_DIN_DMA_Stop(void)
{
  if(DIN_DMA.akt_status==DIN_DMA_RUNNING) {
    DIN_DMA.akt_status=DIN_DMA_READY;

    // timer stopen
    TIM_Cmd(DIN_DMA_TIM, DISABLE);
    // adressen reset
    UB_DIN_DMA_ReInitDMA();
  }
}


//--------------------------------------------------------------
// liesst den Wert von Port, an einer Position aus
//
// position   : [0 bis ??]
//
// ret_wert   : 16bit
//--------------------------------------------------------------
uint16_t UB_DIN_DMA_Read_PORT(uint32_t position)
{
  uint16_t port_wert;
  uint16_t ret_wert=0;
  uint32_t maxpos;
  uint32_t adr;

  maxpos=(DIN_DMA.buf_len*2*DIN_DMA.max_buf_nr)-1;
  if(position>maxpos) position=maxpos;

  adr=DIN_DMA.buf_start_adr_a+(position*2); // *2 wegen 16bit

  // wert von port an position auslesen
  port_wert=*(volatile uint16_t*)(adr);
  ret_wert=(port_wert&0xF90A); // PC1, PC3, PC8, PC11, PC12, PC13, PC14, PC15

  return(ret_wert);
}



//--------------------------------------------------------------
// interne Funktionen
// init aller IO-Pins
//--------------------------------------------------------------
void P_DIN_DMA_InitIO(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure;

  //---------------------------------------------
  // init aller Pins
  // als normale GPIOs Eingaenge (mit PullUp)
  //---------------------------------------------

  // Clock Enable
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

  // Config als Digital-Eingang
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_3 | GPIO_Pin_8 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;

  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
}


//--------------------------------------------------------------
// interne Funktionen
// init vom Timer
//--------------------------------------------------------------
void P_DIN_DMA_InitTIM(void)
{
  //---------------------------------------------
  // init vom Timer
  // f�r GPIO-Daten per DMA
  //---------------------------------------------

  // Clock enable
  RCC_APB2PeriphClockCmd(DIM_DMA_TIM_CLOCK, ENABLE);

  // reinit
  UB_DIN_DMA_ReInitTimer();
}


//--------------------------------------------------------------
// interne Funktionen
// init vom DMA
//--------------------------------------------------------------
void P_DIN_DMA_InitDMA(void)
{
  DMA_InitTypeDef DMA_InitStructure;

  // blockieren
  DIN_DMA.dma_blocked=1;

  //---------------------------------------------
  // DMA per Timer-Update
  // (siehe Seite 305 vom Referenz Manual)
  // DMA=2, Channel=6, Stream=5
  //---------------------------------------------

  // Clock Enable (DMA)
  RCC_AHB1PeriphClockCmd(DIN_DMA_CLOCK, ENABLE);

  // DMA-Disable
  DMA_Cmd(DIN_DMA_STREAM, DISABLE);
  // warten bis DMA-Stream disable
  while(DMA_GetCmdStatus(DIN_DMA_STREAM)==ENABLE);
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
  DIN_DMA_STREAM->CR|=(uint32_t)DMA_SxCR_DBM;
  DIN_DMA_STREAM->M1AR=(uint32_t)DIN_DMA.buf_start_adr_b;

  // Flags loeschen
  DMA_ClearITPendingBit(DIN_DMA_STREAM, DIN_DMA_TCFLAG);
  DMA_ClearITPendingBit(DIN_DMA_STREAM, DIN_DMA_HTFLAG);

  // DMA enable
  DMA_Cmd(DIN_DMA_STREAM, ENABLE);
  // warten bis DMA-Stream enable
  while(DMA_GetCmdStatus(DIN_DMA_STREAM)==DISABLE);

  // freigeben
  DIN_DMA.dma_blocked=0;

}




//--------------------------------------------------------------
// interne Funktion
// init vom NVIC
//--------------------------------------------------------------
void P_DIN_DMA_InitNVIC(void)
{
  NVIC_InitTypeDef NVIC_InitStructure;

  //---------------------------------------------
  // init vom DMA Timer-Update
  // f�r TransferComplete Interrupt
  // und HalfTransferComplete Interrupt
  // DMA2, Stream5, Channel6
  //---------------------------------------------

  DMA_ITConfig(DIN_DMA_STREAM, DMA_IT_TC | DMA_IT_HT, ENABLE);

  // NVIC konfig
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
  TIM_DMACmd(DIN_DMA_TIM,TIM_DMA_Update,ENABLE); // DMA-Timer-Update enable

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
 // UB_Led_Toggle(LED_GREEN);

  if(DMA_GetITStatus(DIN_DMA_STREAM, DIN_DMA_HTFLAG))
  {
    // HalfTransferInterruptComplete Interrupt von DMA2 ist aufgetreten
    DMA_ClearITPendingBit(DIN_DMA_STREAM, DIN_DMA_HTFLAG);

    if(DIN_DMA.dma_blocked==0) {
      if((DIN_DMA_STREAM->CR & DMA_SxCR_CT) == 0) {
    	// BUF-A1
    	if(DIN_DMA.akt_buf_nr>0) {
          // Adresse von Buffer-B hochsetzen
          DIN_DMA.buf_start_adr_b+=DIN_DMA.buf_adr_delta;
          DIN_DMA_STREAM->M1AR=(uint32_t)DIN_DMA.buf_start_adr_b;
    	}
      }
      else {
    	// BUF-B1
        // Adresse von Buffer-A hochsetzen
    	DIN_DMA.buf_start_adr_a+=DIN_DMA.buf_adr_delta;
    	DIN_DMA_STREAM->M0AR=(uint32_t)DIN_DMA.buf_start_adr_a;
      }
    }
  }
  else if(DMA_GetITStatus(DIN_DMA_STREAM, DIN_DMA_TCFLAG))
  {
    // TransferInterruptComplete Interrupt von DMA2 ist aufgetreten
    DMA_ClearITPendingBit(DIN_DMA_STREAM, DIN_DMA_TCFLAG);

    if(DIN_DMA.dma_blocked==0) {
      if((DIN_DMA_STREAM->CR & DMA_SxCR_CT) != 0) {
      // BUF-A2
      }
      else {
    	// BUF-B2
    	// (Ende von einem Block)
        DIN_DMA.akt_buf_nr++;
        // check ob letzter Block errreicht
        if(DIN_DMA.akt_buf_nr>=DIN_DMA.max_buf_nr) {
          // timer stoppen
          TIM_Cmd(DIN_DMA_TIM, DISABLE);
          // adressen reset
          UB_DIN_DMA_ReInitDMA();
          DIN_DMA.akt_status=DIN_DMA_READY;
          // rote LED ausschalten
          //   UB_Led_Off(LED_RED);
        }
      }
    }
  }  
}





