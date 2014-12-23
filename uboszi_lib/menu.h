//--------------------------------------------------------------
// File     : menu.h
//--------------------------------------------------------------

//--------------------------------------------------------------
#ifndef __STM32F4_UB_MENU_H
#define __STM32F4_UB_MENU_H


//--------------------------------------------------------------
// Includes
//--------------------------------------------------------------
#include "stm32f4xx.h"
#include "stm32_ub_lcd_ili9341.h"
#include "stm32_ub_touch_stmpe811.h"
#include "stm32_ub_font.h"
#include "stm32_ub_graphic2d.h"
#include <stdio.h>
#include <string.h>

//--------------------------------------------------------------
// Defines der Farben
//--------------------------------------------------------------
#define  MENU_VG_COL        RGB_COL_WHITE     // Menu Schrift-Farbe
#define  MENU_BG_COL        RGB_COL_BLUE      // Menu Hintergrund
#define  MENU_AK_COL        RGB_COL_RED       // Aktiver Hintergrund

//--------------------------------------------------------------
// Defines der Schrift (nicht �ndern !!)
//--------------------------------------------------------------
#define  FONT_H             10 // hoehe der Schriftart
#define  FONT_W             7  // breite der Schriftart

//--------------------------------------------------------------
// Defines zum Umrechnen von Spannungen
//--------------------------------------------------------------
#define  FAKTOR_ADC            3.0/4095

//--------------------------------------------------------------
// Defines zum Umrechnen von Spannungen
//--------------------------------------------------------------
#define  VFAKTOR_5V             5.0/25
#define  VFAKTOR_2V             2.0/25
#define  VFAKTOR_1V             1.0/25
#define  VFAKTOR_0V5            0.5/25
#define  VFAKTOR_0V2            0.2/25
#define  VFAKTOR_0V1            0.1/25


//--------------------------------------------------------------
// Defines zum umrechnen von Zeiten
//--------------------------------------------------------------
#define  TFAKTOR_5				5.0f / (4095 / 12)
#define  TFAKTOR_2				2.0f / (4095 / 12)
#define  TFAKTOR_1				1.0f / (4095 / 12)
#define  TFAKTOR_500			500.0f / (4095 / 12)
#define  TFAKTOR_200			200.0f / (4095 / 12)
#define  TFAKTOR_100			100.0f / (4095 / 12)
#define  TFAKTOR_50				50.0f / (4095 / 12)
#define  TFAKTOR_20				20.0f / (4095 / 12)
#define  TFAKTOR_10				10.0f / (4095 / 12)
#define  TFAKTOR_25				25.0f / (4095 / 12)


//--------------------------------------------------------------
// Defines zum umrechnen von FFT-Werte
//--------------------------------------------------------------
#define  FFAKTOR_5s              5.0f / 4095 / 2
#define  FFAKTOR_2s             12.5f / 4095 / 2
#define  FFAKTOR_1s             25.0f / 4095 / 2
#define  FFAKTOR_500m           50.0f / 4095 / 2
#define  FFAKTOR_200m          125.0f / 4095 / 2
#define  FFAKTOR_100m          250.0f / 4095 / 2
#define  FFAKTOR_50m           500.0f / 4095 / 2
#define  FFAKTOR_20m          1250.0f / 4095 / 2
#define  FFAKTOR_10m          2500.0f / 4095 / 2
#define  FFAKTOR_5m           5000.0f / 4095 / 2
#define  FFAKTOR_2m          12500.0f / 4095 / 2
#define  FFAKTOR_1m          25000.0f / 4095 / 2
#define  FFAKTOR_500u        50000.0f / 4095 / 2
#define  FFAKTOR_200u          125.0f / 4095 / 2
#define  FFAKTOR_100u          250.0f / 4095 / 2
#define  FFAKTOR_50u           500.0f / 4095 / 2
#define  FFAKTOR_25u          1000.0f / 4095 / 2

//--------------------------------------------------------------
// Anzahl der Nachkommastellen bei Float
//  Faktor ->  100 = 2 Nachkommastellen,  Formatierung -> "%d.%02d"
//  Faktor -> 1000 = 3 Nachkommastellen,  Formatierung -> "%d.%03d"
//  usw
//--------------------------------------------------------------
#define  STRING_FLOAT_FAKTOR	100			// 100 = 2 Nachkommastellen
#define  STRING_FLOAT_FORMAT	"%d.%02d"	// Formatierung
#define  STRING_FLOAT_FORMAT2	"-%d.%02d"	// Formatierung


//--------------------------------------------------------------
// position der GUI
//--------------------------------------------------------------
#define  GUI_YPOS       15
#define  GUI_XPOS_OFF   0
#define  GUI_XPOS_LEFT  10
#define  GUI_XPOS_MID   108
#define  GUI_XPOS_RIGHT 204

//--------------------------------------------------------------
// Button der GUI
//--------------------------------------------------------------
typedef enum {
	GUI_BTN_NONE =0,
	GUI_BTN_UP,
	GUI_BTN_DOWN,
	GUI_BTN_LEFT,
	GUI_BTN_RIGHT
} GUI_Button_t;

//--------------------------------------------------------------
// returnwerte der GUI
//--------------------------------------------------------------
typedef enum {
	MENU_NO_CHANGE =0,
	MENU_CHANGE_GUI,
	MENU_CHANGE_NORMAL,
	MENU_CHANGE_FRQ,
	MENU_CHANGE_MODE,
	MENU_CHANGE_VALUE,
	MENU_SEND_DATA
} MENU_Status_t;

