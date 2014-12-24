//--------------------------------------------------------------
// File     : la.c
// Datum    : 01.10.2013
// Version  : 1.0
// Autor    : UB
// EMail    : mc-4u(@)t-online.de
// Web      : www.mikrocontroller-4u.de
// CPU      : STM32F429
// IDE      : CooCox CoIDE 1.7.4
// GCC      : 4.7 2012q4
// Module   : 
// Funktion : 
//--------------------------------------------------------------


//--------------------------------------------------------------
// Includes
//--------------------------------------------------------------
#include "la.h"

//--------------------------------------------------------------
// Settings Frq
//--------------------------------------------------------------
const LA_TIM_Item_t LA_TIM_Item[] = {
  // frq, prescale, periode
  {    1000,167,999}, // 1kHz
  {    2500,167,399}, // 2,5kHz
  {    5000,167,199}, // 5kHz
  {   10000,167,99},  // 10kHz
  {   25000,167,39},  // 25kHz (default)
  {   50000,167,19},  // 50kHz
  {  100000,167,9},   // 100kHz
  {  250000,41,15},   // 250kHz
  {  500000,41,7},    // 500kHz
  { 1000000,20,7},    // 1MHz
  { 2000000,1,41},    // 2MHz
  { 4000000,1,20},    // 4MHz
  { 6000000,1,13},    // 6MHz
  { 8000000,2,6},     // 8MHz
  {10500000,3,3},     // 10,5MHz
  {12000000,1,6},     // 12MHz
  {14000000,2,3},     // 14MHz
  {21000000,1,3},     // 21MHz
  {24000000,0,6},     // 24MHz
};
uint32_t frq_item_anz=sizeof(LA_TIM_Item)/sizeof(LA_TIM_Item[0]);


//--------------------------------------------------------------
// Settings Sample
//--------------------------------------------------------------
const LA_SAMPLE_Item_t LA_SAMPLE_Item[] = {
  // sump  ,len    , size,anzahl
  {  0x007F,    512,  256,1},  // 512 S
  {  0x00FF,   1024,  512,1},  // 1024 1k
  {  0x01FF,   2048, 1024,1},  // 2048 2k
  {  0x03FF,   4096, 2048,1},  // 4096 4k
  {  0x07FF,   8192, 4096,1},  // 8192 8k (default)
  {  0x0FFF,  16384, 8192,1},  // 16384 16k
  {  0x1FFF,  32768,16384,1},  // 32768 32k
  {  0x4FFF,  65536,32768,1},  // 65536 64k
  {  0x7FFF, 131072,32768,2},  // 131072 128k
  {  0xFFFF, 262144,32768,4},  // 262144 256k
  {  0xE847, 500000,50000,5},  // 5000000 500T
  {  0xD08F,1000000,50000,10}, // 1000000 1M
};
uint32_t sampl_item_anz=sizeof(LA_SAMPLE_Item)/sizeof(LA_SAMPLE_Item[0]);


//--------------------------------------------------------------
// interne Funktionen
//--------------------------------------------------------------
void p_la_init_sw(void);
void p_la_init_hw(void);
void p_la_update_setting_frq(void);
void p_la_update_setting_sample(void);
void p_la_update_setting_mask(void);
void p_la_update_setting_value(void);

//--------------------------------------------------------------
// init vom Logic-Analyzer
//--------------------------------------------------------------
void la_init(void)
{
  // init der Hardware (vor der Software)
  p_la_init_hw();

  // init der Software
  p_la_init_sw();
}


//--------------------------------------------------------------
void la_check_trigger(void)
{
  uint16_t port_c,trg_wert_c;

  port_c=*(volatile uint16_t*)(DIN_DMA_GPIO_IDR_ADDRESS);
  trg_wert_c=(port_c^LA.trigger.value_c); // gleiche bits werden zu 0
  trg_wert_c&=LA.trigger.maske_c; // unnoetige bits werden zu 0

  if(trg_wert_c==0x00) {
    UB_DIN_DMA_Capture();
  }
}

//--------------------------------------------------------
uint8_t la_readPegel(uint16_t ch, uint32_t adr)
{
  uint8_t ret_wert=0;
  uint16_t maske=0,wert=0;

  maske=LA.ch[ch].bitmask;

  wert=UB_DIN_DMA_Read_PORT(adr);
  if((wert&maske)!=0) ret_wert=1;

  return(ret_wert);
}



