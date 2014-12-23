//--------------------------------------------------------------
// File     : stm32_ub_touch_stmpe811.c
// Datum    : 02.11.2013
// Version  : 1.0
// Autor    : UB
// EMail    : mc-4u(@)t-online.de
// Web      : www.mikrocontroller-4u.de
// CPU      : STM32F429
// IDE      : CooCox CoIDE 1.7.4
// GCC      : 4.7 2012q4
// Module   : GPIO, STM32_UB_I2C3
// Funktion : Touch-Controller (Chip = STMPE811)
//
// Hinweis  : Settings :
//            I2C-Slave-ADR = [0x82]
//            FRQ-Max = 100kHz
//            I2C3 [SCL=PA8, SDA=PC9]
//--------------------------------------------------------------


//--------------------------------------------------------------
// Includes
//--------------------------------------------------------------
#include "stm32_ub_touch_stmpe811.h"

Touch_Data_t Touch_Data;

//--------------------------------------------------------------
// interne Funktionen
//--------------------------------------------------------------
void P_Touch_Reset(void);
uint8_t P_Touch_FnctCmd(uint8_t Fct, FunctionalState NewState);
void P_Touch_Config(void);
uint16_t P_Touch_ReadID(void);
uint8_t P_Touch_IOAFConfig(uint8_t IO_Pin, FunctionalState NewState);
uint16_t P_Touch_Read_X(void);
uint16_t P_Touch_Read_Y(void);
uint16_t P_Touch_Read_16b(uint32_t RegisterAddr);

//--------------------------------------------------------------
// Init vom Touch
// Return_wert :
//  -> ERROR   , wenn Touch nicht gefunden wurde
//  -> SUCCESS , wenn Touch OK
//--------------------------------------------------------------
ErrorStatus UB_Touch_Init(void)
{
	uint16_t stmpe_id = 0;

	// init vom I2C
	UB_I2C3_Init();

	// check vom STMPE811
	stmpe_id = P_Touch_ReadID();
	if (stmpe_id != STMPE811_ID) {
		return(ERROR);
	}

	// SW-Reset vom Touch
	P_Touch_Reset();

	// init
	P_Touch_FnctCmd(IOE_ADC_FCT, ENABLE);
	P_Touch_Config();

	return(SUCCESS);
}


//--------------------------------------------------------------
// auslesen vom Touch-Status und der Touch-Koordinaten
// Return_wert :
//  -> ERROR   , wenn Touch nicht gefunden wurde
//  -> SUCCESS , wenn Touch OK
//
// Touch_Data.status : [TOUCH_PRESSED, TOUCH_RELEASED]
// Touch_Data.xp     : [0...239]
// Touch_Data.yp     : [0...319] 
//--------------------------------------------------------------
ErrorStatus UB_Touch_Read(void)
{
	uint16_t xDiff, yDiff, x, y;
	int16_t i2c_status;

	i2c_status = UB_I2C3_ReadByte(STMPE811_I2C_ADDR, IOE_REG_TP_CTRL);
	if (i2c_status < 0) return(ERROR);

	Touch_Data.Status = ((i2c_status & 0x80) == 0) ? TOUCH_RELEASED : TOUCH_PRESSED;

	if (Touch_Data.Status == TOUCH_PRESSED)
	{
		x = P_Touch_Read_X();
		y = P_Touch_Read_Y();
		xDiff = (x > Touch_Data.XOld) ? (x - Touch_Data.XOld) : (Touch_Data.XOld - x);
		yDiff = (y > Touch_Data.YOld) ? (y - Touch_Data.YOld) : (Touch_Data.YOld - y);
		if (xDiff + yDiff > 5)
		{
			Touch_Data.XOld = x;
			Touch_Data.YOld = y;
		}
	}

	Touch_Data.XPos = Touch_Data.XOld;
	Touch_Data.YPos = Touch_Data.YOld;

	UB_I2C3_WriteByte(STMPE811_I2C_ADDR, IOE_REG_FIFO_STA, 0x01);
	UB_I2C3_WriteByte(STMPE811_I2C_ADDR, IOE_REG_FIFO_STA, 0x00);

	return(SUCCESS);
}


//--------------------------------------------------------------
// interne Funktion
//--------------------------------------------------------------
void P_Touch_Reset(void)
{
	UB_I2C3_WriteByte(STMPE811_I2C_ADDR, IOE_REG_SYS_CTRL1, 0x02);
	UB_I2C3_Delay(STMPE811_DELAY);
	UB_I2C3_WriteByte(STMPE811_I2C_ADDR, IOE_REG_SYS_CTRL1, 0x00);
}


//--------------------------------------------------------------
// interne Funktion
// return : 0=ok, >0 = error
//--------------------------------------------------------------
uint8_t P_Touch_FnctCmd(uint8_t Fct, FunctionalState NewState)
{
	uint8_t tmp = 0;
	int16_t i2c_data;

	i2c_data = UB_I2C3_ReadByte(STMPE811_I2C_ADDR, IOE_REG_SYS_CTRL2);
	if (i2c_data < 0)
		return (1);

	tmp = (uint8_t)(i2c_data);
	if (NewState != DISABLE)
		tmp &= ~(uint8_t)Fct;
	else
		tmp |= (uint8_t)Fct;

	UB_I2C3_WriteByte(STMPE811_I2C_ADDR, IOE_REG_SYS_CTRL2, tmp);
	return (0);
}


