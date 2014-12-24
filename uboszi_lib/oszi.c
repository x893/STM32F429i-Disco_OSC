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

float32_t FFT_DATA_IN[FFT_LENGTH];
uint16_t FFT_UINT_DATA[FFT_VISIBLE_LENGTH];

//--------------------------------------------------------------
// Private functions
//--------------------------------------------------------------
void p_oszi_clear_all(void);
void p_oszi_draw_background(void);
void p_oszi_draw_scale(void);
void p_oszi_draw_line_h(uint16_t xp, uint16_t c, uint16_t m);
void p_oszi_draw_line_v(uint16_t yp, uint16_t c, uint16_t m);
void p_oszi_sort_adc(void);
void p_oszi_fill_fft(void);
void p_oszi_draw_adc(void);
int16_t oszi_adc2pixel(uint16_t adc, uint16_t faktor);
void p_oszi_send_data(void);
void p_oszi_send_uart(char *ptr);
void p_oszi_send_screen(void);

//--------------------------------------------------------------
// BMP-Transfer Header
// (fixed as a full screen (320x240) in landscape mode)
//--------------------------------------------------------------
const uint8_t BMP_HEADER[] =
{	0x42, 0x4D, 0x36, 0x84, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,	// ID=BM, Filsize=(240x320x3+54)
	0x36, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00,				// Offset=54d, Headerlen=40d
	0x40, 0x01, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x01, 0x00,	// W=320d, H=240d (landscape)
	0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x84, 0x03, 0x00,	// 24bpp, unkomprimiert, Data=(240x320x3)
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,				// nc
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00				// nc
};

void oszi_error(const char *msg)
{
	UB_LCD_FillLayer(BACKGROUND_COL);
	UB_Font_DrawString(10, 10, msg, &Arial_7x10, FONT_COL, BACKGROUND_COL);
	UB_Led_On(LED_RED);
	while (1)
		__WFI();
}

//--------------------------------------------------------------
// Init Oszi
//--------------------------------------------------------------
void oszi_init(void)
{
	SystemInit();
	DBGMCU_Config(DBGMCU_SLEEP | DBGMCU_STOP | DBGMCU_STANDBY, ENABLE);

	UB_Systick_Init();
	UB_Led_Init();
	UB_Button_Init();
	UB_Uart_Init();

	UB_LCD_Init();
	UB_LCD_LayerInit_Fullscreen();
	UB_LCD_SetMode(LANDSCAPE);

	p_oszi_clear_all();

	ADC_Init_ALL();

	p_oszi_send_uart("OSZI 4 STM32F429 [UB]");

	if (UB_Touch_Init() != SUCCESS)
		oszi_error("Touch ERR");

	if (ADC_ARRAY_LEN != SCALE_W)
		oszi_error("Wrong ADC Array-LEN");

	fft_init();

	ADC_change_Frq(Menu.Timebase);
}

