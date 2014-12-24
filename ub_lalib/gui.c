//--------------------------------------------------------------
// File     : gui.c
// Datum    : 01.10.2013
// Version  : 1.0
// Autor    : UB
// EMail    : mc-4u(@)t-online.de
// Web      : www.mikrocontroller-4u.de
// CPU      : STM32F429
// IDE      : CooCox CoIDE 1.7.4
// GCC      : 4.7 2012q4
// Module   : 
// Funktion : Menu vom Logic-Analyzer
//--------------------------------------------------------------


//--------------------------------------------------------------
// Includes
//--------------------------------------------------------------
#include "gui.h"


//--------------------------------------------------------------
// globale variable
//--------------------------------------------------------------
char str_buf[50];


//--------------------------------------------------------------
// Untermenu :
//--------------------------------------------------------------
const XF_Item_t XF_Item[] = {
  // TXT, Faktor, Step
  {"0x1  ",0.1,        1},
  {"0x25 ",0.25,       2},
  {"0x5  ",0.5,        5},
  {"1x   ",1.0,       10},
  {"2x   ",2.0,       20},
  {"4x   ",4.0,       40},
  {"8x   ",8.0,       80},
  {"16x  ",16.0,     160},
  {"32x  ",32.0,     320},
  {"64x  ",64.0,     640},
  {"128x ",128.0,   1280},
  {"256x ",256.0,   2560},
  {"512x ",512.0,   5120},
  {"1024x",1024.0, 10240},
};
uint32_t xf_item_anz=sizeof(XF_Item)/sizeof(XF_Item[0]);



//--------------------------------------------------------------
// GUI-Buttons
//--------------------------------------------------------------
const GUI_BTN_POS_t GUI_BTN_POS[] = {
  // Name, x1, y1, x2, y2
  {GUI_BTN_DOWN,0,0,50,101},
  {GUI_BTN_UP,51,0,100,101},
  {GUI_BTN_LEFT,101,0,195,50},
  {GUI_BTN_RIGHT,101,51,195,101},
  {GUI_BTN_SETUP,196,0,239,44},
};
uint32_t btn_item_anz=sizeof(GUI_BTN_POS)/sizeof(GUI_BTN_POS[0]);


//--------------------------------------------------------------
// Settings Frq
//--------------------------------------------------------------
const TXT_Item_t GUI_FRQ_Item[] = {
  // TXT
  {"1 kHz  "},
  {"2,5 kHz"},
  {"5 kHz  "},
  {"10 kHz "},
  {"25 kHz "},
  {"50 kHz "},
  {"100 kHz"},
  {"250 kHz"},
  {"500 kHz"},
  {"1 MHz  "},
  {"2 MHz  "},
  {"4 MHz  "},
  {"6 MHz  "},
  {"8 MHz  "},
  {"10,5MHz"},
  {"12 MHz "},
  {"14 MHz "},
  {"21 MHz "},
  {"24 MHz "},
};

//--------------------------------------------------------------
// Settings Samples
//--------------------------------------------------------------
const TXT_Item_t GUI_SAMPLE_Item[] = {
  // TXT
  {"512 S "},
  {"1 kS  "},
  {"2 kS  "},
  {"4 kS  "},
  {"8 kS  "},
  {"16 kS "},
  {"32 kS "},
  {"64 kS "},
  {"128 kS"},
  {"256 kS"},
  {"512 kS"},
  {"1 MS  "},
};

//--------------------------------------------------------------
// Channels
//--------------------------------------------------------------
const CH_Item_t GUI_CH_Item[] = {
  // TXT, Color
  {"PC1 ",RGB_COL_RED},      // CH0
  {"PC3 ",RGB_COL_BLUE},     // CH1
  {"PC8 ",RGB_COL_MAGENTA},  // CH2
  {"PC11",RGB_COL_YELLOW},   // CH3
  {"PC12",RGB_COL_GREEN},    // CH4
  {"PC13",RGB_COL_RED},      // CH5
  {"PC14",RGB_COL_BLUE},     // CH6
  {"PC15",RGB_COL_MAGENTA},  // CH7
};



