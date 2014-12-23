//--------------------------------------------------------------
// File     : menu->c
// Datum    : 24.03.2014
// Version  : 1.6
// Autor    : UB
// EMail    : mc-4u(@)t-online.de
// Web      : www.mikrocontroller-4u.de
// CPU      : STM32F429
// IDE      : CooCox CoIDE 1.7.4
// GCC      : 4.7 2012q4
// Module   : keine
// Funktion : Menu
//--------------------------------------------------------------

//--------------------------------------------------------------
// Includes
//--------------------------------------------------------------
#include "Menu.h"

Menu_t Menu;
GUI_t GUI;

//--------------------------------------------------------------
// Private functions
//--------------------------------------------------------------
void p_menu_draw_BOT(uint16_t mm_nr, const SM_Item_t um[], uint16_t um_nr, uint16_t mode);
void p_menu_draw_BOT_TRG(void);
void p_menu_draw_BOT_CH1(void);
void p_menu_draw_BOT_CH2(void);
void p_menu_draw_BOT_CUR(void);
void p_menu_draw_BOT_FFT(void);
void p_menu_draw_BOT_SEND(void);
void p_menu_draw_BOT_VERSION(void);
void p_menu_draw_BOT_HELP(void);
void P_FloatToDezStr(float wert);
float P_Volt_to_Float(uint32_t faktor, int16_t pos);
float P_Time_to_Float(uint32_t faktor, uint16_t pos);
float P_FFT_to_Float(uint32_t faktor, uint16_t pos);
uint16_t LINE(uint16_t n);
uint16_t GET_LINE(uint16_t xp);
void p_menu_draw_GUI(void);
void p_gui_draw_TOP(uint16_t mm_nr, const SM_Item_t um[], uint16_t um_nr);
void p_get_GUI_button(uint16_t x, uint16_t y);
MENU_Status_t p_make_GUI_changes(void);
MENU_Status_t p_gui_inc_menu(void);
MENU_Status_t p_gui_dec_menu(void);
uint16_t inc_uintval(uint16_t wert, uint16_t startwert);
uint16_t dec_uintval(uint16_t wert, uint16_t startwert);
int16_t inc_intval(int16_t wert, uint16_t startwert);
int16_t dec_intval(int16_t wert, uint16_t startwert);

//--------------------------------------------------------------
// Global Buffers (for printf)
//--------------------------------------------------------------
char buf[30];
char bval[10];

//--------------------------------------------------------------
// Menu-Einträge
// Gleiche Reihenfolge wie "MM_Akt_Item_t"
// TXT,Ypos,Items
//--------------------------------------------------------------
const MM_Item_t MM_ITEM[] =
{
	{ "",		 0 * FONT_W,	 0 },
	{ "CH1=",	 1 * FONT_W,	 6 }, // CH1            [UM_01]
	{ "CH2=",	11 * FONT_W,	 6 }, // CH2            [UM_01]
	{ "T=",		21 * FONT_W,	17 }, // TIME           [UM_02]
	{ "Menu=",	31 * FONT_W,	 8 }, // Menu           [UM_03]
	{ "S=",		 1 * FONT_W,	MENU_TRIGGER_SOURCE_LAST	}, // TRG Source     [UM_04]
	{ "E=",		 8 * FONT_W,	 2 }, // TRG Edge       [UM_05]
	{ "M=",		14 * FONT_W,	MENU_TRIGGER_MODE_LAST		}, // TRG Mode       [UM_06]
	{ "V=",		24 * FONT_W,	 1 }, // TRG Value      [UM_07]
	{ "",		35 * FONT_W,	 1 }, // TRG Reset      [UM_10]
	{ "V=",		 1 * FONT_W,	MENU_CH_VISIBLE_LAST		}, // CH Visible     [UM_08]
	{ "P=",		 8 * FONT_W,	 1 }, // CH Position    [UM_07]
	{ "M=",		 1 * FONT_W,	MENU_CURSOR_MODE_LAST		}, // CUR Mode       [UM_09]
	{ "A=",		 9 * FONT_W,	 1 }, // CUR A Position [UM_07]
	{ "B=",		21 * FONT_W,	 1 }, // CUR B Position [UM_07]
	{ "M=",		 1 * FONT_W,	MENU_SEND_MODE_LAST			}, // SEND Mode      [UM_11]
	{ "S=",		16 * FONT_W,	 2 }, // SEND Screen    [UM_14]
	{ "",		28 * FONT_W,	 2 }, // SEND Data      [UM_12]
	{ "FFT=",	 1 * FONT_W,	 3 }, // FFT Mode       [UM_13]
};

//--------------------------------------------------------------
// Untermenu : CH1/CH2 Vorteiler
//--------------------------------------------------------------
const SM_Item_t UM_01[] =
{
	{ "5.0V" },
	{ "2.0V" },
	{ "1.0V" },
	{ "0.5V" },
	{ "0.2V" },
	{ "0.1V" },
};

//--------------------------------------------------------------
// Untermenu : Timebase
//--------------------------------------------------------------
const SM_Item_t UM_02[] =
{
	{ "5.0s " },
	{ "2.0s " },
	{ "1.0s " },
	{ "500ms" },
	{ "200ms" },
	{ "100ms" },
	{ "50ms " },
	{ "20ms " },
	{ "10ms " },
	{ "5ms  " },
	{ "2ms  " },
	{ "1ms  " },
	{ "500us" },
	{ "200us" },
	{ "100us" },
	{ "50us " },
	{ "25us " },
};

//--------------------------------------------------------------
// Untermenu : Settings
//--------------------------------------------------------------
const SM_Item_t UM_03[] =
{
	{ "TRIGGER" },
	{ "CH1    " },
	{ "CH2    " },
	{ "CURSOR " },
	{ "FFT    " },
	{ "SEND   " },
	{ "VERSION" },
	{ "HELP   " },
};

//--------------------------------------------------------------
// Submenu-4 : TRG Source
//--------------------------------------------------------------
const SM_Item_t UM_04[] =
{
	{ "CH1" },
	{ "CH2" },
};

//--------------------------------------------------------------
// Submenu-5 : TRG Edge
//--------------------------------------------------------------
const SM_Item_t UM_05[] =
{
	{ "Hi" },
	{ "Lo" },
};

//--------------------------------------------------------------
// Submenu-6 : TRG Mode
//--------------------------------------------------------------
const SM_Item_t UM_06[] =
{
	{ "Normal" },
	{ "Auto  " },
	{ "Single" },
};

//--------------------------------------------------------------
// Submenu-7 (dummy)
//--------------------------------------------------------------
const SM_Item_t UM_07[] =
{
	{ "" },
};

//--------------------------------------------------------------
// Submenu-8 : CH Visible
//--------------------------------------------------------------
const SM_Item_t UM_08[] =
{
	{ "On " },
	{ "Off" },
};

//--------------------------------------------------------------
// Submenu-9 : CUR Mode
//--------------------------------------------------------------
const SM_Item_t UM_09[] =
{
	{ "Off " },
	{ "CH1 " },
	{ "CH2 " },
	{ "Time" },
	{ "FFT " },
};