//--------------------------------------------------------------
// Main-Men�-Punkte
//--------------------------------------------------------------
typedef enum {
	MM_NONE =0,
	MM_CH1,
	MM_CH2,
	MM_TIME,
	MM_SETTING,
	MM_TRG_SOURCE,
	MM_TRG_EDGE,
	MM_TRG_MODE,
	MM_TRG_VAL,
	MM_TRG_RESET,
	MM_CH_VIS,
	MM_CH_POS,
	MM_CUR_MODE,
	MM_CUR_P1,
	MM_CUR_P2,
	MM_SEND_MODE,
	MM_SEND_SCREEN,
	MM_SEND_DATA,
	MM_FFT_MODE
} MM_Akt_Item_t;

//--------------------------------------------------------------
#define SETTING_TRIGGER  0
#define SETTING_CH1      1
#define SETTING_CH2      2
#define SETTING_CURSOR   3
#define SETTING_FFT      4
#define SETTING_SEND     5
#define SETTING_VERSION  6
#define SETTING_HELP     7

typedef struct {
	const char	*Text;		// linke Seite vom Menu-Text
	uint16_t	YPos;		// Ypos to which the menu is drawn
	uint16_t	um_cnt;		// Number of menu sub-items
} MM_Item_t;

typedef struct {
	const char *Text;		// Text
} SM_Item_t;


typedef struct {
  uint16_t	Factor;		// current prescaler (5V, 2V, 1V, etc)
  uint16_t	Visible;	// visible (On, Off)
  int16_t	Position;	// Y-position
} Channel_t;

typedef struct {
	uint16_t Value;		// Wert Timebase (5s,2s,1s,500ms usw)
} Timebase_t;

enum MENU_TRIGGER_MODE_e {
	MENU_TRIGGER_MODE_NORMAL	= 0,
	MENU_TRIGGER_MODE_AUTO		= 1,
	MENU_TRIGGER_MODE_SINGLE	= 2,
	MENU_TRIGGER_MODE_LAST
};

enum MENU_CH_VISIBLE_e {
	MENU_CH_VISIBLE_ON	= 0,
	MENU_CH_VISIBLE_OFF	= 1,
	MENU_CH_VISIBLE_LAST
};

enum MENU_SEND_MODE_e {
	MENU_SEND_MODE_CH1			= 0,
	MENU_SEND_MODE_CH1_FFT		= 1,
	MENU_SEND_MODE_CH2			= 2,
	MENU_SEND_MODE_CH2_FFT		= 3,
	MENU_SEND_MODE_CH1_CH2		= 4,
	MENU_SEND_MODE_CH1_CH2_FFT	= 5,
	MENU_SEND_MODE_SCREEN		= 6,
	MENU_SEND_MODE_LAST
};

typedef struct {
	uint16_t Source;	// aktuelle Source (CH1, CH2)
	uint16_t Edge;		// Trigger-Flanke (HI, LO)
	uint16_t Mode;		// Mode (Normal, Auto, Single)
	uint16_t Single;	// Single-Status
	uint16_t ValueCh1;	// Trigger-Value (CH1)
	uint16_t ValueCh2;	// Trigger-Value (CH2)
} Trigger_t;

enum  MENU_CURSOR_MODE_e {
	MENU_CURSOR_MODE_OFF	= 0,
	MENU_CURSOR_MODE_CH1	= 1,
	MENU_CURSOR_MODE_CH2	= 2,
	MENU_CURSOR_MODE_TIME	= 3,
	MENU_CURSOR_MODE_FFT	= 4,
	MENU_CURSOR_MODE_LAST
};

enum MENU_TRIGGER_SOURCE_e {
	MENU_TRIGGER_SOURCE_CH1	= 0,
	MENU_TRIGGER_SOURCE_CH2	= 1,
	MENU_TRIGGER_SOURCE_LAST
};

typedef struct {
	uint16_t Mode;	// aktueller Mode (Off, CH1, CH2, Time)
	uint16_t P1;		// Wert Cursor-A (bei CH1 oder CH2)
	uint16_t P2;		// Wert Cursor-B (bei CH1 oder CH2)
	uint16_t T1;		// Wert Cursor-A (bei Time)
	uint16_t T2;		// Wert Cursor-B (bei Time)
	uint16_t F1;		// Wert Cursor-A (bei FFT)
} Cursor_t;

typedef struct {
	uint16_t Mode;		// Mode (CH1,CH2,CH1+CH2)
	uint16_t Screen;	// Screen
	uint16_t Data;		// Datamode (Offline, Start)
} Send_t;

typedef struct {
	uint16_t Mode;		// Mode (Off,CH1,CH2)
} FFT_t;

typedef struct {
	uint16_t Transparency;		// aktuelle Transparenz
	uint16_t Setting;			// Active Setting
	uint16_t OldX;
	uint16_t OldY;
	uint16_t GuiChanged;

	Channel_t	Ch1;			// Daten "Channel-1"
	Channel_t	Ch2;			// Daten "Channel-2"
	Timebase_t	Timebase;		// Daten "Timebase"
	Trigger_t	Trigger;		// Daten "Trigger"
	Cursor_t	Cursor;			// Daten "Cursor"
	Send_t		Send;			// Daten "Send"
	FFT_t		FFT;			// Daten "FFT"
} Menu_t;
extern Menu_t Menu;

typedef struct {
	MM_Akt_Item_t	MenuActive;
	MM_Akt_Item_t	MenuOld;
	GUI_Button_t	ButtonActive;
	GUI_Button_t	ButtonOld;
	uint16_t		GuiXPpos;
} GUI_t;
extern GUI_t GUI;

void menu_draw_all(void);
MENU_Status_t menu_check_touch(void);


//--------------------------------------------------------------
#endif // __STM32F4_UB_MENU_H