//--------------------------------------------------------------
// interne Funktionen
//--------------------------------------------------------------
void p_gui_init_var(void);
void p_gui_clear(void);
void p_draw_all(void);
void p_gui_draw_data(void);
void p_gui_drawChannel(uint16_t ch, uint32_t pos);
MENU_Status_t p_check_touch(void);
void p_menu_draw_GUI(void);
void p_get_GUI_button(uint16_t x, uint16_t y);
MENU_Status_t p_make_GUI_changes(void);
void p_menu_draw_Caption(void);


//--------------------------------------------------------------
// init der GUI
//--------------------------------------------------------------
void gui_init(void)
{
  // init aller Variabeln
  p_gui_init_var();

  // init vom Systick
  UB_Systick_Init();

  // init vom Display
  UB_LCD_Init();
  // auf Landscape umschalten
  UB_LCD_SetMode(LANDSCAPE);

  // Display einschalten
  UB_LCD_LayerInit_Fullscreen();
  UB_LCD_SetLayer_2();
  UB_LCD_FillLayer(RGB_COL_WHITE);

  // init vom Touch
  if(UB_Touch_Init()!=SUCCESS) {
    // Status Text ausgeben
    UB_Font_DrawString(0,0,"Touch error, please restart...",&Arial_7x10,RGB_COL_WHITE,RGB_COL_RED);
    while(1);
  }

  p_gui_clear();

  GUI_Counter=GUI_DELAY_MS;

}

//--------------------------------------------------------------
// GUI bearbeiten
//--------------------------------------------------------------
GUI_CHANGE_t gui_do(GUI_DRAW_t draw_status)
{
  GUI_CHANGE_t ret_wert=GUI_CHANGE_NONE;
  MENU_Status_t status;

  // check ob refreshzeit abgelaufen
  if((GUI_Counter==0) || (draw_status!=GUI_DRAW_NONE)) {
    GUI_Counter=GUI_DELAY_MS;

    // check Touch
    status=p_check_touch();

    if(draw_status==GUI_DRAW_AT_TRIGGER_POS) {
      Menu.xpos_start=LA.trigger.pos;
    }

    if((status!=MENU_NO_CHANGE) || (draw_status!=GUI_DRAW_NONE)) {
      p_draw_all();
    }
  }


  return(ret_wert);
}

//--------------------------------------------------------------
void gui_show_status(uint8_t mode)
{
  p_draw_all();
  if(mode==0) UB_Font_DrawString(0,0,"search trigger...",&Arial_7x10,RGB_COL_WHITE,RGB_COL_RED);
  if(mode==1) UB_Font_DrawString(0,0,"sampling...",&Arial_7x10,RGB_COL_WHITE,RGB_COL_RED);
  if(mode==2) UB_Font_DrawString(0,0,"send data...",&Arial_7x10,RGB_COL_WHITE,RGB_COL_RED);
  if(mode==3) UB_Font_DrawString(0,0,"ARM trigger...",&Arial_7x10,RGB_COL_WHITE,RGB_COL_RED);
  UB_LCD_Refresh();
}

//--------------------------------------------------------------
void p_gui_init_var(void)
{
  //--------------------------------------
  // Default Einstellungen
  //--------------------------------------
  Menu.akt_transparenz=100;
  Menu.xpos_start=0;
  Menu.xadr=0;
  Menu.xfaktor=3;
  Menu.xdelta=1.0;

  //--------------------------------------
  GUI.gui_xpos=GUI_XPOS_OFF;  // GUI ausgeblendet
  GUI.akt_button=GUI_BTN_NONE;
  GUI.old_button=GUI_BTN_NONE;

}

