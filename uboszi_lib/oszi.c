//--------------------------------------------------------------
// File     : stm32_ub_oszi.c
// Datum    : 24.03.2014
// Version  : 1.6
// Autor    : UB
// EMail    : mc-4u(@)t-online.de
// Web      : www.mikrocontroller-4u.de
// CPU      : STM32F429
// IDE      : CooCox CoIDE 1.7.4
// GCC      : 4.7 2012q4
// Module   : keine
// Funktion : Oszilloskop
//--------------------------------------------------------------

//--------------------------------------------------------------
// Includes
//--------------------------------------------------------------
#include "oszi.h"
#include <stdbool.h>

float32_t FFT_DATA_IN[FFT_LENGTH];
uint16_t FFT_UINT_DATA[FFT_VISIBLE_LENGTH];

//--------------------------------------------------------------
// interne Funktionen
//--------------------------------------------------------------
uint32_t p_oszi_hw_init(void);
void p_oszi_sw_init(void);
void p_oszi_clear_all(void);
void p_oszi_draw_background(void);
void p_oszi_draw_scale(void);
void p_oszi_draw_line_h(uint16_t xp, uint16_t c, uint16_t m);
void p_oszi_draw_line_v(uint16_t yp, uint16_t c, uint16_t m);
void p_oszi_sort_adc(void);
void p_oszi_fill_fft(void);
void p_oszi_draw_adc(void);
int16_t oszi_adc2pixel(uint16_t adc, uint32_t faktor);
void p_oszi_send_data(void);
void p_oszi_send_uart(char *ptr);
void p_oszi_send_screen(void);

//--------------------------------------------------------------
// Header fuer BMP-Transfer
// (fix als einen kompletten Screen (320x240) im Landscape-Mode)
//--------------------------------------------------------------
const uint8_t BMP_HEADER[BMP_HEADER_LEN] =
{	0x42, 0x4D, 0x36, 0x84, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,	// ID=BM, Filsize=(240x320x3+54)
	0x36, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00,				// Offset=54d, Headerlen=40d
	0x40, 0x01, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x01, 0x00,	// W=320d, H=240d (landscape)
	0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x84, 0x03, 0x00,	// 24bpp, unkomprimiert, Data=(240x320x3)
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// nc
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00	// nc
};

//--------------------------------------------------------------
// init vom Oszi
//--------------------------------------------------------------
void oszi_init(void)
{
	uint32_t check;

	//---------------------------------------------
	// Hardware init
	//---------------------------------------------
	check = p_oszi_hw_init();

	p_oszi_send_uart("OSZI 4 STM32F429 [UB]");
	if (check)
	{
		UB_LCD_FillLayer(BACKGROUND_COL);
		if (check & 0x01)
		{	// Touch init error
			UB_Font_DrawString(10, 10, "Touch ERR", &Arial_7x10, FONT_COL, BACKGROUND_COL);
		}
		else if (check & 0x02)
		{
			// ADC array size definiton error
			UB_Font_DrawString(10, 10, "Wrong ADC Array-LEN", &Arial_7x10, FONT_COL, BACKGROUND_COL);
		}
		UB_Led_On(LED_RED);
		while (1)
		{ }
	}

	//---------------------------------------------
	// FFT init
	//---------------------------------------------
	fft_init();

	//---------------------------------------------
	// Software init
	//---------------------------------------------
	p_oszi_sw_init();
	ADC_change_Frq(Menu.timebase.value);
}