//--------------------------------------------------------------
// Submenu-10 : TRG Reset
//--------------------------------------------------------------
const SM_Item_t UM_10[] =
{
	{ "RUN  " },
	{ "STOP " },
	{ "RUN  " },
	{ "WAIT " },
	{ "READY" },
	{ "WAIT " },
};

//--------------------------------------------------------------
// Submenu-11 : SEND Mode
//--------------------------------------------------------------
const SM_Item_t UM_11[] =
{
	{ "CH1        " },
	{ "CH1+FFT    " },
	{ "CH2        " },
	{ "CH2+FFT    " },
	{ "CH1+CH2    " },
	{ "CH1+CH2+FFT" },
	{ "Screen-BMP " },
};

//--------------------------------------------------------------
// Submenu-12 : SEND Data
//--------------------------------------------------------------
const SM_Item_t UM_12[] =
{
	{ "START" },
	{ "WAIT " },
};

//--------------------------------------------------------------
// Submenu-13 : FFT Mode
//--------------------------------------------------------------
const SM_Item_t UM_13[] =
{
	{ "Off" },
	{ "CH1" },
	{ "CH2" },
};

//--------------------------------------------------------------
// Submenu-14 : SEND Screen
//--------------------------------------------------------------
const SM_Item_t UM_14[] =
{
	{ "TRIGGER" },
	{ "CURSOR " },
};

//--------------------------------------------------------------
// Records the complete Menu
// (all TOP BOTTOM and all menu items and the GUI)
//--------------------------------------------------------------
void menu_draw_all(void)
{
	register GUI_t *gui = &GUI;
	register Menu_t *menu = &Menu;

	//---------------------------------
	// upper menu bar
	//---------------------------------
	// Background bar
	UB_Graphic2D_DrawFullRectDMA(
			LCD_MAXX - FONT_H - 2, 0,
			LCD_MAXY, FONT_H + 2,
			MENU_BG_COL);
	// TOP-Menu
	p_gui_draw_TOP(MM_CH1, UM_01, menu->Ch1.Factor);
	p_gui_draw_TOP(MM_CH2, UM_01, menu->Ch2.Factor);
	p_gui_draw_TOP(MM_TIME, UM_02, menu->Timebase.Value);

	if (menu->Send.Data == 0)
	{
		p_gui_draw_TOP(MM_SETTING, UM_03, menu->Setting);
	}
	else
	{
		UB_Font_DrawString(LINE(1), MM_ITEM[MM_SETTING].YPos, "please wait", &Arial_7x10, MENU_VG_COL, MENU_AK_COL);
	}

	//---------------------------------
	// lower menu bar
	//---------------------------------
	// Background bar
	UB_Graphic2D_DrawFullRectDMA(0, 0, LCD_MAXY, FONT_H + 2, MENU_BG_COL);

	// Bottom-Menu
	if (menu->Setting == SETTING_TRIGGER)
		p_menu_draw_BOT_TRG();
	else if (menu->Setting == SETTING_CH1)
		p_menu_draw_BOT_CH1();
	else if (menu->Setting == SETTING_CH2)
		p_menu_draw_BOT_CH2();
	else if (menu->Setting == SETTING_CURSOR)
		p_menu_draw_BOT_CUR();
	else if (menu->Setting == SETTING_FFT)
		p_menu_draw_BOT_FFT();
	else if (menu->Setting == SETTING_SEND)
		p_menu_draw_BOT_SEND();
	else if (menu->Setting == SETTING_VERSION)
		p_menu_draw_BOT_VERSION();
	else if (menu->Setting == SETTING_HELP)
		p_menu_draw_BOT_HELP();

	if (gui->GuiXPpos == GUI_XPOS_OFF)
	{
		menu->Transparency = 100;
	}
	else
	{
		menu->Transparency = 200;
		//--------------------------
		// GUI
		//--------------------------
		p_menu_draw_GUI();
	}
}

//--------------------------------------------------------------
// Draws the GUI
// (also records the actuated button)
//--------------------------------------------------------------
void p_menu_draw_GUI(void)
{
	register GUI_t *gui = &GUI;
	DMA2D_Coord coord;

	//--------------------------
	// Draw blank GUI
	//--------------------------
	coord.SrcX = 0;
	coord.SrcY = 0;
	coord.Width = GUI1.width;
	coord.Height = GUI1.height;
	coord.DstX = GUI_YPOS;
	coord.DstY = gui->GuiXPpos;

	UB_Graphic2D_CopyImgDMA(&GUI1, &coord);

	//--------------------------
	// Draw-operated button
	//--------------------------
	if (gui->ButtonActive == GUI_BTN_RIGHT)
	{
		coord.SrcX = GUI1.width / 2;
		coord.SrcY = GUI1.height / 2;
		coord.Width = GUI1.width / 2;
		coord.Height = GUI1.height / 2;
		coord.DstX = GUI_YPOS + (GUI1.width / 2);
		coord.DstY = gui->GuiXPpos + (GUI1.height / 2);
	}
	else if (gui->ButtonActive == GUI_BTN_LEFT)
	{
		coord.SrcX = GUI1.width / 2;
		coord.SrcY = 0;
		coord.Width = GUI1.width / 2;
		coord.Height = GUI1.height / 2;
		coord.DstX = GUI_YPOS + (GUI1.width / 2);
		coord.DstY = gui->GuiXPpos;
	}
	else if (gui->ButtonActive == GUI_BTN_UP)
	{
		coord.SrcX = GUI1.width / 4;
		coord.SrcY = 0;
		coord.Width = GUI1.width / 4;
		coord.Height = GUI1.height;
		coord.DstX = GUI_YPOS + (GUI1.width / 4);
		coord.DstY = gui->GuiXPpos;
	}
	else if (gui->ButtonActive == GUI_BTN_DOWN)
	{
		coord.SrcX = 0;
		coord.SrcY = 0;
		coord.Width = GUI1.width / 4;
		coord.Height = GUI1.height;
		coord.DstX = GUI_YPOS;
		coord.DstY = gui->GuiXPpos;
	}
	else
		return;

	UB_Graphic2D_CopyImgDMA(&GUI2, &coord);
}

//--------------------------------------------------------------
// Test which is engaged the four buttons in the GUI
// x, y = Touch-Position
//--------------------------------------------------------------
void p_get_GUI_button(uint16_t x, uint16_t y)
{
	register GUI_t *gui = &GUI;
	if (x > (GUI_YPOS + (GUI1.width / 2)))
	{
		// left/right
		if (y > (gui->GuiXPpos + (GUI1.height / 2)))
		{
			gui->ButtonActive = GUI_BTN_RIGHT;
		}
		else
		{
			gui->ButtonActive = GUI_BTN_LEFT;
		}
	}
	else
	{
		// up/down
		if (x > (GUI_YPOS + (GUI1.width / 4)))
		{
			gui->ButtonActive = GUI_BTN_UP;
		}
		else
		{
			gui->ButtonActive = GUI_BTN_DOWN;
		}
	}
}