//--------------------------------------------------------------
void p_gui_clear(void)
{
  UB_LCD_SetLayer_2();
  UB_LCD_SetTransparency(255);
  UB_LCD_FillLayer(MENU_BG_COL);
  UB_LCD_Copy_Layer2_to_Layer1();
  UB_LCD_SetLayer_Menu();
  UB_LCD_FillLayer(MENU_BG_COL);
  UB_LCD_SetLayer_LA();
  UB_LCD_FillLayer(MENU_BG_COL);
  UB_LCD_SetLayer_Back();
}

//--------------------------------------------------------------
void p_draw_all(void)
{
  UB_LCD_SetLayer_Menu();
  UB_LCD_FillLayer(MENU_BG_COL);

  p_menu_draw_Caption();


  if(GUI.gui_xpos==GUI_XPOS_OFF) {
    Menu.akt_transparenz=100;
  }
  else {
    Menu.akt_transparenz=200;
    //--------------------------
    // GUI
    //--------------------------
    p_menu_draw_GUI();
  }

  p_gui_draw_data();

  UB_LCD_SetLayer_Back();

  if(GUI.gui_xpos==GUI_XPOS_OFF) {
    // ohne GUI => ohne Transparenz zeichnen
    UB_Graphic2D_Copy1DMA();
  }
  else {
    // mit GUI => mit Transparenz zeichnen
    UB_Graphic2D_Copy2DMA(Menu.akt_transparenz);
  }

  // Refreh vom LCD
  UB_LCD_Refresh();
}


//--------------------------------------------------------------
void p_gui_draw_data(void)
{
  uint32_t n;
  uint16_t ch;
  float delta=0.0;


  Menu.xadr=Menu.xpos_start;
  for(n=0;n<GUI_CH_LCDPIXEL_ANZ;n++) {
    for(ch=0;ch<LA_CHANNEL_ANZ;ch++) {
      if(LA.ch[ch].visible==true) {
        p_gui_drawChannel(ch,n);
      }
    }
    // adresse hochsetzen
    if(Menu.xdelta>=1.0) {
      Menu.xadr+=Menu.xdelta;
    }
    else {
      delta+=Menu.xdelta;
      if(delta>=1.0) {
        delta=0.0;
        Menu.xadr++;
      }
    }
  }
}


//--------------------------------------------------------------
void p_gui_drawChannel(uint16_t ch, uint32_t pos)
{
  uint8_t pegel;
  uint16_t xp,yp;
  uint16_t color;

  // wert auslesen
  pegel=la_readPegel(ch,Menu.xadr);
  if(pegel!=LA.ch[ch].old_pegel) {
    LA.ch[ch].old_pegel=pegel;
    pegel=2;
  }
  color=GUI_CH_Item[ch].color;

  xp=GRAPH_START_X;
  yp=LA.ch[ch].ypos;


  if(pegel==0) {
    UB_Graphic2D_DrawPixelNormal(yp,xp+pos,color);
  }else if(pegel==1) {
    UB_Graphic2D_DrawPixelNormal(yp+GRAPH_HEIGHT,xp+pos,color);
  }else {
    UB_Graphic2D_DrawStraightDMA(yp,xp+pos,GRAPH_HEIGHT+1,LCD_DIR_VERTICAL,color);
  }
}


