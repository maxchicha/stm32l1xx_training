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

#include "SHT20.h"
#include "PH_OEM.h"

NBQueue q;
NBUart nb;
UDPConnection main_udp, dns_udp;
DNSClient dns;
CoAPClient ap;
Microgear mg;

RTC_InitTypeDef   RTC_InitStructure;
RTC_TimeTypeDef   RTC_TimeStructure;
RTC_AlarmTypeDef  RTC_AlarmStructure;

#define APPID    "ATS01aa0000Zpp39F"
#define KEY      "5Su4y1eNNp8B6AA"
#define SECRET   "ucdUwFd2okOVS1P5D28rmzEMu"

/* Private setup functions ---------------------------------------------------------*/
void RCC_setup_HSI(void);
void GPIO_setup(void);
void USART_Config(void);
void RCC_Config(void);
void USART2_setup(void);
void RTC_WKUP_IRQHandler(void);
void EXTI20_Config(void);
void RTC_Wakeup_Init(void);

/* Private user define functions ---------------------------------------------------------*/
void delay(unsigned long ms);
void send_byte(uint8_t b);
void usart_puts(char* s);
void USART2_IRQHandler(void);

void ADC_Config(void)
{

	//ADC
	ADC_InitTypeDef ADC_InitStructure;
	GPIO_InitTypeDef  GPIO_InitStructure;

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);
	// input of ADC PB12 (it doesn't seem to be needed, as default GPIO state is floating input)
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AN;
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/* Configure PB13 as Output push-pull */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;

	GPIO_Init(GPIOB, &GPIO_InitStructure);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
	ADC_InitStructure.ADC_ScanConvMode =  DISABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;
	ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfConversion = 1;
	ADC_Init(ADC1, &ADC_InitStructure);

	ADC_RegularChannelConfig(ADC1, ADC_Channel_18, 1, ADC_SampleTime_4Cycles);
	ADC_ContinuousModeCmd(ADC1, ENABLE);

	ADC_Cmd(ADC1, ENABLE);//enable ADC1
		/* Wait until the ADC1 is ready */
	while(ADC_GetFlagStatus(ADC1, ADC_FLAG_ADONS) == RESET)
	{
	}
	// start conversion
	ADC_SoftwareStartConv(ADC1);// start conversion (will be endless as we are in continuous mode)

}


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

void GPIO_setup(void)
{
	/* GPIO Sturcture */
	GPIO_InitTypeDef GPIO_InitStructure;
	/* Enable Peripheral Clock AHB for GPIOB */
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB,ENABLE);
	/* Configure PC13 as Output push-pull */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;

	GPIO_Init(GPIOB, &GPIO_InitStructure);
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

	}
}

void ADC_deinit(void){

	ADC_Cmd(ADC1, DISABLE);
	ADC_DeInit(ADC1);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, DISABLE);
	
	GPIO_InitTypeDef  GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

}


