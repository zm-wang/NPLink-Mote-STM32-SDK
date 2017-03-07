/*
(C)2015 NPLink

Description: app task implementation����APP���̿�ʵ��LORAMAC��PHYMAC���͹���3��Ӧ��֮����л�
						�û�Ӧ�ø�����Ҫ�����߼��޸�
License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Robxr
*/

/* Includes ------------------------------------------------------------------*/
#include "stm32l0xx.h"
#include "stm32l0xx_hal_rtc.h"
#include "stm32l0xx_hal_iwdg.h"
#include <string.h>
#include <stdio.h>

#include "osal_memory.h"
#include "oled_board.h"
#include "app_osal.h"
#include "loraMac_osal.h"
#include "LoRaMacUsr.h"
#include "Comissioning.h"

#include "radio.h"
#include "timer.h"
#include "uart_board.h"
#include "led_board.h"
#include "rtc_board.h"
#include "iwdg_board.h"
#include "sx1276.h"
#include "at.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
/*!
 * Defines the application data transmission duty cycle. 5s, value in [ms].
 */
#define APP_TX_DUTYCYCLE                       5000

/*!
 * When set to 1 the application uses the Over-the-Air activation procedure
 * When set to 0 the application uses the Personalization activation procedure
 */
#define OTAA_NETJOIN                           1
#define ABP_NETJOIN                            0

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
u8 APP_taskID;
bool IsNetworkJoined = false;
u32 Tx_dutycycle = APP_TX_DUTYCYCLE;
LoRaMacAppPara_t g_appData;//����APP �����ṹ��
LoRaMacMacPara_t g_macData;//����MAC�����ṹ��

/* extern variables -----------------------------------------------------------*/
extern UART_HandleTypeDef UartHandle;

/* Private function prototypes -----------------------------------------------*/
void APP_ParaConfig( void );
void APP_EnterlowPowerMode( void );

/* Private functions ---------------------------------------------------------*/
void APP_Init(u8 task_id)
{
	u8 appeui[] = LORAWAN_DEVICE_EUI;
	u8 deveui[] = LORAWAN_APPLICATION_EUI;
	u8 appkey[] = LORAWAN_APPLICATION_KEY;
	
	APP_taskID = task_id;

	//����LORAWAN Э���CLASS ����,Ŀǰ֧��class a��class c
  g_macData.class_mode = CLASS_A;
	LoRaMac_setMacLayerParameter(&g_macData, PARAMETER_CLASS_MODE );
	
  //���÷��书��
  g_appData.txPower = TX_POWER_20_DBM;
 	LoRaMac_setAppLayerParameter(&g_appData, PARAMETER_DEV_TXPOWER );
	
	//������OTAA������APPEUI
	osal_memcpy(g_appData.appEUI, appeui, APP_EUI_LEN);	

	//������OTAA������DEVEUI
	osal_memcpy(g_appData.devEUI, deveui, DEV_EUI_LEN);	

	//������OTAA������APPKEY
	osal_memcpy(g_appData.appKey, appkey, APP_KEY_LEN);	
	
	LoRaMac_setAppLayerParameter(&g_appData, PARAMETER_DEV_EUI | PARAMETER_APP_EUI | PARAMETER_APP_KEY);
 	
	//��ʾMote ID
	LoRaMac_getAppLayerParameter(&g_appData, PARAMETER_DEV_ADDR);
	APP_ShowMoteID(g_appData.devAddr);

	//������־
	LoRaMac_getAppLayerParameter(&g_appData, PARAMETER_DEV_NETWORKJOINED);
	IsNetworkJoined = g_appData.isnetworkjoined;

	memset( &g_macData , 0 , sizeof(g_macData) );

	//����Ƶ������á�����ģʽ�����á����а����͵������Լ�ADR�Ŀ���
	APP_ParaConfig();

	//�豸����
	osal_set_event(APP_taskID,APP_NET_JOIN);
	
	//�͹��Ĳ���
	//osal_set_event(APP_taskID,APP_LOWPOWER_MODE);
}