//--------------------------------------------------------------
// Incremented the currently active menu item
//--------------------------------------------------------------
MENU_Status_t p_gui_inc_menu(void)
{
	register GUI_t *gui = &GUI;
	register Menu_t *menu = &Menu;

	MENU_Status_t status = MENU_NO_CHANGE;
	uint32_t max;

	if (gui->MenuActive == MM_NONE)
		return (status);

	// All the "normal" menu items can be activated only once
	if (gui->MenuActive != MM_TRG_VAL
	&&	gui->MenuActive != MM_CH_POS
	&&	gui->MenuActive != MM_CUR_P1
	&&	gui->MenuActive != MM_CUR_P2
		)
	{
		// If pressed before
		if (gui->ButtonOld == gui->ButtonActive)
		{
			// Exit without changing anything
			return (status);
		}
	}

	// Maxumum-Value des Menupunktes
	max = MM_ITEM[gui->MenuActive].um_cnt;

	// default returnwert
	status = MENU_CHANGE_NORMAL;

	if (gui->MenuActive == MM_CH1)
	{
		if (menu->Ch1.Factor < max - 1)
			menu->Ch1.Factor++;
	}
	else if (gui->MenuActive == MM_CH2)
	{
		if (menu->Ch2.Factor < max - 1)
			menu->Ch2.Factor++;
	}
	else if (gui->MenuActive == MM_TIME)
	{
		if (menu->Timebase.Value < max - 1)
			menu->Timebase.Value++;
		status = MENU_CHANGE_FRQ;
	}
	else if (gui->MenuActive == MM_SETTING)
	{
		if (menu->Setting < max - 1)
			menu->Setting++;
	}
	else if (gui->MenuActive == MM_TRG_SOURCE)
	{
		if (menu->Trigger.Source < max - 1)
			menu->Trigger.Source++;
	}
	else if (gui->MenuActive == MM_TRG_EDGE)
	{
		if (menu->Trigger.Edge < max - 1)
			menu->Trigger.Edge++;
	}
	else if (gui->MenuActive == MM_TRG_MODE)
	{
		if (menu->Trigger.Mode < max - 1)
			menu->Trigger.Mode++;
		status = MENU_CHANGE_MODE;
	}
	else if (gui->MenuActive == MM_TRG_VAL)
	{
		if (menu->Trigger.Source == MENU_TRIGGER_SOURCE_CH1)
		{	// CH1
			menu->Trigger.ValueCh1 =
				(gui->ButtonOld != gui->ButtonActive)
				? dec_uintval(menu->Trigger.ValueCh1, 10)
				: dec_uintval(menu->Trigger.ValueCh1, 0);
		}
		else
		{	// CH2
			menu->Trigger.ValueCh2 =
				(gui->ButtonOld != gui->ButtonActive)
				? dec_uintval(menu->Trigger.ValueCh2, 10)
				: dec_uintval(menu->Trigger.ValueCh2, 0);
		}
		status = MENU_CHANGE_VALUE;
	}
	else if (gui->MenuActive == MM_TRG_RESET)
	{
		if (menu->Trigger.Mode == MENU_TRIGGER_MODE_SINGLE)
		{	// "Single"
			if (menu->Trigger.Single == 4)
				menu->Trigger.Single = 5; // "Ready" to "Stop"
		}
		else
		{	// "Normal" oder "Auto"
			if (menu->Trigger.Single == 0)
				menu->Trigger.Single = 1; // "Run" to "Stop"
			else if (menu->Trigger.Single == 1)
				menu->Trigger.Single = 2; // "Stop" on "Next"
		}
	}
	else if (gui->MenuActive == MM_CH_VIS)
	{
		if (menu->Setting == SETTING_CH1)
		{	// CH1
			if (menu->Ch1.Visible < max - 1)
				menu->Ch1.Visible++;
		}
		else if (menu->Setting == SETTING_CH2)
		{	// CH2
			if (menu->Ch2.Visible < max - 1)
				menu->Ch2.Visible++;
		}
	}
	else if (gui->MenuActive == MM_CH_POS)
	{
		if (menu->Setting == SETTING_CH1)
		{	// CH1
			menu->Ch1.Position =
				(gui->ButtonOld != gui->ButtonActive)
				? dec_intval(menu->Ch1.Position, 1)
				: dec_intval(menu->Ch1.Position, 0);
			
		}
		else if (menu->Setting == SETTING_CH2)
		{	// CH2
			menu->Ch2.Position =
				(gui->ButtonOld != gui->ButtonActive)
				? dec_intval(menu->Ch2.Position, 1)
				: dec_intval(menu->Ch2.Position, 0);
		}
		status = MENU_CHANGE_VALUE;
	}
	else if (gui->MenuActive == MM_CUR_MODE)
	{
		if (menu->Cursor.Mode < max - 1)
			menu->Cursor.Mode++;
	}
	else if (gui->MenuActive == MM_CUR_P1)
	{
		if (menu->Cursor.Mode == MENU_CURSOR_MODE_TIME)
		{	// TIME
			menu->Cursor.T1 =
				(gui->ButtonOld != gui->ButtonActive)
				? dec_uintval(menu->Cursor.T1, 1)
				: dec_uintval(menu->Cursor.T1, 0);
			status = MENU_CHANGE_VALUE;
		}
		else if ((menu->Cursor.Mode == MENU_CURSOR_MODE_CH1) || (menu->Cursor.Mode == MENU_CURSOR_MODE_CH2))
		{	// CH1+CH2
			menu->Cursor.P1 =
				(gui->ButtonOld != gui->ButtonActive)
				? dec_uintval(menu->Cursor.P1, 10)
				: dec_uintval(menu->Cursor.P1, 0);
			status = MENU_CHANGE_VALUE;
		}
		else if (menu->Cursor.Mode == MENU_CURSOR_MODE_FFT)
		{	// FFT
			menu->Cursor.F1 =
				(gui->ButtonOld != gui->ButtonActive)
				? dec_uintval(menu->Cursor.F1, 1)
				: dec_uintval(menu->Cursor.F1, 0);
			status = MENU_CHANGE_VALUE;
		}
	}
	else if (gui->MenuActive == MM_CUR_P2)
	{
		if (menu->Cursor.Mode == MENU_CURSOR_MODE_TIME)
		{	// TIME
			menu->Cursor.T2 =
				(gui->ButtonOld != gui->ButtonActive)
				? dec_uintval(menu->Cursor.T2, 1)
				: dec_uintval(menu->Cursor.T2, 0);
			status = MENU_CHANGE_VALUE;
		}
		else if ((menu->Cursor.Mode == MENU_CURSOR_MODE_CH1) || (menu->Cursor.Mode == MENU_CURSOR_MODE_CH2))
		{ // CH1+CH2
			menu->Cursor.P2 =
				(gui->ButtonOld != gui->ButtonActive)
				? dec_uintval(menu->Cursor.P2, 10)
				: dec_uintval(menu->Cursor.P2, 0);
			status = MENU_CHANGE_VALUE;
		}
	}
	else if (gui->MenuActive == MM_FFT_MODE)
	{
		if (menu->FFT.Mode < max - 1)
			menu->FFT.Mode++;
	}
	else if (gui->MenuActive == MM_SEND_MODE)
	{
		if (menu->Send.Mode < max - 1)
			menu->Send.Mode++;
	}
	else if (gui->MenuActive == MM_SEND_SCREEN)
	{
		if (menu->Send.Screen < max - 1)
			menu->Send.Screen++;
	}
	else if (gui->MenuActive == MM_SEND_DATA)
	{
		if (menu->Send.Data == 0)
			menu->Send.Data = 1;
		status = MENU_SEND_DATA;
	}

	return (status);
}