void i2c1_Init(void)
{
	I2C_InitTypeDef   I2C_InitStructure;
	GPIO_InitTypeDef  GPIO_InitStructure;
	// i2c1_DeInit();
	/*!< LM75_I2C Periph clock enable */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

	/*!< LM75_I2C_SCL_GPIO_CLK, LM75_I2C_SDA_GPIO_CLK 
			 and LM75_I2C_SMBUSALERT_GPIO_CLK Periph clock enable */
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB , ENABLE);

	/* Connect PXx to I2C_SCL */
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_I2C1);

	/* Connect PXx to I2C_SDA */
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_I2C1); 

	/*!< Configure I2C1 pins: SCL and SDA*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);


	I2C_DeInit(I2C1);
	I2C_InitStructure.I2C_ClockSpeed = 50000;
	I2C_InitStructure.I2C_Mode =  I2C_Mode_I2C;
	I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
	I2C_InitStructure.I2C_OwnAddress1 =0x00;
	I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
	I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;


	I2C_Init(I2C1, &I2C_InitStructure);

	I2C_Cmd(I2C1, ENABLE);
}

void i2c1_deinit(void){

	GPIO_InitTypeDef  GPIO_InitStructure;

	I2C_Cmd(I2C1, DISABLE);
	I2C_DeInit(I2C1);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, DISABLE);

	/*!< Configure I2C1 pins: SCL and SDA*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
}

void i2c1_Init_Remap(void)
{
	I2C_InitTypeDef   I2C_InitStructure;
	GPIO_InitTypeDef  GPIO_InitStructure;
	// i2c1_DeInit();
	/*!< LM75_I2C Periph clock enable */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

	/*!< LM75_I2C_SCL_GPIO_CLK, LM75_I2C_SDA_GPIO_CLK 
			 and LM75_I2C_SMBUSALERT_GPIO_CLK Periph clock enable */
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB , ENABLE);

	/* Connect PXx to I2C_SCL */
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource8, GPIO_AF_I2C1);

	/* Connect PXx to I2C_SDA */
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource9, GPIO_AF_I2C1); 

	/*!< Configure I2C1 pins: SCL and SDA*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);


	I2C_DeInit(I2C1);
	I2C_InitStructure.I2C_ClockSpeed = 50000;
	I2C_InitStructure.I2C_Mode =  I2C_Mode_I2C;
	I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
	I2C_InitStructure.I2C_OwnAddress1 =0x00;
	I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
	I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;


	I2C_Init(I2C1, &I2C_InitStructure);

	I2C_Cmd(I2C1, ENABLE);
}

void i2c1_remap_deinit(void){

	GPIO_InitTypeDef  GPIO_InitStructure;

	I2C_Cmd(I2C1, DISABLE);
	I2C_DeInit(I2C1);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, DISABLE);

	/*!< Configure I2C1 pins: SCL and SDA*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
}

/**
	* @brief  Main program.
	* @param  None
	* @retval None


	*/

void RCC_Config(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);

	/* Allow access to RTC Domain */
	PWR_RTCAccessCmd(ENABLE);

	/* Check if the StandBy flag is set */
	if (PWR_GetFlagStatus(PWR_FLAG_SB) != RESET)
	{
		/* Clear StandBy flag */
		PWR_ClearFlag(PWR_FLAG_SB);


	// usart1_puts("C_SB\r\n");
		/* Turn on LED2 */
		// STM_EVAL_LEDOn(LED2); ////DO SOMETHING
		
		/* Wait for RTC APB registers synchronisation */
		RTC_WaitForSynchro();
		/* No need to configure the RTC as the RTC config(clock source, enable,
			 prescaler,...) are kept after wake-up from STANDBY */
	}else if(PWR_GetFlagStatus(PWR_FLAG_WU) != RESET){

		PWR_ClearFlag(PWR_FLAG_WU);

	// usart1_puts("C_WU\r\n");
		RTC_WaitForSynchro();

	}
	else
	{
		/* RTC Configuration ******************************************************/
		/* Reset RTC Domain */
		RCC_RTCResetCmd(ENABLE);
		RCC_RTCResetCmd(DISABLE);

	// usart1_puts("Reset_\r\n");
		/* Enable the LSE OSC */
		RCC_LSEConfig(RCC_LSE_ON);
		
		/* Wait till LSE is ready */
		while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET)
		{}

		/* Select the RTC Clock Source */
	RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);

		/* Enable the RTC Clock */
	RCC_RTCCLKCmd(ENABLE);

		/* Wait for RTC APB registers synchronisation */
	RTC_WaitForSynchro();

		/* Set the RTC time base to 1s */
	RTC_InitStructure.RTC_HourFormat = RTC_HourFormat_24;
	RTC_InitStructure.RTC_AsynchPrediv = 0x7F;
	RTC_InitStructure.RTC_SynchPrediv = 0x00FF;
	RTC_Init(&RTC_InitStructure);


		/* Set the time to 01h 00mn 00s AM */
	RTC_TimeStructure.RTC_H12     = RTC_H12_AM;
	RTC_TimeStructure.RTC_Hours   = 0x21;
	RTC_TimeStructure.RTC_Minutes = 0x07;
	RTC_TimeStructure.RTC_Seconds = 0x40;  
	RTC_SetTime(RTC_Format_BCD, &RTC_TimeStructure);
	delay (50);
}

	/* Clear RTC Alarm A flag */ 
RTC_ClearFlag(RTC_FLAG_ALRAF);
}

void RTC_WKUP_IRQHandler(void){

	if(RTC_GetFlagStatus(RTC_FLAG_WUTF) != RESET){

		RTC_ClearFlag(RTC_FLAG_WUTF);
	}


}

