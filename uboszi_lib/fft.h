//--------------------------------------------------------------
// File     : stm32_ub_fft.h
//--------------------------------------------------------------
#ifndef __STM32F4_UB_FFT_H
#define __STM32F4_UB_FFT_H


#include "stm32f4xx.h"
#include <math.h>
#include "arm_math.h"

//--------------------------------------------------------------
// FFT Buffer size
//--------------------------------------------------------------
#define FFT_LENGTH              512 // dont change
#define FFT_CMPLX_LENGTH        FFT_LENGTH*2
#define FFT_VISIBLE_LENGTH      FFT_LENGTH/2

//--------------------------------------------------------------
#define FFT_UINT_MAXWERT        100  // max = 100 Pixel

//--------------------------------------------------------------
// FFT Buffer
//--------------------------------------------------------------
extern float32_t FFT_DATA_IN[FFT_LENGTH];
extern uint16_t FFT_UINT_DATA[FFT_VISIBLE_LENGTH];


uint32_t fft_init(void);
void fft_calc(void);


#endif // __STM32F4_UB_FFT_H