//--------------------------------------------------------------
// decrementiert den gerade aktuellen Menupunkt
//--------------------------------------------------------------
MENU_Status_t p_gui_dec_menu(void)
{
	register GUI_t *gui = &GUI;
	register Menu_t *menu = &Menu;
	MENU_Status_t status = MENU_NO_CHANGE;

	if (gui->MenuActive == MM_NONE)
		return (status);

	// alle "normalen" Menupunkte koennen nur einmal betaetigt werden
	if ((gui->MenuActive != MM_TRG_VAL) && (gui->MenuActive != MM_CH_POS)
			&& (gui->MenuActive != MM_CUR_P1) && (gui->MenuActive != MM_CUR_P2))
	{
		// wenn schon mal betaetigt
		if (gui->ButtonOld == gui->ButtonActive)
		{
			// verlassen ohne was zu aendern
			return (status);
		}
	}

	// default returnwert
	status = MENU_CHANGE_NORMAL;

	if (gui->MenuActive == MM_CH1)
	{
		if (menu->Ch1.Factor > 0)
			menu->Ch1.Factor--;
	}
	else if (gui->MenuActive == MM_CH2)
	{
		if (menu->Ch2.Factor > 0)
			menu->Ch2.Factor--;
	}
	else if (gui->MenuActive == MM_TIME)
	{
		if (menu->Timebase.Value > 0)
			menu->Timebase.Value--;
		status = MENU_CHANGE_FRQ;
	}
	else if (gui->MenuActive == MM_SETTING)
	{
		if (menu->Setting > 0)
			menu->Setting--;
	}
	else if (gui->MenuActive == MM_TRG_SOURCE)
	{
		if (menu->Trigger.Source > MENU_TRIGGER_SOURCE_CH1)
			menu->Trigger.Source--;
	}
	else if (gui->MenuActive == MM_TRG_EDGE)
	{
		if (menu->Trigger.Edge > 0)
			menu->Trigger.Edge--;
	}
	else if (gui->MenuActive == MM_TRG_MODE)
	{
		if (menu->Trigger.Mode > 0)
			menu->Trigger.Mode--;
		status = MENU_CHANGE_MODE;
	}
	else if (gui->MenuActive == MM_TRG_VAL)
	{
		if (menu->Trigger.Source == MENU_TRIGGER_SOURCE_CH1)
		{ // CH1
			menu->Trigger.ValueCh1 =
				(gui->ButtonOld != gui->ButtonActive)
				? inc_uintval(menu->Trigger.ValueCh1, 10)
				: inc_uintval(menu->Trigger.ValueCh1, 0);
		}
		else
		{ // CH2
			menu->Trigger.ValueCh2 =
				(gui->ButtonOld != gui->ButtonActive)
				? inc_uintval(menu->Trigger.ValueCh2, 10)
				: inc_uintval(menu->Trigger.ValueCh2, 0);
		}
		status = MENU_CHANGE_VALUE;
	}
	else if (gui->MenuActive == MM_TRG_RESET)
	{
		if (menu->Trigger.Mode == MENU_TRIGGER_MODE_SINGLE)
		{	// "single"
			if (menu->Trigger.Single == 4)
				menu->Trigger.Single = 5; // "Ready" to "Stop"
		}
		else
		{	// "normal" oder "auto"
			if (menu->Trigger.Single == 0)
				menu->Trigger.Single = 1; // "Run" to "Stop"
			else if (menu->Trigger.Single == 1)
				menu->Trigger.Single = 2; // "Stop" to "Weiter"
		}
	}
	else if (gui->MenuActive == MM_CH_VIS)
	{
		if (menu->Setting == SETTING_CH1)
		{ // CH1
			if (menu->Ch1.Visible > MENU_CH_VISIBLE_ON)
				menu->Ch1.Visible--;
		}
		else if (menu->Setting == SETTING_CH2)
		{ // CH2
			if (menu->Ch2.Visible > MENU_CH_VISIBLE_ON)
				menu->Ch2.Visible--;
		}
	}
	else if (gui->MenuActive == MM_CH_POS)
	{
		if (menu->Setting == SETTING_CH1)
		{ // CH1
			menu->Ch1.Position =
				(gui->ButtonOld != gui->ButtonActive)
				? inc_intval(menu->Ch1.Position, 1)
				: inc_intval(menu->Ch1.Position, 0);
		}
		else if (menu->Setting == SETTING_CH2)
		{ // CH2
			menu->Ch2.Position =
				(gui->ButtonOld != gui->ButtonActive)
				? inc_intval(menu->Ch2.Position, 1)
				: inc_intval(menu->Ch2.Position, 0);
		}
		status = MENU_CHANGE_VALUE;
	}
	else if (gui->MenuActive == MM_CUR_MODE)
	{
		if (menu->Cursor.Mode > MENU_CURSOR_MODE_OFF)
			menu->Cursor.Mode--;
	}
	else if (gui->MenuActive == MM_CUR_P1)
	{
		if (menu->Cursor.Mode == MENU_CURSOR_MODE_TIME)
		{	// TIME
			menu->Cursor.T1 =
				(gui->ButtonOld != gui->ButtonActive)
				? inc_uintval(menu->Cursor.T1, 1)
				: inc_uintval(menu->Cursor.T1, 0);
			status = MENU_CHANGE_VALUE;
		}
		else if ((menu->Cursor.Mode == MENU_CURSOR_MODE_CH1) || (menu->Cursor.Mode == MENU_CURSOR_MODE_CH2))
		{	// CH1+CH2
			menu->Cursor.P1 =
				(gui->ButtonOld != gui->ButtonActive)
				? inc_uintval(menu->Cursor.P1, 10)
				: inc_uintval(menu->Cursor.P1, 0);
			status = MENU_CHANGE_VALUE;
		}
		else if (menu->Cursor.Mode == MENU_CURSOR_MODE_FFT)
		{	// FFT
			menu->Cursor.F1 =
				(gui->ButtonOld != gui->ButtonActive)
				? inc_uintval(menu->Cursor.F1, 1)
				: inc_uintval(menu->Cursor.F1, 0);
			status = MENU_CHANGE_VALUE;
		}
	}
	else if (gui->MenuActive == MM_CUR_P2)
	{
		if (menu->Cursor.Mode == MENU_CURSOR_MODE_TIME)
		{	// TIME
			menu->Cursor.T2 =
				(gui->ButtonOld != gui->ButtonActive)
				? inc_uintval(menu->Cursor.T2, 1)
				: inc_uintval(menu->Cursor.T2, 0);
			status = MENU_CHANGE_VALUE;
		}
		else if ((menu->Cursor.Mode == MENU_CURSOR_MODE_CH1) || (menu->Cursor.Mode == MENU_CURSOR_MODE_CH2))
		{	// CH1+CH2
			menu->Cursor.P2 =
				(gui->ButtonOld != gui->ButtonActive)
				? inc_uintval(menu->Cursor.P2, 10)
				: inc_uintval(menu->Cursor.P2, 0);
			status = MENU_CHANGE_VALUE;
		}
	}
	else if (gui->MenuActive == MM_FFT_MODE)
	{
		if (menu->FFT.Mode > 0)
			menu->FFT.Mode--;
	}
	else if (gui->MenuActive == MM_SEND_MODE)
	{
		if (menu->Send.Mode > MENU_SEND_MODE_CH1)
			menu->Send.Mode--;
	}
	else if (gui->MenuActive == MM_SEND_SCREEN)
	{
		if (menu->Send.Screen > 0)
			menu->Send.Screen--;
	}
	else if (gui->MenuActive == MM_SEND_DATA)
	{
		if (menu->Send.Data == 0)
			menu->Send.Data = 1;
		status = MENU_SEND_DATA;
	}

	return (status);
}