//--------------------------------------------------------------
MENU_Status_t p_check_touch(void)
{
  MENU_Status_t ret_wert=MENU_NO_CHANGE;
  uint16_t x,y;
  static uint16_t x_old=999,y_old=999;
  static uint16_t gui_changed=0;

  //------------------------
  // Touch auslesen
  //------------------------
  UB_Touch_Read();
  if(Touch_Data.status==TOUCH_PRESSED) {
    // Touch ist betätigt
    x=Touch_Data.xp;
    y=Touch_Data.yp;

    if((x!=x_old) || (y!=y_old)) {
      x_old=x;
      y_old=y;
    }
    else {
      if(GUI.gui_xpos==GUI_XPOS_OFF) {
        // GUI ist im Moment noch AUS
        if(gui_changed==0) {
          // GUI einschalten (an einer von 3 positionen)
          GUI.gui_xpos=GUI_XPOS_RIGHT;
          if(y<GUI_XPOS_RIGHT) GUI.gui_xpos=GUI_XPOS_MID;
          if(y<GUI_XPOS_MID) GUI.gui_xpos=GUI_XPOS_LEFT;
          gui_changed=1;
          ret_wert=MENU_CHANGE_GUI;
        }
      }
      else if(GUI.gui_xpos==GUI_XPOS_RIGHT) {
        // GUI-rechts ist aktiv
        if(gui_changed==0) {
          if(y<GUI_XPOS_RIGHT) {
            // GUI ausschalten
            GUI.gui_xpos=GUI_XPOS_OFF;
            gui_changed=1;
            ret_wert=MENU_CHANGE_GUI;
          }
          else {
            // GUI check
            p_get_GUI_button(x,y);
            ret_wert=p_make_GUI_changes();
          }
        }
      }
      else if(GUI.gui_xpos==GUI_XPOS_LEFT) {
        // GUI-links ist aktiv
        if(gui_changed==0) {
          if(y>GUI_XPOS_MID) {
            // GUI ausschalten
            GUI.gui_xpos=GUI_XPOS_OFF;
            gui_changed=1;
            ret_wert=MENU_CHANGE_GUI;
          }
          else {
            // GUI check
            p_get_GUI_button(x,y);
            ret_wert=p_make_GUI_changes();
          }
        }
      }
      else {
        // GUI-mitte ist aktiv
        if(gui_changed==0) {
          if((y<GUI_XPOS_MID) || (y>GUI_XPOS_RIGHT)) {
            // GUI ausschalten
            GUI.gui_xpos=GUI_XPOS_OFF;
            gui_changed=1;
            ret_wert=MENU_CHANGE_GUI;
          }
          else {
            // GUI check
            p_get_GUI_button(x,y);
            ret_wert=p_make_GUI_changes();
          }
        }
      }
    }
  }
  else {
    // Touch ist nicht betätigt
    gui_changed=0;
    if(GUI.old_button!=GUI_BTN_NONE) {
      ret_wert=MENU_CHANGE_GUI;
    }
    GUI.akt_button=GUI_BTN_NONE;
    GUI.old_button=GUI_BTN_NONE;
  }

  return(ret_wert);
}

//--------------------------------------------------------------
// zeichnet die GUI
// (zeichnet auch den betaetigten Button)
//--------------------------------------------------------------
void p_menu_draw_GUI(void)
{
  DMA2D_Koord koord;
  uint32_t n;

  //--------------------------
  // leere GUI zeichnen
  //--------------------------
  koord.source_xp=0;
  koord.source_yp=0;
  koord.source_w=GUI1.width;
  koord.source_h=GUI1.height;
  koord.dest_xp=0;
  koord.dest_yp=GUI.gui_xpos;

  UB_Graphic2D_CopyImgDMA(&GUI1,koord);

  if(GUI.akt_button==GUI_BTN_NONE) return;

  //--------------------------
  // betaetigten Button zeichnen
  //--------------------------
  for(n=0;n<btn_item_anz;n++) {
    if(GUI.akt_button==GUI_BTN_POS[n].name) {
      koord.source_xp=GUI_BTN_POS[n].x1;
      koord.source_yp=GUI_BTN_POS[n].y1;
      koord.source_w=GUI_BTN_POS[n].x2-GUI_BTN_POS[n].x1;
      koord.source_h=GUI_BTN_POS[n].y2-GUI_BTN_POS[n].y1;;
      koord.dest_xp=GUI_BTN_POS[n].x1;
      koord.dest_yp=GUI.gui_xpos+GUI_BTN_POS[n].y1;
      break;
    }
  }

  UB_Graphic2D_CopyImgDMA(&GUI2,koord);
}

