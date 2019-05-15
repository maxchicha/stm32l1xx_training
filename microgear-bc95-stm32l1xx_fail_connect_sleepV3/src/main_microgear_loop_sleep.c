/**
  ******************************************************************************
  * @file    Project/STM32L1xx_StdPeriph_Templates/main.c 
  * @author  MCD Application Team
  * @version V1.2.0
  * @date    16-May-2014
  * @brief   Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2014 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "NBQueue.h"
#include "NBUart.h"
#include "NBDNS.h"
#include "NBCoAP.h"
#include "NBMicrogear.h"

NBQueue q;
NBUart nb;
UDPConnection main_udp, dns_udp;
DNSClient dns;
CoAPClient ap;
Microgear mg;

#define APPID    "testSTM32iot"
#define KEY      "fpqdyJ2TSoYbjW3"
#define SECRET   "qZzPf0KjlFkYeLHsr3DbgU1ZB"

/* Private setup functions ---------------------------------------------------------*/
void RCC_setup_HSI(void);
void RCC_setup_MSI(void);
void GPIO_setup(void);
void USART_Config(void);
/* Private user define functions ---------------------------------------------------------*/
void delay(unsigned long ms);
void send_byte(uint8_t b);
void usart_puts(char* s);
void USART2_IRQHandler(void);

void send_byte2(uint8_t b)
{
  /* Send one byte */
  USART_SendData(USART2, b);

  /* Loop until USART2 DR register is empty */
  while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
}

void usart_puts2(char* s)
{
    while(*s) {
      send_byte2(*s);
        s++;
    }
}


void USART2_Config(void)
{
  USART_InitTypeDef USART_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;
  
  /* Enable GPIO clock */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
  
  /* Enable USART clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2,ENABLE);
  
  /* Connect PXx to USARTx_Tx */
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2);
  
  /* Connect PXx to USARTx_Rx */
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART3);

  
  /* Configure USART Tx and Rx as alternate function push-pull */
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  /* USARTx configuration ----------------------------------------------------*/
  /* USARTx configured as follow:
  - BaudRate = 230400 baud  
  - Word Length = 8 Bits
  - One Stop Bit
  - No parity
  - Hardware flow control disabled (RTS and CTS signals)
  - Receive and transmit enabled
  */
  USART_InitStructure.USART_BaudRate = 9600;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  USART_Init(USART2, &USART_InitStructure);
  
  /* NVIC configuration */
  /* Configure the Priority Group to 2 bits */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
  
  /* Enable the USARTx Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  

  USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
  USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
  /* Enable USART */
  USART_Cmd(USART2, ENABLE);
}

void send_byte1(uint8_t b)
{
  /* Send one byte */
  USART_SendData(USART1, b);

  /* Loop until USART2 DR register is empty */
  while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
}

void usart_puts1(char* s)
{
    while(*s) {
      send_byte1(*s);
        s++;
    }
}


void USART1_Config(void)
{
  USART_InitTypeDef USART_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;
  
  /* Enable GPIO clock */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
  
  /* Enable USART clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);
  
  /* Connect PXx to USARTx_Tx */
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1);
  
  /* Connect PXx to USARTx_Rx */
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);

  
  /* Configure USART Tx and Rx as alternate function push-pull */
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

   //USARTx configuration ----------------------------------------------------
  //  USARTx configured as follow:
  // - BaudRate = 230400 baud  
  // - Word Length = 8 Bits
  // - One Stop Bit
  // - No parity
  // - Hardware flow control disabled (RTS and CTS signals)
  // - Receive and transmit enabled
  
  USART_InitStructure.USART_BaudRate = 9600;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  USART_Init(USART1, &USART_InitStructure);
  
  /* NVIC configuration */
  /* Configure the Priority Group to 2 bits */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
  
  /* Enable the USARTx Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  

  USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
  // USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
  /* Enable USART */
  USART_Cmd(USART1, ENABLE);
}

/*---------------------Function----------------------------*/
void RCC_setup_HSI(void)
{
  /* RCC system reset(for debug purpose) */
  RCC_DeInit();
  /* Enable Internal Clock HSI */
  RCC_HSICmd(ENABLE);
  /* Wait till HSI is Ready */
  while(RCC_GetFlagStatus(RCC_FLAG_HSIRDY)==RESET);
  RCC_HCLKConfig(RCC_SYSCLK_Div1);
  RCC_PCLK1Config(RCC_HCLK_Div2);
  RCC_PCLK2Config(RCC_HCLK_Div2);
  FLASH_SetLatency(FLASH_Latency_0);
  /* Enable PrefetchBuffer */
  FLASH_PrefetchBufferCmd(ENABLE);
  /* Set HSI Clock Source */
  RCC_SYSCLKConfig(RCC_SYSCLKSource_HSI);
  /* Wait Clock source stable */
  while(RCC_GetSYSCLKSource()!=0x04);
}
/*---------------------Function----------------------------*/
void RCC_setup_MSI(void)
{
  /* RCC system reset(for debug purpose) */
  RCC_DeInit();
  /* Enable Internal Clock HSI */
  RCC_MSIRangeConfig(RCC_MSIRange_0);
  RCC_MSICmd(ENABLE);
  /* Wait till HSI is Ready */
  while(RCC_GetFlagStatus(RCC_FLAG_MSIRDY)==RESET);
  RCC_HCLKConfig(RCC_SYSCLK_Div1);
  RCC_PCLK1Config(RCC_HCLK_Div2);
  RCC_PCLK2Config(RCC_HCLK_Div2);
  FLASH_SetLatency(FLASH_Latency_0);
  /* Enable PrefetchBuffer */
  FLASH_PrefetchBufferCmd(ENABLE);
  /* Set HSI Clock Source */
  RCC_SYSCLKConfig(RCC_SYSCLKSource_MSI);
  /* Wait Clock source stable */
  while(RCC_GetSYSCLKSource()!=0x00);
}