//--------------------------------------------------------------
// aendert das Menu, je nach gedruecktem GUI-Button
//--------------------------------------------------------------
MENU_Status_t p_make_GUI_changes(void)
{
	register GUI_t *gui = &GUI;
	register Menu_t *menu = &Menu;
	MENU_Status_t status = MENU_NO_CHANGE;

	if (gui->ButtonActive == GUI_BTN_RIGHT)
	{
		if (gui->ButtonOld != gui->ButtonActive)
		{
			status = MENU_CHANGE_GUI;
			// Bottom-Trigger
			if (gui->MenuActive == MM_TRG_VAL)
				gui->MenuActive = MM_TRG_RESET;
			if (gui->MenuActive == MM_TRG_MODE)
				gui->MenuActive = MM_TRG_VAL;
			if (gui->MenuActive == MM_TRG_EDGE)
				gui->MenuActive = MM_TRG_MODE;
			if (gui->MenuActive == MM_TRG_SOURCE)
				gui->MenuActive = MM_TRG_EDGE;

			// Bottom-CH
			if (gui->MenuActive == MM_CH_VIS)
				gui->MenuActive = MM_CH_POS;

			// Bottom-Cursor
			if (gui->MenuActive == MM_CUR_P1)
			{
				if (menu->Cursor.Mode != MENU_CURSOR_MODE_FFT)
					gui->MenuActive = MM_CUR_P2;
			}
			if (gui->MenuActive == MM_CUR_MODE)
			{
				if (menu->Cursor.Mode > MENU_CURSOR_MODE_OFF)
					gui->MenuActive = MM_CUR_P1;
			}

			// Bottom-Send
			if (gui->MenuActive == MM_SEND_SCREEN)
				gui->MenuActive = MM_SEND_DATA;
			if (gui->MenuActive == MM_SEND_MODE)
				gui->MenuActive = MM_SEND_SCREEN;

			// TOP (last Entry)
			if (gui->MenuActive == MM_SETTING)
			{
				if (menu->Setting == SETTING_TRIGGER)
					gui->MenuActive = MM_TRG_SOURCE;
				if (menu->Setting == SETTING_CH1)
					gui->MenuActive = MM_CH_VIS;
				if (menu->Setting == SETTING_CH2)
					gui->MenuActive = MM_CH_VIS;
				if (menu->Setting == SETTING_CURSOR)
					gui->MenuActive = MM_CUR_MODE;
				if (menu->Setting == SETTING_FFT)
					gui->MenuActive = MM_FFT_MODE;
				if (menu->Setting == SETTING_SEND)
					gui->MenuActive = MM_SEND_MODE;
			}

			// TOP
			if (gui->MenuActive == MM_TIME)
				gui->MenuActive = MM_SETTING;
			if (gui->MenuActive == MM_CH2)
				gui->MenuActive = MM_TIME;
			if (gui->MenuActive == MM_CH1)
				gui->MenuActive = MM_CH2;
		}
	}
	else if (gui->ButtonActive == GUI_BTN_LEFT)
	{
		if (gui->ButtonOld != gui->ButtonActive)
		{
			status = MENU_CHANGE_GUI;
			// TOP
			if (gui->MenuActive == MM_CH2)
				gui->MenuActive = MM_CH1;
			if (gui->MenuActive == MM_TIME)
				gui->MenuActive = MM_CH2;
			if (gui->MenuActive == MM_SETTING)
				gui->MenuActive = MM_TIME;

			// Bottom-Trigger
			if (gui->MenuActive == MM_TRG_SOURCE)
				gui->MenuActive = MM_SETTING;
			if (gui->MenuActive == MM_TRG_EDGE)
				gui->MenuActive = MM_TRG_SOURCE;
			if (gui->MenuActive == MM_TRG_MODE)
				gui->MenuActive = MM_TRG_EDGE;
			if (gui->MenuActive == MM_TRG_VAL)
				gui->MenuActive = MM_TRG_MODE;
			if (gui->MenuActive == MM_TRG_RESET)
				gui->MenuActive = MM_TRG_VAL;

			// Bottom-CH
			if (gui->MenuActive == MM_CH_VIS)
				gui->MenuActive = MM_SETTING;
			if (gui->MenuActive == MM_CH_POS)
				gui->MenuActive = MM_CH_VIS;

			// Bottom-Cursor
			if (gui->MenuActive == MM_CUR_MODE)
				gui->MenuActive = MM_SETTING;
			if (gui->MenuActive == MM_CUR_P1)
				gui->MenuActive = MM_CUR_MODE;
			if (gui->MenuActive == MM_CUR_P2)
				gui->MenuActive = MM_CUR_P1;

			// Bottom-FFT
			if (gui->MenuActive == MM_FFT_MODE)
				gui->MenuActive = MM_SETTING;

			// Bottom-Send
			if (gui->MenuActive == MM_SEND_MODE)
				gui->MenuActive = MM_SETTING;
			if (gui->MenuActive == MM_SEND_SCREEN)
				gui->MenuActive = MM_SEND_MODE;
			if (gui->MenuActive == MM_SEND_DATA)
				gui->MenuActive = MM_SEND_SCREEN;
		}
	}
	else if (gui->ButtonActive == GUI_BTN_DOWN)
	{
		status = p_gui_inc_menu();
	}
	else if (gui->ButtonActive == GUI_BTN_UP)
	{
		status = p_gui_dec_menu();
	}

	gui->ButtonOld = gui->ButtonActive;

	return status;
}