//--------------------------------------------------------------
// Start vom Oszi (Endlosloop)
//--------------------------------------------------------------
void oszi_start(void)
{
	register MENU_Status_t status;
	register Menu_t *menu = &Menu;

	p_oszi_draw_background();
	UB_Graphic2D_Copy2DMA(menu->Transparency);

	while (1)
	{
		//---------------------------------------------
		// Wait until GUI timer expires
		//---------------------------------------------
		if (GUI_Timer_ms != 0)
			continue;

		GUI_Timer_ms = GUI_INTERVALL_MS;
		//--------------------------------------
		// User Button Scan (for RUN / STOP)
		//--------------------------------------
		if (UB_Button_OnClick(BTN_USER))
		{
			status = MENU_NO_CHANGE;
			if (menu->trigger.mode == 2)
			{	// "single"
				if (menu->trigger.single == 4)
				{
					menu->trigger.single = 5;	// "Ready" to "Stop"
					status = MENU_CHANGE_NORMAL;
				}
			}
			else
			{	// "normal" or "auto"
				if (menu->trigger.single == 0)
				{
					menu->trigger.single = 1; // From "Run" to "Stop"
					status = MENU_CHANGE_NORMAL;
				}
				else if (menu->trigger.single == 1)
				{
					menu->trigger.single = 2; // "Stop" on "Next"
					status = MENU_CHANGE_NORMAL;
				}
			}
		}
		else
		{
			//--------------------------------------
			// Test ob Touch panel
			//--------------------------------------
			status = menu_check_touch();
		}

		if (status != MENU_NO_CHANGE)
		{
			p_oszi_draw_background();

			if (status == MENU_CHANGE_FRQ)
				ADC_change_Frq(menu->timebase.value);

			if (status == MENU_CHANGE_MODE)
			{
				ADC_change_Mode(menu->trigger.mode);
				if (menu->trigger.mode != 2)
				{
					menu->trigger.single = 0;
				}
				else
				{
					menu->trigger.single = 3;
				}
				p_oszi_draw_background(); // Draw again, to update
				ADC_UB.status = ADC_VORLAUF;
				TIM_Cmd(TIM2, ENABLE);
			}
			if (status == MENU_SEND_DATA)
			{
				p_oszi_draw_background(); // Draw again, to update
				p_oszi_draw_adc();
				// Will be sent at the end
			}
		}

		if (menu->trigger.mode == 1)
		{
			//--------------------------------------
			// Trigger-Mode = "AUTO"
			// Always redraw Screen
			//--------------------------------------
			if (menu->trigger.single == 0)
			{
				if ((ADC1_DMA_STREAM->CR & DMA_SxCR_CT) == 0)
				{
					ADC_UB.trigger_pos = SCALE_X_MITTE;
					ADC_UB.trigger_quarter = 2;
				}
				else
				{
					ADC_UB.trigger_pos = SCALE_X_MITTE;
					ADC_UB.trigger_quarter = 4;
				}
				p_oszi_sort_adc();
				p_oszi_fill_fft();
				if (menu->fft.mode != 0)
					fft_calc();
				p_oszi_draw_adc();
				ADC_UB.status = ADC_VORLAUF;
				UB_Led_Toggle(LED_RED);
			}
			else if (menu->trigger.single == 1)
			{
				// Button "STOP" button was pressed
				TIM_Cmd(TIM2, DISABLE);
				if (status != MENU_NO_CHANGE)
					p_oszi_draw_adc();
			}
			else if (menu->trigger.single == 2)
			{
				// "START" button has been pressed
				menu->trigger.single = 0;
				ADC_UB.status = ADC_VORLAUF;
				TIM_Cmd(TIM2, ENABLE);
				if (status != MENU_NO_CHANGE)
					p_oszi_draw_adc();
			}
		}
		else if (menu->trigger.mode == 0)
		{
			//--------------------------------------
			// Trigger-Mode = "NORMAL"
			// Screen draw only after Trigger Event
			//--------------------------------------
			if (menu->trigger.single == 0)
			{
				if (ADC_UB.status == ADC_READY)
				{
					UB_Led_Toggle(LED_RED);
					p_oszi_sort_adc();
					p_oszi_fill_fft();
					if (menu->fft.mode != 0)
						fft_calc();
					p_oszi_draw_adc();
					ADC_UB.status = ADC_VORLAUF;
					TIM_Cmd(TIM2, ENABLE);
				}
				else
				{
					// Or if the menu has been changed
					if (status != MENU_NO_CHANGE)
						p_oszi_draw_adc();
				}
			}
			else if (menu->trigger.single == 1)
			{
				// Button "STOP" button was pressed
				TIM_Cmd(TIM2, DISABLE);
				if (status != MENU_NO_CHANGE)
					p_oszi_draw_adc();
			}
			else if (menu->trigger.single == 2)
			{
				// Button "START" button was pressed
				menu->trigger.single = 0;
				ADC_UB.status = ADC_VORLAUF;
				TIM_Cmd(TIM2, ENABLE);
				if (status != MENU_NO_CHANGE)
					p_oszi_draw_adc();
			}
		}
		else
		{
			//--------------------------------------
			// Trigger-Mode = "SINGLE"
			// Screnn draw only once, after the trigger event
			//--------------------------------------
			if (menu->trigger.single == 3)
			{
				// Wait for trigger event
				if (ADC_UB.status == ADC_READY)
				{
					menu->trigger.single = 4;
					UB_Led_Toggle(LED_RED);
					p_oszi_sort_adc();
					p_oszi_fill_fft();
					if (menu->fft.mode != 0)
						fft_calc();
					p_oszi_draw_adc();
				}
				else
				{
					// Or if the menu has been changed
					if (status != MENU_NO_CHANGE)
						p_oszi_draw_adc();
				}
			}
			else if (menu->trigger.single == 5)
			{
				// Button "Reset" button was pressed
				menu->trigger.single = 3;
				p_oszi_draw_adc();
				ADC_UB.status = ADC_VORLAUF;
				TIM_Cmd(TIM2, ENABLE);
			}
			else
			{
				// Or if the menu has been changed
				if (status != MENU_NO_CHANGE)
					p_oszi_draw_adc();
			}
		}

		if (GUI.gui_xpos == GUI_XPOS_OFF)
		{
			// Draw without GUI => without transparency
			UB_Graphic2D_Copy1DMA();
		}
		else
		{
			// Draw with GUI => with transparency
			UB_Graphic2D_Copy2DMA(menu->Transparency);
		}

		UB_LCD_Refresh();

		if (menu->send.data != 0)
		{
			p_oszi_send_data();
			menu->send.data = 0;
		}
	}
}

