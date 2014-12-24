//--------------------------------------------------------------
// File     : com.c
// Datum    : 03.04.2014
// Version  : 1.0
// Autor    : UB
// EMail    : mc-4u(@)t-online.de
// Web      : www.mikrocontroller-4u.de
// CPU      : STM32F429
// IDE      : CooCox CoIDE 1.7.4
// GCC      : 4.7 2012q4
// Module   : USB_CDC
// Funktion : Kommunikation vom Funktionsgenerator
//
//--------------------------------------------------------------


//--------------------------------------------------------------
// Includes
//--------------------------------------------------------------
#include "com.h"


static char metaData[] = {
  SUMP_META_NAME, 'S', 'T', 'M', '3', '2', 'F', '4', 'U', 'B', 0,
  SUMP_META_FPGA_VERSION, 'n', 'o', 'n', 'e', 0,
  SUMP_META_CPU_VERSION, 'D', '.', '0', '3', 0,
  SUMP_META_SAMPLE_RATE, BYTE4(maxSampleRate), BYTE3(maxSampleRate), BYTE2(maxSampleRate), BYTE1(maxSampleRate),
  SUMP_META_SAMPLE_RAM, BYTE4(maxSampleMemory), BYTE3(maxSampleMemory), BYTE2(maxSampleMemory), BYTE1(maxSampleMemory),
  SUMP_META_PROBES_B, maxChannelAnz,
  SUMP_META_PROTOCOL_B, 2,
  SUMP_META_END
};
uint32_t memData_len=sizeof(metaData);


//--------------------------------------------------------------
// interne Funktionen
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
// COM bearbeiten
//--------------------------------------------------------------
LA_COM_t com_do(void)
{
  LA_COM_t ret_wert=LA_COM_NONE;
  USB_CDC_STATUS_t usb_status;
  USB_CDC_RXSTATUS_t rx_status;

  // test ob USB-Verbindung besteht
  usb_status=UB_USB_CDC_GetStatus();
  if(usb_status==USB_CDC_CONNECTED) {
    // test ob SUMP-Kommando empfangen
    rx_status=UB_USB_CDC_ReceiveSump(com_rx_buf);
    if(rx_status==RX_READY) {
      ret_wert=p_cmd_check();
    }
  }

  return(ret_wert);
}


//--------------------------------------------------------------
// sendet einen String
//--------------------------------------------------------------
void com_send_string(char *ptr)
{
  UB_USB_CDC_SendString(ptr,COM_END_BYTE);
}


//--------------------------------------------------------------
void com_send_all_data(void)
{
  uint32_t n,anz,m;
  uint8_t wert,check;

  UB_Led_On(LED_RED);
  UB_Led_Off(LED_GREEN);

  anz=(DIN_DMA.buf_len*2*DIN_DMA.max_buf_nr);
  m=0;
  check=0;
  for(n=0;n<anz;n++) {
    wert=la_readData(n);
    UB_USB_CDC_SendByte(wert);
    m++;
    if(m>512) {
      m=0;
      check=p_pause(100);
      if(check!=0) break;
    }
  }
	
  if(check==0) {
    // null-bytes senden  (als endekennung)
    for(n=0;n<anz;n++) {
      UB_USB_CDC_SendByte(0);
      m++;
      if(m>512) {
        m=0;
        check=p_pause(100);
        if(check!=0) break;
      }
    }
  }

  UB_Led_Off(LED_RED);
}

//--------------------------------------------------------------
uint8_t p_pause(uint32_t t)
{
  uint8_t ret_wert=0;
  volatile uint32_t n,p;
  LA_COM_t check;

  for(n=0;n<t;n++) {
    check=com_do();
    if(check!=LA_COM_NONE) {
      ret_wert=1;
      break;
    }
    for(p=0;p<1000;p++);
  }

  return(ret_wert);
}

//--------------------------------------------------------------
// interne Funktion
// Auswertung vom Empfangenen SUMP-Kommando
//--------------------------------------------------------------
LA_COM_t p_cmd_check(void)
{
  LA_COM_t ret_wert=LA_COM_NONE;
  char id_nr;

  id_nr=com_rx_buf[0];
  LA.sump.id=id_nr;
  switch(id_nr) {
    case SUMP_CMD_ID_RESET :
      ret_wert=LA_COM_RESET;
    break;
    case SUMP_CMD_ID_ARM_TRG :
      ret_wert=LA_COM_ARM_TRIGGER;
    break;
    case SUMP_CMD_ID_GET_ID :
      ret_wert=LA_COM_GET_ID;
      UB_USB_CDC_SendData("1ALS",4);
    break;
    case SUMP_CMD_ID_GET_META :
      ret_wert=LA_COM_GET_META;
      UB_USB_CDC_SendData(metaData,memData_len);
    break;
    case SUMP_CMD_ID_SET_FRQ :
      ret_wert=LA_COM_SET_FRQ;
      LA.sump.b0=com_rx_buf[1];
      LA.sump.b1=com_rx_buf[2];
      LA.sump.b2=com_rx_buf[3];
      LA.sump.b3=0x00;
    break;
    case SUMP_CMD_ID_SET_LEN :
      ret_wert=LA_COM_SET_SAMPLE;
      LA.sump.b0=com_rx_buf[1];
      LA.sump.b1=com_rx_buf[2];
      LA.sump.b2=com_rx_buf[3];
      LA.sump.b3=com_rx_buf[4];
    break;
    case SUMP_CMD_ID_SET_MASK0 :
      ret_wert=LA_COM_SET_MASK;
      LA.sump.b0=com_rx_buf[1];
      LA.sump.b1=com_rx_buf[2];
      LA.sump.b2=com_rx_buf[3];
      LA.sump.b3=com_rx_buf[4];
    break;
    case SUMP_CMD_ID_SET_VAL0 :
      ret_wert=LA_COM_SET_VALUE;
      LA.sump.b0=com_rx_buf[1];
      LA.sump.b1=com_rx_buf[2];
      LA.sump.b2=com_rx_buf[3];
      LA.sump.b3=com_rx_buf[4];
    break;
    default :
      ret_wert=LA_COM_UNKNOWN;
    break;
  }

  return(ret_wert);
}