//--------------------------------------------------------------
// checks and updates the GUI
// GUI to display or hide
// change or an active menu item
//--------------------------------------------------------------
MENU_Status_t menu_check_touch(void)
{
	register GUI_t *gui = &GUI;
	register Menu_t *menu = &Menu;
	MENU_Status_t status = MENU_NO_CHANGE;
	uint16_t x, y;

	//------------------------
	// Touch reading
	//------------------------
	UB_Touch_Read();
	if (Touch_Data.Status == TOUCH_PRESSED)
	{
		// Touch is operated
		x = Touch_Data.XPos;
		y = Touch_Data.YPos;

		if (x != menu->OldX || y != menu->OldY)
		{
			menu->OldX = x;
			menu->OldY = y;
		}
		else
		{
			if (gui->GuiXPpos == GUI_XPOS_OFF)
			{
				// GUI is at the moment OFF
				if (menu->GuiChanged == 0)
				{	// GUI on (at a 3 Positions)
					gui->GuiXPpos = GUI_XPOS_RIGHT;
					if (y < GUI_XPOS_RIGHT)
						gui->GuiXPpos = GUI_XPOS_MID;
					if (y < GUI_XPOS_MID)
						gui->GuiXPpos = GUI_XPOS_LEFT;
					gui->MenuActive = gui->MenuOld;
					menu->GuiChanged = 1;
					status = MENU_CHANGE_GUI;
				}
			}
			else if (gui->GuiXPpos == GUI_XPOS_RIGHT)
			{
				// GUI right is active
				if (menu->GuiChanged == 0)
				{
					if (y < GUI_XPOS_RIGHT)
					{
						// GUI off
						gui->GuiXPpos = GUI_XPOS_OFF;
						gui->MenuOld = gui->MenuActive;
						gui->MenuActive = MM_NONE;
						menu->GuiChanged = 1;
						status = MENU_CHANGE_GUI;
					}
					else
					{
						// GUI check
						p_get_GUI_button(x, y);
						status = p_make_GUI_changes();
					}
				}
			}
			else if (gui->GuiXPpos == GUI_XPOS_LEFT)
			{
				// GUI left is active
				if (menu->GuiChanged == 0)
				{
					if (y > GUI_XPOS_MID)
					{
						// GUI off
						gui->GuiXPpos = GUI_XPOS_OFF;
						gui->MenuOld = gui->MenuActive;
						gui->MenuActive = MM_NONE;
						menu->GuiChanged = 1;
						status = MENU_CHANGE_GUI;
					}
					else
					{
						// GUI check
						p_get_GUI_button(x, y);
						status = p_make_GUI_changes();
					}
				}
			}
			else
			{
				// GUI center is active
				if (menu->GuiChanged == 0)
				{
					if ((y < GUI_XPOS_MID) || (y > GUI_XPOS_RIGHT))
					{
						// GUI off
						gui->GuiXPpos = GUI_XPOS_OFF;
						gui->MenuOld = gui->MenuActive;
						gui->MenuActive = MM_NONE;
						menu->GuiChanged = 1;
						status = MENU_CHANGE_GUI;
					}
					else
					{
						// GUI check
						p_get_GUI_button(x, y);
						status = p_make_GUI_changes();
					}
				}
			}

			// Send the data from the GUI off
			if (status == MENU_SEND_DATA)
			{
				// GUI off
				gui->GuiXPpos = GUI_XPOS_OFF;
				gui->MenuOld = MM_SETTING;
				gui->MenuActive = MM_NONE;
				menu->GuiChanged = 1;
				if (menu->Send.Mode == MENU_SEND_MODE_SCREEN)
				{
					// Switch on Selected Menu
					if (menu->Send.Screen == 0)
						menu->Setting = SETTING_TRIGGER;
					if (menu->Send.Screen == 1)
						menu->Setting = SETTING_CURSOR;
				}
				else
				{
					menu->Setting = SETTING_SEND;
				}
			}
		}
	}
	else
	{
		// Touch is not operated
		menu->GuiChanged = 0;
		if (gui->ButtonOld != GUI_BTN_NONE)
		{
			status = MENU_CHANGE_GUI;
		}
		gui->ButtonActive = GUI_BTN_NONE;
		gui->ButtonOld = GUI_BTN_NONE;
	}

	return status;
}

//--------------------------------------------------------------
// Draws a TOP-menu item
//--------------------------------------------------------------
void p_gui_draw_TOP(uint16_t mm_nr, const SM_Item_t um[], uint16_t um_nr)
{
	sprintf(buf, "%s%s", MM_ITEM[mm_nr].Text, um[um_nr].Text);
	UB_Font_DrawString(LINE(1), MM_ITEM[mm_nr].YPos, buf, &Arial_7x10, MENU_VG_COL,
		(GUI.MenuActive == mm_nr)
		? MENU_AK_COL
		: MENU_BG_COL
	);
}

//--------------------------------------------------------------
// Draws a BOTTOM-menu item
//--------------------------------------------------------------
void p_menu_draw_BOT(uint16_t mm_nr, const SM_Item_t um[], uint16_t um_nr, uint16_t mode)
{
	register GUI_t *gui = &GUI;
	register Menu_t *menu = &Menu;

	switch (mode)
	{
		case 0:
			// Standard menu item
			sprintf(buf, "%s%s", MM_ITEM[mm_nr].Text, um[um_nr].Text);
			break;
		case 1:
			// Menu item: "Trigger Value"
			if (menu->Trigger.Source == MENU_TRIGGER_SOURCE_CH1)
			{
				P_FloatToDezStr(FAKTOR_ADC * menu->Trigger.ValueCh1);
				sprintf(buf, "%s%sV", MM_ITEM[mm_nr].Text, bval);
			}
			if (menu->Trigger.Source == MENU_TRIGGER_SOURCE_CH2)
			{
				P_FloatToDezStr(FAKTOR_ADC * menu->Trigger.ValueCh2);
				sprintf(buf, "%s%sV", MM_ITEM[mm_nr].Text, bval);
			}
			break;
		case 2:
			// Menu item: "Channel position"
			if (menu->Setting == SETTING_CH1)
			{
				P_FloatToDezStr(
						P_Volt_to_Float(menu->Ch1.Factor, menu->Ch1.Position));
				sprintf(buf, "%s%sV", MM_ITEM[mm_nr].Text, bval);
			}
			if (menu->Setting == SETTING_CH2)
			{
				P_FloatToDezStr(
						P_Volt_to_Float(menu->Ch2.Factor, menu->Ch2.Position));
				sprintf(buf, "%s%sV", MM_ITEM[mm_nr].Text, bval);
			}
			break;
		case 3:
			// Menu item: "CH1/CH2 Cursor-A position"
			P_FloatToDezStr(FAKTOR_ADC * menu->Cursor.P1);
			sprintf(buf, "%s%sV", MM_ITEM[mm_nr].Text, bval);
			break;
		case 4:
			// Menu item: "CH1/CH2 Cursor-B position"
			P_FloatToDezStr(FAKTOR_ADC * menu->Cursor.P2);
			sprintf(buf, "%s%sV", MM_ITEM[mm_nr].Text, bval);
			break;
		case 5:
			// Menu item: "TIME Cursor-A position"
			P_FloatToDezStr(P_Time_to_Float(menu->Timebase.Value, menu->Cursor.T1));
			sprintf(buf, "%s%s", MM_ITEM[mm_nr].Text, bval);
			break;
		case 6:
			// Menu item: "TIME Cursor-B position"
			P_FloatToDezStr(P_Time_to_Float(menu->Timebase.Value, menu->Cursor.T2));
			sprintf(buf, "%s%s", MM_ITEM[mm_nr].Text, bval);
			break;
		case 7:
			// Menu item: "Trigger Reset"
			sprintf(buf, "%s%s", MM_ITEM[mm_nr].Text, um[um_nr].Text);
			break;
		case 8:
			// Menu item: "FFT Cursor-A position"
			P_FloatToDezStr(P_FFT_to_Float(menu->Timebase.Value, menu->Cursor.F1));
			if (menu->Timebase.Value <= 12)
				sprintf(buf, "%s%sHz", MM_ITEM[mm_nr].Text, bval);
			else
				sprintf(buf, "%s%skHz", MM_ITEM[mm_nr].Text, bval);
			break;
	}
	UB_Font_DrawString(LINE(24), MM_ITEM[mm_nr].YPos, buf, &Arial_7x10, MENU_VG_COL,
		(gui->MenuActive == mm_nr)
		? MENU_AK_COL
		: MENU_BG_COL
	);
}