//--------------------------------------------------------------
uint8_t la_readData(uint32_t adr)
{
  uint8_t ret_wert=0;
  uint16_t wert;

  wert=UB_DIN_DMA_Read_PORT(adr);
  if((wert&LA.ch[0].bitmask)!=0) ret_wert|=0x01;
  if((wert&LA.ch[1].bitmask)!=0) ret_wert|=0x02;
  if((wert&LA.ch[2].bitmask)!=0) ret_wert|=0x04;
  if((wert&LA.ch[3].bitmask)!=0) ret_wert|=0x08;
  if((wert&LA.ch[4].bitmask)!=0) ret_wert|=0x10;
  if((wert&LA.ch[5].bitmask)!=0) ret_wert|=0x20;
  if((wert&LA.ch[6].bitmask)!=0) ret_wert|=0x40;
  if((wert&LA.ch[7].bitmask)!=0) ret_wert|=0x80;

  return(ret_wert);
}




//--------------------------------------------------------------
void la_update_settings(LA_COM_t com_status)
{
  uint16_t n,maske;

  if(DIN_DMA.akt_status==DIN_DMA_WAITING) {
    if(com_status==LA_COM_SET_FRQ) {
      p_la_update_setting_frq();
    }

    if(com_status==LA_COM_SET_SAMPLE) {
   	  p_la_update_setting_sample();
    }

    if(com_status==LA_COM_SET_MASK) {
   	  p_la_update_setting_mask();
    }

    if(com_status==LA_COM_SET_VALUE) {
   	  p_la_update_setting_value();
    }

    if(com_status==LA_COM_SET_CH_EN) {
      maske=0x01;
      for(n=0;n<LA_CHANNEL_ANZ;n++) {
        if((LA.setting.ch_en&maske)!=0) {
          LA.ch[n].enable=true;
        }
        else {
          LA.ch[n].enable=false;
        }
        maske=(maske<<1);
      }
    }

    if(com_status==LA_COM_SET_CH_VIS) {
      maske=0x01;
      for(n=0;n<LA_CHANNEL_ANZ;n++) {
        if((LA.setting.ch_vis&maske)!=0) {
          LA.ch[n].visible=true;
        }
        else {
          LA.ch[n].visible=false;
        }
        maske=(maske<<1);
      }
    }
  }
}


//--------------------------------------------------------------
void p_la_update_setting_frq(void)
{
  uint16_t n;
  uint32_t wert,frq;

  if(LA.sump.id==SUMP_CMD_ID_SET_FRQ) {
    // zuerst auf default
    LA.setting.frq=0;

    // devider
    wert=(LA.sump.b2<<16);
    wert|=(LA.sump.b1<<8);
    wert|=LA.sump.b0;
    wert+=1;
    // wert als frq
    frq=(LA_SUMP_CALC_FRQ/wert);
    // eintrag suchen
    for(n=0;n<LA.setting.frq_anz;n++) {
      if(frq>=LA_TIM_Item[n].frq) {
        // wert speichern
        LA.setting.frq=n;
      }
    }
  }

  DIN_DMA.tim_prescale=LA_TIM_Item[LA.setting.frq].prescale;
  DIN_DMA.tim_periode=LA_TIM_Item[LA.setting.frq].periode;

  UB_DIN_DMA_ReInitTimer();
}


//--------------------------------------------------------------
void p_la_update_setting_sample(void)
{
  uint16_t n;
  uint32_t wert;

  if(LA.sump.id==SUMP_CMD_ID_SET_LEN) {
    // zuerst auf default
    LA.setting.sample=0;

    // anzahl der sample werte
    wert=(LA.sump.b1<<8);
    wert|=LA.sump.b0;
    // eintrag suchen (genauer wert)
    for(n=0;n<LA.setting.sample_anz;n++) {
      if(wert==LA_SAMPLE_Item[n].sump_len) {
        // wert speichern
        LA.setting.sample=n;
        break;
      }
    }
  }

  DIN_DMA.buf_len=LA_SAMPLE_Item[LA.setting.sample].size;
  DIN_DMA.buf_anz=LA_SAMPLE_Item[LA.setting.sample].anz;

  UB_DIN_DMA_ReInitDMA();
}


