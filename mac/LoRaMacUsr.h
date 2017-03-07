/*
(C)2015 NPLink

Description: LoRa MAC layer user application interface.

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Robxr

*/

#ifndef __LORAMACUSR_H__
#define __LORAMACUSR_H__

/* Includes ------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "oled_board.h"
#include <string.h>
#include "utilities.h"

/*Macros--------------------------------------------------------------------*/
//define work mode of mote (LoRaMac or PhyMac)
//����MAC�Ĺ���ģʽ��������LoRaMac(��Э��)��phyMac(����Э��)
#define MODE_LORAMAC											0 
#define MODE_PHY													1

//define modulation mode of mote (LoRa or FSK)
//���������оƬ�ĵ��Ʒ�ʽ��������LoRa���ƻ�FSK����
#define MODULATION_FSK										0 
#define MODULATION_LORA										1		

//define working frequency of mote (confirmer or unconfirmer)
//�������а������ͣ�����ʹȷ�ϰ����ȷ�ϰ�
#define UNCONFIRMED_UP								    0 
#define CONFIRMED_UP								      1	

//define return status
//����MAC�㴦��״̬
#define LORAMAC_USR_SUCCESS								0//�ɹ�
#define LORAMAC_USR_INVALID_PARAMETER			1//��Ч����
#define LORAMAC_USR_FAILURE								0xFF//ʧ��

//define parameter IDs of LoRaMac application layer
//����Ӧ�ò����ID��
#define PARAMETER_DEV_ADDR 								(u32)(1 << 0)//�豸��ַ(4B)
#define PARAMETER_DEV_EUI									(u32)(1 << 1)
#define PARAMETER_APP_EUI									(u32)(1 << 2)//LoRaWAN AppEUIֵ
#define PARAMETER_APP_KEY									(u32)(1 << 3)//LoRaWAN AppKey����ʹ��over-the-air activationʱʹ��
#define PARAMETER_NWK_SKEY								(u32)(1 << 4)//LoRaWAN NwkSkey����activation by personalizationʱʹ��
#define PARAMETER_APP_SKEY								(u32)(1 << 5)//LoRaWAN AppSkey����activation by personalizationʱʹ��
#define PARAMETER_DEV_TXPOWER 				    (u32)(1 << 6)//���ݷ��͹���
#define PARAMETER_DEV_NETWORKJOINED 			(u32)(1 << 7)//�豸��������

//define parameter IDs of LoRaMac mac layer
//����MAC�����ID��
#define PARAMETER_BANDS 									(u32)(1 << 0)//LoRaWAN MACģʽ����ʱʹ�õ�Ƶ�㣬��ǰ֧��1��Ƶ��
#define PARAMETER_CHANNELS								(u32)(1 << 1)//LoRaWAN MACģʽ����ʱ����Ƶ����ʹ�õ��ŵ�����ǰ֧�����16���ŵ�
#define PARAMETER_DATARATE								(u32)(1 << 2)//LoRaWAN MACģʽ����ʱ����������
#define PARAMETER_PACKET_TYPE							(u32)(1 << 3)//LoRaWAN MACģʽ����ʱ�����������а�����
#define PARAMETER_CLASS_MODE							(u32)(1 << 4)//LoRaWAN MACģʽ����ʱ��ѡ���class����
#define PARAMETER_ADR_SWITCH							(u32)(1 << 5)//LoRaWAN MACģʽ����ʱ��ADRʹ�ܻ�ȥʹ��
#define PARAMETER_PHY_FREQUENCY			      (u32)(1 << 6)//phy MACģʽ����ʱ������Ƶ��
#define PARAMETER_PHY_SPREADING_FACTOR		(u32)(1 << 7)//phy MACģʽ����ʱ��LORA���Ʒ�ʽ�µ���Ƶ���ӣ���ЧֵΪ7-12
#define PARAMETER_PHY_MODULATION_MODE			(u32)(1 << 8)//phy MACģʽ����ʱ�����Ʒ�ʽ��ѡ��
#define PARAMETER_FSK_FDEV								(u32)(1 << 9)//FSK���Ʒ�ʽ�µ�Ƶƫ
#define PARAMETER_FSK_DATARATE						(u32)(1 << 10)//FSK���Ʒ�ʽ�µ�����
#define PARAMETER_FSK_BANDEIDTH						(u32)(1 << 11)//FSK���Ʒ�ʽ�µĴ���
#define PARAMETER_FSK_AFC_BANDWIDTH				(u32)(1 << 12)//FSK���Ʒ�ʽ�µ�afcbandwidth

