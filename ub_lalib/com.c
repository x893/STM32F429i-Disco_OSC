//--------------------------------------------------------------
// File     : com.c
// Datum    : 23.08.2014
// Version  : 1.0
// Autor    : UB
// EMail    : mc-4u(@)t-online.de
// Web      : www.mikrocontroller-4u.de
// CPU      : STM32F429
// IDE      : CooCox CoIDE 1.7.4
// GCC      : 4.7 2012q4
// Module   : USB_CDC
// Funktion : Kommunikation vom Logic-Analyzer
//            per "SUMP-Protokoll"
//--------------------------------------------------------------


//--------------------------------------------------------------
// Includes
//--------------------------------------------------------------
#include "com.h"


//--------------------------------------------------------------
// SUMP : Meta Daten (dont change)
//--------------------------------------------------------------
const char metaData[] = {
	SUMP_META_NAME, 'S', 'T', 'M', '3', '2', 'F', '4', 'U', 'B', 0,
	SUMP_META_FPGA_VERSION, 'n', 'o', 'n', 'e', 0,
	SUMP_META_CPU_VERSION, '1', '.', '0', '0', 0,
	SUMP_META_SAMPLE_RATE, BYTE4(maxSampleRate), BYTE3(maxSampleRate), BYTE2(maxSampleRate), BYTE1(maxSampleRate),
	SUMP_META_SAMPLE_RAM, BYTE4(maxSampleMemory), BYTE3(maxSampleMemory), BYTE2(maxSampleMemory), BYTE1(maxSampleMemory),
	SUMP_META_PROBES, maxChannelAnz,
	SUMP_META_PROTOCOL, 2,
	SUMP_META_END
};

char com_rx_buf[COM_RX_BUF_LEN];
char com_tx_buf[COM_TX_BUF_LEN];

//--------------------------------------------------------------
// Private functions
//--------------------------------------------------------------
LA_COM_t p_cmd_check(void);
uint8_t p_pause(uint32_t n);

//--------------------------------------------------------------
// COM init
//--------------------------------------------------------------
void com_init(void)
{
	// init vom USB_CDC
	UB_USB_CDC_Init();
}

//--------------------------------------------------------------
// Edit COM
//--------------------------------------------------------------
LA_COM_t com_do(void)
{
	LA_COM_t command = LA_COM_NONE;
	USB_CDC_STATUS_t usb_status;
	USB_CDC_RXSTATUS_t rx_status;

	// Test if USB connection
	usb_status = UB_USB_CDC_GetStatus();
	if (usb_status == USB_CDC_CONNECTED)
	{
		// Test whether received SUMP command
		rx_status = UB_USB_CDC_ReceiveSump(com_rx_buf);
		if (rx_status == RX_READY)
			command = p_cmd_check();
	}

	return command;
}


//--------------------------------------------------------------
// sendet einen String
//--------------------------------------------------------------
void com_send_string(char *ptr)
{
	UB_USB_CDC_SendString(ptr, COM_END_BYTE);
}


//--------------------------------------------------------------
// Sends a string
// (green LED flashes)
//--------------------------------------------------------------
void com_send_all_data(void)
{
	uint32_t n, anz, m;
	uint8_t check;

	UB_Led_Off(LED_RED);
	UB_Led_Off(LED_GREEN);

	anz = (DIN_DMA.buf_len * 2 * DIN_DMA.buf_anz);
	m = 0;
	check = 0;
	for (n = 0; n < anz; n++)
	{
		UB_USB_CDC_SendByte(la_readData_8b(n));
		m++;
		if (m > 512)
		{
			m = 0;
			UB_Led_Toggle(LED_GREEN);
			check = p_pause(100);
			if (check != 0)
				break;
		}
	}

	if (check == 0)
	{
		// Null bytes send (as an end identifier)
		for (n = 0; n < anz; n++)
		{
			UB_USB_CDC_SendByte(0);
			m++;
			if (m > 512)
			{
				m = 0;
				if (p_pause(100) != 0)
					break;
			}
		}
	}

	UB_Led_Off(LED_GREEN);
}


//--------------------------------------------------------------
// Little break
//--------------------------------------------------------------
uint8_t p_pause(uint32_t t)
{
	uint8_t result = 0;
	uint32_t n;
	volatile uint32_t p;
	LA_COM_t check;

	for (n = 0; n < t; n++)
	{
		check = com_do();
		if (check != LA_COM_NONE)
		{
			result = 1;
			break;
		}
		for (p = 0; p < 1000; p++)
			__NOP();
	}

	return result;
}

//--------------------------------------------------------------
// interne Funktion
// Evaluation of Received SUMP command
// 
// command = ID of the received command
//
// Command is in LA.sump.b0 to LA.sump.b3
//--------------------------------------------------------------
LA_COM_t p_cmd_check(void)
{
	LA_COM_t command = LA_COM_NONE;
	uint8_t id_nr;

	id_nr = com_rx_buf[0];
	LA.sump.id = id_nr;
	switch (id_nr)
	{
		case SUMP_CMD_ID_RESET:
			command = LA_COM_RESET;
			break;
			case SUMP_CMD_ID_ARM_TRG:
			command = LA_COM_ARM_TRIGGER;
			break;
		case SUMP_CMD_ID_GET_ID:
			command = LA_COM_GET_ID;
			UB_USB_CDC_SendData("1ALS", 4);
			break;
			case SUMP_CMD_ID_GET_META:
			command = LA_COM_GET_META;
			UB_USB_CDC_SendData((char *)metaData, sizeof(metaData));
			break;
		case SUMP_CMD_ID_SET_FRQ:
			command = LA_COM_SET_FRQ;
			LA.sump.b0 = com_rx_buf[1];
			LA.sump.b1 = com_rx_buf[2];
			LA.sump.b2 = com_rx_buf[3];
			LA.sump.b3 = 0x00;
			break;
		case SUMP_CMD_ID_SET_SIZE:
			command = LA_COM_SET_SIZE;
			LA.sump.b0 = com_rx_buf[1];
			LA.sump.b1 = com_rx_buf[2];
			LA.sump.b2 = com_rx_buf[3];
			LA.sump.b3 = com_rx_buf[4];
			break;
		case SUMP_CMD_ID_SET_MASK0:
			command = LA_COM_SET_MASK;
			LA.sump.b0 = com_rx_buf[1];
			LA.sump.b1 = com_rx_buf[2];
			LA.sump.b2 = com_rx_buf[3];
			LA.sump.b3 = com_rx_buf[4];
			break;
		case SUMP_CMD_ID_SET_VAL0:
			command = LA_COM_SET_VALUE;
			LA.sump.b0 = com_rx_buf[1];
			LA.sump.b1 = com_rx_buf[2];
			LA.sump.b2 = com_rx_buf[3];
			LA.sump.b3 = com_rx_buf[4];
			break;
		default:
			command = LA_COM_UNKNOWN;
			break;
	}

	return command;
}
