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
#include "stm32_ub_systick.h"
#include "stm32_ub_led.h"
#include "stm32_ub_din_dma.h"




//--------------------------------------------------------------
// enums der Kommunikation
//--------------------------------------------------------------
typedef enum {
	LA_COM_NONE = 0,	// kein Kommando
	LA_COM_RESET,		// Reset-Kommando
	LA_COM_GET_ID,		// Abfrage der ID
	LA_COM_GET_META,	// Abfrage der Meta-Daten
	LA_COM_ARM_TRIGGER,	// Trigger scharfstellen
	LA_COM_SET_FRQ,		// Samplerate Kommando
	LA_COM_SET_SIZE,	// Capturesize Kommando
	LA_COM_SET_MASK,	// Trigger-Maske
	LA_COM_SET_VALUE,	// Trigger-Value
	LA_COM_UNKNOWN		// Kommando unbekannt
} LA_COM_t;

#include "com.h" // include nach "LA_COM_t"




//--------------------------------------------------------------
// SUMP-Config : "device.channel.count" (dont change)
//--------------------------------------------------------------
#define  LA_CHANNEL_ANZ          8  // Anzahl der Kanäle (0...7)





//--------------------------------------------------------------
// SUMP-Config "device.dividerClockspeed" (dont change)
//--------------------------------------------------------------
#define  LA_SUMP_CALC_FRQ        1000000000 // 1 GHz



//--------------------------------------------------------------
#define  BLINK_DELAY_MS   100

//--------------------------------------------------------------
// Trigger-Mode von einem Channel
//--------------------------------------------------------------
typedef enum {
	Trigger_NONE = 0,  // dont care
	Trigger_LO,       // trigger bei Lo-Pegel
	Trigger_HI        // trigger bei Hi-Pegel
}Trigger_Mode_t;


//--------------------------------------------------------------
// Struktur von einem Channel
//--------------------------------------------------------------
typedef struct {
	uint16_t bitmask;        // Port-Bitmaske            
	Trigger_Mode_t trigger;  // Trigger-Mode    
}Channel_t;


//--------------------------------------------------------------
// Struktur vom Trigger
//--------------------------------------------------------------
typedef struct {
	uint16_t maske;  // Maske (0=dont care, 1=trigger)
	uint16_t value;  // Value (0=Lo-Pegel, 1=Hi-Pegel)
}Trigger_t;




//--------------------------------------------------------------
// Struktur fuer die "samplerate"
//--------------------------------------------------------------
typedef struct {
	uint32_t frq;         // Sample-Frq in Hz
	uint16_t prescale;    // prescale Einstellung vom Timer
	uint16_t periode;     // periode Einstellung vom Timer
}LA_TIM_Item_t;




//--------------------------------------------------------------
// Struktur fuer die "capturesize"
//--------------------------------------------------------------
typedef struct {
	uint32_t sump_len;  // laenge (von sump uebermittelt)
	uint32_t len;       // laenge in bytes
	uint16_t size;      // buffer size (max 65535)
	uint16_t anz;       // buffer anzahl
}LA_SAMPLE_Item_t;



//--------------------------------------------------------------
// Struktur der Settings
//--------------------------------------------------------------
typedef struct {
	uint8_t frq_anz;       // Anzahl der Eintrage von "LA_TIM_Item_t"
	uint8_t frq;           // gewaehlter Eintrag
	uint8_t sample_anz;    // Anzahl der Eintrage von "LA_SAMPLE_Item_t"
	uint8_t sample;        // gewaehlter Eintrag
}Setting_t;


//--------------------------------------------------------------
// Struktur fuer Sump Protokoll
//--------------------------------------------------------------
typedef struct {
	uint8_t id;   // ID
	uint8_t b0;   // Byte-0
	uint8_t b1;   // Byte-1
	uint8_t b2;   // Byte-2
	uint8_t b3;   // Byte-3
}SUMP_t;


//--------------------------------------------------------------
// Struktur vom Logic-Analyzer
//--------------------------------------------------------------
typedef struct {
	Channel_t ch[LA_CHANNEL_ANZ];
	Trigger_t trigger;
	Setting_t setting;
	SUMP_t sump;
}LA_t;
extern LA_t LA;





//--------------------------------------------------------------
// Globale Funktionen
//--------------------------------------------------------------
void la_init(void);
void la_check_trigger(void);
void la_update_settings(LA_COM_t com_status);
uint8_t la_readData_8b(uint32_t adr);


//--------------------------------------------------------------
#endif // __STM32F4_UB_LA_H