//--------------------------------------------------------------
// Submenu: "Trigger"
//--------------------------------------------------------------
void p_menu_draw_BOT_TRG(void)
{
	register Menu_t *menu = &Menu;
	p_menu_draw_BOT(MM_TRG_SOURCE, UM_04, menu->Trigger.Source, 0);
	p_menu_draw_BOT(MM_TRG_EDGE, UM_05, menu->Trigger.Edge, 0);
	p_menu_draw_BOT(MM_TRG_MODE, UM_06, menu->Trigger.Mode, 0);
	p_menu_draw_BOT(MM_TRG_VAL, UM_07, 0, 1);
	p_menu_draw_BOT(MM_TRG_RESET, UM_10, menu->Trigger.Single, 7);
}

//--------------------------------------------------------------
// Submenu: "CH1"
//--------------------------------------------------------------
void p_menu_draw_BOT_CH1(void)
{
	p_menu_draw_BOT(MM_CH_VIS, UM_08, Menu.Ch1.Visible, 0);
	p_menu_draw_BOT(MM_CH_POS, UM_07, 0, 2);
}

//--------------------------------------------------------------
// Submenu: "CH2"
//--------------------------------------------------------------
void p_menu_draw_BOT_CH2(void)
{
	p_menu_draw_BOT(MM_CH_VIS, UM_08, Menu.Ch2.Visible, 0);
	p_menu_draw_BOT(MM_CH_POS, UM_07, 0, 2);
}

//--------------------------------------------------------------
// Submenu: "CURSOR"
//--------------------------------------------------------------
void p_menu_draw_BOT_CUR(void)
{
	register Menu_t *menu = &Menu;
	register uint16_t delta;

	p_menu_draw_BOT(MM_CUR_MODE, UM_09, menu->Cursor.Mode, 0);
	if ((menu->Cursor.Mode == MENU_CURSOR_MODE_CH1) || (menu->Cursor.Mode == MENU_CURSOR_MODE_CH2))
	{
		// Cursor = CH1/CH2
		p_menu_draw_BOT(MM_CUR_P1, UM_07, 0, 3);
		p_menu_draw_BOT(MM_CUR_P2, UM_07, 0, 4);
		if (menu->Cursor.P1 >= menu->Cursor.P2)
			delta = menu->Cursor.P1 - menu->Cursor.P2;
		else
			delta = menu->Cursor.P2 - menu->Cursor.P1;

		P_FloatToDezStr(FAKTOR_ADC * delta);
		sprintf(buf, "~=%sV", bval);
		UB_Font_DrawString(LINE(24), 33 * FONT_W, buf, &Arial_7x10, MENU_VG_COL,
				MENU_BG_COL);
	}
	else if (menu->Cursor.Mode == MENU_CURSOR_MODE_TIME)
	{
		// Cursor = TIME
		p_menu_draw_BOT(MM_CUR_P1, UM_07, 0, 5);
		p_menu_draw_BOT(MM_CUR_P2, UM_07, 0, 6);
		if (menu->Cursor.T1 >= menu->Cursor.T2)
			delta = menu->Cursor.T1 - menu->Cursor.T2;
		else
			delta = menu->Cursor.T2 - menu->Cursor.T1;

		P_FloatToDezStr(P_Time_to_Float(menu->Timebase.Value, (delta + 2048)));
		if (menu->Timebase.Value < 3)
			sprintf(buf, "~=%ss", bval);
		else if (menu->Timebase.Value < 12)
			sprintf(buf, "~=%sms", bval);
		else
			sprintf(buf, "~=%sus", bval);

		UB_Font_DrawString(LINE(24), 33 * FONT_W, buf, &Arial_7x10, MENU_VG_COL, MENU_BG_COL);
	}
	else if (menu->Cursor.Mode == MENU_CURSOR_MODE_FFT)
	{	// Cursor = FFT
		p_menu_draw_BOT(MM_CUR_P1, UM_07, 0, 8);
	}
}

//--------------------------------------------------------------
// Submenu: "FFT"
//--------------------------------------------------------------
void p_menu_draw_BOT_FFT(void)
{
	p_menu_draw_BOT(MM_FFT_MODE, UM_13, Menu.FFT.Mode, 0);
}

//--------------------------------------------------------------
// Submenu: "SEND"
//--------------------------------------------------------------
void p_menu_draw_BOT_SEND(void)
{
	register Menu_t *menu = &Menu;
	p_menu_draw_BOT(MM_SEND_MODE, UM_11, menu->Send.Mode, 0);
	p_menu_draw_BOT(MM_SEND_SCREEN, UM_14, menu->Send.Screen, 0);
	p_menu_draw_BOT(MM_SEND_DATA, UM_12, menu->Send.Data, 0);
}

//--------------------------------------------------------------
// Submenu : "VERSION" or "HELP"
//--------------------------------------------------------------
void p_menu_draw_BOT_Text(const char *msg)
{
	UB_Font_DrawString(LINE(24), 10, msg, &Arial_7x10, MENU_VG_COL, MENU_BG_COL);
}

//--------------------------------------------------------------
// Submenu : "VERSION"
//--------------------------------------------------------------
void p_menu_draw_BOT_VERSION(void)
{
	p_menu_draw_BOT_Text("STM32F429-Oszi | UB | V:1.6 | 24.03.2014");
}

