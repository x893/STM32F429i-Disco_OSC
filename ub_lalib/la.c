//--------------------------------------------------------------
// File     : la.c
// Datum    : 23.08.2014
// Version  : 1.0
// Autor    : UB
// EMail    : mc-4u(@)t-online.de
// Web      : www.mikrocontroller-4u.de
// CPU      : STM32F429
// IDE      : CooCox CoIDE 1.7.4
// GCC      : 4.7 2012q4
// Module   : STM32_UB_SYSTICK, STM32_UB_LED, STM32_UB_DIN_DMA
// Funktion : Logic-Analyzer
//--------------------------------------------------------------


//--------------------------------------------------------------
// Includes
//--------------------------------------------------------------
#include "la.h"



//--------------------------------------------------------------
// Einstellungen fuer die "samplerate"
// SUMP-Config : "device.samplerates"
//
// frq      = Sample-Frq in Hz
// prescale = prescale Einstellung vom Timer
// periode  = periode Einstellung vom Timer
//--------------------------------------------------------------
const LA_TIM_Item_t LA_TIM_Item[] = {
	// frq, prescale, periode
	{ 1000, 167, 999 }, // 1kHz
	{ 2500, 167, 399 }, // 2,5kHz
	{ 5000, 167, 199 }, // 5kHz
	{ 10000, 167, 99 },  // 10kHz
	{ 25000, 167, 39 },  // 25kHz (default)
	{ 50000, 167, 19 },  // 50kHz
	{ 100000, 167, 9 },   // 100kHz
	{ 250000, 41, 15 },   // 250kHz
	{ 500000, 41, 7 },    // 500kHz
	{ 1000000, 20, 7 },    // 1MHz
	{ 2000000, 1, 41 },    // 2MHz
	{ 4000000, 1, 20 },    // 4MHz
	{ 6000000, 1, 13 },    // 6MHz
	{ 8000000, 2, 6 },     // 8MHz
	{ 10500000, 3, 3 },     // 10,5MHz
	{ 12000000, 1, 6 },     // 12MHz
	{ 14000000, 2, 3 },     // 14MHz
	{ 21000000, 1, 3 },     // 21MHz
	{ 24000000, 0, 6 },     // 24MHz
};
uint32_t frq_item_anz = sizeof(LA_TIM_Item) / sizeof(LA_TIM_Item[0]);


//--------------------------------------------------------------
// Einstellungen fuer die "capturesize"
// SUMP-Config : "device.capturesizes"
//
// sump   = laenge (von sump uebermittelt)
// len    = laenge in bytes
// size   = buffer size (max 65535)
// anzahl = buffer anzahl
//--------------------------------------------------------------
const LA_SAMPLE_Item_t LA_SAMPLE_Item[] = {
	// sump  ,len    , size,anzahl
	{ 0x007F, 512, 256, 1 },  // 512 S
	{ 0x00FF, 1024, 512, 1 },  // 1024 1k
	{ 0x01FF, 2048, 1024, 1 },  // 2048 2k
	{ 0x03FF, 4096, 2048, 1 },  // 4096 4k
	{ 0x07FF, 8192, 4096, 1 },  // 8192 8k (default)
	{ 0x0FFF, 16384, 8192, 1 },  // 16384 16k
	{ 0x1FFF, 32768, 16384, 1 },  // 32768 32k
	{ 0x4FFF, 65536, 32768, 1 },  // 65536 64k
	{ 0x7FFF, 131072, 32768, 2 },  // 131072 128k
	{ 0xFFFF, 262144, 32768, 4 },  // 262144 256k
	{ 0xE847, 500000, 50000, 5 },  // 5000000 500T
	{ 0xD08F, 1000000, 50000, 10 }, // 1000000 1M
};
uint32_t sampl_item_anz = sizeof(LA_SAMPLE_Item) / sizeof(LA_SAMPLE_Item[0]);


//--------------------------------------------------------------
// interne Funktionen
//--------------------------------------------------------------
void p_la_init_sw(void);
void p_la_init_hw(void);
void p_la_update_setting_frq(void);
void p_la_update_setting_size(void);
void p_la_update_setting_mask(void);
void p_la_update_setting_value(void);



//--------------------------------------------------------------
// init vom Logic-Analyzer
//--------------------------------------------------------------
void la_init(void)
{
	p_la_init_hw();
	p_la_init_sw();
}


