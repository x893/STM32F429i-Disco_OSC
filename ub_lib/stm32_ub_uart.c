//--------------------------------------------------------------
// File     : stm32_ub_uart.c
// Datum    : 28.11.2013
// Version  : 1.1
// Autor    : UB
// EMail    : mc-4u(@)t-online.de
// Web      : www.mikrocontroller-4u.de
// CPU      : STM32F429
// IDE      : CooCox CoIDE 1.7.4
// GCC      : 4.7 2012q4
// Module   : GPIO, USART, MISC
// Funktion : UART (RS232) In und OUT
//            Receive wird per Interrupt behandelt
//
// Hinweis  : mögliche Pinbelegungen
//            UART1 : TX:[PA9,PB6] RX:[PA10,PB7]
//            UART2 : TX:[PA2,PD5] RX:[PA3,PD6]
//            UART3 : TX:[PB10,PC10,PD8] RX:[PB11,PC11,PD9]
//            UART4 : TX:[PA0,PC10] RX:[PA1,PC11]
//            UART5 : TX:[PC12] RX:[PD2]
//            UART6 : TX:[PC6,PG14] RX:[PC7,PG9]
//            UART7 : TX:[PE8,PF7] RX:[PE7,PF6]
//            UART8 : TX:[PE1] RX:[PE0]
//
// Vorsicht : Als Endekennung beim Empfangen, muss der Sender
//            das Zeichen "0x0D" = Carriage-Return
//            an den String anhängen !!
//--------------------------------------------------------------


#include "stm32_ub_uart.h"

UART_RX_t UART_RX[UART_LAST];

#define USE_USART1
#define USE_USART1_IDX	COM1

//--------------------------------------------------------------
// UARTs Definition
//--------------------------------------------------------------
const UART_t UART[1] = {
		// Name	Clock					AF-UART			UART	Baud	Interrupt
	{
		COM1,	RCC_APB2Periph_USART1,	GPIO_AF_USART1,	USART1,	115200, USART1_IRQn, // UART1 115200 Baud
		// PORT, PIN      , Clock              , Source
		{ GPIOA,	GPIO_Pin_9,		RCC_AHB1Periph_GPIOA,	GPIO_PinSource9 },  // TX an PA9
		{ GPIOA,	GPIO_Pin_10,	RCC_AHB1Periph_GPIOA,	GPIO_PinSource10 }
	}, // RX an PA10
};

//--------------------------------------------------------------
// Preparing RX buffer
//--------------------------------------------------------------
void UB_RX_Clear(UART_NAME_t nr)
{
	UART_RX[nr].rx_buffer[0] = RX_END_CHR;
	UART_RX[nr].wr_ptr = 0;
	UART_RX[nr].status = RX_EMPTY;
}

//--------------------------------------------------------------
// Init all UARTs
//--------------------------------------------------------------
void UB_Uart_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	UART_NAME_t nr;

	for (nr = UART_FIRST; nr < UART_LAST; nr++)
	{
		// Clock enable for TX und RX Pins
		RCC_AHB1PeriphClockCmd(UART[nr].TX.CLK, ENABLE);
		RCC_AHB1PeriphClockCmd(UART[nr].RX.CLK, ENABLE);

		// Clock enable for UART
		if (UART[nr].UART == USART1
		||	UART[nr].UART == USART6
			)
		{
			RCC_APB2PeriphClockCmd(UART[nr].CLK, ENABLE);
		}
		else
		{
			RCC_APB1PeriphClockCmd(UART[nr].CLK, ENABLE);
		}

		// Connect UART alternative-function with the IO pins
		GPIO_PinAFConfig(UART[nr].TX.PORT, UART[nr].TX.SOURCE, UART[nr].AF);
		GPIO_PinAFConfig(UART[nr].RX.PORT, UART[nr].RX.SOURCE, UART[nr].AF);

		// UART as an alternative function with push-pull
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
		GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
		// TX-Pin
		GPIO_InitStructure.GPIO_Pin = UART[nr].TX.PIN;
		GPIO_Init(UART[nr].TX.PORT, &GPIO_InitStructure);
		// RX-Pin
		GPIO_InitStructure.GPIO_Pin = UART[nr].RX.PIN;
		GPIO_Init(UART[nr].RX.PORT, &GPIO_InitStructure);

		// Oversampling
		USART_OverSampling8Cmd(UART[nr].UART, ENABLE);

		// Init baudrate, 8 Databits, 1 stop bit, no parity, no RTS + CTS
		USART_InitStructure.USART_BaudRate = UART[nr].BAUD;
		USART_InitStructure.USART_WordLength = USART_WordLength_8b;
		USART_InitStructure.USART_StopBits = USART_StopBits_1;
		USART_InitStructure.USART_Parity = USART_Parity_No;
		USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
		USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
		USART_Init(UART[nr].UART, &USART_InitStructure);

		// UART enable
		USART_Cmd(UART[nr].UART, ENABLE);

		// RX-Interrupt enable
		USART_ITConfig(UART[nr].UART, USART_IT_RXNE, ENABLE);

		// Enable UART Interrupt-Vector
		NVIC_InitStructure.NVIC_IRQChannel = UART[nr].INT;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&NVIC_InitStructure);

		UB_RX_Clear(nr);
	}
}

