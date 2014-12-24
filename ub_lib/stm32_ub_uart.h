//--------------------------------------------------------------
// File     : stm32_ub_uart.h
//--------------------------------------------------------------

#ifndef __STM32F4_UB_UART_H
#define __STM32F4_UB_UART_H

#include "stm32f4xx.h"

//--------------------------------------------------------------
// Liste aller UARTs
// (keine Nummer doppelt und von 0 beginnend)
//--------------------------------------------------------------
typedef enum
{
	UART_FIRST	= 0,
	COM1 = 0,	// COM1 (TX=PA9, RX=PA10)
	UART_LAST
} UART_NAME_t;

//--------------------------------------------------------------
// End ID when sending
//--------------------------------------------------------------
typedef enum {
	NONE = 0,  // no end identifier
	LFCR,      // Line feed + Carriage return (0x0A, 0x0D)
	CRLF,      // Carriage return + Line feed (0x0D, 0x0A)
	LF,        // only Line feed (0x0A)
	CR         // only Carriage return (0x0D)
} UART_LASTBYTE_t;

//--------------------------------------------------------------
// Status when receiving
//--------------------------------------------------------------
typedef enum {
	RX_EMPTY = 0,  // Received anything
	RX_READY,      // It is what in the receive buffer
	RX_FULL        // RX buffer is full
} UART_RXSTATUS_t;

//--------------------------------------------------------------
// Structure of a UART pins
//--------------------------------------------------------------
typedef struct {
	GPIO_TypeDef* PORT;		// Port
	const uint16_t PIN;		// Pin
	const uint32_t CLK;		// Clock
	const uint8_t SOURCE;	// Source
} UART_PIN_t;

//--------------------------------------------------------------
// Structure of a UART
//--------------------------------------------------------------
typedef struct {
	UART_NAME_t UART_NAME;	// Name
	const uint32_t CLK;		// Clock
	const uint8_t AF;		// AF
	USART_TypeDef* UART;	// UART
	const uint32_t BAUD;	// Baudrate
	const uint8_t INT;		// Interrupt
	UART_PIN_t TX;			// TX-Pin
	UART_PIN_t RX;			// RX-Pin
} UART_t;

//--------------------------------------------------------------
// Defines for receiving
//--------------------------------------------------------------
#define  RX_BUF_SIZE   50	// Size of the RX buffer in bytes
#define  RX_FIRST_CHR  ' '	// First allowed characters (ASCII value)
#define  RX_LAST_CHR   0x7E	// Last allowed characters (ASCII value)
#define  RX_END_CHR    '\r'	// End code (ASCII value)

//--------------------------------------------------------------
// Structure for UART_RX
//--------------------------------------------------------------
typedef struct {
	uint8_t Index;				// Write pointer
	UART_RXSTATUS_t Status;		// RX status
	char RxBuffer[RX_BUF_SIZE];	// RX buffer
} UART_RX_t;
extern UART_RX_t UART_RX[UART_LAST];

//--------------------------------------------------------------
// Global Functions
//--------------------------------------------------------------
void UB_Uart_Init(void);
void UB_Uart_SendByte(UART_NAME_t uart, uint16_t data);
void UB_Uart_SendString(UART_NAME_t uart, const char *src, UART_LASTBYTE_t end_cmd);
UART_RXSTATUS_t UB_Uart_ReceiveString(UART_NAME_t uart, char *ptr);

//--------------------------------------------------------------
#endif // __STM32F4_UB_UART_H