//--------------------------------------------------------------
// test ob Triggerbedingung erfuellt ist
// falls ja -> start vom capture
//--------------------------------------------------------------
void la_check_trigger(void)
{
	uint16_t port_wert, trg_wert;

	port_wert = *(volatile uint16_t*)(DIN_DMA_GPIO_IDR_ADDRESS);
	trg_wert = (port_wert^LA.trigger.value); // gleiche bits werden zu 0
	trg_wert &= LA.trigger.maske; // unnoetige bits werden zu 0

	if (trg_wert == 0x00) {
		UB_DIN_DMA_Capture();
	}
}


//--------------------------------------------------------------
// daten von adresse (adr) auslesen
// ret_wert : 8bit = Kanal-0 bis Kanal-7
//--------------------------------------------------------------
uint8_t la_readData_8b(uint32_t adr)
{
	uint8_t ret_wert = 0;
	uint16_t wert;

	wert = UB_DIN_DMA_Read_PORT(adr);
	if ((wert&LA.ch[0].bitmask) != 0) ret_wert |= 0x01;
	if ((wert&LA.ch[1].bitmask) != 0) ret_wert |= 0x02;
	if ((wert&LA.ch[2].bitmask) != 0) ret_wert |= 0x04;
	if ((wert&LA.ch[3].bitmask) != 0) ret_wert |= 0x08;
	if ((wert&LA.ch[4].bitmask) != 0) ret_wert |= 0x10;
	if ((wert&LA.ch[5].bitmask) != 0) ret_wert |= 0x20;
	if ((wert&LA.ch[6].bitmask) != 0) ret_wert |= 0x40;
	if ((wert&LA.ch[7].bitmask) != 0) ret_wert |= 0x80;

	return(ret_wert);
}


//--------------------------------------------------------------
// update der Einstellungen (je nach kommando)
//--------------------------------------------------------------
void la_update_settings(LA_COM_t com_status)
{
	if (DIN_DMA.Status == DIN_DMA_WAITING) {
		if (com_status == LA_COM_SET_FRQ) {
			p_la_update_setting_frq();
		}

		if (com_status == LA_COM_SET_SIZE) {
			p_la_update_setting_size();
		}

		if (com_status == LA_COM_SET_MASK) {
			p_la_update_setting_mask();
		}

		if (com_status == LA_COM_SET_VALUE) {
			p_la_update_setting_value();
		}
	}
}


//--------------------------------------------------------------
// interne Funktion
// update der Sample-Frq-Einstellung
//--------------------------------------------------------------
void p_la_update_setting_frq(void)
{
	uint16_t n;
	uint32_t wert, frq;

	if (LA.sump.id == SUMP_CMD_ID_SET_FRQ) {
		// zuerst auf default
		LA.setting.frq = 0;

		// devider
		wert = (LA.sump.b2 << 16);
		wert |= (LA.sump.b1 << 8);
		wert |= LA.sump.b0;
		wert += 1;
		// wert als frq
		frq = (LA_SUMP_CALC_FRQ / wert);
		// eintrag suchen
		for (n = 0; n < LA.setting.frq_anz; n++) {
			if (frq >= LA_TIM_Item[n].frq) {
				// wert speichern
				LA.setting.frq = n;
			}
		}
	}

	DIN_DMA.tim_prescale = LA_TIM_Item[LA.setting.frq].prescale;
	DIN_DMA.tim_periode = LA_TIM_Item[LA.setting.frq].periode;

	UB_DIN_DMA_ReInitTimer();
}


//--------------------------------------------------------------
// interne Funktion
// update der Sample-Dauer-Einstellung
//--------------------------------------------------------------
void p_la_update_setting_size(void)
{
	uint16_t n;
	uint32_t wert;

	if (LA.sump.id == SUMP_CMD_ID_SET_SIZE) {
		// zuerst auf default
		LA.setting.sample = 0;

		// anzahl der sample werte
		wert = (LA.sump.b1 << 8);
		wert |= LA.sump.b0;
		// eintrag suchen (genauer wert)
		for (n = 0; n < LA.setting.sample_anz; n++) {
			if (wert == LA_SAMPLE_Item[n].sump_len) {
				// wert speichern
				LA.setting.sample = n;
				break;
			}
		}
	}

	DIN_DMA.buf_len = LA_SAMPLE_Item[LA.setting.sample].size;
	DIN_DMA.buf_anz = LA_SAMPLE_Item[LA.setting.sample].anz;

	UB_DIN_DMA_ReInitDMA();
}