void EXTI20_Config(void)
{
	EXTI_InitTypeDef   EXTI_InitStructure;
	NVIC_InitTypeDef   NVIC_InitStructure;
	/* Enable SYSCFG clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);


	RTC_ITConfig(RTC_IT_WUT,ENABLE);

	/* Configure EXTI0 line */
	EXTI_InitStructure.EXTI_Line = EXTI_Line20;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;  
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);

	// /* Enable and set EXTI0 Interrupt to the lowest priority */
	NVIC_InitStructure.NVIC_IRQChannel = RTC_WKUP_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0F;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

void RTC_Wakeup_Init(void){

	PWR_ClearFlag(PWR_FLAG_WU);
	EXTI20_Config();
	RTC_OutputConfig(RTC_Output_WakeUp, RTC_OutputPolarity_High);

	/* Clear PWR WakeUp flag */
	PWR_ClearFlag(PWR_FLAG_WU);

	/* Clear RTC Alarm A flag */ 
	RTC_ClearFlag(RTC_FLAG_WUTF);
	
	RTC_WakeUpCmd(DISABLE);
	RTC_WakeUpClockConfig(RTC_WakeUpClock_CK_SPRE_16bits);
	RTC_SetWakeUpCounter(1800);
	RTC_WakeUpCmd(ENABLE);

}
void RCC_setup_MSI(uint32_t msi_speed);
/*---------------------Function----------------------------*/
void RCC_setup_MSI(uint32_t msi_speed)
{
	/* RCC system reset(for debug purpose) */
	// RCC_DeInit();
	/* Enable Internal Clock HSI */
	RCC_MSIRangeConfig(msi_speed);
	RCC_MSICmd(ENABLE);
	/* Wait till HSI is Ready */
	while(RCC_GetFlagStatus(RCC_FLAG_MSIRDY)==RESET);
	RCC_HCLKConfig(RCC_SYSCLK_Div1);
	RCC_PCLK1Config(RCC_HCLK_Div1);
	RCC_PCLK2Config(RCC_HCLK_Div1);
	FLASH_SetLatency(FLASH_Latency_0);
	/* Enable PrefetchBuffer */
	FLASH_PrefetchBufferCmd(ENABLE);
	/* Set HSI Clock Source */
	RCC_SYSCLKConfig(RCC_SYSCLKSource_MSI);
	/* Wait Clock source stable */
	while(RCC_GetSYSCLKSource()!=0x00);
}

float rawTemperature1 = 0;
float rawHumidity1 = 0;
float rawTemperature2 = 0;
float rawHumidity2 = 0;
float rawPH = 0;
int adc_value = 0;
float v_in_adc = 0;
char buffer[300] = {'\0'};
int cnt = 0;
uint16_t packetid = 0;

void readSensor(){
	// Sensor 1
	i2c1_Init_Remap();
	delay(5000);
	{
		if(SHT20ReadTemperature(I2C1, &rawTemperature2))
		{
			usart_puts1("READ TEMPERATURE 2 OK\r\n");
			sprintf(buffer, "TEMP: %d\n", (int)(rawTemperature2*100.0f));
			usart_puts1(buffer);
		}
		else
		{
			usart_puts1("READ TEMPERATURE 2 ERROR\r\n");
		}

		if(SHT20ReadHumidity(I2C1, &rawHumidity2))
		{
			usart_puts1("READ HUMIDITY 2 OK\r\n");
			sprintf(buffer, "HUMI: %d\n", (int)(rawHumidity2*10000.0f/108.0f));
			usart_puts1(buffer);
		}
		else
		{
			usart_puts1("READ HUMIDITY 2 ERROR\r\n");
		}
	}
	i2c1_remap_deinit();

	// Sensor 2
	i2c1_Init();
	usart_puts1("i2c_init...\r\n");
	OEM_ACTIVE(I2C1);
	delay(5000);
	usart_puts1("i2c_init ready\r\n");
	{
		if(SHT20ReadTemperature(I2C1, &rawTemperature1))
		{
			usart_puts1("READ TEMPERATURE 1 OK\r\n");
			sprintf(buffer, "TEMP: %d\n", (int)(rawTemperature1*100.0f));
			usart_puts1(buffer);
		}
		else
		{
			usart_puts1("READ TEMPERATURE 1 ERROR\r\n");
		}

		if(SHT20ReadHumidity(I2C1, &rawHumidity1))
		{
			usart_puts1("READ HUMIDITY 1 OK\r\n");
			sprintf(buffer, "HUMI: %d\n", (int)(rawHumidity1*10000.0f/108.0f));
			usart_puts1(buffer);
		}
		else
		{
			usart_puts1("READ HUMIDITY 1 ERROR\r\n");
		}
		usart_puts1("reading...\r\n");
		if(OEM_READ_PH(I2C1, &rawPH))
		{
			usart_puts1("READ pH OK\r\n");
			sprintf(buffer, "PH: %d\n", (int)(rawPH*100.0f));
			usart_puts1(buffer);
		}
		else
		{
			usart_puts1("READ pH ERROR\r\n");
		}
	}	
	OEM_DEACTIVE(I2C1);
	delay(500);
	i2c1_deinit();
	delay(1000);

	// ADC
	ADC_Config();
	GPIO_ResetBits(GPIOB,GPIO_Pin_13);
	delay(1000);
	adc_value = ADC_GetConversionValue(ADC1);
	v_in_adc = (float)(adc_value)/4095.0f*3.3f*1.5f*100.0f;
	sprintf(buffer, "adc: %d\n", (int)(v_in_adc));
	usart_puts1(buffer);
	ADC_deinit();

}