//--------------------------------------------------------------
// Submenu: "HELP"
//--------------------------------------------------------------
void p_menu_draw_BOT_HELP(void)
{
	p_menu_draw_BOT_Text("CH1=PA5 | CH2=PA7 | TX=PA9 | 500Hz=PB2");
}

//--------------------------------------------------------------
// Value Change (Increment without sign)
// Range: 0 to 4095
// automatically increases the step size
//--------------------------------------------------------------
uint16_t inc_uintval(uint16_t wert, uint16_t startwert)
{
	int16_t signed_int_wert;
	static uint16_t inc_delay = 0, inc_val = 1;

	if (startwert > 0)
	{
		inc_delay = 0;
		inc_val = startwert;
	}
	else
	{
		inc_delay++;
		if (inc_delay > 10)
		{
			inc_delay = 0;
			inc_val += 10;
		}
	}

	signed_int_wert = wert;
	signed_int_wert += inc_val;

	if (signed_int_wert > 4095)
		signed_int_wert = 4095;

	return (uint16_t)(signed_int_wert);
}

//--------------------------------------------------------------
// Value Change (Decrement without sign)
// Range: 0 to 4095
// automatically increases the step size
//--------------------------------------------------------------
uint16_t dec_uintval(uint16_t wert, uint16_t startwert)
{
	int16_t signed_int_wert;
	static uint16_t dec_delay = 0, dec_val = 1;

	if (startwert > 0)
	{
		dec_delay = 0;
		dec_val = startwert;
	}
	else
	{
		dec_delay++;
		if (dec_delay > 10)
		{
			dec_delay = 0;
			dec_val += 10;
		}
	}

	signed_int_wert = wert;
	signed_int_wert -= dec_val;

	if (signed_int_wert < 0)
		signed_int_wert = 0;

	return (uint16_t)(signed_int_wert);
}

//--------------------------------------------------------------
// Value Change (Increment with sign)
// Range: -200 to +200
// automatically increases the step size
//--------------------------------------------------------------
int16_t inc_intval(int16_t wert, uint16_t startwert)
{
	int16_t signed_int_wert;
	static uint16_t inc_delay = 0, inc_val = 1;

	if (startwert > 0)
	{
		inc_delay = 0;
		inc_val = startwert;
	}
	else
	{
		inc_delay++;
		if (inc_delay > 10)
		{
			inc_delay = 0;
			inc_val += 10;
		}
	}

	signed_int_wert = wert;
	signed_int_wert += inc_val;

	if (signed_int_wert > 200)
		signed_int_wert = 200;

	return (int16_t)(signed_int_wert);
}

//--------------------------------------------------------------
// Value Change (Decrement with sign)
// Range: -200 to +200
// automatically increases the step size
//--------------------------------------------------------------
int16_t dec_intval(int16_t wert, uint16_t startwert)
{
	int16_t signed_int_wert;
	static uint16_t dec_delay = 0, dec_val = 1;

	if (startwert > 0)
	{
		dec_delay = 0;
		dec_val = startwert;
	}
	else
	{
		dec_delay++;
		if (dec_delay > 10)
		{
			dec_delay = 0;
			dec_val += 10;
		}
	}

	signed_int_wert = wert;
	signed_int_wert -= dec_val;

	if (signed_int_wert < -200)
		signed_int_wert = -200;

	return (int16_t)(signed_int_wert);
}

//--------------------------------------------------------------
// Conversion: Float number to a string
//--------------------------------------------------------------
void P_FloatToDezStr(float value)
{
	int16_t int_part;
	uint16_t dec_part;
	float rest;

	if ((value > 32767) || (value < -32767))
	{
		// Number too large or too small
		sprintf(bval, "%s", "OVF");
		return;
	}

	int_part = (int16_t)(value);
	if (value >= 0.0f)
	{
		rest = value - (float) (int_part);
		dec_part = (uint16_t)(rest * (float) (STRING_FLOAT_FAKTOR) + 0.5f);
		sprintf(bval, STRING_FLOAT_FORMAT, int_part, dec_part);
	}
	else
	{
		rest = (float) (int_part) - value;
		dec_part = (uint16_t)(rest * (float) (STRING_FLOAT_FAKTOR) + 0.5f);
		if (value <= -1.0)
			sprintf(bval, STRING_FLOAT_FORMAT, int_part, dec_part);
		else
			sprintf(bval, STRING_FLOAT_FORMAT2, int_part, dec_part);
	}
}

//--------------------------------------------------------------
// Conversion: value in volts (depending on prescaler)
//--------------------------------------------------------------
const float Volt_Factors[] = {
	VFAKTOR_5V, VFAKTOR_2V, VFAKTOR_1V, VFAKTOR_0V5, VFAKTOR_0V2, VFAKTOR_0V1
};

float P_Volt_to_Float(uint32_t factor, int16_t pos)
{
	if (factor <= 5)
		return (((float)pos) * Volt_Factors[factor]);
	return 0.0f;
}

//--------------------------------------------------------------
// Conversion: value in time (depending on the time base)
//--------------------------------------------------------------
const float Time_Factors[] = {
	TFAKTOR_5, TFAKTOR_2, TFAKTOR_1,
	TFAKTOR_500, TFAKTOR_200, TFAKTOR_100,
	TFAKTOR_50, TFAKTOR_20, TFAKTOR_10,
	TFAKTOR_5, TFAKTOR_2, TFAKTOR_1,
	TFAKTOR_500,TFAKTOR_200, TFAKTOR_100,
	TFAKTOR_50, TFAKTOR_25
};
float P_Time_to_Float(uint32_t factor, uint16_t pos)
{
	if (factor <= 16)
		return ((float)((int16_t)(pos - 2048)) * Time_Factors[factor]);
	return 0.0f;
}

//--------------------------------------------------------------
// Conversion: value in FFT (depending on the time base)
//--------------------------------------------------------------
const float FFT_Factors[] = {
	FFAKTOR_5s, FFAKTOR_2s, FFAKTOR_1s,
	FFAKTOR_500m, FFAKTOR_200m, FFAKTOR_100m,
	FFAKTOR_50m, FFAKTOR_20m, FFAKTOR_10m,
	FFAKTOR_5m, FFAKTOR_2m, FFAKTOR_1m,
	FFAKTOR_500u, FFAKTOR_200u, FFAKTOR_100u,
	FFAKTOR_50u, FFAKTOR_25u
};
float P_FFT_to_Float(uint32_t faktor, uint16_t pos)
{
	if (faktor <= 16)
		return (float)((int16_t)pos) * FFT_Factors[faktor];
	return 0.0f;
}

//--------------------------------------------------------------
// Convert from "line number" to "pixel position"
// n : [1...24]
//--------------------------------------------------------------
uint16_t LINE(uint16_t n)
{
	return (LCD_MAXX - (n * FONT_H) - 1);
}

//--------------------------------------------------------------
// Convert from "pixel position" to "row number"
// xp : [0...249]
//--------------------------------------------------------------
uint16_t GET_LINE(uint16_t xp)
{
	return (((LCD_MAXX - xp) / FONT_H) + 1);
}