//--------------------------------------------------------------
// test welcher der Buttons der GUI betaetigt ist
// y,x = Touch-Position
//--------------------------------------------------------------
void p_get_GUI_button(uint16_t x, uint16_t y)
{
  uint32_t n;

  y-=GUI.gui_xpos;

  GUI.akt_button=GUI_BTN_NONE;
  for(n=0;n<btn_item_anz;n++) {
    if((y>=GUI_BTN_POS[n].y1) && (y<=GUI_BTN_POS[n].y2)) {
      if((x>=GUI_BTN_POS[n].x1) && (x<=GUI_BTN_POS[n].x2)) {
        GUI.akt_button=GUI_BTN_POS[n].name;
        break;
      }
    }
  }
}


//--------------------------------------------------------------
// aendert das Menu, je nach gedruecktem GUI-Button
//--------------------------------------------------------------
MENU_Status_t p_make_GUI_changes(void)
{
  MENU_Status_t ret_wert=MENU_NO_CHANGE;

  if(GUI.akt_button==GUI_BTN_RIGHT) {
    ret_wert=MENU_CHANGE_GUI;
    Menu.xpos_start+=XF_Item[Menu.xfaktor].step;
  }
  else if(GUI.akt_button==GUI_BTN_LEFT) {
    ret_wert=MENU_CHANGE_GUI;
    if(Menu.xpos_start>XF_Item[Menu.xfaktor].step) {
      Menu.xpos_start-=XF_Item[Menu.xfaktor].step;
    }
    else {
      Menu.xpos_start=0;
    }
  }
  else if(GUI.akt_button==GUI_BTN_DOWN) {
    ret_wert=MENU_CHANGE_GUI;
    if(GUI.old_button!=GUI.akt_button) {
      if(Menu.xfaktor>0) Menu.xfaktor--;
      Menu.xdelta=XF_Item[Menu.xfaktor].faktor;
    }
  }
  else if(GUI.akt_button==GUI_BTN_UP) {
    ret_wert=MENU_CHANGE_GUI;
    if(GUI.old_button!=GUI.akt_button) {
      if(Menu.xfaktor<(xf_item_anz-1)) Menu.xfaktor++;
      Menu.xdelta=XF_Item[Menu.xfaktor].faktor;
    }
  }
  else if(GUI.akt_button==GUI_BTN_SETUP) {
    ret_wert=MENU_CHANGE_GUI;
  }

  GUI.old_button=GUI.akt_button;

  return(ret_wert);
}

//--------------------------------------------------------------
void p_menu_draw_Caption(void)
{
  uint16_t ch;
  uint16_t pos,n;

  pos=GRAPH_START_Y;
  n=0;
  for(ch=0;ch<LA_CHANNEL_ANZ;ch++) {
    if(LA.ch[ch].visible==true) {
      if(n<GRAPH_MAX) {
        LA.ch[ch].ypos=pos;
        pos=LA.ch[ch].ypos;
        sprintf(str_buf,"%s",GUI_CH_Item[ch].txt);
        UB_Font_DrawString(pos,0,str_buf,&Arial_7x10,MENU_VG_COL,MENU_BG_COL);
        pos-=GRAPH_DISTANCE;
        n+=1;
      }
      else {
        LA.ch[ch].visible=false;
      }
    }
  }

  sprintf(str_buf,"Frq = %s",GUI_FRQ_Item[LA.setting.frq].txt);
  UB_Font_DrawString(30 ,0,str_buf,&Arial_7x10,MENU_VG_COL,MENU_BG_COL);

  sprintf(str_buf,"Samples = %s",GUI_SAMPLE_Item[LA.setting.sample].txt);
  UB_Font_DrawString(30 ,100,str_buf,&Arial_7x10,MENU_VG_COL,MENU_BG_COL);

  sprintf(str_buf,"Pos = %07d",(int)(Menu.xpos_start));
  UB_Font_DrawString(10,0,str_buf,&Arial_7x10,MENU_VG_COL,MENU_BG_COL);

  sprintf(str_buf,"X = %s",XF_Item[Menu.xfaktor].txt);
  UB_Font_DrawString(10,120,str_buf,&Arial_7x10,MENU_VG_COL,MENU_BG_COL);
}