bool sendNB(int retry){
	bool ret = false;
	// initialize NB-iot Queue
	NBQueueInit(&q);
	// initialize NB-iot UART
	NBUartInit(&nb, USART2, &q);
	// initialize NB-iot main UDP
	ret = UDPInit(&main_udp, &nb);
	if(!ret){
		usart_puts1("MAIN UDP init error.\r\n");
		return false;		
	}
	// initialize NB-iot dns UDP
	ret = UDPInit(&dns_udp, &nb);
	if(!ret){
		usart_puts1("DNS UDP init error.\r\n");
		return false;
	}

	// initialize ok
	delay(3000);
	usart_puts1("init ok...\r\n");

	for(volatile int i=0;i<3;i++){
		// IMI testing
		usart_puts1("device IMI: ");
		delay(3000);
		ret = NBUartGetIMI(&nb, buffer);
		if(ret){
			usart_puts1(buffer);
			usart_puts1("\r\n");
		}else{
			return false;
		}
	}

	// Attach network
	for(volatile int i=0; i<30;i++){
		usart_puts1("attach... >> ");
		ret = NBUartAttachNetwork(&nb);
		if(ret){
			usart_puts1("\r\n Attach OK\r\n");
			break;
		}else{
			if(i >= 29){
				usart_puts1("reseting...\r\n");
				return false;
			}
			usart_puts1("ERROR >> retry...\r\n");
		}
		delay(100);
	}

	// Microgear initialize
	ret = MicrogearInit(&mg, &main_udp, &dns_udp, "203.154.135.240", APPID, KEY, SECRET);
	if(!ret){
		usart_puts1("\r\n Microgear init fail\r\n");
		return false;
	}
	usart_puts1("NB-iot OK!!!!!");

	// sending
	for(int j=0;j<3;j++){
		// Microgear publish
		uint8_t retry_count = 0;
		while(true){
			sprintf(buffer, "%d", (int)rawTemperature1);
			packetid = MicrogearPublishString(&mg, "/nbiot/temp1", buffer);
			if(packetid == 0){
				usart_puts1("PUBLISH FAIL >> retry...\r\n"); 
				if(retry_count >= 3){
					return false;
				}
				retry_count++;
				delay(100);
				continue;    
			} else {
				usart_puts1("PUBLISH OK\r\n");
				//delay(5000);
				break;
			}
		}

		while(true){
			sprintf(buffer, "%d", (int)rawTemperature2);
			packetid = MicrogearPublishString(&mg, "/nbiot/temp2", buffer);
			if(packetid == 0){
				usart_puts1("PUBLISH FAIL >> retry...\r\n"); 
				if(retry_count >= 3){					
					return false;
				}
				retry_count++;
				delay(100);
				continue;    
			} else {
				usart_puts1("PUBLISH OK\r\n");
				//delay(5000);
				break;
			}
		}

		while(true){
			sprintf(buffer, "%d", (int)rawHumidity1);
			packetid = MicrogearPublishString(&mg, "/nbiot/humi1", buffer);
			if(packetid == 0){
				usart_puts1("PUBLISH FAIL >> retry...\r\n"); 
				if(retry_count >= 3){
					return false;
				}
				retry_count++;
				delay(100);
				continue;    
			} else {
				usart_puts1("PUBLISH OK\r\n");
				//delay(5000);
				break;
			}
		}

		while(true){
			sprintf(buffer, "%d", (int)rawHumidity2);
			packetid = MicrogearPublishString(&mg, "/nbiot/humi2", buffer);
			if(packetid == 0){
				usart_puts1("PUBLISH FAIL >> retry...\r\n"); 
				if(retry_count >= 3){
					return false;
				}
				retry_count++;
				delay(100);
				continue;    
			} else {
				usart_puts1("PUBLISH OK\r\n");
				//delay(5000);
				break;
			}
		}

		while(true){
			sprintf(buffer, "%d", (int)rawPH);
			packetid = MicrogearPublishString(&mg, "/nbiot/ph", buffer);
			if(packetid == 0){
				usart_puts1("PUBLISH FAIL >> retry...\r\n"); 
				if(retry_count >= 3){
					return false;
				}
				retry_count++;
				delay(100);
				continue;    
			} else {
				usart_puts1("PUBLISH OK\r\n");
				//delay(5000);
				break;
			}
		}
		// Microgear feed
		sprintf(buffer, "temp1:%d,humi1:%d,temp2:%d,humi2:%d,pH:%d,vbat:%d", (int)(rawTemperature1*100.0f), (int)(rawHumidity1*10000.0f/108.0f), (int)(rawTemperature2*100.0f), (int)(rawHumidity2*10000.0f/108.0f), (int)(rawPH*100.0f), (int)(v_in_adc));
		usart_puts1(">>WriteFeed");
		packetid = MicrogearWriteFeed(&mg, "ATS01aa00009ee39F", buffer);
		if(packetid){
			usart_puts1("WriteFeed OK\r\n");
		}else{
			usart_puts1("WriteFeed fail");
		}
	}
	delay(2000);
	return true;
}

