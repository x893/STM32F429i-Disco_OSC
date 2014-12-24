//--------------------------------------------------------------
// File     : gui.h
//--------------------------------------------------------------

//--------------------------------------------------------------
#ifndef __STM32F4_UB_GUI_H
#define __STM32F4_UB_GUI_H


//--------------------------------------------------------------
// Includes
//--------------------------------------------------------------
#include "stm32f4xx.h"
#include "stm32_ub_lcd_ili9341.h"
#include "stm32_ub_font.h"
#include "stm32_ub_touch_stmpe811.h"
#include "stm32_ub_graphic2d.h"
#include "stm32_ub_systick.h"
#include <stdio.h>
#include "la.h"

//--------------------------------------------------------------
#define    MENU_BG_COL   RGB_COL_BLACK   // Farbe vom Hintergrund
#define    MENU_VG_COL   RGB_COL_WHITE   // Schriftfarbe





//--------------------------------------------------------------
#define   GUI_DELAY_MS     50  // refreshzeit der GUI/Touch


//--------------------------------------------------------------
#define  GUI_CH_LCDPIXEL_ANZ    250  // Anzahl der Pixel fuer die Anzeige auf dem LCD


//--------------------------------------------------------------
#define   GRAPH_HEIGHT           20
#define   GRAPH_START_X          30
#define   GRAPH_START_Y          210
#define   GRAPH_DISTANCE         40
#define   GRAPH_MAX               5   // max 5 signale


//--------------------------------------------------------------
// Struktur von einem Untermenu
//--------------------------------------------------------------
typedef struct {
  char *txt;           // Text
  float faktor;        // faktor
  uint32_t step;       // step
}XF_Item_t;


//--------------------------------------------------------------
// Struktur von einem Untermenu
//--------------------------------------------------------------
typedef struct {
  char *txt;           // Text
}TXT_Item_t;


//--------------------------------------------------------------
// Struktur von einem Untermenu
//--------------------------------------------------------------
typedef struct {
  char *txt;           // Text
  uint16_t color;      // farbe
}CH_Item_t;



//--------------------------------------------------------------
// returnwerte der GUI
//--------------------------------------------------------------
typedef enum {
  MENU_NO_CHANGE =0,
  MENU_CHANGE_GUI
}MENU_Status_t;


//--------------------------------------------------------------
// Struktur von "Menu"
//--------------------------------------------------------------
typedef struct {
  uint32_t akt_transparenz;  // aktuelle Transparenz
  uint32_t xpos_start;
  uint32_t xadr;
  uint32_t xfaktor;
  float xdelta;
}Menu_t;
Menu_t Menu;



//--------------------------------------------------------------
// position der GUI
//--------------------------------------------------------------
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
  GUI_BTN_RIGHT,
  GUI_BTN_SETUP
}GUI_Button_t;


typedef struct {
  GUI_Button_t name;
  uint16_t x1;
  uint16_t y1;
  uint16_t x2;
  uint16_t y2;
}GUI_BTN_POS_t;


//--------------------------------------------------------------
// Struktur der "GUI"
//--------------------------------------------------------------
typedef struct {
  uint32_t gui_xpos;
  GUI_Button_t akt_button;
  GUI_Button_t old_button;
}GUI_t;
GUI_t GUI;



//--------------------------------------------------------------
typedef enum {
  GUI_CHANGE_NONE =0,     // keine Aenderung
  GUI_CHANGE_FRQ          // FRQ-Wert aendern
}GUI_CHANGE_t;



typedef enum {
  GUI_DRAW_NONE =0,  // nichts zeichnen
  GUI_DRAW_NORMAL,
  GUI_DRAW_AT_TRIGGER_POS
}GUI_DRAW_t;

//--------------------------------------------------------------
// Globale Funktionen
//--------------------------------------------------------------
void gui_init(void);
GUI_CHANGE_t gui_do(GUI_DRAW_t draw_status);
void gui_show_status(uint8_t mode);



//--------------------------------------------------------------
#endif // __STM32F4_UB_GUI_H
