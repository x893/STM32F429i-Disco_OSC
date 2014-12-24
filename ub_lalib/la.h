//--------------------------------------------------------------
// File     : la.h
//--------------------------------------------------------------

//--------------------------------------------------------------
#ifndef __STM32F4_UB_LA_H
#define __STM32F4_UB_LA_H


//--------------------------------------------------------------
// Includes
//--------------------------------------------------------------
#include "stm32f4xx.h"
#include "stm32_ub_led.h"
#include "stm32_ub_button.h"
#include "stm32_ub_din_dma.h"


//--------------------------------------------------------------
// Versions-Nummer (als String)
//--------------------------------------------------------------
#define   LA_VERSION  "V:D.03"



typedef enum {
  LA_COM_NONE =0,
  LA_COM_RESET,
  LA_COM_GET_ID,
  LA_COM_GET_META,
  LA_COM_ARM_TRIGGER,
  LA_COM_SET_FRQ,
  LA_COM_SET_SAMPLE,
  LA_COM_SET_MASK,
  LA_COM_SET_VALUE,
  LA_COM_SET_CH_EN,
  LA_COM_SET_CH_VIS,
  LA_COM_UNKNOWN
}LA_COM_t;


//--------------------------------------------------------------
// Benutzung vom LCD/Touch kann hier abgeschaltet werden
// (ohne LCD -> Steuerung nur per Com-Schnittstelle)
//--------------------------------------------------------------
//#define   LA_USE_LCD   0  // ohne LCD+Touch
#define   LA_USE_LCD   1  // mit LCD+Touch


#if LA_USE_LCD==1
  #include "gui.h"
#endif




//--------------------------------------------------------------
// Benutzung der COM-Schnittstelle kann hier abgeschaltet werden
// (ohne COM -> Steuerung nur per LCD/Touch)
//--------------------------------------------------------------
//#define   LA_USE_COM   0  // ohne COM
#define   LA_USE_COM   1  // mit COM


#if LA_USE_COM==1
  #include "com.h"
#endif


//--------------------------------------------------------------
#if LA_USE_LCD == 0 && LA_USE_COM==0
#error aktivate LCD or COM.
#endif



//--------------------------------------------------------------
#define  LA_CHANNEL_ANZ          8  // Anzahl der Kanäle (0...7)


//--------------------------------------------------------------
#define  LA_SETTING_CH_MAX       2047 // [0...2047]
#define  LA_SETTING_TRG_MAX      2    // [0...2]

#define  LA_SUMP_CALC_FRQ        1000000000 // 1 GHz


//--------------------------------------------------------------
typedef enum {
  Trigger_NONE =0,
  Trigger_LO,
  Trigger_HI
}Trigger_Mode_t;



typedef struct {
  bool enable;
  uint16_t bitmask;
  bool visible;
  Trigger_Mode_t trigger;
  uint16_t ypos;
  uint8_t old_pegel;
}Channel_t;

typedef struct {
  uint16_t maske_c;
  uint16_t value_c;
  uint8_t hi_maske_c;
  uint8_t lo_maske_c;
  uint8_t hi_maske_e;
  uint8_t lo_maske_e;
  uint32_t pos;
}Trigger_t;



//--------------------------------------------------------------
// Struktur von einem Untermenu
//--------------------------------------------------------------
typedef struct {
  uint32_t frq;         // frq
  uint16_t prescale;    // prescale
  uint16_t periode;     // periode
}LA_TIM_Item_t;

//--------------------------------------------------------------
// Struktur von einem Untermenu
//--------------------------------------------------------------
typedef struct {
  uint32_t sump_len;  // laenge von sump
  uint32_t len;       // laenge in bytes
  uint16_t size;      // buffer size
  uint16_t anz;       // buffer anzahl
}LA_SAMPLE_Item_t;




typedef struct {
  uint8_t frq_anz;
  uint8_t frq;
  uint8_t sample_anz;
  uint8_t sample;
  uint16_t ch_en;
  uint16_t ch_vis;
}Setting_t;

typedef struct {
  uint8_t id;
  uint8_t b0;
  uint8_t b1;
  uint8_t b2;
  uint8_t b3;
}SUMP_t;

typedef struct {
  Channel_t ch[LA_CHANNEL_ANZ];
  Trigger_t trigger;
  Setting_t setting;
  SUMP_t sump;
}LA_t;
LA_t LA;





//--------------------------------------------------------------
// Globale Funktionen
//--------------------------------------------------------------
void la_init(void);
void la_check_trigger(void);
uint8_t la_readPegel(uint16_t ch, uint32_t adr);
void la_update_settings(LA_COM_t com_status);
uint8_t la_readData(uint32_t adr);


//--------------------------------------------------------------
#endif // __STM32F4_UB_LA_H