//--------------------------------------------------------------
// Init Hardware
//--------------------------------------------------------------
uint32_t p_oszi_hw_init(void)
{
	uint32_t result = 0;

	// Initialize Touch
	if (UB_Touch_Init() != SUCCESS)
		result |= 0x01; // Touch error

	// Check ADC Defines
	if (ADC_ARRAY_LEN != SCALE_W)
		result |= 0x02; // Define error

	UB_Systick_Init();	// Init Systick
	UB_Led_Init();		// Init LEDs
	UB_Button_Init();	// Init Button
	UB_Uart_Init();		// Init UART

	UB_LCD_Init();		// Init LCD (and SD-RAM)
	UB_LCD_LayerInit_Fullscreen();
	UB_LCD_SetMode(LANDSCAPE);

	p_oszi_clear_all();	// Clear all buffers
	ADC_Init_ALL();		// Init ADC

	return (result);
}

//--------------------------------------------------------------
// init der Software
//--------------------------------------------------------------
void p_oszi_sw_init(void)
{
	register Menu_t *menu = &Menu;

	//--------------------------------------
	// Default Einstellungen
	//--------------------------------------
	menu->Transparency = 100;
	menu->Setting = SETTING_TRIGGER;

	menu->ch1.faktor = 1;		// 2v/div
	menu->ch1.visible = 0;		// visible = true
	menu->ch1.position = 25;

	menu->ch2.faktor = 2;		// 1v/div
	menu->ch2.visible = 0;		// visible = true
	menu->ch2.position = -75;

	menu->timebase.value = 9;	// 5ms/div

	menu->trigger.source = MENU_TRIGGER_CH1;		// trigger = CH1
	menu->trigger.edge = 0;		// hi-flanke
	menu->trigger.mode = 1;		// auto
	menu->trigger.single = 0;
	menu->trigger.value_ch1 = 1024;
	menu->trigger.value_ch2 = 2048;

	menu->cursor.mode = MENU_CURSOR_MODE_CH1;
	menu->cursor.p1 = 2048;
	menu->cursor.p2 = 3072;
	menu->cursor.t1 = 1700;
	menu->cursor.t2 = 2300;
	menu->cursor.f1 = 1000;

	menu->send.mode = 0;
	menu->send.screen = SETTING_TRIGGER;
	menu->send.data = 0;

	menu->fft.mode = 1;				// FFT=CH1

	GUI.gui_xpos = GUI_XPOS_OFF;
	GUI.akt_menu = MM_NONE;
	GUI.old_menu = MM_CH1;
	GUI.akt_button = GUI_BTN_NONE;
	GUI.old_button = GUI_BTN_NONE;
}

//--------------------------------------------------------------
// löscht alle Speicher
//--------------------------------------------------------------
void p_oszi_clear_all(void)
{
	UB_LCD_SetLayer_2();
	UB_LCD_SetTransparency(255);
	UB_LCD_FillLayer(BACKGROUND_COL);
	UB_LCD_Copy_Layer2_to_Layer1();
	UB_LCD_SetLayer_Menu();
	UB_LCD_FillLayer(BACKGROUND_COL);
	UB_LCD_SetLayer_ADC();
	UB_LCD_FillLayer(BACKGROUND_COL);
	UB_LCD_SetLayer_Back();
}

//--------------------------------------------------------------
// draws the background of the oscilloscope
// (scale, cursors, menus, etc)
// Destination address in the SD-RAM = [MENU]
//--------------------------------------------------------------
void p_oszi_draw_background(void)
{
	UB_LCD_SetLayer_Menu();
	UB_LCD_FillLayer(BACKGROUND_COL);

	menu_draw_all();		// GUI first draw
	p_oszi_draw_scale();	// then draw scale and cursor

	UB_LCD_SetLayer_Back();
}