//--------------------------------------------------------------
void p_la_update_setting_mask(void)
{
  if(LA.sump.id==SUMP_CMD_ID_SET_MASK0) {
    LA.trigger.maske_c=0x00;
    if((LA.sump.b0&0x01)!=0) LA.trigger.maske_c|=LA.ch[0].bitmask;
    if((LA.sump.b0&0x02)!=0) LA.trigger.maske_c|=LA.ch[1].bitmask;
    if((LA.sump.b0&0x04)!=0) LA.trigger.maske_c|=LA.ch[2].bitmask;
    if((LA.sump.b0&0x08)!=0) LA.trigger.maske_c|=LA.ch[3].bitmask;
    if((LA.sump.b0&0x10)!=0) LA.trigger.maske_c|=LA.ch[4].bitmask;
    if((LA.sump.b0&0x20)!=0) LA.trigger.maske_c|=LA.ch[5].bitmask;
    if((LA.sump.b0&0x40)!=0) LA.trigger.maske_c|=LA.ch[6].bitmask;
    if((LA.sump.b0&0x80)!=0) LA.trigger.maske_c|=LA.ch[7].bitmask;
  }
}

//--------------------------------------------------------------
void p_la_update_setting_value(void)
{
  if(LA.sump.id==SUMP_CMD_ID_SET_VAL0) {
    LA.trigger.value_c=0x00;
    if((LA.sump.b0&0x01)!=0) LA.trigger.value_c|=LA.ch[0].bitmask;
    if((LA.sump.b0&0x02)!=0) LA.trigger.value_c|=LA.ch[1].bitmask;
    if((LA.sump.b0&0x04)!=0) LA.trigger.value_c|=LA.ch[2].bitmask;
    if((LA.sump.b0&0x08)!=0) LA.trigger.value_c|=LA.ch[3].bitmask;
    if((LA.sump.b0&0x10)!=0) LA.trigger.value_c|=LA.ch[4].bitmask;
    if((LA.sump.b0&0x20)!=0) LA.trigger.value_c|=LA.ch[5].bitmask;
    if((LA.sump.b0&0x40)!=0) LA.trigger.value_c|=LA.ch[6].bitmask;
    if((LA.sump.b0&0x80)!=0) LA.trigger.value_c|=LA.ch[7].bitmask;
  }
}

//--------------------------------------------------------------
// interne Funktion
// init aller Software Komponenten
//--------------------------------------------------------------
void p_la_init_sw(void)
{
	uint32_t n;

  LA.setting.frq_anz=frq_item_anz;
  LA.setting.frq=4;     // 4=25kHz
  LA.setting.sample_anz=sampl_item_anz;
  LA.setting.sample=4;  // 4=8 kSamples
  LA.setting.ch_en=31;  // 15=ch1 bis ch5
  LA.setting.ch_vis=31; // 15=ch1 bis ch5

  //--------------------------------------
  LA.trigger.maske_c=0x00;
  LA.trigger.value_c=0x00;
  LA.trigger.hi_maske_c=0x00;
  LA.trigger.lo_maske_c=0xFF;
  LA.trigger.hi_maske_e=0x00;
  LA.trigger.lo_maske_e=0xFF;

  for(n=0;n<LA_CHANNEL_ANZ;n++) {
    LA.ch[n].enable=false;
    LA.ch[n].bitmask=0;
    LA.ch[n].visible=false;
    LA.ch[n].trigger=Trigger_NONE;
    LA.ch[n].ypos=10;
    LA.ch[n].old_pegel=2;
  }
  LA.ch[0].trigger=Trigger_LO;


  LA.ch[0].bitmask=0x0002; // PC1
  LA.ch[1].bitmask=0x0008; // PC3
  LA.ch[2].bitmask=0x0100; // PC8
  LA.ch[3].bitmask=0x0800; // PC11
  LA.ch[4].bitmask=0x1000; // PC12
  LA.ch[5].bitmask=0x2000; // PC13
  LA.ch[6].bitmask=0x4000; // PC14
  LA.ch[7].bitmask=0x8000; // PC15

  //--------------------------------------
  LA.sump.id=0x00;
  LA.sump.b0=0x00;
  LA.sump.b1=0x00;
  LA.sump.b2=0x00;
  LA.sump.b3=0x00;

  //--------------------------------------
  la_update_settings(LA_COM_SET_FRQ);
  la_update_settings(LA_COM_SET_SAMPLE);
  la_update_settings(LA_COM_SET_CH_EN);
  la_update_settings(LA_COM_SET_CH_VIS);

}

//--------------------------------------------------------------
// interne Funktion
// init aller Hardware Komponenten
//--------------------------------------------------------------
void p_la_init_hw(void)
{
  // init der LEDs
  UB_Led_Init();

  // init vom User-Button
  UB_Button_Init();

  // init vom DIN per DMA
  UB_DIN_DMA_Init();

}