int main(void)
{  
	// configuration function
	RCC_setup_HSI();
	GPIO_setup();

	// usart config
	USART1_Config();
	USART2_Config();

	RCC_Config();

	usart_puts1("\r\nSTART\r\n");
	bool ret = false;
	uint8_t retry = 0;

	while(1){
		// switch on NB-iot
		GPIO_SetBits(GPIOB,GPIO_Pin_10);
		delay(1000);

		// switch on
		GPIO_SetBits(GPIOB,GPIO_Pin_0);
		GPIO_SetBits(GPIOB,GPIO_Pin_1);
		GPIO_SetBits(GPIOB,GPIO_Pin_5);

		// read value
		readSensor();

		// switch off
		GPIO_ResetBits(GPIOB,GPIO_Pin_0);
		GPIO_ResetBits(GPIOB,GPIO_Pin_1);
		GPIO_ResetBits(GPIOB,GPIO_Pin_5);		

		// send data via NB-iot
		ret = sendNB(3);
		if(!ret){
		//send err
			if(retry <= 1){
				// switch off NB-iot
				GPIO_ResetBits(GPIOB,GPIO_Pin_10);
				delay(100);
				retry++;
				continue;
				}
			else{
				
				GPIO_ResetBits(GPIOB,GPIO_Pin_10);
				usart_puts1("\r\nNBfail-SLEEP\r\n");
				RTC_Wakeup_Init();
				RCC_setup_MSI(RCC_MSIRange_6);
				delay(100);

				//usart_puts1(">>STANDBY>>\r\n");
				PWR_EnterSTANDBYMode();
				while(1);	
		        }
		}

		// microgear stop
		MicrogearStop(&mg);

		// switch off NB-iot
		GPIO_ResetBits(GPIOB,GPIO_Pin_10);

		if(ret){
			usart_puts1("\r\nSLEEP\r\n");
			RTC_Wakeup_Init();
			RCC_setup_MSI(RCC_MSIRange_6);
			delay(100);

			//usart_puts1(">>STANDBY>>\r\n");
			PWR_EnterSTANDBYMode();
			while(1);
		}
	}
}