//--------------------------------------------------------------
// zeichnet die Skala und die Cursor vom Oszi
//--------------------------------------------------------------
void p_oszi_draw_scale(void)
{
	register Menu_t *menu = &Menu;
	uint32_t n, m;
	uint16_t xs, ys;
	int16_t signed_int;

	xs = SCALE_START_X;
	ys = SCALE_START_Y;

	//---------------------------------------------
	// Grid of individual dots
	//---------------------------------------------
	for (m = 0; m <= SCALE_H; m += SCALE_SPACE)
	{
		for (n = 0; n <= SCALE_W; n += SCALE_SPACE)
		{
			UB_Graphic2D_DrawPixelNormal(m + xs, n + ys, SCALE_COL);
		}
	}

	// X-axis (horizontal middle line)
	p_oszi_draw_line_h(SCALE_Y_MITTE + xs, SCALE_COL, 0);

	// Y-axis (vertical middle line)
	p_oszi_draw_line_v(SCALE_X_MITTE + ys, SCALE_COL, 0);

	//---------------------------------------------
	// Border
	UB_Graphic2D_DrawStraightDMA(xs - 1, ys - 1, SCALE_W + 2, LCD_DIR_HORIZONTAL, SCALE_COL);			// Bottom line
	UB_Graphic2D_DrawStraightDMA(xs + SCALE_H + 1, ys - 1, SCALE_W + 2, LCD_DIR_HORIZONTAL, SCALE_COL);	// Top line
	UB_Graphic2D_DrawStraightDMA(xs - 1, ys - 1, SCALE_H + 2, LCD_DIR_VERTICAL, SCALE_COL);				// Left line
	UB_Graphic2D_DrawStraightDMA(xs - 1, ys + SCALE_W + 1, SCALE_H + 2, LCD_DIR_VERTICAL, SCALE_COL);	// Right line

	//---------------------------------------------
	// Trigger Line (always visible)
	//---------------------------------------------
	if (menu->trigger.source == MENU_TRIGGER_CH1)
	{
		signed_int = oszi_adc2pixel(menu->trigger.value_ch1, menu->ch1.faktor);
		signed_int += SCALE_Y_MITTE + SCALE_START_X + menu->ch1.position;
		if (signed_int < SCALE_START_X)
			signed_int = SCALE_START_X;
		if (signed_int > SCALE_MX_PIXEL)
			signed_int = SCALE_MX_PIXEL;

		p_oszi_draw_line_h(signed_int, ADC_CH1_COL, 1);
		UB_Font_DrawString(signed_int - 3, 0, "T", &Arial_7x10, ADC_CH1_COL, BACKGROUND_COL);
	}
	else if (menu->trigger.source == MENU_TRIGGER_CH2)
	{
		signed_int = oszi_adc2pixel(menu->trigger.value_ch2, menu->ch2.faktor);
		signed_int += SCALE_Y_MITTE + SCALE_START_X + menu->ch2.position;
		if (signed_int < SCALE_START_X)
			signed_int = SCALE_START_X;
		if (signed_int > SCALE_MX_PIXEL)
			signed_int = SCALE_MX_PIXEL;

		p_oszi_draw_line_h(signed_int, ADC_CH2_COL, 1);
		UB_Font_DrawString(signed_int - 3, 0, "T", &Arial_7x10, ADC_CH2_COL, BACKGROUND_COL);
	}

	//---------------------------------------------
	// Cursor lines (only if enabled)
	//---------------------------------------------
	if (menu->cursor.mode == MENU_CURSOR_MODE_CH1)
	{
		//-------------------------------
		// Cursor (CH1)
		//-------------------------------
		signed_int = oszi_adc2pixel(menu->cursor.p1, menu->ch1.faktor);
		signed_int += SCALE_Y_MITTE + SCALE_START_X + menu->ch1.position;
		if (signed_int < SCALE_START_X)
			signed_int = SCALE_START_X;
		if (signed_int > SCALE_MX_PIXEL)
			signed_int = SCALE_MX_PIXEL;

		p_oszi_draw_line_h(signed_int, CURSOR_COL, 2);
		UB_Font_DrawString(signed_int - 3, 312, "A", &Arial_7x10, CURSOR_COL,
				BACKGROUND_COL);

		signed_int = oszi_adc2pixel(menu->cursor.p2, menu->ch1.faktor);
		signed_int += SCALE_Y_MITTE + SCALE_START_X + menu->ch1.position;
		if (signed_int < SCALE_START_X)
			signed_int = SCALE_START_X;
		if (signed_int > SCALE_MX_PIXEL)
			signed_int = SCALE_MX_PIXEL;

		p_oszi_draw_line_h(signed_int, CURSOR_COL, 2);
		UB_Font_DrawString(signed_int - 3, 312, "B", &Arial_7x10, CURSOR_COL,
				BACKGROUND_COL);
	}
	else if (menu->cursor.mode == MENU_CURSOR_MODE_CH2)
	{
		//-------------------------------
		// Cursor (CH2)
		//-------------------------------
		signed_int = oszi_adc2pixel(menu->cursor.p1, menu->ch2.faktor);
		signed_int += SCALE_Y_MITTE + SCALE_START_X + menu->ch2.position;
		if (signed_int < SCALE_START_X)
			signed_int = SCALE_START_X;
		if (signed_int > SCALE_MX_PIXEL)
			signed_int = SCALE_MX_PIXEL;

		p_oszi_draw_line_h(signed_int, CURSOR_COL, 2);
		UB_Font_DrawString(signed_int - 3, 312, "A", &Arial_7x10, CURSOR_COL,
				BACKGROUND_COL);

		signed_int = oszi_adc2pixel(menu->cursor.p2, menu->ch2.faktor);
		signed_int += SCALE_Y_MITTE + SCALE_START_X + menu->ch2.position;
		if (signed_int < SCALE_START_X)
			signed_int = SCALE_START_X;
		if (signed_int > SCALE_MX_PIXEL)
			signed_int = SCALE_MX_PIXEL;

		p_oszi_draw_line_h(signed_int, CURSOR_COL, 2);
		UB_Font_DrawString(signed_int - 3, 312, "B", &Arial_7x10, CURSOR_COL,
				BACKGROUND_COL);
	}
	else if (menu->cursor.mode == MENU_CURSOR_MODE_TIME)
	{
		//-------------------------------
		// Cursor (TIME)
		//-------------------------------
		signed_int = menu->cursor.t1 * FAKTOR_T;
		signed_int += SCALE_START_Y;
		if (signed_int < SCALE_START_Y)
			signed_int = SCALE_START_Y;
		if (signed_int > SCALE_MY_PIXEL)
			signed_int = SCALE_MY_PIXEL;

		p_oszi_draw_line_v(signed_int, CURSOR_COL, 2);
		UB_Font_DrawString(215, signed_int - 3, "A", &Arial_7x10, CURSOR_COL,
				BACKGROUND_COL);

		signed_int = menu->cursor.t2 * FAKTOR_T;
		signed_int += SCALE_START_Y;
		if (signed_int < SCALE_START_Y)
			signed_int = SCALE_START_Y;
		if (signed_int > SCALE_MY_PIXEL)
			signed_int = SCALE_MY_PIXEL;

		p_oszi_draw_line_v(signed_int, CURSOR_COL, 2);
		UB_Font_DrawString(215, signed_int - 3, "B", &Arial_7x10, CURSOR_COL,
				BACKGROUND_COL);
	}
	else if (menu->cursor.mode == MENU_CURSOR_MODE_FFT)
	{
		//-------------------------------
		// Cursor (FFT)
		//-------------------------------
		signed_int = menu->cursor.f1 * FAKTOR_F;
		signed_int += FFT_START_Y + 1;
		if (signed_int < FFT_START_Y)
			signed_int = FFT_START_Y;
		if (signed_int > (FFT_START_Y + FFT_VISIBLE_LENGTH))
			signed_int = (FFT_START_Y + FFT_VISIBLE_LENGTH);

		p_oszi_draw_line_v(signed_int, CURSOR_COL, 2);
		UB_Font_DrawString(215, signed_int - 3, "A", &Arial_7x10, CURSOR_COL,
				BACKGROUND_COL);
	}
}