//--------------------------------------------------------------
// interne Funktion
//--------------------------------------------------------------
void P_Touch_Config(void)
{
	P_Touch_FnctCmd(IOE_TP_FCT, ENABLE);
	UB_I2C3_WriteByte(STMPE811_I2C_ADDR, IOE_REG_ADC_CTRL1, 0x49);
	UB_I2C3_Delay(STMPE811_DELAY);
	UB_I2C3_WriteByte(STMPE811_I2C_ADDR, IOE_REG_ADC_CTRL2, 0x01);
	P_Touch_IOAFConfig((uint8_t)TOUCH_IO_ALL, DISABLE);
	UB_I2C3_WriteByte(STMPE811_I2C_ADDR, IOE_REG_TP_CFG, 0x9A);
	UB_I2C3_WriteByte(STMPE811_I2C_ADDR, IOE_REG_FIFO_TH, 0x01);
	UB_I2C3_WriteByte(STMPE811_I2C_ADDR, IOE_REG_FIFO_STA, 0x01);
	UB_I2C3_WriteByte(STMPE811_I2C_ADDR, IOE_REG_FIFO_STA, 0x00);
	UB_I2C3_WriteByte(STMPE811_I2C_ADDR, IOE_REG_TP_FRACT_XYZ, 0x01);
	UB_I2C3_WriteByte(STMPE811_I2C_ADDR, IOE_REG_TP_I_DRIVE, 0x01);
	UB_I2C3_WriteByte(STMPE811_I2C_ADDR, IOE_REG_TP_CTRL, 0x03);
	UB_I2C3_WriteByte(STMPE811_I2C_ADDR, IOE_REG_INT_STA, 0xFF);

	Touch_Data.Status = TOUCH_RELEASED;
	Touch_Data.XPos = 0;
	Touch_Data.YPos = 0;
}


//--------------------------------------------------------------
// interne Funktion
// ID auslesen
//--------------------------------------------------------------
uint16_t P_Touch_ReadID(void)
{
	int16_t i2c_data1, i2c_data2;

	i2c_data1 = UB_I2C3_ReadByte(STMPE811_I2C_ADDR, 0);
	i2c_data2 = UB_I2C3_ReadByte(STMPE811_I2C_ADDR, 1);

	if (i2c_data1 < 0 || i2c_data2 < 0)
		return 0;
	return (uint16_t)((i2c_data1 << 8) | i2c_data2);
}


//--------------------------------------------------------------
// interne Funktion
// return : 0=ok, >0 = error
//--------------------------------------------------------------
uint8_t P_Touch_IOAFConfig(uint8_t IO_Pin, FunctionalState NewState)
{
	uint8_t tmp;
	int16_t i2c_data;

	i2c_data = UB_I2C3_ReadByte(STMPE811_I2C_ADDR, IOE_REG_GPIO_AF);
	if (i2c_data < 0)
		return(1);

	tmp = i2c_data;
	if (NewState != DISABLE)
		tmp |= (uint8_t)IO_Pin;
	else
		tmp &= ~(uint8_t)IO_Pin;

	UB_I2C3_WriteByte(STMPE811_I2C_ADDR, IOE_REG_GPIO_AF, tmp);
	return (0);
}


//--------------------------------------------------------------
// interne Funktion
//--------------------------------------------------------------
uint16_t P_Touch_Read_X(void)
{
	int32_t x, xr;

	x = P_Touch_Read_16b(IOE_REG_TP_DATA_X);

	if (x <= 3000)
		x = 3870 - x;
	else
		x = 3800 - x;

	xr = x / 15;
	if (xr <= 0)
		xr = 0;
	else if (xr >= 240)
		xr = 239;

	return (uint16_t)(xr);
}


//--------------------------------------------------------------
// interne Funktion
//--------------------------------------------------------------
uint16_t P_Touch_Read_Y(void)
{
	int32_t y, yr;

	y = P_Touch_Read_16b(IOE_REG_TP_DATA_Y);
	y -= 360;
	yr = y / 11;

	if (yr <= 0)
		yr = 0;
	else if (yr >= 320)
		yr = 319;

	return (uint16_t)(yr);
}


//--------------------------------------------------------------
// interne Funktion
//--------------------------------------------------------------
uint16_t P_Touch_Read_16b(uint32_t reg)
{
	int16_t i2c_data1, i2c_data2;

	i2c_data1 = UB_I2C3_ReadByte(STMPE811_I2C_ADDR, reg);
	i2c_data2 = UB_I2C3_ReadByte(STMPE811_I2C_ADDR, reg + 1);

	if (i2c_data1 < 0 || i2c_data2 < 0)
		return 0;

	return ((i2c_data1 << 8) | i2c_data2);
}