//--------------------------------------------------------------
// Send a byte via UART
//--------------------------------------------------------------
void UB_Uart_SendByte(UART_NAME_t uart, uint16_t wert)
{
	// Wait until TX buffer empty
	while (USART_GetFlagStatus(UART[uart].UART, USART_FLAG_TXE) == RESET)
	{ }
	USART_SendData(UART[uart].UART, wert);
}

//--------------------------------------------------------------
// Send string via UART senden
//--------------------------------------------------------------
void UB_Uart_SendString(UART_NAME_t uart, const char *ptr, UART_LASTBYTE_t end_cmd)
{
	// sende kompletten String
	while (*ptr != '\0')
	{
		UB_Uart_SendByte(uart, *ptr++);
	}

	// Try sending end identifier
	if (end_cmd == LFCR)
	{
		UB_Uart_SendByte(uart, '\n');
		UB_Uart_SendByte(uart, '\r');
	}
	else if (end_cmd == CRLF)
	{
		UB_Uart_SendByte(uart, '\r');
		UB_Uart_SendByte(uart, '\n');
	}
	else if (end_cmd == LF)
	{
		UB_Uart_SendByte(uart, '\n');
	}
	else if (end_cmd == CR)
	{
		UB_Uart_SendByte(uart, '\r');
	}
}

//--------------------------------------------------------------
// einen String per UART empfangen
// (der Empfang wird per Interrupt abgehandelt)
// diese Funktion muss zyklisch gepollt werden
// Return Wert :
//  -> wenn nichts empfangen = RX_EMPTY
//  -> wenn String empfangen = RX_READY -> String steht in *ptr
//  -> wenn Puffer voll      = RX_FULL
//--------------------------------------------------------------
UART_RXSTATUS_t UB_Uart_ReceiveString(UART_NAME_t uart, char *ptr)
{
	UART_RXSTATUS_t result = RX_EMPTY;
	uint8_t n, ch;

	if (UART_RX[uart].status == RX_READY)
	{
		result = RX_READY;
		n = 0;
		do
		{
			ch = UART_RX[uart].rx_buffer[n];
			if (ch != RX_END_CHR)
			{
				ptr[n++] = ch;
			}
		} while (ch != RX_END_CHR);

		// Add end of line
		ptr[n] = '\0';
		UB_RX_Clear(uart);
	}
	else if (UART_RX[uart].status == RX_FULL)
	{
		result = RX_FULL;
		UB_RX_Clear(uart);
	}

	return result;
}