//--------------------------------------------------------------
// Draws a horizontal line on the oscilloscope grid
// to "xp" with color "c" and mode "m"
//--------------------------------------------------------------
void p_oszi_draw_line_h(uint16_t xp, uint16_t c, uint16_t m)
{
	uint32_t n, t;

	if (m == 0)
	{
		// Linie : "X----X----X----X----X----X"
		for (n = 0; n <= SCALE_W; n += 5)
		{
			UB_Graphic2D_DrawPixelNormal(xp, n + SCALE_START_Y, c);
		}
	}
	else if (m == 1)
	{
		// Linie : "X-X-X-X-X-X-X-X-X"
		for (n = 0; n <= SCALE_W; n += 2)
		{
			UB_Graphic2D_DrawPixelNormal(xp, n + SCALE_START_Y, c);
		}
	}
	else if (m == 2)
	{
		// Linie : "XX---XX---XX---XX---XX"
		t = 0;
		for (n = 0; n <= SCALE_W; n++)
		{
			if (t < 2)
				UB_Graphic2D_DrawPixelNormal(xp, n + SCALE_START_Y, c);
			t++;
			if (t > 4)
				t = 0;
		}
	}
}

//--------------------------------------------------------------
// Draws a vertical line on the oscilloscope grid
// to "yp" with color "c" and mode "m"
//--------------------------------------------------------------
void p_oszi_draw_line_v(uint16_t yp, uint16_t c, uint16_t m)
{
	uint32_t n, t;

	if (m == 0)
	{
		// Linie : "X----X----X----X----X----X"
		for (n = 0; n <= SCALE_H; n += 5)
		{
			UB_Graphic2D_DrawPixelNormal(n + SCALE_START_X, yp, c);
		}
	}
	else if (m == 1)
	{
		// Linie : "X-X-X-X-X-X-X-X-X"
		for (n = 0; n <= SCALE_H; n += 2)
		{
			UB_Graphic2D_DrawPixelNormal(n + SCALE_START_X, yp, c);
		}
	}
	else if (m == 2)
	{
		// Linie : "XX---XX---XX---XX---XX"
		t = 0;
		for (n = 0; n <= SCALE_H; n++)
		{
			if (t < 2)
				UB_Graphic2D_DrawPixelNormal(n + SCALE_START_X, yp, c);
			t++;
			if (t > 4)
				t = 0;
		}
	}
}