u16 APP_ProcessEvent( u8 task_id, u16 events )
{
 loraMAC_msg_t* pMsgSend = NULL;
 loraMAC_msg_t* pMsgRecieve = NULL;

 u8 txErrorString[20];
 u8 Rx_buf[256]; //buffer for oled display
 static u16 txErrorNum = 0;
 static u16 txTimeoutNum = 0;

  //system event
  if(events & SYS_EVENT_MSG)
  {
		//receive msg loop
		while(NULL != (pMsgRecieve = (loraMAC_msg_t*)osal_msg_receive(APP_taskID)))
		{
		//pMsgRecieve[0] is system event type
		switch(pMsgRecieve->msgID)
		{
		//�����ɹ�
		case NETJOINOK :
				LoRaMac_getAppLayerParameter(&g_appData, PARAMETER_DEV_ADDR);
				APP_ShowMoteID(g_appData.devAddr);

				LoRaMac_getAppLayerParameter(&g_appData, PARAMETER_DEV_NETWORKJOINED);
				IsNetworkJoined = g_appData.isnetworkjoined;
				
				osal_set_event(APP_taskID,APP_PERIOD_SEND);
				break;
				
		//����ʧ��
		case NETJOINFAIL:
				LoRaMac_getAppLayerParameter(&g_appData, PARAMETER_DEV_ADDR);
				APP_ShowMoteID(g_appData.devAddr);

				osal_set_event(LoraMAC_taskID,LORAMAC_NET_JION);	
				break;		
			 
		//�������
		case TXDONE :		
				
			if( (IsNetworkJoined == true) && (g_at_set_tx == true ))
			{
				HalLedSet (HAL_LED_1, HAL_LED_MODE_OFF);
				display_sx1276_tx_pac_parm( pMsgRecieve->frame_no ); 
				osal_start_timerEx(APP_taskID, APP_PERIOD_SEND,Tx_dutycycle);
			}
					
			break;
		
		//���ʹ���δ�ҵ������ŵ�
		case TXERR_STATUS:
			if( IsNetworkJoined == true )
			{
				txErrorNum++;
				sprintf((char*)txErrorString,"TxErr:%d,txTO:%d",txErrorNum,txTimeoutNum);
				OLED_Clear_Line(5,12);
				OLED_ShowString( 0,48, (u8*)txErrorString,12 );
				OLED_Refresh_Gram();
				osal_start_timerEx(APP_taskID, APP_PERIOD_SEND,Tx_dutycycle);
			}
			break;
		
		//���ʹ��󣬷��ͳ�ʱ
		case TXTIMEOUT:
			if( IsNetworkJoined == true )
			{
				txTimeoutNum++;
				sprintf((char*)txErrorString,"TxErr:%d,txTO:%d",txErrorNum,txTimeoutNum);
				OLED_Clear_Line(5,12);
				OLED_ShowString( 0,48, (u8*)txErrorString,12 );
				OLED_Refresh_Gram();
				osal_start_timerEx(APP_taskID, APP_PERIOD_SEND,Tx_dutycycle);
			}
			break;

		//�������
		case RXDONE:
			
			HalLedSet (HAL_LED_2, HAL_LED_MODE_ON);
			display_sx1276_rx_pac_parm( pMsgRecieve->msgRxRssi,pMsgRecieve->msgRxSnr);//��3����ʾ���ղ���
			OLED_Clear_Half();//�Ȱ���Ļ��һ�����
			APP_ShowMoteID(g_appData.devAddr);

			memset(Rx_buf , 0 ,sizeof(Rx_buf));                               
			osal_memcpy(Rx_buf,pMsgRecieve->msgData,pMsgRecieve->msgLen);

			OLED_Clear_Line(4,12);//��������ݣ�����ʾ
			OLED_Clear_Line(5,12);
			OLED_ShowString( 0,36, (u8*)Rx_buf,12 );//��4����ʾ��������
			OLED_Refresh_Gram();
			
			HAL_UART_SendBytes(Rx_buf, pMsgRecieve->msgLen);
			HalLedSet (HAL_LED_2, HAL_LED_MODE_OFF);	
			
			break;
			
	    default:
		    break;
			}
			osal_msg_deallocate((u8*)pMsgRecieve);
		}
		return (events ^ SYS_EVENT_MSG);
	}

	//��LoRaMAC�㷢��Ϣ�������������
	if(events & APP_NET_JOIN)
	{
		loraMAC_msg_t* pMsgSend = (loraMAC_msg_t*)osal_msg_allocate( 9 + 1 );
		if(NULL != pMsgSend)
		{
			osal_memset(pMsgSend,0,10);
			pMsgSend->msgID = NETJOINREQUEST;
			pMsgSend->msgLen = 1;
			pMsgSend->msgData[0] = ABP_NETJOIN;//������ʽ��ѡ��
			osal_msg_send(LoraMAC_taskID,(u8*)pMsgSend);
		}
		return (events ^ APP_NET_JOIN);
	}

	//����һ�����ݰ�
	if(events & APP_PERIOD_SEND)  
	{
		HalLedSet (HAL_LED_1, HAL_LED_MODE_ON);
		//send a packet to LoRaMac osal (then can be send by the radio)
		pMsgSend = (loraMAC_msg_t*)osal_msg_allocate(sizeof(loraMAC_msg_t));
		if(pMsgSend != NULL)
		{
			osal_memset(pMsgSend,0,sizeof(loraMAC_msg_t));
			pMsgSend->msgID = TXREQUEST;
			pMsgSend->msgLen = 8;
			for(u8 dataCount = 0; dataCount < 8; dataCount++)
			{
				pMsgSend->msgData[dataCount] = dataCount;
			}
				osal_msg_send(LoraMAC_taskID,(u8*)pMsgSend);
		}
	   //osal_start_timerEx(APP_taskID, APP_PERIOD_SEND,5000);//��ʱ��������
		return (events ^ APP_PERIOD_SEND);
	}	

	//���͹��Ĳ�ģʽ
	if(events & APP_LOWPOWER_MODE)  
	{	
		RtcSetTimeout(10000);
		APP_EnterlowPowerMode();
		LoRaMac_setMode(MODE_LORAMAC);		
		osal_set_event(APP_taskID,APP_LOWPOWER_MODE);
		
		return (events ^ APP_LOWPOWER_MODE);
	}
	return 0 ;
}

