//--------------------------------------------------------------
// File     : usb_conf.h
//--------------------------------------------------------------



#ifndef __USB_CONF__H__
#define __USB_CONF__H__

//--------------------------------------------------------------
// Includes
//--------------------------------------------------------------

#include "stm32f4xx.h"








/****************** USB OTG HS CONFIGURATION **********************************/

#define RX_FIFO_HS_SIZE                          512
#define TX0_FIFO_HS_SIZE                          64
#define TX1_FIFO_HS_SIZE                         372
#define TX2_FIFO_HS_SIZE                          64
#define TX3_FIFO_HS_SIZE                           0
#define TX4_FIFO_HS_SIZE                           0
#define TX5_FIFO_HS_SIZE                           0










#define __ALIGN_BEGIN
#define __ALIGN_END



#define __packed    __attribute__ ((__packed__))





#endif