//--------------------------------------------------------------
// sortiert die Daten der ADC-Kanäle von Buffer_A und Buffer_B
// in den Buffer_C um
// die Daten werden so sortiert, das das Trigger-Event in der Mitte
// vom Datenbreich liegt (das ist später die Mitte vom Screen)
//
// Der Trigger-Punkt kann in einem der 4 Quadranten
// vom Daten-Puffer liegen
// Quadrant-1 = erste hälfte vom Buffer-A
// Quadrant-2 = zweite hälfte vom Buffer-A
// Quadrant-3 = erste hälfte vom Buffer-B
// Quadrant-4 = zweite hälfte vom Buffer-B
//--------------------------------------------------------------
void p_oszi_sort_adc(void)
{
	uint32_t n = 0;
	uint32_t start = 0, anz1 = 0, anz2 = 0;
	uint16_t wert;

	if (ADC_UB.trigger_quarter == 1)
	{
		//-------------------------------
		// Trigger-Punkt liegt in Q1
		//-------------------------------
		anz1 = (SCALE_X_MITTE - ADC_UB.trigger_pos);
		start = SCALE_W - anz1;

		//-------------------------------
		// linker Teil kopieren
		//-------------------------------
		for (n = 0; n < anz1; n++)
		{
			wert = ADC_DMA_Buffer_B[(start + n) * 2];
			ADC_DMA_Buffer_C[n * 2] = wert;
			wert = ADC_DMA_Buffer_B[((start + n) * 2) + 1];
			ADC_DMA_Buffer_C[(n * 2) + 1] = wert;
		}
		//-------------------------------
		// rechter Teil kopieren
		//-------------------------------
		anz2 = SCALE_W - anz1;
		start = 0;
		for (n = 0; n < anz2; n++)
		{
			wert = ADC_DMA_Buffer_A[(start + n) * 2];
			ADC_DMA_Buffer_C[(n + anz1) * 2] = wert;
			wert = ADC_DMA_Buffer_A[((start + n) * 2) + 1];
			ADC_DMA_Buffer_C[((n + anz1) * 2) + 1] = wert;
		}
	}
	else if (ADC_UB.trigger_quarter == 2)
	{
		//-------------------------------
		// Trigger-Punkt liegt in Q2
		//-------------------------------
		anz1 = SCALE_W - ((ADC_UB.trigger_pos - SCALE_X_MITTE));
		start = SCALE_W - anz1;

		//-------------------------------
		// linker Teil kopieren
		//-------------------------------
		for (n = 0; n < anz1; n++)
		{
			wert = ADC_DMA_Buffer_A[(start + n) * 2];
			ADC_DMA_Buffer_C[n * 2] = wert;
			wert = ADC_DMA_Buffer_A[((start + n) * 2) + 1];
			ADC_DMA_Buffer_C[(n * 2) + 1] = wert;
		}
		//-------------------------------
		// rechter Teil kopieren
		//-------------------------------
		anz2 = SCALE_W - anz1;
		start = 0;
		for (n = 0; n < anz2; n++)
		{
			wert = ADC_DMA_Buffer_B[(start + n) * 2];
			ADC_DMA_Buffer_C[(n + anz1) * 2] = wert;
			wert = ADC_DMA_Buffer_B[((start + n) * 2) + 1];
			ADC_DMA_Buffer_C[((n + anz1) * 2) + 1] = wert;
		}
	}
	else if (ADC_UB.trigger_quarter == 3)
	{
		//-------------------------------
		// Trigger-Punkt liegt in Q3
		//-------------------------------
		anz1 = (SCALE_X_MITTE - ADC_UB.trigger_pos);
		start = SCALE_W - anz1;

		//-------------------------------
		// linker Teil kopieren
		//-------------------------------
		for (n = 0; n < anz1; n++)
		{
			wert = ADC_DMA_Buffer_A[(start + n) * 2];
			ADC_DMA_Buffer_C[n * 2] = wert;
			wert = ADC_DMA_Buffer_A[((start + n) * 2) + 1];
			ADC_DMA_Buffer_C[(n * 2) + 1] = wert;
		}
		//-------------------------------
		// rechter Teil kopieren
		//-------------------------------
		anz2 = SCALE_W - anz1;
		start = 0;
		for (n = 0; n < anz2; n++)
		{
			wert = ADC_DMA_Buffer_B[(start + n) * 2];
			ADC_DMA_Buffer_C[(n + anz1) * 2] = wert;
			wert = ADC_DMA_Buffer_B[((start + n) * 2) + 1];
			ADC_DMA_Buffer_C[((n + anz1) * 2) + 1] = wert;
		}
	}
	else if (ADC_UB.trigger_quarter == 4)
	{
		//-------------------------------
		// Trigger-Punkt liegt in Q4
		//-------------------------------
		anz1 = SCALE_W - ((ADC_UB.trigger_pos - SCALE_X_MITTE));
		start = SCALE_W - anz1;

		//-------------------------------
		// linker Teil kopieren
		//-------------------------------
		for (n = 0; n < anz1; n++)
		{
			wert = ADC_DMA_Buffer_B[(start + n) * 2];
			ADC_DMA_Buffer_C[n * 2] = wert;
			wert = ADC_DMA_Buffer_B[((start + n) * 2) + 1];
			ADC_DMA_Buffer_C[(n * 2) + 1] = wert;
		}
		//-------------------------------
		// rechter Teil kopieren
		//-------------------------------
		anz2 = SCALE_W - anz1;
		start = 0;
		for (n = 0; n < anz2; n++)
		{
			wert = ADC_DMA_Buffer_A[(start + n) * 2];
			ADC_DMA_Buffer_C[(n + anz1) * 2] = wert;
			wert = ADC_DMA_Buffer_A[((start + n) * 2) + 1];
			ADC_DMA_Buffer_C[((n + anz1) * 2) + 1] = wert;
		}
	}
}

//--------------------------------------------------------------
// fuellt den FFT-Input-Puffer
// mit den Sample-Daten von CH1 oder CH2
// (rest mit 0 auffuellen)
//--------------------------------------------------------------
void p_oszi_fill_fft(void)
{
	register Menu_t *menu = &Menu;
	uint32_t n, m = 0;

	if (menu->fft.mode == 1)
	{
		for (n = 0; n < FFT_LENGTH; n++)
		{
			if (m < SCALE_W)
			{
				FFT_DATA_IN[n] = (float32_t)(((float)(ADC_DMA_Buffer_C[(m * 2)]) - 2048.0f) / 1000.0f);
			}
			else
			{
				FFT_DATA_IN[n] = 0.0;
			}
			m++;
		}
	}
	else if (menu->fft.mode == 2)
	{
		for (n = 0; n < FFT_LENGTH; n++)
		{
			if (m < SCALE_W)
			{
				FFT_DATA_IN[n] = (float32_t)((((float)ADC_DMA_Buffer_C[(m * 2) + 1]) - 2048.0f) / 1000.0f);
			}
			else
			{
				FFT_DATA_IN[n] = 0.0;
			}
			m++;
		}
	}
}

