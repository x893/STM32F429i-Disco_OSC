//--------------------------------------------------------------
// File     : stm32_ub_usb_cdc.c
// Datum    : 02.11.2013
// Version  : 1.1 (Anpassungen wegen SUMP-Protokoll)
// Autor    : UB
// EMail    : mc-4u(@)t-online.de
// Web      : www.mikrocontroller-4u.de
// CPU      : STM32F429
// IDE      : CooCox CoIDE 1.7.4
// GCC      : 4.7 2012q4
// Module   : GPIO, MISC
// Funktion : USB-COM-Verbindung per USB-OTG-Port am Discovery
//
// Hinweis  : auf dem PC muss der Treiber von ST
//            "VirtualComportDriver" installiert sein (V:1.3.1)
//            nur dann wird beim Verbinden ein VirtuellerComPort
//            im Gerätemanager angezeigt
//
//            Beim Discovery-Modul funktioniert nur der
//            USB-FullSpeed-Mode (USB-High-Speed geht nicht)
//
// Vorsicht : Als Endekennung beim Empfangen, muss der Sender
//            das Zeichen "0x0D" = Carriage-Return
//            an den String anhängen !!
//--------------------------------------------------------------
//              PB13  -> USB_OTG_VBUS
//              PB12  -> USB_OTG_ID
//              PB14  -> USB_OTG_DM
//              PB15  -> USB_OTG_DP
//--------------------------------------------------------------

//--------------------------------------------------------------
// Includes
//--------------------------------------------------------------
#include "stm32_ub_usb_cdc.h"

//--------------------------------------------------------------
// Globale Variabeln
//--------------------------------------------------------------
USB_OTG_CORE_HANDLE  USB_OTG_dev;
USB_CDC_STATUS_t USB_CDC_STATUS;

//--------------------------------------------------------------
// Init vom USB-OTG-Port als CDC-Device
// (Virtueller ComPort)
//--------------------------------------------------------------
void UB_USB_CDC_Init(void)
{
  USB_CDC_STATUS=USB_CDC_DETACHED;
  USBD_Init(&USB_OTG_dev, USB_OTG_HS_CORE_ID, &USR_desc, &USBD_CDC_cb, &USR_cb);
}

//--------------------------------------------------------------
// auslesen vom Status der USB-Schnittstelle
// Return_wert :
//  -> USB_CDC_NO_INIT   , USB-Schnittstelle noch nicht initialisiert
//  -> USB_CDC_DETACHED  , USB-Verbindung getrennt
//  -> USB_CDC_CONNECTED , USB-Verbindung hergestellt
//--------------------------------------------------------------
USB_CDC_STATUS_t UB_USB_CDC_GetStatus(void)
{
  return(USB_CDC_STATUS);
}


//--------------------------------------------------------------
// Sendet einen String per USB-OTG-Schnittstelle
// end_cmd : [NONE, LFCR, CRLF, LF, CR]
// Return_wert :
//  -> ERROR   , wenn String gesendet wurde
//  -> SUCCESS , wenn String nicht gesendet wurde
//--------------------------------------------------------------
ErrorStatus UB_USB_CDC_SendString(char *ptr, USB_CDC_LASTBYTE_t end_cmd)
{

  if(USB_CDC_STATUS!=USB_CDC_CONNECTED) {
    // senden nur, wenn Verbindung hergestellt ist
    return(ERROR);
  }

  // kompletten String senden
  while (*ptr != 0) {
    UB_VCP_DataTx(*ptr);
    ptr++;
  }
  // eventuell Endekennung senden
  if(end_cmd==LFCR) {
    UB_VCP_DataTx(0x0A); // LineFeed senden
    UB_VCP_DataTx(0x0D); // CariageReturn senden
  }
  else if(end_cmd==CRLF) {
    UB_VCP_DataTx(0x0D); // CariageReturn senden
    UB_VCP_DataTx(0x0A); // LineFeed senden
  }
  else if(end_cmd==LF) {
    UB_VCP_DataTx(0x0A); // LineFeed senden
  }
  else if(end_cmd==CR) {
    UB_VCP_DataTx(0x0D); // CariageReturn senden
  }

  return(SUCCESS);
}



//--------------------------------------------------------------
// Sendet Daten per USB-OTG-Schnittstelle
// anz_data = anzahl der daten
// Return_wert :
//  -> ERROR   , wenn Daten gesendet wurde
//  -> SUCCESS , wenn Daten nicht gesendet wurde
//--------------------------------------------------------------
ErrorStatus UB_USB_CDC_SendData(char *ptr, uint32_t anz_data)
{
  uint32_t n;

  if(USB_CDC_STATUS!=USB_CDC_CONNECTED) {
    // senden nur, wenn Verbindung hergestellt ist
    return(ERROR);
  }

  // alles senden
  for(n=0;n<anz_data;n++) {
    UB_VCP_DataTx(*ptr);
    ptr++;
  }

  return(SUCCESS);
}


//--------------------------------------------------------------
// Sendet ein Byte per USB-OTG-Schnittstelle
// Return_wert :
//  -> ERROR   , wenn Byte gesendet wurde
//  -> SUCCESS , wenn Byte nicht gesendet wurde
//--------------------------------------------------------------
ErrorStatus UB_USB_CDC_SendByte(uint8_t wert)
{
  if(USB_CDC_STATUS!=USB_CDC_CONNECTED) {
    // senden nur, wenn Verbindung hergestellt ist
    return(ERROR);
  }

  // byte senden
  UB_VCP_DataTx(wert);

  return(SUCCESS);
}


//--------------------------------------------------------------
// einen String per USB-OTG-Schnittstelle empfangen
// (der Empfang wird per Interrupt abgehandelt)
// diese Funktion muss zyklisch gepollt werden
// Return Wert :
//  -> wenn USB nicht bereit = RX_USB_ERR
//  -> wenn nichts empfangen = RX_EMPTY
//  -> wenn String empfangen = RX_READY -> String steht in *ptr
//--------------------------------------------------------------
USB_CDC_RXSTATUS_t UB_USB_CDC_ReceiveString(char *ptr)
{
  uint16_t check;

  if(USB_CDC_STATUS!=USB_CDC_CONNECTED) {
    // empfangen nur, wenn Verbindung hergestellt ist
    return(RX_USB_ERR);
  }

  check=UB_VCP_StringRx(ptr);
  if(check==0) {
    ptr[0]=0x00;
    return(RX_EMPTY);
  }

  return(RX_READY);
}


//--------------------------------------------------------------
// eine SUMP-Kommando per USB-OTG-Schnittstelle empfangen
// (der Empfang wird per Interrupt abgehandelt)
// diese Funktion muss zyklisch gepollt werden
// Return Wert :
//  -> wenn USB nicht bereit = RX_USB_ERR
//  -> wenn nichts empfangen = RX_EMPTY
//  -> wenn String empfangen = RX_READY -> Kommando steht in *ptr
//--------------------------------------------------------------
USB_CDC_RXSTATUS_t UB_USB_CDC_ReceiveSump(char *ptr)
{
  uint16_t check;

  if(USB_CDC_STATUS!=USB_CDC_CONNECTED) {
    // empfangen nur, wenn Verbindung hergestellt ist
    return(RX_USB_ERR);
  }

  check=UB_VCP_SumpRx(ptr);
  if(check==0) {
    ptr[0]=0x00;
    return(RX_EMPTY);
  }

  return(RX_READY);
}
