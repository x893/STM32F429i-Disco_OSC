//--------------------------------------------------------------
// File     : usb_bsp.c
//--------------------------------------------------------------

//--------------------------------------------------------------
// Includes
//--------------------------------------------------------------
#include "usb_bsp.h"
#include "usbd_conf.h"

extern USB_OTG_CORE_HANDLE           USB_OTG_dev;
extern uint32_t USBD_OTG_ISR_Handler (USB_OTG_CORE_HANDLE *pdev);
extern uint32_t USBD_OTG_EP1IN_ISR_Handler (USB_OTG_CORE_HANDLE *pdev);
extern uint32_t USBD_OTG_EP1OUT_ISR_Handler (USB_OTG_CORE_HANDLE *pdev);




//--------------------------------------------------------------
void USB_OTG_BSP_Init(USB_OTG_CORE_HANDLE *pdev)
{

  GPIO_InitTypeDef GPIO_InitStructure;


  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14 | // PB14 OTG_HS_DM
                                GPIO_Pin_15;  // PB15 OTG_HS_DP

  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  GPIO_PinAFConfig(GPIOB,GPIO_PinSource14,GPIO_AF_OTG2_FS); // PB14 OTG_FS_DM
  GPIO_PinAFConfig(GPIOB,GPIO_PinSource15,GPIO_AF_OTG2_FS); // PB15 OTG_FS_DP

  /* Configure VBUS Pin */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13; // PB13 VBUS_FS (GPIO)
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  /* Configure ID pin */
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_12; // PB12 OTG_FS_ID
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  GPIO_PinAFConfig(GPIOB,GPIO_PinSource12,GPIO_AF_OTG2_FS); // PB12 OTG_HS_ID  12

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_OTG_HS, ENABLE);


}

//--------------------------------------------------------------
void USB_OTG_BSP_EnableInterrupt(USB_OTG_CORE_HANDLE *pdev)
{
  NVIC_InitTypeDef NVIC_InitStructure;

  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
  NVIC_InitStructure.NVIC_IRQChannel = OTG_HS_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
  NVIC_InitStructure.NVIC_IRQChannel = OTG_HS_EP1_OUT_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
  NVIC_InitStructure.NVIC_IRQChannel = OTG_HS_EP1_IN_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

}



//--------------------------------------------------------------
void USB_OTG_BSP_uDelay (const uint32_t usec)
{


  __IO uint32_t count = 0;
  const uint32_t utime = (120 * usec / 7);
  do
  {
    if ( ++count > utime )
    {
      return;
    }
  }
  while (1);


}


//--------------------------------------------------------------
void USB_OTG_BSP_mDelay (const uint32_t msec)
{

  USB_OTG_BSP_uDelay(msec * 1000);

}



//--------------------------------------------------------------
void OTG_HS_IRQHandler(void)
{
  USBD_OTG_ISR_Handler (&USB_OTG_dev);
}

//--------------------------------------------------------------
void OTG_HS_EP1_IN_IRQHandler(void)
{
  USBD_OTG_EP1IN_ISR_Handler (&USB_OTG_dev);
}

//--------------------------------------------------------------
void OTG_HS_EP1_OUT_IRQHandler(void)
{
  USBD_OTG_EP1OUT_ISR_Handler (&USB_OTG_dev);
}