//--------------------------------------------------------------
// zeichnet die Daten der zwei ADC-Kanäle (und die FFT)
//--------------------------------------------------------------
void p_oszi_draw_adc(void)
{
	register Menu_t *menu = &Menu;
	uint32_t n = 0;
	int16_t ch1_wert1, ch1_wert2;
	int16_t ch2_wert1, ch2_wert2;
	int16_t fft_wert1, fft_wert2;

	p_oszi_draw_background();
	UB_LCD_SetLayer_Menu();

	// startwerte
	ch1_wert1 = oszi_adc2pixel(ADC_DMA_Buffer_C[0], menu->ch1.faktor);
	ch1_wert1 += SCALE_Y_MITTE + SCALE_START_X + menu->ch1.position;
	if (ch1_wert1 < SCALE_START_X)
		ch1_wert1 = SCALE_START_X;
	if (ch1_wert1 > SCALE_MX_PIXEL)
		ch1_wert1 = SCALE_MX_PIXEL;

	ch2_wert1 = oszi_adc2pixel(ADC_DMA_Buffer_C[1], menu->ch2.faktor);
	ch2_wert1 += SCALE_Y_MITTE + SCALE_START_X + menu->ch2.position;
	if (ch2_wert1 < SCALE_START_X)
		ch2_wert1 = SCALE_START_X;
	if (ch2_wert1 > SCALE_MX_PIXEL)
		ch2_wert1 = SCALE_MX_PIXEL;

	fft_wert1 = FFT_UINT_DATA[0];
	fft_wert1 += FFT_START_X;
	if (fft_wert1 < SCALE_START_X)
		fft_wert1 = SCALE_START_X;
	if (fft_wert1 > SCALE_MX_PIXEL)
		fft_wert1 = SCALE_MX_PIXEL;

	// komplette Kurve
	for (n = 1; n < SCALE_W; n++)
	{
		if (menu->ch1.visible == 0)
		{
			ch1_wert2 = oszi_adc2pixel(ADC_DMA_Buffer_C[n * 2],
					menu->ch1.faktor);
			ch1_wert2 += SCALE_Y_MITTE + SCALE_START_X + menu->ch1.position;
			if (ch1_wert2 < SCALE_START_X)
				ch1_wert2 = SCALE_START_X;
			if (ch1_wert2 > SCALE_MX_PIXEL)
				ch1_wert2 = SCALE_MX_PIXEL;
			UB_Graphic2D_DrawLineNormal(ch1_wert1, SCALE_START_Y + n, ch1_wert2,
					SCALE_START_Y + n + 1, ADC_CH1_COL);
			ch1_wert1 = ch1_wert2;
		}

		if (menu->ch2.visible == 0)
		{
			ch2_wert2 = oszi_adc2pixel(ADC_DMA_Buffer_C[(n * 2) + 1],
					menu->ch2.faktor);
			ch2_wert2 += SCALE_Y_MITTE + SCALE_START_X + menu->ch2.position;
			if (ch2_wert2 < SCALE_START_X)
				ch2_wert2 = SCALE_START_X;
			if (ch2_wert2 > SCALE_MX_PIXEL)
				ch2_wert2 = SCALE_MX_PIXEL;
			UB_Graphic2D_DrawLineNormal(ch2_wert1, SCALE_START_Y + n, ch2_wert2,
					SCALE_START_Y + n + 1, ADC_CH2_COL);
			ch2_wert1 = ch2_wert2;
		}
	}

	// nur die linke hälfte der FFT zeichnen
	// (die rechte ist das Spiegelbild)
	if (menu->fft.mode != 0)
	{
		for (n = 1; n < FFT_VISIBLE_LENGTH; n++)
		{
			fft_wert2 = FFT_UINT_DATA[n];
			fft_wert2 += FFT_START_X;
			if (fft_wert2 < SCALE_START_X)
				fft_wert2 = SCALE_START_X;
			if (fft_wert2 > SCALE_MX_PIXEL)
				fft_wert2 = SCALE_MX_PIXEL;
			UB_Graphic2D_DrawLineNormal(fft_wert1, FFT_START_Y + n, fft_wert2,
					FFT_START_Y + n + 1, FFT_COL);
			fft_wert1 = fft_wert2;
		}
	}

	UB_LCD_SetLayer_Back();
}

//--------------------------------------------------------------
// Zum umrechnen von adc-Wert in Pixel-Position
//--------------------------------------------------------------
int16_t oszi_adc2pixel(uint16_t adc, uint32_t faktor)
{
	switch (faktor)
	{
		case 0:
			return adc * FAKTOR_5V;
		case 1:
			return adc * FAKTOR_2V;
		case 2:
			return adc * FAKTOR_1V;
		case 3:
			return adc * FAKTOR_0V5;
		case 4:
			return adc * FAKTOR_0V2;
		case 5:
			return adc * FAKTOR_0V1;
	}
	return 0;
}