//--------------------------------------------------------------
// Store the received character in the buffer
//--------------------------------------------------------------
void P_UART_Receive(UART_NAME_t uart, uint16_t ch)
{
	if (UART_RX[uart].wr_ptr < RX_BUF_SIZE)
	{
		// If still space in the buffer
		if (UART_RX[uart].status == RX_EMPTY)
		{
			// If no end code has been received
			if ((ch >= RX_FIRST_CHR) && (ch <= RX_LAST_CHR))
			{
				// Save the byte in the buffer
				UART_RX[uart].rx_buffer[UART_RX[uart].wr_ptr] = ch;
				UART_RX[uart].wr_ptr++;
			}
			if (ch == RX_END_CHR)
			{
				// If end identifier received
				UART_RX[uart].rx_buffer[UART_RX[uart].wr_ptr] = ch;
				UART_RX[uart].status = RX_READY;
			}
		}
	}
	else
	{
		// If the buffer is full
		UART_RX[uart].status = RX_FULL;
	}
}


//--------------------------------------------------------------
// UART-Interrupt-Function
// Interrupt number must be passed
//--------------------------------------------------------------
void P_UART_RX_INT(uint8_t int_nr, uint16_t ch)
{
	UART_NAME_t nr;

	// Search the appropriate entry
	for (nr = UART_FIRST; nr < UART_LAST; nr++)
	{
		if (UART[nr].INT == int_nr)
		{
			// Entry found, save byte
			P_UART_Receive(nr, ch);
			break;
		}
	}
}


//--------------------------------------------------------------
// UART1-Interrupt
//--------------------------------------------------------------
#ifdef USE_USART1
void USART1_IRQHandler(void)
{
	if (USART_GetITStatus(USART1, USART_IT_RXNE) == SET)
	{
		P_UART_Receive(USE_USART1_IDX, USART_ReceiveData(USART1));
	}
}
#endif

//--------------------------------------------------------------
// UART2-Interrupt
//--------------------------------------------------------------
#ifdef USE_USART2
void USART2_IRQHandler(void)
{
	if (USART_GetITStatus(USART2, USART_IT_RXNE) == SET)
	{
		P_UART_RX_INT(USART2_IRQn, USART_ReceiveData(USART2));
	}
}
#endif

//--------------------------------------------------------------
// UART3-Interrupt
//--------------------------------------------------------------
#ifdef USE_USART3
void USART3_IRQHandler(void)
{
	if (USART_GetITStatus(USART3, USART_IT_RXNE) == SET)
	{
		P_UART_RX_INT(USART3_IRQn, USART_ReceiveData(USART3));
	}
}
#endif

//--------------------------------------------------------------
// UART4-Interrupt
//--------------------------------------------------------------
#ifdef USE_UART4
void UART4_IRQHandler(void)
{
	if (USART_GetITStatus(UART4, USART_IT_RXNE) == SET)
	{
		P_UART_RX_INT(UART4_IRQn, USART_ReceiveData(UART4));
	}
}
#endif

//--------------------------------------------------------------
// UART5-Interrupt
//--------------------------------------------------------------
#ifdef USE_UART5
void UART5_IRQHandler(void) {
	if (USART_GetITStatus(UART5, USART_IT_RXNE) == SET)
	{
		P_UART_RX_INT(UART5_IRQn, USART_ReceiveData(UART5));
	}
}
#endif

//--------------------------------------------------------------
// UART6-Interrupt
//--------------------------------------------------------------
#ifdef USE_USART6
void USART6_IRQHandler(void)
{
	if (USART_GetITStatus(USART6, USART_IT_RXNE) == SET)
	{
		P_UART_RX_INT(USART6_IRQn, USART_ReceiveData(USART6));
	}
}
#endif

//--------------------------------------------------------------
// UART7-Interrupt
//--------------------------------------------------------------
#ifdef USE_UART7
void UART7_IRQHandler(void)
{
	if (USART_GetITStatus(UART7, USART_IT_RXNE) == SET)
	{
		// Byte speichern
		P_UART_RX_INT(UART7_IRQn, USART_ReceiveData(UART7));
	}
}
#endif

//--------------------------------------------------------------
// UART8-Interrupt
//--------------------------------------------------------------
#ifdef USE_UART8
void UART8_IRQHandler(void)
{
	if (USART_GetITStatus(UART8, USART_IT_RXNE) == SET)
	{
		// Byte speichern
		P_UART_RX_INT(UART8_IRQn, USART_ReceiveData(UART8));
	}
}
#endif