void APP_ParaConfig( void )
{
#if 1
	
#if defined( USE_BAND_433 )
	
		//����LORAMAC����ģʽ�Ĳ���(LoRa����)
		//�����ŵ�1
		g_macData.channels[0].Frequency = 433175000;//Ƶ��
		g_macData.channels[0].DrRange.Value = ( ( DR_5 << 4 ) | DR_0 ); //���ʷ�Χ:((�������<<4) | (�������))
		g_macData.channels[0].Band = 0;
		//�����ŵ�2
		g_macData.channels[1].Frequency = 433375000;
		g_macData.channels[1].DrRange.Value = ( ( DR_5 << 4 ) | DR_0 );
		g_macData.channels[1].Band = 0;
		//�����ŵ�3
		g_macData.channels[2].Frequency = 433575000;
		g_macData.channels[2].DrRange.Value = ( ( DR_5 << 4 ) | DR_0 );
		g_macData.channels[2].Band = 0;
		//�����ŵ�4
		g_macData.channels[3].Frequency = 433775000;
		g_macData.channels[3].DrRange.Value = ( ( DR_5 << 4 ) | DR_0 );
		g_macData.channels[3].Band = 0;
		//�����ŵ�5
		g_macData.channels[4].Frequency = 434175000;
		g_macData.channels[4].DrRange.Value = ( ( DR_5 << 4 ) | DR_0 );
		g_macData.channels[4].Band = 0;
		//�����ŵ�6
		g_macData.channels[5].Frequency = 434375000;
		g_macData.channels[5].DrRange.Value = ( ( DR_5 << 4 ) | DR_0 );
		g_macData.channels[5].Band = 0;
		//�����ŵ�7
		g_macData.channels[6].Frequency = 434575000;
		g_macData.channels[6].DrRange.Value = ( ( DR_5 << 4 ) | DR_0 );
		g_macData.channels[6].Band = 0;
		//�����ŵ�8
		g_macData.channels[7].Frequency = 434775000;
		g_macData.channels[7].DrRange.Value = ( ( DR_5 << 4 ) | DR_0 );
		g_macData.channels[7].Band = 0;
		//��������
		g_macData.datarate = DR_5;
		//�������а�����
		g_macData.packet_type = UNCONFIRMED_UP;
		//ADR������ر�
		g_macData.lora_mac_adr_switch = TRUE;
		LoRaMac_setMacLayerParameter(&g_macData, PARAMETER_CHANNELS | PARAMETER_ADR_SWITCH | PARAMETER_DATARATE | PARAMETER_PACKET_TYPE );
		//����ʹ��LoRaMAC
		LoRaMac_setMode(MODE_LORAMAC);
		
#elif defined( USE_BAND_470 )
	
		//����LORAMAC����ģʽ�Ĳ���(LoRa����)
		//�����ŵ�1
		g_macData.channels[0].Frequency = 470300000;//Ƶ��
		g_macData.channels[0].DrRange.Value = ( ( DR_5 << 4 ) | DR_0 ); //���ʷ�Χ:((�������<<4) | (�������))
		g_macData.channels[0].Band = 0;
		//�����ŵ�2
		g_macData.channels[1].Frequency = 470500000;
		g_macData.channels[1].DrRange.Value = ( ( DR_5 << 4 ) | DR_0 );
		g_macData.channels[1].Band = 0;
		//�����ŵ�3
		g_macData.channels[2].Frequency = 470700000;
		g_macData.channels[2].DrRange.Value = ( ( DR_5 << 4 ) | DR_0 );
		g_macData.channels[2].Band = 0;
		//�����ŵ�4
		g_macData.channels[3].Frequency = 470900000;
		g_macData.channels[3].DrRange.Value = ( ( DR_5 << 4 ) | DR_0 );
		g_macData.channels[3].Band = 0;
		//�����ŵ�5
		g_macData.channels[4].Frequency = 471100000;
		g_macData.channels[4].DrRange.Value = ( ( DR_5 << 4 ) | DR_0 );
		g_macData.channels[4].Band = 0;
		//�����ŵ�6
		g_macData.channels[5].Frequency = 471300000;
		g_macData.channels[5].DrRange.Value = ( ( DR_5 << 4 ) | DR_0 );
		g_macData.channels[5].Band = 0; 
		//�����ŵ�7
		g_macData.channels[6].Frequency = 471500000;
		g_macData.channels[6].DrRange.Value = ( ( DR_5 << 4 ) | DR_0 );
		g_macData.channels[6].Band = 0;
		//�����ŵ�8
		g_macData.channels[7].Frequency = 471700000;
		g_macData.channels[7].DrRange.Value = ( ( DR_5 << 4 ) | DR_0 );
		g_macData.channels[7].Band = 0;
		//��������
		g_macData.datarate = DR_5;
		//�������а�����
		g_macData.packet_type = UNCONFIRMED_UP;
		//ADR������ر�
		g_macData.lora_mac_adr_switch = TRUE;
		LoRaMac_setMacLayerParameter(&g_macData, PARAMETER_CHANNELS | PARAMETER_ADR_SWITCH | PARAMETER_DATARATE | PARAMETER_PACKET_TYPE );
		//����ʹ��LoRaMAC
		LoRaMac_setMode(MODE_LORAMAC);
		
#elif defined( USE_BAND_490 )
	
		//����LORAMAC����ģʽ�Ĳ���(LoRa����)
		//�����ŵ�1
		g_macData.channels[0].Frequency = 490300000;//Ƶ��
		g_macData.channels[0].DrRange.Value = ( ( DR_5 << 4 ) | DR_0 ); //���ʷ�Χ:((�������<<4) | (�������))
		g_macData.channels[0].Band = 0;
		//�����ŵ�2
		g_macData.channels[1].Frequency = 490500000;
		g_macData.channels[1].DrRange.Value = ( ( DR_5 << 4 ) | DR_0 );
		g_macData.channels[1].Band = 0;
		//�����ŵ�3
		g_macData.channels[2].Frequency = 490700000;
		g_macData.channels[2].DrRange.Value = ( ( DR_5 << 4 ) | DR_0 );
		g_macData.channels[2].Band = 0; 
		//�����ŵ�4
		g_macData.channels[3].Frequency = 490900000;
		g_macData.channels[3].DrRange.Value = ( ( DR_5 << 4 ) | DR_0 );
		g_macData.channels[3].Band = 0;
		//�����ŵ�5
		g_macData.channels[4].Frequency = 491100000;
		g_macData.channels[4].DrRange.Value = ( ( DR_5 << 4 ) | DR_0 );
		g_macData.channels[4].Band = 0;
		//�����ŵ�6
		g_macData.channels[5].Frequency = 491300000;
		g_macData.channels[5].DrRange.Value = ( ( DR_5 << 4 ) | DR_0 );
		g_macData.channels[5].Band = 0; 
		//�����ŵ�7
		g_macData.channels[6].Frequency = 491500000;
		g_macData.channels[6].DrRange.Value = ( ( DR_5 << 4 ) | DR_0 );
		g_macData.channels[6].Band = 0;
		//�����ŵ�8
		g_macData.channels[7].Frequency = 491700000;
		g_macData.channels[7].DrRange.Value = ( ( DR_5 << 4 ) | DR_0 );
		g_macData.channels[7].Band = 0;
		//��������
		g_macData.datarate = DR_5;
		//�������а�����
		g_macData.packet_type = UNCONFIRMED_UP;
		//ADR������ر�
		g_macData.lora_mac_adr_switch = TRUE;
		LoRaMac_setMacLayerParameter(&g_macData, PARAMETER_CHANNELS | PARAMETER_ADR_SWITCH | PARAMETER_DATARATE | PARAMETER_PACKET_TYPE );
		//����ʹ��LoRaMAC
		LoRaMac_setMode(MODE_LORAMAC);
		
#elif defined( USE_BAND_780 )
	
		//����LORAMAC����ģʽ�Ĳ���(LoRa����)
		//�����ŵ�1
		g_macData.channels[0].Frequency = 779500000;//Ƶ��
		g_macData.channels[0].DrRange.Value = ( ( DR_5 << 4 ) | DR_0 ); //���ʷ�Χ:((�������<<4) | (�������))
		g_macData.channels[0].Band = 0;

		//�����ŵ�2	
		g_macData.channels[1].Frequency = 779700000;
		g_macData.channels[1].DrRange.Value = ( ( DR_5 << 4 ) | DR_0 );
		g_macData.channels[1].Band = 0;
		//�����ŵ�3
		g_macData.channels[2].Frequency = 779900000;
		g_macData.channels[2].DrRange.Value = ( ( DR_5 << 4 ) | DR_0 );
		g_macData.channels[2].Band = 0; 
		//�����ŵ�4
		g_macData.channels[3].Frequency = 780100000;
		g_macData.channels[3].DrRange.Value = ( ( DR_5 << 4 ) | DR_0 );
		g_macData.channels[3].Band = 0;
		//�����ŵ�5
		g_macData.channels[4].Frequency = 786500000;
		g_macData.channels[4].DrRange.Value = ( ( DR_5 << 4 ) | DR_0 );
		g_macData.channels[4].Band = 0; 
		//�����ŵ�6
		g_macData.channels[5].Frequency = 786700000;
		g_macData.channels[5].DrRange.Value = ( ( DR_5 << 4 ) | DR_0 );
		g_macData.channels[5].Band = 0;
		//�����ŵ�7
		g_macData.channels[6].Frequency = 786900000;
		g_macData.channels[6].DrRange.Value = ( ( DR_5 << 4 ) | DR_0 );
		g_macData.channels[6].Band = 0;
		//�����ŵ�8
		g_macData.channels[7].Frequency = 787100000;
		g_macData.channels[7].DrRange.Value = ( ( DR_5 << 4 ) | DR_0 );
		g_macData.channels[7].Band = 0;
		//��������
		g_macData.datarate = DR_5;
		//�������а�����
		g_macData.packet_type = UNCONFIRMED_UP;
		//ADR������ر�
		g_macData.lora_mac_adr_switch = TRUE;
		LoRaMac_setMacLayerParameter(&g_macData, PARAMETER_CHANNELS | PARAMETER_ADR_SWITCH | PARAMETER_DATARATE | PARAMETER_PACKET_TYPE );
		//����ʹ��LoRaMAC
		LoRaMac_setMode(MODE_LORAMAC);
		
#elif defined( USE_BAND_868 )
	
		//����LORAMAC����ģʽ�Ĳ���(LoRa����)
		//�����ŵ�1
		g_macData.channels[0].Frequency = 868100000;//Ƶ��
		g_macData.channels[0].DrRange.Value = ( ( DR_5 << 4 ) | DR_0 ); //���ʷ�Χ:((�������<<4) | (�������))
		g_macData.channels[0].Band = 0;
		//�����ŵ�2
		g_macData.channels[1].Frequency = 868300000;
		g_macData.channels[1].DrRange.Value = ( ( DR_5 << 4 ) | DR_0 );
		g_macData.channels[1].Band = 0;
		//�����ŵ�3
		g_macData.channels[2].Frequency = 868500000;
		g_macData.channels[2].DrRange.Value = ( ( DR_5 << 4 ) | DR_0 );
		g_macData.channels[2].Band = 0; 
		//�����ŵ�4
		g_macData.channels[3].Frequency = 868700000;
		g_macData.channels[3].DrRange.Value = ( ( DR_5 << 4 ) | DR_0 );
		g_macData.channels[3].Band = 0;
		//�����ŵ�5
		g_macData.channels[4].Frequency = 869100000;
		g_macData.channels[4].DrRange.Value = ( ( DR_5 << 4 ) | DR_0 );
		g_macData.channels[4].Band = 0;
		//�����ŵ�6
		g_macData.channels[5].Frequency = 869300000;
		g_macData.channels[5].DrRange.Value = ( ( DR_5 << 4 ) | DR_0 );
		g_macData.channels[5].Band = 0;
		//�����ŵ�7
		g_macData.channels[6].Frequency = 869500000;
		g_macData.channels[6].DrRange.Value = ( ( DR_5 << 4 ) | DR_0 );
		g_macData.channels[6].Band = 0;
		//�����ŵ�8
		g_macData.channels[7].Frequency = 869700000;
		g_macData.channels[7].DrRange.Value = ( ( DR_5 << 4 ) | DR_0 );
		g_macData.channels[7].Band = 0;
		//��������
		g_macData.datarate = DR_5;
		//�������а�����
		g_macData.packet_type = UNCONFIRMED_UP;
		//ADR������ر�
		g_macData.lora_mac_adr_switch = TRUE;
		LoRaMac_setMacLayerParameter(&g_macData, PARAMETER_CHANNELS | PARAMETER_ADR_SWITCH | PARAMETER_DATARATE | PARAMETER_PACKET_TYPE );
		//����ʹ��LoRaMAC
		LoRaMac_setMode(MODE_LORAMAC);
		
#elif  defined( USE_BAND_915 ) || defined( USE_BAND_915_HYBRID )
		
		//����LORAMAC����ģʽ�Ĳ���(LoRa����)
    // 125 kHz channels
    for( uint8_t i = 0; i < LORA_MAX_NB_CHANNELS - 8; i++ )
    {
			g_macData.channels[i].Frequency = 902.3e6 + i * 200e3;
			g_macData.channels[i].DrRange.Value =  ( DR_3 << 4 ) | DR_0;
			g_macData.channels[i].Band = 0;			
    }
    // 500 kHz channels
    for( uint8_t i = LORA_MAX_NB_CHANNELS - 8; i < LORA_MAX_NB_CHANNELS; i++ )
		{
			g_macData.channels[i].Frequency = 903.0e6 + ( i - ( LORA_MAX_NB_CHANNELS - 8 ) ) * 1.6e6;
			g_macData.channels[i].DrRange.Value =  ( DR_4 << 4 ) | DR_4;
			g_macData.channels[i].Band = 0;
		}
		
		//��������
		g_macData.datarate = DR_5;
		//�������а�����
		g_macData.packet_type = UNCONFIRMED_UP;
		//ADR������ر�
		g_macData.lora_mac_adr_switch = TRUE;
		LoRaMac_setMacLayerParameter(&g_macData, PARAMETER_CHANNELS | PARAMETER_ADR_SWITCH | PARAMETER_DATARATE | PARAMETER_PACKET_TYPE );
		//����ʹ��LoRaMAC
		LoRaMac_setMode(MODE_LORAMAC);

#endif
		
#endif

#if 0
		//����LORAMAC����ģʽ�Ĳ���(FSK����)
		//�����ŵ�1
		g_macData.channels[0].Frequency = 779500000;//Ƶ��
		g_macData.channels[0].DrRange.Value = ( ( DR_7 << 4 ) | DR_7 ); //���ʷ�Χ:((�������<<4) | (�������))
		g_macData.channels[0].Band = 0;
		//�����ŵ�2
		g_macData.channels[1].Frequency = 779700000;
		g_macData.channels[1].DrRange.Value = ( ( DR_7 << 4 ) | DR_7 );
		g_macData.channels[1].Band = 0;
		//�����ŵ�3
		g_macData.channels[2].Frequency = 779900000;
		g_macData.channels[2].DrRange.Value = ( ( DR_7 << 4 ) | DR_7 );
		g_macData.channels[2].Band = 0;
		//��������
		g_macData.datarate = DR_7;
		//�������а�����
		g_macData.packet_type = UNCONFIRMED_UP;
		//ADR������ر�
		g_macData.lora_mac_adr_switch = FALSE;
		//����FSK����
		g_macData.fskFdev = 25000;//FSK�����µ�Ƶƫ
		g_macData.fskDatarate= 50000;//FSK�����µķ�������
		g_macData.fskBandwidth = 50000;//FSK�����µĽ��մ���
		g_macData.fskAfcBandwidth = 83333;//FSK�����µ�AFC����
		LoRaMac_setMacLayerParameter(&g_macData, PARAMETER_DATARATE | PARAMETER_ADR_SWITCH \
																	|  PARAMETER_CHANNELS| PARAMETER_FSK_FDEV | PARAMETER_FSK_DATARATE \
																	| PARAMETER_FSK_BANDEIDTH | PARAMETER_FSK_AFC_BANDWIDTH | PARAMETER_PACKET_TYPE );
		//����ʹ��LoRaMAC
		LoRaMac_setMode(MODE_LORAMAC);
#endif

#if 0
		//����PHYMAC����ģʽ�Ĳ���(LoRa���Ʒ�ʽ)
		g_macData.phyFrequency = 779700000;//Ƶ��(Hz)
		g_macData.phySF = 7; //��Ƶ����(7-12)
		g_macData.phyModulation = MODULATION_LORA;//���Ʒ�ʽ(FSK or LORA)
		LoRaMac_setMacLayerParameter(&g_macData, PARAMETER_PHY_FREQUENCY | PARAMETER_PHY_SPREADING_FACTOR \
																| PARAMETER_PHY_MODULATION_MODE );	//����ʹ��PhyMAC
		LoRaMac_setMode(MODE_PHY);
#endif

#if 0
		//����PHYMAC����ģʽ�Ĳ���(FSK���Ʒ�ʽ)
		g_macData.fskFdev = 25000;//FSK�����µ�Ƶƫ
		g_macData.fskDatarate= 45000;//FSK�����µķ�������
		g_macData.fskBandwidth = 50000;//FSK�����µĴ���
		g_macData.fskAfcBandwidth = 83333;//FSK�����µ�AFC����
		g_macData.phyFrequency = 779700000;//Ƶ��(Hz)
		g_macData.phyModulation = MODULATION_FSK;//���Ʒ�ʽ(FSK or LORA)	
		LoRaMac_setMacLayerParameter(&g_macData, PARAMETER_PHY_FREQUENCY | PARAMETER_PHY_MODULATION_MODE \
																| PARAMETER_FSK_FDEV | PARAMETER_FSK_DATARATE | PARAMETER_FSK_BANDEIDTH \
																| PARAMETER_FSK_AFC_BANDWIDTH );
		//����ʹ��PhyMAC
		LoRaMac_setMode(MODE_PHY);
#endif

}

/* Private functions ---------------------------------------------------------*/
void APP_EnterlowPowerMode( void )
{
	Radio.Sleep();	
	LoRaMac_setlowPowerMode(TRUE);
	RtcEnterLowPowerStopMode();
}

//display NPLink mote ID on the OLED
void APP_ShowMoteID( u32 moteID )
{
	u8 	MoteIDString[32] ;
	u8* pIDString = MoteIDString;
	u32 ZeroNum = 0 ;

	//count the zero num in front of moteID string
	for(u8 i = 28; i > 0; i = i - 4)
	{
		if((moteID >> i ) % 16 == 0)
		{
			ZeroNum = ZeroNum + 1 ;
		}
		else
		{
			break;
		}
	}
	sprintf((char*)pIDString,"ID:");
	pIDString += 3;
	while(ZeroNum--)
	{
		sprintf((char*)pIDString,"0");
		pIDString++;
	}
	sprintf((char*)pIDString,"%x",moteID);

	OLED_ShowString( 0,0,MoteIDString,12 );
	OLED_Refresh_Gram();
}
/******************* (C) COPYRIGHT 2015 NPLink *****END OF FILE****/