//--------------------------------------------------------------
// Send data via UART
//--------------------------------------------------------------
void p_oszi_send_data(void)
{
	register Menu_t *menu = &Menu;
	uint32_t n;
	uint16_t wert1, wert2;
	char buf[10];
	extern const SM_Item_t UM_01[];
	extern const SM_Item_t UM_02[];

	//--------------------------------
	// send Screen as Bitmap
	//--------------------------------
	if (menu->send.mode == 6)
	{
		p_oszi_send_screen();
		return;
	}

	//--------------------------------
	// send settings
	//--------------------------------
	p_oszi_send_uart("SETTINGS:");
	if ((menu->send.mode == 0) || (menu->send.mode == 1) || (menu->send.mode == 4)
			|| (menu->send.mode == 5))
	{
		sprintf(buf, "CH1=%s/div", UM_01[menu->ch1.faktor].stxt);
		p_oszi_send_uart(buf);
	}
	if ((menu->send.mode == 2) || (menu->send.mode == 3) || (menu->send.mode == 4)
			|| (menu->send.mode == 5))
	{
		sprintf(buf, "CH2=%s/div", UM_01[menu->ch2.faktor].stxt);
		p_oszi_send_uart(buf);
	}
	sprintf(buf, "Time=%s/div", UM_02[menu->timebase.value].stxt);
	p_oszi_send_uart(buf);
	p_oszi_send_uart("1div=25");

	sprintf(buf, "count=%d", SCALE_W);
	p_oszi_send_uart(buf);

	//--------------------------------
	// send data
	//--------------------------------
	p_oszi_send_uart("DATA:");
	if ((menu->send.mode == 0) || (menu->send.mode == 1))
	{
		p_oszi_send_uart("CH1");
		for (n = 0; n < SCALE_W; n++)
		{
			wert1 = ADC_DMA_Buffer_C[n * 2];
			sprintf(buf, "%d", wert1);
			p_oszi_send_uart(buf);
		}
	}
	else if ((menu->send.mode == 2) || (menu->send.mode == 3))
	{
		p_oszi_send_uart("CH2");
		for (n = 0; n < SCALE_W; n++)
		{
			wert2 = ADC_DMA_Buffer_C[(n * 2) + 1];
			sprintf(buf, "%d", wert2);
			p_oszi_send_uart(buf);
		}
	}
	else if ((menu->send.mode == 4) || (menu->send.mode == 5))
	{
		p_oszi_send_uart("CH1,CH2");
		for (n = 0; n < SCALE_W; n++)
		{
			wert1 = ADC_DMA_Buffer_C[n * 2];
			wert2 = ADC_DMA_Buffer_C[(n * 2) + 1];
			sprintf(buf, "%d,%d", wert1, wert2);
			p_oszi_send_uart(buf);
		}
	}
	//--------------------------------
	// send fft
	//--------------------------------
	if ((menu->send.mode == 1) || (menu->send.mode == 3) || (menu->send.mode == 5))
	{
		if (menu->fft.mode == 1)
		{
			p_oszi_send_uart("FFT:");
			p_oszi_send_uart("CH1");
			sprintf(buf, "count=%d", FFT_VISIBLE_LENGTH);
			p_oszi_send_uart(buf);
			for (n = 0; n < FFT_VISIBLE_LENGTH; n++)
			{
				wert2 = FFT_UINT_DATA[n];
				sprintf(buf, "%d", wert2);
				p_oszi_send_uart(buf);
			}
		}
		else if (menu->fft.mode == 2)
		{
			p_oszi_send_uart("FFT:");
			p_oszi_send_uart("CH2");
			sprintf(buf, "count=%d", FFT_VISIBLE_LENGTH);
			p_oszi_send_uart(buf);
			for (n = 0; n < FFT_VISIBLE_LENGTH; n++)
			{
				wert2 = FFT_UINT_DATA[n];
				sprintf(buf, "%d", wert2);
				p_oszi_send_uart(buf);
			}
		}
	}
	p_oszi_send_uart("END.");
}

//--------------------------------------------------------------
// String per UART senden
//--------------------------------------------------------------
void p_oszi_send_uart(char *ptr)
{
	UB_Uart_SendString(COM1, ptr, CRLF);
}

//--------------------------------------------------------------
// Screen als Bitmap (*.bmp) per UART senden
// takes about 20 seconds at 115200 baud
//--------------------------------------------------------------
void p_oszi_send_screen(void)
{
	uint32_t n, adr;
	uint16_t x, y, color;
	uint8_t r, g, b;

	// Send BMP-Header
	for (n = 0; n < BMP_HEADER_LEN; n++)
	{
		UB_Uart_SendByte(COM1, BMP_HEADER[n]);
	}

	// The buffer Richigen raussuchen to send
	if (LCD_Context.LCD_CurrentLayer == 1)
	{
		adr = LCD_FRAME_BUFFER;
	}
	else
	{
		adr = LCD_FRAME_BUFFER + LCD_FRAME_OFFSET;
	}

	// Send all color data
	for (x = 0; x < LCD_MAXX; x++)
	{
		for (y = 0; y < LCD_MAXY; y++)
		{
			n = y * (LCD_MAXX * 2) + (x * 2);
			color = *(volatile uint16_t*) (adr + n);
			r = ((color & 0xF800) >> 8); // 5bit rot
			g = ((color & 0x07E0) >> 3); // 6bit gruen
			b = ((color & 0x001F) << 3); // 5bit blau
			UB_Uart_SendByte(COM1, b);
			UB_Uart_SendByte(COM1, g);
			UB_Uart_SendByte(COM1, r);
		}
	}
}