//--------------------------------------------------------------
// interne Funktion
// update der Trigger-Maske-Einstellung
//--------------------------------------------------------------
void p_la_update_setting_mask(void)
{
	if (LA.sump.id == SUMP_CMD_ID_SET_MASK0) {
		LA.trigger.maske = 0x00;
		if ((LA.sump.b0 & 0x01) != 0) LA.trigger.maske |= LA.ch[0].bitmask;
		if ((LA.sump.b0 & 0x02) != 0) LA.trigger.maske |= LA.ch[1].bitmask;
		if ((LA.sump.b0 & 0x04) != 0) LA.trigger.maske |= LA.ch[2].bitmask;
		if ((LA.sump.b0 & 0x08) != 0) LA.trigger.maske |= LA.ch[3].bitmask;
		if ((LA.sump.b0 & 0x10) != 0) LA.trigger.maske |= LA.ch[4].bitmask;
		if ((LA.sump.b0 & 0x20) != 0) LA.trigger.maske |= LA.ch[5].bitmask;
		if ((LA.sump.b0 & 0x40) != 0) LA.trigger.maske |= LA.ch[6].bitmask;
		if ((LA.sump.b0 & 0x80) != 0) LA.trigger.maske |= LA.ch[7].bitmask;
	}
}


//--------------------------------------------------------------
// interne Funktion
// update der Trigger-Value-Einstellung
//--------------------------------------------------------------
void p_la_update_setting_value(void)
{
	if (LA.sump.id == SUMP_CMD_ID_SET_VAL0) {
		LA.trigger.value = 0x00;
		if ((LA.sump.b0 & 0x01) != 0) LA.trigger.value |= LA.ch[0].bitmask;
		if ((LA.sump.b0 & 0x02) != 0) LA.trigger.value |= LA.ch[1].bitmask;
		if ((LA.sump.b0 & 0x04) != 0) LA.trigger.value |= LA.ch[2].bitmask;
		if ((LA.sump.b0 & 0x08) != 0) LA.trigger.value |= LA.ch[3].bitmask;
		if ((LA.sump.b0 & 0x10) != 0) LA.trigger.value |= LA.ch[4].bitmask;
		if ((LA.sump.b0 & 0x20) != 0) LA.trigger.value |= LA.ch[5].bitmask;
		if ((LA.sump.b0 & 0x40) != 0) LA.trigger.value |= LA.ch[6].bitmask;
		if ((LA.sump.b0 & 0x80) != 0) LA.trigger.value |= LA.ch[7].bitmask;
	}
}


//--------------------------------------------------------------
// interne Funktion
// init aller Software Komponenten
//--------------------------------------------------------------
void p_la_init_sw(void)
{
	uint32_t n;

	//--------------------------------------
	// default Einstellung vom Logic-Analyzer
	// samplerate  = 25KHz
	// capturesize = 8 kSamples
	//--------------------------------------
	LA.setting.frq_anz = frq_item_anz;
	LA.setting.frq = 4;     // 4=25kHz
	LA.setting.sample_anz = sampl_item_anz;
	LA.setting.sample = 4;  // 4=8 kSamples


	//--------------------------------------
	// init aller Variabeln
	//--------------------------------------
	LA.trigger.maske = 0x00;
	LA.trigger.value = 0x00;

	for (n = 0; n < LA_CHANNEL_ANZ; n++) {
		LA.ch[n].bitmask = 0;
		LA.ch[n].trigger = Trigger_NONE;
	}

	//--------------------------------------
	LA.ch[0].bitmask = DIN_DMA_GPIO_PIN0;
	LA.ch[1].bitmask = DIN_DMA_GPIO_PIN1;
	LA.ch[2].bitmask = DIN_DMA_GPIO_PIN2;
	LA.ch[3].bitmask = DIN_DMA_GPIO_PIN3;
	LA.ch[4].bitmask = DIN_DMA_GPIO_PIN4;
	LA.ch[5].bitmask = DIN_DMA_GPIO_PIN5;
	LA.ch[6].bitmask = DIN_DMA_GPIO_PIN6;
	LA.ch[7].bitmask = DIN_DMA_GPIO_PIN7;

	//--------------------------------------
	LA.sump.id = 0x00;
	LA.sump.b0 = 0x00;
	LA.sump.b1 = 0x00;
	LA.sump.b2 = 0x00;
	LA.sump.b3 = 0x00;

	//--------------------------------------
	la_update_settings(LA_COM_SET_FRQ);
	la_update_settings(LA_COM_SET_SIZE);
}


//--------------------------------------------------------------
// Init all hardware components
//--------------------------------------------------------------
void p_la_init_hw(void)
{
	UB_Systick_Init();
	UB_Led_Init();
	UB_DIN_DMA_Init();
}







