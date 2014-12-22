//--------------------------------------------------------------
// File     : stm32_ub_fft.c
// Datum    : 11.03.2014
// Version  : 1.1
// Autor    : UB
// EMail    : mc-4u(@)t-online.de
// Web      : www.mikrocontroller-4u.de
// CPU      : STM32F429
// IDE      : CooCox CoIDE 1.7.4
// GCC      : 4.7 2012q4
// Module   : ARM_DSP-Module
// Funktion : FFT
//
// Hineise :
// "Config/Compile/Options"  : "FPU soft" oder "FPU hard"
// "Config/Compile/Defines"  : "ARM_MATH_CM4" , "__FPU_PRESENT=1"
// "Config/Link/MiscControl" : "-lm; -lgcc; -lc; " hinzufьgen
//--------------------------------------------------------------

#include "fft.h"

static float32_t FFT_CMPLX_DATA[FFT_CMPLX_LENGTH];
static float32_t FFT_MAG_DATA[FFT_LENGTH];

arm_cfft_radix4_instance_f32 S_CFFT;
arm_rfft_instance_f32 S;

//--------------------------------------------------------------
// init the FFT module
// return : 0 = failed
//          1 = init ok
//--------------------------------------------------------------
uint32_t fft_init(void)
{
	// FFT init
	if (arm_rfft_init_f32(&S, &S_CFFT, FFT_LENGTH, 0, 1) != ARM_MATH_SUCCESS)
		return 0;
	return 1;
}

//--------------------------------------------------------------
// calculates the FFT
// expects the data still in pixel values ​​to
//--------------------------------------------------------------
void fft_calc(void)
{
	float32_t maxValue;
	uint32_t n;

	// Calculate FFT
	arm_rfft_f32(&S, FFT_DATA_IN, FFT_CMPLX_DATA);
	arm_cmplx_mag_f32(FFT_CMPLX_DATA, FFT_MAG_DATA, FFT_LENGTH);

	// Search Maximum manually
	// die ersten beiden Eintrдge ьberspringen
	maxValue = 0.1;
	for (n = 2; n < FFT_VISIBLE_LENGTH; n++)
	{
		if (FFT_MAG_DATA[n] > maxValue)
			maxValue = FFT_MAG_DATA[n];
	}

	// normalizing all values ​​to the maximum
	FFT_UINT_DATA[0] = 0;
	FFT_UINT_DATA[1] = 0;
	for (n = 2; n < FFT_VISIBLE_LENGTH; n++)
	{
		FFT_UINT_DATA[n] = (uint16_t)(FFT_UINT_MAXWERT * FFT_MAG_DATA[n] / maxValue);
	}
}