void GPIO_setup(void)
{
  /* GPIO Sturcture */
  GPIO_InitTypeDef GPIO_InitStructure;
  /* Enable Peripheral Clock AHB for GPIOB */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB,ENABLE);
  /* Configure PC13 as Output push-pull */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 ;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;

  GPIO_Init(GPIOB, &GPIO_InitStructure);

  /* Enable Peripheral Clock AHB for GPIOB */
  // RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA,ENABLE);
  // /* Configure PC13 as Output push-pull */
  // GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 ;
  // GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
  // GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  // GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;

  // GPIO_Init(GPIOA, &GPIO_InitStructure);
}
// delay 1 ms per count @ Crystal 16.0 MHz 
void delay(unsigned long ms)
{
  volatile unsigned long i,j;
  for (i = 0; i < ms; i++ )
  for (j = 0; j < 1227; j++ );
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif


void USART2_IRQHandler(void)
{
    char b;
    if(USART_GetFlagStatus(USART2, USART_FLAG_RXNE) == SET) {

          b =  USART_ReceiveData(USART2);
          NBQueueInsert(&q, (uint8_t)b);
          // usart_puts1(">");
          // send_byte1(b);
          // usart_puts1("\r\n");
          // send_byte1(b);
        }
}

void responseHandler(CoapPacket *packet, char* remoteIP, int remotePort){
  char buff[6];
  usart_puts1("CoAP Response Code: ");
  sprintf(buff, "%d.%02d\n", packet->Code >> 5, packet->Code & 0b00011111);
    usart_puts1(buff);

    for (int i=0; i< packet->Payloadlen; i++) {
        //Serial.print((char) (packet->payload[i]));
        char x[1];
        sprintf(x,"%c", (char) (packet->Payload[i]));
        usart_puts1(x);
    }
}

/**
  * @brief  Main program.
  * @param  None
  * @retval None


  */

int main(void)
{
  
  RCC_setup_HSI();
  GPIO_setup();
  
  GPIO_SetBits(GPIOB,GPIO_Pin_10);
  delay(1000);

  USART1_Config();
  USART2_Config();

  int cnt = 0;

  start:
  usart_puts1("\r\nSTART>>\r\n");
  //usart_puts2("START2\r\n");

  // I2C1_init();
  bool ret = false;


  // // // // initialize NB-iot Queue
  NBQueueInit(&q);
  // // // // // initialize NB-iot UART
  NBUartInit(&nb, USART2, &q);
  // initialize NB-iot main UDP
  ret = UDPInit(&main_udp, &nb);
  if(!ret){
    usart_puts1("MAIN UDP init error.\r\n");
    return 0;
  }
  // initialize NB-iot dns UDP
  ret = UDPInit(&dns_udp, &nb);
  if(!ret){
    usart_puts1("DNS UDP init error.\r\n");
    return 0;
  }

  delay(5000);
  usart_puts1("init ok...\r\n");

  // get IMI
  char text[100];
  usart_puts1("device IMI: ");
  ret = NBUartGetIMI(&nb, text);
  if(ret){
    usart_puts1(text);
    usart_puts1("\r\n");
  } else{
    NBUartReset(&nb);
    delay(1000);
    goto start;
  }

  usart_puts1("device IMI: ");
  ret = NBUartGetIMI(&nb, text);
  if(ret){
    usart_puts1(text);
    usart_puts1("\r\n");
  }

  usart_puts1("device IMI: ");
  ret = NBUartGetIMI(&nb, text);
  if(ret){
    usart_puts1(text);
    usart_puts1("\r\n");
  }

  //attach to the network
  uint8_t attach_retry = 0;
  do{
    usart_puts1("attact...\r\n");
    // GPIO_SetBits(GPIOB,GPIO_Pin_7);
    ret = NBUartAttachNetwork(&nb);
    attach_retry++;
    if(attach_retry >= 100){
      NBUartReset(&nb);
      delay(30);      
    }
    // GPIO_ResetBits(GPIOB,GPIO_Pin_7);
    delay(1000);
  }while(!ret);

  ret = MicrogearInit(&mg, &main_udp, &dns_udp, "203.154.135.240", APPID, KEY, SECRET);
  if(!ret){
    usart_puts1("\r\n Microgear init fail\r\n");
    return 0;
  }

  usart_puts1("start publishing loop.\r\n");

  cnt++;
  uint16_t packetid = MicrogearPublishInt(&mg, "/nbiot/rssi", cnt);
  if(packetid == 0){
    usart_puts1("Microgear fail\r\n");      
  } else {
    usart_puts1("publish ok\r\n");
  }

  cnt++;
  packetid = MicrogearPublishInt(&mg, "/nbiot/rssi", cnt);
  if(packetid == 0){
    usart_puts1("Microgear fail\r\n");      
  } else {
    usart_puts1("publish ok\r\n");
  }

  cnt++;
  packetid = MicrogearPublishInt(&mg, "/nbiot/rssi", cnt);
  if(packetid == 0){
    usart_puts1("Microgear fail\r\n");      
  } else {
    usart_puts1("publish ok\r\n");
  }


  MicrogearStop(&mg);
  usart_puts1("END\r\n");
  delay(20000);
  usart_puts1(">>>\r\n");
  goto start;
}