//define length of parameters
//�����������
#define APP_EUI_LEN												8
#define DEV_EUI_LEN												8
#define MOTE_DEV_ADDR_LEN									4
#define APP_KEY_LEN												16
#define NWK_SKEY_LEN											16
#define APP_SKEY_LEN											16

#if defined( USE_BAND_868 )
	#define LORA_MAX_NB_BANDS 								5//five band is supported currentlly.
#else
	#define LORA_MAX_NB_BANDS 								1//only one band is supported currentlly.
#endif

#if defined( USE_BAND_915 )
	#define LORA_MAX_NB_CHANNELS 							72// channels are supported currentlly.
#else
	#define LORA_MAX_NB_CHANNELS 							16//only 16 channels are supported currentlly.
#endif
//define TxPower 
//���巢�͹���
#define TX_POWER_MAX_INDEX							 8
#define TX_POWER_20_DBM            			 0
#define TX_POWER_17_DBM             		 1
#define TX_POWER_16_DBM             		 2
#define TX_POWER_14_DBM             		 3
#define TX_POWER_12_DBM             		 4
#define TX_POWER_10_DBM             		 5
#define TX_POWER_07_DBM             		 6
#define TX_POWER_05_DBM             		 7
#define TX_POWER_02_DBM             		 8

/*!
 * LoRaMac datarates definition
 */
#define DR_0                             0  // SF12 - BW125
#define DR_1                             1  // SF11 - BW125
#define DR_2                             2  // SF10 - BW125
#define DR_3                             3  // SF9  - BW125
#define DR_4                             4  // SF8  - BW125
#define DR_5                             5  // SF7  - BW125
#define DR_6                             6  // SF7  - BW250
#define DR_7                             7  // FSK

/* typedef -----------------------------------------------------------*/
typedef struct
{
    uint16_t DCycle;
    int8_t TxMaxPower;
    uint64_t LastTxDoneTime;
    uint64_t TimeOff;
}  Band_t;

/*!
 * LoRaWAN devices classes definition
 */
typedef enum eDeviceClass
{
	CLASS_A, //LoRaWAN device class A
	CLASS_B, //LoRaWAN device class B
	CLASS_C, //LoRaWAN device class C
}DeviceClass_t;

/*!
 * LoRaMAC channels parameters definition
 */
typedef union
{
    int8_t Value;
    struct
    {
			int8_t Min : 4;
			int8_t Max : 4;
    } Fields;
} DrRange_t;

typedef struct
{
    uint32_t Frequency; // Hz
    DrRange_t DrRange;  // Max datarate [0: SF12, 1: SF11, 2: SF10, 3: SF9, 4: SF8, 5: SF7, 6: SF7, 7: FSK]
                        // Min datarate [0: SF12, 1: SF11, 2: SF10, 3: SF9, 4: SF8, 5: SF7, 6: SF7, 7: FSK]
    uint8_t Band;       // Band index
} ChannelParams_t;

/* typedef -----------------------------------------------------------*/
typedef struct LoRaMacMacPara
{
	Band_t bands[LORA_MAX_NB_BANDS];//LORA MAC��Ƶ�㶨��
	ChannelParams_t channels[LORA_MAX_NB_CHANNELS];//LORA MAC���ŵ�����
	u8 datarate;//LORA MAC�ķ�������
	u8 packet_type;//���а�����
	DeviceClass_t class_mode;//LORAWAN Э���class����
	bool lora_mac_adr_switch ;//LORA MAC��ADRʹ�����
	u32 phyFrequency;//phy MAC��Ƶ�㶨��
	u8  phySF;//phy MAC��LORA���Ʒ�ʽʱ����Ƶ���� 
	u8  phyModulation;//phy MAC�ĵ��Ʒ�ʽ����
	u32 fskFdev;
	u32 fskDatarate;
	u32 fskBandwidth;
	u32 fskAfcBandwidth;
}LoRaMacMacPara_t;

