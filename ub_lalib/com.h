//--------------------------------------------------------------
// File     : com.h
//--------------------------------------------------------------

//--------------------------------------------------------------
#ifndef __STM32F4_UB_COM_H
#define __STM32F4_UB_COM_H


//--------------------------------------------------------------
// Includes
//--------------------------------------------------------------
#include "stm32f4xx.h"
#include "stm32_ub_usb_cdc.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "la.h"


//--------------------------------------------------------------
#define   COM_DELAY_MS     50  // refreshzeit vom COM

//--------------------------------------------------------------
#define   COM_END_BYTE    LFCR  // Endekennung beim senden




//--------------------------------------------------------------
// SUMP : Kommdando ID
//--------------------------------------------------------------
#define   SUMP_CMD_ID_RESET          0x00
#define   SUMP_CMD_ID_ARM_TRG        0x01
#define   SUMP_CMD_ID_GET_ID         0x02
#define   SUMP_CMD_ID_GET_META       0x04

#define   SUMP_CMD_ID_SET_FRQ        0x80
#define   SUMP_CMD_ID_SET_SIZE       0x81
#define   SUMP_CMD_ID_SET_MASK0      0xC0
#define   SUMP_CMD_ID_SET_VAL0       0xC1

//--------------------------------------------------------------
// SUMP : META-Angaben
//--------------------------------------------------------------
#define   SUMP_META_END	             0x00
#define   SUMP_META_NAME	     0x01
#define   SUMP_META_FPGA_VERSION     0x02
#define   SUMP_META_CPU_VERSION	     0x03
#define   SUMP_META_SAMPLE_RAM	     0x21
#define   SUMP_META_SAMPLE_RATE	     0x23
#define   SUMP_META_PROBES	     0x40
#define   SUMP_META_PROTOCOL	     0x41


//--------------------------------------------------------------
// SUMP : META-Settings
//--------------------------------------------------------------
#define maxSampleRate        24000000   // 24 MHz
#define maxSampleMemory     (1000*1000) // 1M
#define maxChannelAnz               8   // 8



//--------------------------------------------------------------
// Byteumwandlung
//--------------------------------------------------------------
#define BYTE1(v) ((uint8_t)v & 0xff)         //LSB
#define BYTE2(v) ((uint8_t)(v >> 8) & 0xff)  //
#define BYTE3(v) ((uint8_t)(v >> 16) & 0xff) //
#define BYTE4(v) ((uint8_t)(v >> 24) & 0xff) //MSB


//--------------------------------------------------------------
// RX-Puffer
//--------------------------------------------------------------
#define  COM_RX_BUF_LEN    100  // laenge in bytes
#define  COM_TX_BUF_LEN    100  // laenge in bytes
extern char com_rx_buf[COM_RX_BUF_LEN];
extern char com_tx_buf[COM_TX_BUF_LEN];





//--------------------------------------------------------------
// Globale Funktionen
//--------------------------------------------------------------
void com_init(void);
LA_COM_t com_do(void);
void com_send_string(char *ptr);
void com_send_all_data(void);


//--------------------------------------------------------------
#endif // __STM32F4_UB_COM_H