//--------------------------------------------------------------
// Start oscilloscope (infinite loop)
//--------------------------------------------------------------
int main(void) __attribute__((noreturn));
int main(void)
{
	MENU_Status_t status;
	Menu_t *menu = &Menu;

	oszi_init();

	p_oszi_draw_background();
	UB_Graphic2D_Copy2DMA(menu->Transparency);

	while (1)
	{
		//---------------------------------------------
		// Wait until GUI timer expires
		//---------------------------------------------
		while (GUI_Timer_ms != 0)
			__WFI();
		GUI_Timer_ms = GUI_INTERVALL_MS;

		//--------------------------------------
		// User Button Scan (for RUN / STOP)
		//--------------------------------------
		if (UB_Button_OnClick(BTN_USER))
		{	// Button pressed
			status = MENU_NO_CHANGE;
			if (menu->Trigger.Mode == MENU_TRIGGER_MODE_SINGLE)
			{	// "single"
				if (menu->Trigger.Single == MENU_TRIGGER_SIGNLE_READY)
				{
					menu->Trigger.Single = MENU_TRIGGER_SIGNLE_WAIT5;	// "Ready" to "Stop"
					status = MENU_CHANGE_NORMAL;
				}
			}
			else
			{	// "normal" or "auto"
				if (menu->Trigger.Single == MENU_TRIGGER_SIGNLE_RUN0)
				{
					menu->Trigger.Single = MENU_TRIGGER_SIGNLE_STOP; // From "Run" to "Stop"
					status = MENU_CHANGE_NORMAL;
				}
				if (menu->Trigger.Single == MENU_TRIGGER_SIGNLE_STOP)
				{
					menu->Trigger.Single = MENU_TRIGGER_SIGNLE_RUN2; // "Stop" on "Next"
					status = MENU_CHANGE_NORMAL;
				}
			}
		}
		else
		{
			//--------------------------------------
			// Test Touch panel
			//--------------------------------------
			status = menu_check_touch();
		}

		if (status != MENU_NO_CHANGE)
		{
			p_oszi_draw_background();

			if (status == MENU_CHANGE_FRQ)
				ADC_change_Frq(menu->Timebase);

			if (status == MENU_CHANGE_MODE)
			{
				ADC_change_Mode(menu->Trigger.Mode);
				if (menu->Trigger.Mode != MENU_TRIGGER_MODE_SINGLE)
					menu->Trigger.Single = MENU_TRIGGER_SIGNLE_RUN0;
				else
					menu->Trigger.Single = MENU_TRIGGER_SIGNLE_WAIT3;

				p_oszi_draw_background(); // Draw again, to update
				ADC_UB.Status = ADC_START;
				TIM_Cmd(TIM2, ENABLE);
			}
			if (status == MENU_SEND_DATA)
			{
				p_oszi_draw_background(); // Draw again, to update
				p_oszi_draw_adc();
				// Will be sent at the end
			}
		}

		if (menu->Trigger.Mode == MENU_TRIGGER_MODE_AUTO)
		{
			//--------------------------------------
			// Trigger-Mode = "AUTO"
			// Always redraw Screen
			//--------------------------------------
			if (menu->Trigger.Single == MENU_TRIGGER_SIGNLE_RUN0)
			{
				if ((ADC1_DMA_STREAM->CR & DMA_SxCR_CT) == 0)
				{
					ADC_UB.TriggerPos = SCALE_X_MITTE;
					ADC_UB.TriggerQuarter = 2;
				}
				else
				{
					ADC_UB.TriggerPos = SCALE_X_MITTE;
					ADC_UB.TriggerQuarter = 4;
				}
				p_oszi_sort_adc();
				p_oszi_fill_fft();
				if (menu->FFT != MENU_FFT_MODE_OFF)
					fft_calc();
				p_oszi_draw_adc();
				ADC_UB.Status = ADC_START;
				UB_Led_Toggle(LED_RED);
			}
			else if (menu->Trigger.Single == MENU_TRIGGER_SIGNLE_STOP)
			{
				// Button "STOP" button was pressed
				TIM_Cmd(TIM2, DISABLE);
				if (status != MENU_NO_CHANGE)
					p_oszi_draw_adc();
			}
			else if (menu->Trigger.Single == MENU_TRIGGER_SIGNLE_RUN2)
			{
				// "START" button has been pressed
				menu->Trigger.Single = MENU_TRIGGER_SIGNLE_RUN0;
				ADC_UB.Status = ADC_START;
				TIM_Cmd(TIM2, ENABLE);
				if (status != MENU_NO_CHANGE)
					p_oszi_draw_adc();
			}
		}
		else if (menu->Trigger.Mode == MENU_TRIGGER_MODE_NORMAL)
		{
			//--------------------------------------
			// Trigger-Mode = "NORMAL"
			// Screen draw only after Trigger Event
			//--------------------------------------
			if (menu->Trigger.Single == MENU_TRIGGER_SIGNLE_RUN0)
			{
				if (ADC_UB.Status == ADC_READY)
				{
					UB_Led_Toggle(LED_RED);
					p_oszi_sort_adc();
					p_oszi_fill_fft();
					if (menu->FFT != MENU_FFT_MODE_OFF)
						fft_calc();
					p_oszi_draw_adc();
					ADC_UB.Status = ADC_START;
					TIM_Cmd(TIM2, ENABLE);
				}
				else
				{
					// Or if the menu has been changed
					if (status != MENU_NO_CHANGE)
						p_oszi_draw_adc();
				}
			}
			else if (menu->Trigger.Single == MENU_TRIGGER_SIGNLE_STOP)
			{
				// Button "STOP" button was pressed
				TIM_Cmd(TIM2, DISABLE);
				if (status != MENU_NO_CHANGE)
					p_oszi_draw_adc();
			}
			else if (menu->Trigger.Single == MENU_TRIGGER_SIGNLE_RUN2)
			{
				// Button "START" button was pressed
				menu->Trigger.Single = 0;
				ADC_UB.Status = ADC_START;
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
			if (menu->Trigger.Single == MENU_TRIGGER_SIGNLE_WAIT3)
			{
				// Wait for trigger event
				if (ADC_UB.Status == ADC_READY)
				{
					menu->Trigger.Single = MENU_TRIGGER_SIGNLE_READY;
					UB_Led_Toggle(LED_RED);
					p_oszi_sort_adc();
					p_oszi_fill_fft();
					if (menu->FFT != MENU_FFT_MODE_OFF)
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
			else if (menu->Trigger.Single == MENU_TRIGGER_SIGNLE_WAIT5)
			{
				// Button "Reset" button was pressed
				menu->Trigger.Single = MENU_TRIGGER_SIGNLE_WAIT3;
				p_oszi_draw_adc();
				ADC_UB.Status = ADC_START;
				TIM_Cmd(TIM2, ENABLE);
			}
			else
			{
				// Or if the menu has been changed
				if (status != MENU_NO_CHANGE)
					p_oszi_draw_adc();
			}
		}

		if (GUI.GuiXPpos == GUI_XPOS_OFF)
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

		if (menu->Send.Data != MENU_SEND_DATA_START)
		{
			p_oszi_send_data();
			menu->Send.Data = MENU_SEND_DATA_START;
		}
	}
}

//--------------------------------------------------------------
// Clear all memory
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
// Draws the background of the oscilloscope
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
// Records scale and cursor from oscilloscope
//--------------------------------------------------------------
void p_oszi_draw_scale(void)
{
	Menu_t *menu = &Menu;
	uint32_t n, m;
	uint16_t xs, ys;
	int16_t xp;

	xs = SCALE_START_X;
	ys = SCALE_START_Y;

	//---------------------------------------------
	// Grid of individual dots
	//---------------------------------------------
	for (m = 0; m <= SCALE_H; m += SCALE_SPACE)
		for (n = 0; n <= SCALE_W; n += SCALE_SPACE)
			UB_Graphic2D_DrawPixelNormal(m + xs, n + ys, SCALE_COL);

	// X-axis (horizontal middle line)
	p_oszi_draw_line_h(SCALE_Y_MITTE + xs, SCALE_COL, 0);

	// Y-axis (vertical middle line)
	p_oszi_draw_line_v(SCALE_X_MITTE + ys, SCALE_COL, 0);

	//---------------------------------------------
	// Border
	//---------------------------------------------
	UB_Graphic2D_DrawStraightDMA(xs - 1, ys - 1, SCALE_W + 2, LCD_DIR_HORIZONTAL, SCALE_COL);			// Bottom line
	UB_Graphic2D_DrawStraightDMA(xs + SCALE_H + 1, ys - 1, SCALE_W + 2, LCD_DIR_HORIZONTAL, SCALE_COL);	// Top line
	UB_Graphic2D_DrawStraightDMA(xs - 1, ys - 1, SCALE_H + 2, LCD_DIR_VERTICAL, SCALE_COL);				// Left line
	UB_Graphic2D_DrawStraightDMA(xs - 1, ys + SCALE_W + 1, SCALE_H + 2, LCD_DIR_VERTICAL, SCALE_COL);	// Right line

	//---------------------------------------------
	// Trigger Line (always visible)
	//---------------------------------------------
	if (menu->Trigger.Source == MENU_TRIGGER_SOURCE_CH1)
	{
		xp = oszi_adc2pixel(menu->Trigger.ValueCh1, menu->Ch1.Factor);
		xp += SCALE_Y_MITTE + SCALE_START_X + menu->Ch1.Position;
		if (xp < SCALE_START_X)
			xp = SCALE_START_X;
		if (xp > SCALE_MX_PIXEL)
			xp = SCALE_MX_PIXEL;

		p_oszi_draw_line_h(xp, ADC_CH1_COL, 1);
		UB_Font_DrawString(xp - 3, 0, "T", &Arial_7x10, ADC_CH1_COL, BACKGROUND_COL);
	}
	else if (menu->Trigger.Source == MENU_TRIGGER_SOURCE_CH2)
	{
		xp = oszi_adc2pixel(menu->Trigger.ValueCh2, menu->Ch2.Factor);
		xp += SCALE_Y_MITTE + SCALE_START_X + menu->Ch2.Position;
		if (xp < SCALE_START_X)
			xp = SCALE_START_X;
		if (xp > SCALE_MX_PIXEL)
			xp = SCALE_MX_PIXEL;

		p_oszi_draw_line_h(xp, ADC_CH2_COL, 1);
		UB_Font_DrawString(xp - 3, 0, "T", &Arial_7x10, ADC_CH2_COL, BACKGROUND_COL);
	}

	//---------------------------------------------
	// Cursor lines (only if enabled)
	//---------------------------------------------
	if (menu->Cursor.Mode == MENU_CURSOR_MODE_CH1)
	{
		//-------------------------------
		// Cursor (CH1)
		//-------------------------------
		xp = oszi_adc2pixel(menu->Cursor.P1, menu->Ch1.Factor);
		xp += SCALE_Y_MITTE + SCALE_START_X + menu->Ch1.Position;
		if (xp < SCALE_START_X)
			xp = SCALE_START_X;
		if (xp > SCALE_MX_PIXEL)
			xp = SCALE_MX_PIXEL;

		p_oszi_draw_line_h(xp, CURSOR_COL, 2);
		UB_Font_DrawString(xp - 3, 312, "A", &Arial_7x10, CURSOR_COL, BACKGROUND_COL);

		xp = oszi_adc2pixel(menu->Cursor.P2, menu->Ch1.Factor);
		xp += SCALE_Y_MITTE + SCALE_START_X + menu->Ch1.Position;
		if (xp < SCALE_START_X)
			xp = SCALE_START_X;
		if (xp > SCALE_MX_PIXEL)
			xp = SCALE_MX_PIXEL;

		p_oszi_draw_line_h(xp, CURSOR_COL, 2);
		UB_Font_DrawString(xp - 3, 312, "B", &Arial_7x10, CURSOR_COL, BACKGROUND_COL);
	}
	else if (menu->Cursor.Mode == MENU_CURSOR_MODE_CH2)
	{
		//-------------------------------
		// Cursor (CH2)
		//-------------------------------
		xp = oszi_adc2pixel(menu->Cursor.P1, menu->Ch2.Factor);
		xp += SCALE_Y_MITTE + SCALE_START_X + menu->Ch2.Position;
		if (xp < SCALE_START_X)
			xp = SCALE_START_X;
		if (xp > SCALE_MX_PIXEL)
			xp = SCALE_MX_PIXEL;

		p_oszi_draw_line_h(xp, CURSOR_COL, 2);
		UB_Font_DrawString(xp - 3, 312, "A", &Arial_7x10, CURSOR_COL, BACKGROUND_COL);

		xp = oszi_adc2pixel(menu->Cursor.P2, menu->Ch2.Factor);
		xp += SCALE_Y_MITTE + SCALE_START_X + menu->Ch2.Position;
		if (xp < SCALE_START_X)
			xp = SCALE_START_X;
		if (xp > SCALE_MX_PIXEL)
			xp = SCALE_MX_PIXEL;

		p_oszi_draw_line_h(xp, CURSOR_COL, 2);
		UB_Font_DrawString(xp - 3, 312, "B", &Arial_7x10, CURSOR_COL, BACKGROUND_COL);
	}
	else if (menu->Cursor.Mode == MENU_CURSOR_MODE_TIME)
	{
		//-------------------------------
		// Cursor (TIME)
		//-------------------------------
		xp = menu->Cursor.T1 * FAKTOR_T;
		xp += SCALE_START_Y;
		if (xp < SCALE_START_Y)
			xp = SCALE_START_Y;
		if (xp > SCALE_MY_PIXEL)
			xp = SCALE_MY_PIXEL;

		p_oszi_draw_line_v(xp, CURSOR_COL, 2);
		UB_Font_DrawString(215, xp - 3, "A", &Arial_7x10, CURSOR_COL, BACKGROUND_COL);

		xp = menu->Cursor.T2 * FAKTOR_T;
		xp += SCALE_START_Y;
		if (xp < SCALE_START_Y)
			xp = SCALE_START_Y;
		if (xp > SCALE_MY_PIXEL)
			xp = SCALE_MY_PIXEL;

		p_oszi_draw_line_v(xp, CURSOR_COL, 2);
		UB_Font_DrawString(215, xp - 3, "B", &Arial_7x10, CURSOR_COL, BACKGROUND_COL);
	}
	else if (menu->Cursor.Mode == MENU_CURSOR_MODE_FFT)
	{
		//-------------------------------
		// Cursor (FFT)
		//-------------------------------
		xp = menu->Cursor.F1 * FAKTOR_F;
		xp += FFT_START_Y + 1;
		if (xp < FFT_START_Y)
			xp = FFT_START_Y;
		if (xp > (FFT_START_Y + FFT_VISIBLE_LENGTH))
			xp = (FFT_START_Y + FFT_VISIBLE_LENGTH);

		p_oszi_draw_line_v(xp, CURSOR_COL, 2);
		UB_Font_DrawString(215, xp - 3, "A", &Arial_7x10, CURSOR_COL,
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
	{	// Line : "X----X----X----X----X----X"
		for (n = 0; n <= SCALE_W; n += 5)
			UB_Graphic2D_DrawPixelNormal(xp, n + SCALE_START_Y, c);
	}
	else if (m == 1)
	{	// Line : "X-X-X-X-X-X-X-X-X"
		for (n = 0; n <= SCALE_W; n += 2)
			UB_Graphic2D_DrawPixelNormal(xp, n + SCALE_START_Y, c);
	}
	else if (m == 2)
	{	// Line : "XX---XX---XX---XX---XX"
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
			UB_Graphic2D_DrawPixelNormal(n + SCALE_START_X, yp, c);
	}
	else if (m == 1)
	{
		// Linie : "X-X-X-X-X-X-X-X-X"
		for (n = 0; n <= SCALE_H; n += 2)
			UB_Graphic2D_DrawPixelNormal(n + SCALE_START_X, yp, c);
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
	uint16_t data;

	if (ADC_UB.TriggerQuarter == 1)
	{
		//-------------------------------
		// Trigger-Punkt liegt in Q1
		//-------------------------------
		anz1 = (SCALE_X_MITTE - ADC_UB.TriggerPos);
		start = SCALE_W - anz1;

		//-------------------------------
		// linker Teil kopieren
		//-------------------------------
		for (n = 0; n < anz1; n++)
		{
			data = ADC_DMA_Buffer_B[(start + n) * 2];
			ADC_DMA_Buffer_C[n * 2] = data;
			data = ADC_DMA_Buffer_B[((start + n) * 2) + 1];
			ADC_DMA_Buffer_C[(n * 2) + 1] = data;
		}
		//-------------------------------
		// rechter Teil kopieren
		//-------------------------------
		anz2 = SCALE_W - anz1;
		start = 0;
		for (n = 0; n < anz2; n++)
		{
			data = ADC_DMA_Buffer_A[(start + n) * 2];
			ADC_DMA_Buffer_C[(n + anz1) * 2] = data;
			data = ADC_DMA_Buffer_A[((start + n) * 2) + 1];
			ADC_DMA_Buffer_C[((n + anz1) * 2) + 1] = data;
		}
	}
	else if (ADC_UB.TriggerQuarter == 2)
	{
		//-------------------------------
		// Trigger-Punkt liegt in Q2
		//-------------------------------
		anz1 = SCALE_W - ((ADC_UB.TriggerPos - SCALE_X_MITTE));
		start = SCALE_W - anz1;

		//-------------------------------
		// linker Teil kopieren
		//-------------------------------
		for (n = 0; n < anz1; n++)
		{
			data = ADC_DMA_Buffer_A[(start + n) * 2];
			ADC_DMA_Buffer_C[n * 2] = data;
			data = ADC_DMA_Buffer_A[((start + n) * 2) + 1];
			ADC_DMA_Buffer_C[(n * 2) + 1] = data;
		}
		//-------------------------------
		// rechter Teil kopieren
		//-------------------------------
		anz2 = SCALE_W - anz1;
		start = 0;
		for (n = 0; n < anz2; n++)
		{
			data = ADC_DMA_Buffer_B[(start + n) * 2];
			ADC_DMA_Buffer_C[(n + anz1) * 2] = data;
			data = ADC_DMA_Buffer_B[((start + n) * 2) + 1];
			ADC_DMA_Buffer_C[((n + anz1) * 2) + 1] = data;
		}
	}
	else if (ADC_UB.TriggerQuarter == 3)
	{
		//-------------------------------
		// Trigger-Punkt liegt in Q3
		//-------------------------------
		anz1 = (SCALE_X_MITTE - ADC_UB.TriggerPos);
		start = SCALE_W - anz1;

		//-------------------------------
		// linker Teil kopieren
		//-------------------------------
		for (n = 0; n < anz1; n++)
		{
			data = ADC_DMA_Buffer_A[(start + n) * 2];
			ADC_DMA_Buffer_C[n * 2] = data;
			data = ADC_DMA_Buffer_A[((start + n) * 2) + 1];
			ADC_DMA_Buffer_C[(n * 2) + 1] = data;
		}
		//-------------------------------
		// rechter Teil kopieren
		//-------------------------------
		anz2 = SCALE_W - anz1;
		start = 0;
		for (n = 0; n < anz2; n++)
		{
			data = ADC_DMA_Buffer_B[(start + n) * 2];
			ADC_DMA_Buffer_C[(n + anz1) * 2] = data;
			data = ADC_DMA_Buffer_B[((start + n) * 2) + 1];
			ADC_DMA_Buffer_C[((n + anz1) * 2) + 1] = data;
		}
	}
	else if (ADC_UB.TriggerQuarter == 4)
	{
		//-------------------------------
		// Trigger-Punkt liegt in Q4
		//-------------------------------
		anz1 = SCALE_W - ((ADC_UB.TriggerPos - SCALE_X_MITTE));
		start = SCALE_W - anz1;

		//-------------------------------
		// linker Teil kopieren
		//-------------------------------
		for (n = 0; n < anz1; n++)
		{
			data = ADC_DMA_Buffer_B[(start + n) * 2];
			ADC_DMA_Buffer_C[n * 2] = data;
			data = ADC_DMA_Buffer_B[((start + n) * 2) + 1];
			ADC_DMA_Buffer_C[(n * 2) + 1] = data;
		}
		//-------------------------------
		// rechter Teil kopieren
		//-------------------------------
		anz2 = SCALE_W - anz1;
		start = 0;
		for (n = 0; n < anz2; n++)
		{
			data = ADC_DMA_Buffer_A[(start + n) * 2];
			ADC_DMA_Buffer_C[(n + anz1) * 2] = data;
			data = ADC_DMA_Buffer_A[((start + n) * 2) + 1];
			ADC_DMA_Buffer_C[((n + anz1) * 2) + 1] = data;
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
	Menu_t *menu = &Menu;
	uint32_t n, m = 0;

	if (menu->FFT == MENU_FFT_MODE_CH1)
		for (n = 0; n < FFT_LENGTH; n++)
		{
			if (m < SCALE_W)
				FFT_DATA_IN[n] = (float32_t)(((float)(ADC_DMA_Buffer_C[(m * 2)]) - 2048.0f) / 1000.0f);
			else
				FFT_DATA_IN[n] = 0.0;
			m++;
		}
	else if (menu->FFT == MENU_FFT_MODE_CH2)
		for (n = 0; n < FFT_LENGTH; n++)
		{
			if (m < SCALE_W)
				FFT_DATA_IN[n] = (float32_t)((((float)ADC_DMA_Buffer_C[(m * 2) + 1]) - 2048.0f) / 1000.0f);
			else
				FFT_DATA_IN[n] = 0.0;
			m++;
		}
}

//--------------------------------------------------------------
// zeichnet die Daten der zwei ADC-Kanäle (und die FFT)
//--------------------------------------------------------------
void p_oszi_draw_adc(void)
{
	Menu_t *menu = &Menu;
	uint32_t n = 0;
	int16_t ch1_data1, ch1_data2;
	int16_t ch2_data1, ch2_data2;
	int16_t fft_data1, fft_data2;

	p_oszi_draw_background();
	UB_LCD_SetLayer_Menu();

	// Start values
	ch1_data1 = oszi_adc2pixel(ADC_DMA_Buffer_C[0], menu->Ch1.Factor);
	ch1_data1 += SCALE_Y_MITTE + SCALE_START_X + menu->Ch1.Position;
	if (ch1_data1 < SCALE_START_X)
		ch1_data1 = SCALE_START_X;
	if (ch1_data1 > SCALE_MX_PIXEL)
		ch1_data1 = SCALE_MX_PIXEL;

	ch2_data1 = oszi_adc2pixel(ADC_DMA_Buffer_C[1], menu->Ch2.Factor);
	ch2_data1 += SCALE_Y_MITTE + SCALE_START_X + menu->Ch2.Position;
	if (ch2_data1 < SCALE_START_X)
		ch2_data1 = SCALE_START_X;
	if (ch2_data1 > SCALE_MX_PIXEL)
		ch2_data1 = SCALE_MX_PIXEL;

	fft_data1 = FFT_UINT_DATA[0];
	fft_data1 += FFT_START_X;
	if (fft_data1 < SCALE_START_X)
		fft_data1 = SCALE_START_X;
	if (fft_data1 > SCALE_MX_PIXEL)
		fft_data1 = SCALE_MX_PIXEL;

	// Complete curve
	for (n = 1; n < SCALE_W; n++)
	{
		if (menu->Ch1.Visible == MENU_CH_VISIBLE_ON)
		{
			ch1_data2 = oszi_adc2pixel(ADC_DMA_Buffer_C[n * 2], menu->Ch1.Factor);
			ch1_data2 += SCALE_Y_MITTE + SCALE_START_X + menu->Ch1.Position;
			if (ch1_data2 < SCALE_START_X)
				ch1_data2 = SCALE_START_X;
			if (ch1_data2 > SCALE_MX_PIXEL)
				ch1_data2 = SCALE_MX_PIXEL;
			UB_Graphic2D_DrawLineNormal(ch1_data1, SCALE_START_Y + n, ch1_data2,
					SCALE_START_Y + n + 1, ADC_CH1_COL);
			ch1_data1 = ch1_data2;
		}

		if (menu->Ch2.Visible == MENU_CH_VISIBLE_ON)
		{
			ch2_data2 = oszi_adc2pixel(ADC_DMA_Buffer_C[(n * 2) + 1], menu->Ch2.Factor);
			ch2_data2 += SCALE_Y_MITTE + SCALE_START_X + menu->Ch2.Position;
			if (ch2_data2 < SCALE_START_X)
				ch2_data2 = SCALE_START_X;
			if (ch2_data2 > SCALE_MX_PIXEL)
				ch2_data2 = SCALE_MX_PIXEL;
			UB_Graphic2D_DrawLineNormal(ch2_data1, SCALE_START_Y + n, ch2_data2,
					SCALE_START_Y + n + 1, ADC_CH2_COL);
			ch2_data1 = ch2_data2;
		}
	}

	// nur die linke hälfte der FFT zeichnen
	// (die rechte ist das Spiegelbild)
	if (menu->FFT != MENU_FFT_MODE_OFF)
	{
		for (n = 1; n < FFT_VISIBLE_LENGTH; n++)
		{
			fft_data2 = FFT_UINT_DATA[n];
			fft_data2 += FFT_START_X;
			if (fft_data2 < SCALE_START_X)
				fft_data2 = SCALE_START_X;
			if (fft_data2 > SCALE_MX_PIXEL)
				fft_data2 = SCALE_MX_PIXEL;
			UB_Graphic2D_DrawLineNormal(fft_data1, FFT_START_Y + n, fft_data2,
					FFT_START_Y + n + 1, FFT_COL);
			fft_data1 = fft_data2;
		}
	}

	UB_LCD_SetLayer_Back();
}

//--------------------------------------------------------------
// To convert from adc value in pixel position
//--------------------------------------------------------------
const uint32_t adc2pixel_factors[] = {
//	FAKTOR_5V	FAKTOR_2V	FAKTOR_1V	FAKTOR_0V5	FAKTOR_0V2	FAKTOR_0V1
	68250ul,	27300ul,	13650ul,	6825ul,		2730ul,		1365ul
};
int16_t oszi_adc2pixel(uint16_t adc, uint16_t factor)
{
	uint32_t value = (uint32_t)adc;
	if (factor <= 5)
		return (value * SCALE_SPACE * 10) / adc2pixel_factors[factor];
	return 0;
}

//--------------------------------------------------------------
// Send data via UART
//--------------------------------------------------------------
void p_oszi_send_data(void)
{
	Menu_t *menu = &Menu;
	uint32_t n;
	uint16_t data1, data2;
	char buf[10];
	extern const SM_Item_t UM_01[];
	extern const SM_Item_t UM_02[];

	//--------------------------------
	// send Screen as Bitmap
	//--------------------------------
	if (menu->Send.Mode == MENU_SEND_MODE_SCREEN)
	{
		p_oszi_send_screen();
		return;
	}

	//--------------------------------
	// send settings
	//--------------------------------
	p_oszi_send_uart("SETTINGS:");
	if (menu->Send.Mode == MENU_SEND_MODE_CH1
	||	menu->Send.Mode == MENU_SEND_MODE_CH1_FFT
	||	menu->Send.Mode == MENU_SEND_MODE_CH1_CH2
	||	menu->Send.Mode == MENU_SEND_MODE_CH1_CH2_FFT
		)
	{
		sprintf(buf, "CH1=%s/div", UM_01[menu->Ch1.Factor].Text);
		p_oszi_send_uart(buf);
	}
	if (menu->Send.Mode == MENU_SEND_MODE_CH2
	||	menu->Send.Mode == MENU_SEND_MODE_CH2_FFT
	||	menu->Send.Mode == MENU_SEND_MODE_CH1_CH2
	||	menu->Send.Mode == MENU_SEND_MODE_CH1_CH2_FFT
		)
	{
		sprintf(buf, "CH2=%s/div", UM_01[menu->Ch2.Factor].Text);
		p_oszi_send_uart(buf);
	}

	sprintf(buf, "Time=%s/div", UM_02[menu->Timebase].Text);
	p_oszi_send_uart(buf);
	p_oszi_send_uart("1div=25");

	sprintf(buf, "count=%d", SCALE_W);
	p_oszi_send_uart(buf);

	//--------------------------------
	// Send data
	//--------------------------------
	p_oszi_send_uart("DATA:");
	if (menu->Send.Mode == MENU_SEND_MODE_CH1 || menu->Send.Mode == MENU_SEND_MODE_CH1_FFT)
	{
		p_oszi_send_uart("CH1");
		for (n = 0; n < SCALE_W; n++)
		{
			data1 = ADC_DMA_Buffer_C[n * 2];
			sprintf(buf, "%d", data1);
			p_oszi_send_uart(buf);
		}
	}
	else if (menu->Send.Mode == MENU_SEND_MODE_CH2 || menu->Send.Mode == MENU_SEND_MODE_CH2_FFT)
	{
		p_oszi_send_uart("CH2");
		for (n = 0; n < SCALE_W; n++)
		{
			data2 = ADC_DMA_Buffer_C[(n * 2) + 1];
			sprintf(buf, "%d", data2);
			p_oszi_send_uart(buf);
		}
	}
	else if (menu->Send.Mode == MENU_SEND_MODE_CH1_CH2 || menu->Send.Mode == MENU_SEND_MODE_CH1_CH2_FFT)
	{
		p_oszi_send_uart("CH1,CH2");
		for (n = 0; n < SCALE_W; n++)
		{
			data1 = ADC_DMA_Buffer_C[n * 2];
			data2 = ADC_DMA_Buffer_C[(n * 2) + 1];
			sprintf(buf, "%d,%d", data1, data2);
			p_oszi_send_uart(buf);
		}
	}

	//--------------------------------
	// Send FFT
	//--------------------------------
	if (menu->Send.Mode == MENU_SEND_MODE_CH1_FFT
	||	menu->Send.Mode == MENU_SEND_MODE_CH2_FFT
	||	menu->Send.Mode == MENU_SEND_MODE_CH1_CH2_FFT
		)
	{
		if (menu->FFT == MENU_FFT_MODE_CH1)
		{
			p_oszi_send_uart("FFT:");
			p_oszi_send_uart("CH1");
			sprintf(buf, "count=%d", FFT_VISIBLE_LENGTH);
			p_oszi_send_uart(buf);
			for (n = 0; n < FFT_VISIBLE_LENGTH; n++)
			{
				data2 = FFT_UINT_DATA[n];
				sprintf(buf, "%d", data2);
				p_oszi_send_uart(buf);
			}
		}
		else if (menu->FFT == MENU_FFT_MODE_CH2)
		{
			p_oszi_send_uart("FFT:");
			p_oszi_send_uart("CH2");
			sprintf(buf, "count=%d", FFT_VISIBLE_LENGTH);
			p_oszi_send_uart(buf);
			for (n = 0; n < FFT_VISIBLE_LENGTH; n++)
			{
				data2 = FFT_UINT_DATA[n];
				sprintf(buf, "%d", data2);
				p_oszi_send_uart(buf);
			}
		}
	}
	p_oszi_send_uart("END.");
}

//--------------------------------------------------------------
// Send string via COM1 UART
//--------------------------------------------------------------
void p_oszi_send_uart(char *ptr)
{
	UB_Uart_SendString(COM1, ptr, CRLF);
}

//--------------------------------------------------------------
// Send screen as a bitmap (* .bmp) via COM1 UART
// about 20 seconds at 115200 baud
//--------------------------------------------------------------
void p_oszi_send_screen(void)
{
	uint32_t n, addr;
	uint16_t x, y, color;

	// Send BMP-Header
	for (n = 0; n < sizeof(BMP_HEADER); n++)
		UB_Uart_SendByte(COM1, BMP_HEADER[n]);

	// The buffer Richigen raussuchen to send
	addr = LCD_FRAME_BUFFER;
	if (LCD_Context.LCD_CurrentLayer != 1)
		addr += LCD_FRAME_OFFSET;

	// Send all color data
	for (x = 0; x < LCD_MAXX; x++)
		for (y = 0; y < LCD_MAXY; y++)
		{
			n = y * LCD_MAXX * 2 + x * 2;
			color = *(uint16_t*)(addr + n);
			UB_Uart_SendByte(COM1, (color & 0x001F) << 3);	// 5bit blue
			UB_Uart_SendByte(COM1, (color & 0x07E0) >> 3);	// 6bit green;
			UB_Uart_SendByte(COM1, (color & 0xF800) >> 8);	// 5bit red;
		}
}