typedef struct LoRaMacAppPara
{
	u32  devAddr;//�豸��ַ
	u8   txPower;//���͹���
	bool isnetworkjoined ;//�豸�Ƿ�����
	u8   appEUI[APP_EUI_LEN];//LoRaWAN AppEUIֵ
	u8   devEUI[DEV_EUI_LEN];//LoRaWAN DEVEUIֵ
	u8 	 appKey[APP_KEY_LEN];//LoRaWAN AppKey
	u8 	 nwkSKey[NWK_SKEY_LEN];//LoRaWAN NwkSkey
	u8   appSKey[APP_SKEY_LEN];//LoRaWAN AppSkey
}LoRaMacAppPara_t;

/* function prototypes -----------------------------------------------*/
/*!
 * LoRaMac_setAppLayerParameter -- ����APP �����
 *
 * \param [IN]  pdata_in -- ָ�������������ָ�룬���ݿռ��ɵ���������
 *                   parameterIDs -- ����ID�ţ���ʹ��λ��ʽͬʱ���ö��������֧�ֵ�ID������:
 							PARAMETER_DEV_ADDR
 							PARAMETER_APP_EUI
 							PARAMETER_APP_KEY
 							PARAMETER_NWK_SKEY
 							PARAMETER_APP_SKEY
 */
u8 LoRaMac_setAppLayerParameter( void* pdata_in, u32 parameterIDs);

/*!
 * LoRaMac_getAppLayerParameter -- ��ȡAPP �㵱ǰ���ò���ֵ
 *
 * \param [IN]  pdata_out -- ָ�������������ָ�룬���ݿռ��ɵ���������
 *                   parameterIDs -- ����ID�ţ���ʹ��λ��ʽͬʱ���ö��������֧�ֵ�ID������:
 							PARAMETER_DEV_ADDR
 							PARAMETER_APP_EUI
 							PARAMETER_APP_KEY
 							PARAMETER_NWK_SKEY
 							PARAMETER_APP_SKEY
 */
u8 LoRaMac_getAppLayerParameter( void* pdata_out, u32 parameterIDs);

/*!
 * LoRaMac_setMacLayerParameter -- ����MAC �����
 *
 * \param [IN]  pdata_in -- ָ�������������ָ�룬���ݿռ��ɵ���������
 *                   parameterIDs -- ����ID�ţ���ʹ��λ��ʽͬʱ���ö��������֧�ֵ�ID������:
 							PARAMETER_BANDS
 							PARAMETER_CHANNELS
 							PARAMETER_ADR_SWITCH
 							PARAMETER_PHY_FREQUENCY
 							PARAMETER_PHY_SPREADING_FACTOR
 							PARAMETER_PHY_MODULATION_MODE
 */
u8 LoRaMac_setMacLayerParameter( void* pdata_in, u32 parameterIDs);

/*!
 * LoRaMac_getMacLayerParameter -- ��ȡMAC �㵱ǰ���ò���ֵ
 *
 * \param [IN]  pdata_out -- ָ�������������ָ�룬���ݿռ��ɵ���������
 *                   parameterIDs -- ����ID�ţ���ʹ��λ��ʽͬʱ���ö��������֧�ֵ�ID������:
 							PARAMETER_BANDS
 							PARAMETER_CHANNELS
 							PARAMETER_ADR_SWITCH
 							PARAMETER_PHY_FREQUENCY
 							PARAMETER_PHY_SPREADING_FACTOR
 							PARAMETER_PHY_MODULATION_MODE
 */
u8 LoRaMac_getMacLayerParameter( void* pdata_out, u32 parameterIDs);

/*!
 * LoRaMac_setMode -- ����MAC�㹤��ģʽ
 *
 * \param [IN]  mode -- ����ģʽ��ȡֵΪ:
 											MODE_LORAMAC -- LORA MAC��ʽ����
 											MODE_PHY -- phy MAC��ʽ����
 *                   
 */
u8 LoRaMac_setMode(u8 mode);

/*!
 * LoRaMac_setlowPowerMode -- ����radio���ֵ͹���,ʹ�ܺ�radio���ֽ��رվ��񣬲�����sleep״̬
 *
 * \param [IN]  enable -- ʹ�����ȡֵΪ:
 							TRUE -- ʹ�ܵ͹���
 							FALSE -- ȥʹ�ܵ͹���			
 *                   
 */
void LoRaMac_setlowPowerMode(u8 enable);


#endif // __LORAMAC_H__
