//--------------------------------------------------------------
// File     : stm32_ub_font.c
// Datum    : 25.10.2013
// Version  : 1.0
// Autor    : UB
// EMail    : mc-4u(@)t-online.de
// Web      : www.mikrocontroller-4u.de 
// CPU      : STM32F429
// IDE      : CooCox CoIDE 1.7.4
// GCC      : 4.7 2012q4
// Module   : STM32_UB_LCD_ILI9341
// Funktion : Text-LCD Funktionen
//--------------------------------------------------------------


//--------------------------------------------------------------
// Includes
//--------------------------------------------------------------
#include "stm32_ub_font.h"
#include "stm32_ub_lcd_ili9341.h"



//--------------------------------------------------------------
// Zeichnet ein Ascii-Zeichen eines Fonts an x,y Position
// mit Vorder- und Hintergrundfarbe (Font = max 16 Pixel breite)
// -> Font muss mit &-Operator uebergeben werden
//--------------------------------------------------------------
void UB_Font_DrawChar(uint16_t x, uint16_t y, uint8_t ascii, const UB_Font *font, uint16_t fg, uint16_t bg)
{
	uint16_t xn, yn, start_mask, mask;
	const uint16_t *bitmap;

	ascii -= ' ';
	bitmap = &font->table[ascii * font->height];

	start_mask = (font->width > 8) ? 0x8000 : 0x80;
	for (yn = 0; yn < font->height; yn++)
	{
		mask = start_mask;
		// Set cursor
		if (LCD_Context.LCD_DISPLAY_MODE == PORTRAIT)
		{
			UB_LCD_SetCursor2Draw(x, yn + y);
		}
		else
		{
			UB_LCD_SetCursor2Draw(x + font->height - yn, y);
		}
		for (xn = 0; xn < font->width; xn++)
		{
			if ((bitmap[yn] & mask) == 0x00)
			{
					// Draw pixels in background color
				UB_LCD_DrawPixel(bg);
			}
			else {
				// Draw pixels in the foreground color
				UB_LCD_DrawPixel(fg);
			}
			mask >>= 1;
		}
	}
}


//--------------------------------------------------------------
// Draws a string of a font to x, y position
// with foreground and background color (font = max 16 pixels wide)
// -> Font must be handed over with & operator
//--------------------------------------------------------------
void UB_Font_DrawString(uint16_t x, uint16_t y, const char *ptr, const UB_Font *font, uint16_t fg, uint16_t bg)
{
	uint16_t pos;
	char ch;

	if (LCD_Context.LCD_DISPLAY_MODE == PORTRAIT)
	{
		pos = x;
		while ((ch = *ptr++) != 0)
		{
			UB_Font_DrawChar(pos, y, ch, font, fg, bg);
			pos += font->width;
		}
	}
	else
	{
		pos = y;
		while ((ch = *ptr++) != 0)
		{
			UB_Font_DrawChar(x, pos, ch, font, fg, bg);
			pos += font->width;
		}
	}
}


//--------------------------------------------------------------
// Draws an ascii characters in a font of x, y position
// with foreground and background color (font = max 32 pixels wide)
// -> Font must be handed over with & operator
//--------------------------------------------------------------
void UB_Font_DrawChar32(uint16_t x, uint16_t y, uint8_t ascii, const UB_Font32 *font, uint16_t fg, uint16_t bg)
{
	uint16_t xn, yn;
	uint32_t start_mask, mask;
	const uint32_t *bitmap;

	ascii -= ' ';
	bitmap = &font->table[ascii * font->height];

	if (font->width > 16)
		start_mask = 0x80000000;
	else if (font->width > 8)
		start_mask = 0x8000;
	else
		start_mask = 0x80;

	for (yn = 0; yn < font->height; yn++)
	{
		mask = start_mask;
		// Set the cursor
		if (LCD_Context.LCD_DISPLAY_MODE == PORTRAIT)
		{
			UB_LCD_SetCursor2Draw(x, yn + y);
		}
		else
		{
			UB_LCD_SetCursor2Draw(x + font->height - yn, y);
		}

		for (xn = 0; xn < font->width; xn++)
		{
			if ((bitmap[yn] & mask) == 0x00)
			{
				// Draw pixels in background color
				UB_LCD_DrawPixel(bg);
			}
			else {
				// Draw pixels in the foreground color
				UB_LCD_DrawPixel(fg);
			}
			mask >>= 1;
		}
	}
}


//--------------------------------------------------------------
// Zeichnet einen String eines Fonts an x,y Position
// mit Vorder- und Hintergrundfarbe (Font = max 32 Pixel breite)
// -> Font muss mit &-Operator uebergeben werden
//--------------------------------------------------------------
void UB_Font_DrawString32(uint16_t x, uint16_t y, const char *ptr, const UB_Font32 *font, uint16_t fg, uint16_t bg)
{
	uint16_t pos;
	char ch;

	if (LCD_Context.LCD_DISPLAY_MODE == PORTRAIT)
	{
		pos = x;
		while ((ch = *ptr++) != '\0')
		{
			UB_Font_DrawChar32(pos, y, ch, font, fg, bg);
			pos += font->width;
		}
	}
	else
	{
		pos = y;
		while ((ch = *ptr++) != '\0')
		{
			UB_Font_DrawChar32(x, pos, ch, font, fg, bg);
			pos += font->width;
		}
	}
}
