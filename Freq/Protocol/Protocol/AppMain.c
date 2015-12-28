#include "AppMain.h"
#include "Tools.h"
#include  <includes.h>

#define APPCOMMUNICATE_PORT USART3
#define APPCOMMUNICATE_PORT_TIMEOUT		20

#define START_SYMBOL					0XFE
#define PROTOCOL_EDITION				0X01
#define DATA_DIRECTION					0X01
#define	MAINCMD_INTERNET 				0X01		//SCT������
#define	SUBCMD_DT_SCT	 				0X0001		//ʹ��DT��SCT������
#define	SUBCMD_OTHER_SCT	 			0X0002		//ʹ������SCT��������
#define	FIRST_END_SYMBOL	 			0X0D
#define	SECOND_END_SYMBOL	 			0X0A



enum
{
	ERROR_CMD,
	DT_CMD,
	OTHER_CMD,
};

enum
{
	DT_PROTOCOL,
	DT_UNPROTOCOL,
};


enum
{
	APP_NO_ERROR = 0,
	APP_CRC_ERROR,
	APP_MALLOC_ERROR,
};

typedef struct _tagProtocolInfo
{
	U8 u8ProtocolType;
	U8 u8ErrorCode;

}CProtocolInfo, *PProtocolInfo;

#define ARRAY_SIZEOF(s)		(sizeof(s)-1)
	
//***�ɵ�������
CONST U8 szDeviceName[] = "15001";
//***�ɵ�������
CONST U8 szDeviceInfo[] = "SCT";
CONST U8 szErrorInfo1[] = "Flow error";
CONST U8 szErrorInfo2[] = "Command error";
CONST U8 szInfo1[] = "PROTOL";
CONST U8 szInfo2[] = "UNPROTOL";
CONST U8 szEndInfo[] = "\r\n";

CONST U8 szInqueryCmd[] = "DT INFO\r\n";
	
U8 szInquery1Cmd[] = "DT XXXXX INFO\r\n";
U8 szConfigureInfo1[] = "DT xxxxx CONF UNPROTOCOL TYPE\r\n";

U8 szConfigureInfo2[] = "DT xxxxx CONF PROTOCOL TYPE\r\n";
U8 szCmd1[] = "DT XXXXX READ ALL\r\n";
U8 szCmd2[] = "DT XXXXX READ X\r\n";
U8 szResponse[] = "#SCTXXXXX SUCCESS\r\n";

extern PQueueInfo pUart3QueueInfo;
extern unsigned char finishflag;

extern int yuliangcishu;
extern char yuliang[2048];

STATIC VOID AppCommunicatePort(USART_TypeDef* USARTx, U8 *pBuf, U16 u16Length)
{	
	UsartSend(USARTx, pBuf, u16Length);
}

STATIC VOID HandleDataPackage(U8 *pBuf, U16 u16Length, PProtocolInfo pProtocolInfo)
{
	U32 *pData;
	U32 u32Length;
	U32 u32TotalPackageNumber = 0;//�ܵİ���
	U32 u32CurrentPackageNumber = 0;//��ǰ����
	U32 u32CrcValue = 0;
	U16 u16Loop = 0 ;
	U8 *szData;
	u32Length = u16Length + 19;
	
	pData = malloc(u32Length*4);
	
	if(pProtocolInfo->u8ProtocolType == DT_PROTOCOL)
	{
		if(pData != NULL)
		{
			for(u16Loop = 0; u16Loop < u32Length; u16Loop++)
			{
				pData[u16Loop] = pBuf[u16Loop];
			}
			u32CrcValue = SoftGenCrc32(pData, u16Length);
			free(pData);
		}
		else
		{
			pProtocolInfo->u8ErrorCode = APP_MALLOC_ERROR;
		}
		
		szData = malloc(u16Length + 25);
		
		if(pData == NULL)
		{
			pProtocolInfo->u8ErrorCode = APP_MALLOC_ERROR;
		}
		szData[0] = START_SYMBOL;					//FE
		szData[1] = PROTOCOL_EDITION;				//01
		szData[2] = HIBYTE(HIWORD(u32Length));		//����
		szData[3] = LOBYTE(HIWORD(u32Length));		//����
		szData[4] = HIBYTE(LOWORD(u32Length));		//����
		szData[5] = LOBYTE(LOWORD(u32Length));		//����
		szData[6] = DATA_DIRECTION;					//����
		szData[7] = MAINCMD_INTERNET;				//������
		szData[8] = HIBYTE(SUBCMD_DT_SCT);			//������
		szData[9] = LOBYTE(SUBCMD_DT_SCT);			//������
		szData[10] = NENCRYPTION_NCOMPRESSION;		//�������
		szData[11] = HIBYTE(HIWORD(u32TotalPackageNumber));//�������
		szData[12] = LOBYTE(HIWORD(u32TotalPackageNumber));//�������
		szData[13] = HIBYTE(LOWORD(u32TotalPackageNumber));//�������
		szData[14] = LOBYTE(LOWORD(u32TotalPackageNumber));//�������
		szData[15] = HIBYTE(HIWORD(u32CurrentPackageNumber));//��ǰ�����
		szData[16] = LOBYTE(HIWORD(u32CurrentPackageNumber));//��ǰ�����
		szData[17] = HIBYTE(LOWORD(u32CurrentPackageNumber));//��ǰ�����
		szData[18] = LOBYTE(LOWORD(u32CurrentPackageNumber));//��ǰ�����
		memcpy(szData + 19, pBuf, u16Length);
		
		szData[19 + u16Length] = HIBYTE(HIWORD(u32CrcValue));
		szData[20 + u16Length] = LOBYTE(HIWORD(u32CrcValue));
		szData[21 + u16Length] = HIBYTE(LOWORD(u32CrcValue));
		szData[22 + u16Length] = LOBYTE(LOWORD(u32CrcValue));
		
		szData[23 + u16Length] = FIRST_END_SYMBOL;			//��������
		
		szData[24 + u16Length] = SECOND_END_SYMBOL;			//��������
	
		
		AppCommunicatePort(APPCOMMUNICATE_PORT, szData, u32Length + 6);
		
		free(szData);
	}
	else
	{
		AppCommunicatePort(APPCOMMUNICATE_PORT, pBuf, u16Length);
	}
}



STATIC VOID HandleErrorFlowProc(PProtocolInfo pProtocolInfo)
{
	U8 szData[50] = {0};

	sprintf((char *)szData, "%s=%d%s", szErrorInfo1, pProtocolInfo->u8ErrorCode, szEndInfo);
		
	AppCommunicatePort(APPCOMMUNICATE_PORT, szData, strlen((char *)szData));
}

STATIC VOID HandleErrorCmdProc(VOID)
{
	U8 szData[50] = {0};
	U16 u16Length;

	sprintf((char *)szData, "%s", szErrorInfo2);
	
	u16Length = strlen((char *)szData);
	
	sprintf((char *)szData + u16Length, (char *)szEndInfo);
	
	AppCommunicatePort(APPCOMMUNICATE_PORT, szData, strlen((char *)szData));
}

STATIC VOID GetCurrentAppConfigure(PProtocolInfo pProtocolInfo)
{
	U8 szData[100] = {0};
	U16 u16Length;
	U8 szId[50];
	
	read_dev_id(szId);	
	sprintf((char *)szData, "#%s%s=0x%s ", szDeviceInfo, szDeviceName, szId);
	
	u16Length = strlen((char *)szData);

	if(pProtocolInfo->u8ProtocolType == DT_PROTOCOL)
	{
		sprintf((char *)szData + u16Length, (char *)szInfo1);
	}
	else
	{
		sprintf((char *)szData + u16Length, (char *)szInfo2);
	}
	
	u16Length = strlen((char *)szData);
	
	sprintf((char *)szData + u16Length, (char *)szEndInfo);
	
	u16Length = strlen((char *)szData);
		
	AppCommunicatePort(APPCOMMUNICATE_PORT, szData, u16Length);
	
}

STATIC VOID HandleAllModule(PProtocolInfo pProtocolInfo)
{
	U8 szData[4096] = {0};
		
	U8 szId[50];
	
	read_dev_id(szId);
	
//get_time(),dev_ID,get_dev_voltage(get_adc_value()),\
//GetNTCTemperature(LTC2402_GetResistance(5)),\
//GetNTCTemperature(LTC2402_GetResistance(6)),\
//GetNTCTemperature(LTC2402_GetResistance(7)),\
//GetNTCTemperature(LTC2402_GetResistance(8)),\
//GetFreq(1),GetFreq(2),GetFreq(3),GetFreq(4));

sprintf((char *)szData, "#SCT%s=0x%s %s V:%f M1=%s P1=0 M2=0 P2=0 M3=0 P3=0 M4=0 P4=0 M5=0 P5=0 M6=0 P6=0 M7=0 P7=0 M8=0 P8=0%s", szDeviceName, szId,get_time(),\
get_dev_voltage(get_adc_value()),yuliang,
szEndInfo);
	//sentusartdata(yuliang);
	yuliangcishu = 0;
	memset(yuliang,0,4096);
	HandleDataPackage(szData, strlen((char *)szData), pProtocolInfo);
}

STATIC VOID HandleIndividualModule(S8 s8Value, PProtocolInfo pProtocolInfo)
{
	U8 u8Value;
	U8 szId[50];
	U8 szData[150] = {0};
	u8Value = s8Value - 0x30;
	
	
	read_dev_id(szId);
	sprintf((char *)szData, "#SCT%s=0x%s %s V:%f M%d=%f P%d= 0 %s", szDeviceName, szId,get_time(),get_dev_voltage(get_adc_value()),GetFreq(u8Value), u8Value, u8Value, szEndInfo);
	HandleDataPackage(szData, strlen((char *)szData), pProtocolInfo);
}


STATIC U8 HandleAppCmdProc(U8 *pBuf, U16 u16Length, PProtocolInfo pProtocolInfo);

STATIC VOID HandleProtocolData(U8 *pBuf, U16 u16Length, PProtocolInfo pProtocolInfo)
{	
	
	U16 u16Loop;
	
	U32 *pCheckData;
	
	U32 u32CrcValue, u32CheckLength, u32CrcCheckValue;
	
	u32CheckLength = u16Length - 25;

	pCheckData = malloc(u32CheckLength*4);
	
	if(pCheckData != NULL)
	{
		for(u16Loop = 0; u16Loop < u32CheckLength; u16Loop++)
		{
			pCheckData[u16Loop] = pBuf[u16Loop + 19];
		}
		u32CrcValue = SoftGenCrc32(pCheckData, u32CheckLength);
		
		free(pCheckData);
		
		u32CrcCheckValue = MAKELONG(MAKEWORD(pBuf[u16Length-3], pBuf[u16Length-4]), MAKEWORD(pBuf[u16Length-5], pBuf[u16Length-6]));
	
		if(u32CrcValue == u32CrcCheckValue)
		{
			U32 u32Length = 0;
			U8 *pData = malloc(u32Length - 19) ;
		
			u32Length = MAKELONG(MAKEWORD(pBuf[5], pBuf[4]), MAKEWORD(pBuf[3], pBuf[2]));
			
			if(pData != NULL)
			{
				memcpy(pData, pBuf + 19, u32Length - 19);

				HandleAppCmdProc(pData, u32Length - 19, pProtocolInfo);
				
				free(pData);
			}
			else
			{
				pProtocolInfo->u8ErrorCode = APP_MALLOC_ERROR;
			}

			
		}
		else
		{
			pProtocolInfo->u8ErrorCode = APP_CRC_ERROR;
		}
	}
	else
	{
		pProtocolInfo->u8ErrorCode = APP_MALLOC_ERROR;
	}
	
	
}


STATIC U8 HandleAppCmdProc(U8 *pBuf, U16 u16Length, PProtocolInfo pProtocolInfo)
{
	U8 u8Ret = 0;
		
	if((pBuf[0] == 'D') && (pBuf[1] == 'T'))
	{
		u8Ret = DT_CMD;
		if((memcmp(pBuf, szInqueryCmd, ARRAY_SIZEOF(szInqueryCmd)) == 0) && (u16Length == ARRAY_SIZEOF(szInqueryCmd)))
		{
			GetCurrentAppConfigure(pProtocolInfo);
		}
		else
		{
			if((memcmp(pBuf, szConfigureInfo1, ARRAY_SIZEOF(szConfigureInfo1)) == 0) && (u16Length == ARRAY_SIZEOF(szConfigureInfo1)))
			{
				pProtocolInfo->u8ProtocolType = DT_UNPROTOCOL;
				AppCommunicatePort(APPCOMMUNICATE_PORT, szResponse, ARRAY_SIZEOF(szResponse));		
			}
			else if((memcmp(pBuf, szConfigureInfo2, ARRAY_SIZEOF(szConfigureInfo2)) == 0) && (u16Length == ARRAY_SIZEOF(szConfigureInfo2)))
			{
				pProtocolInfo->u8ProtocolType = DT_PROTOCOL;
				AppCommunicatePort(APPCOMMUNICATE_PORT, szResponse, ARRAY_SIZEOF(szResponse));
			}
			
			else if((memcmp(pBuf, szCmd1, ARRAY_SIZEOF(szCmd1)) == 0) && (u16Length == ARRAY_SIZEOF(szCmd1)))
			{
				HandleAllModule(pProtocolInfo);
			}
			else if((memcmp(pBuf, szCmd2, ARRAY_SIZEOF(szCmd2)) == 0) && (u16Length == ARRAY_SIZEOF(szCmd2)))
			{
				HandleIndividualModule(pBuf[14], pProtocolInfo);
			}
			
			else if((memcmp(pBuf, szInquery1Cmd, ARRAY_SIZEOF(szInquery1Cmd)) == 0) && (u16Length == ARRAY_SIZEOF(szInquery1Cmd)))
			{
				GetCurrentAppConfigure(pProtocolInfo);
			}
			
			else
			{
				u8Ret = ERROR_CMD;
			}
		}
	}
	else if((pBuf[0] == 0xFE) && (pBuf[1] == 0X01))
	{
		HandleProtocolData(pBuf, u16Length, pProtocolInfo);
	}
		
	else if((pBuf[0] == 'O') && (pBuf[1] == 'T'))
	{		
		u8Ret = OTHER_CMD;
	}
	else
	{
		u8Ret = ERROR_CMD;
	}
	
	return u8Ret;
	
}

STATIC VOID HandleOtherCmdProc(U8 *pBuf, U16 u16Length)
{
	//TODO  ͸�����ݸ��������ģ��
	
}

STATIC PQueueInfo AppConfigure(USART_TypeDef* USARTx)
{
	PQueueInfo pQueueInfo;
	pQueueInfo = pUart3QueueInfo;
	return pQueueInfo;
}




VOID AppMain(VOID)
{
	U16 u16Length;
	U8 *szData;
	U8 u8Ret;
	PQueueInfo pQueueInfo;
	PProtocolInfo pProtocolInfo = malloc(sizeof(CProtocolInfo));
	if(pProtocolInfo == NULL)
	{
		printf("e");
	}
	szData = malloc(USER_UARTSIZE);
	if(szData == NULL)
	{
		printf("e");
	}

	
	pProtocolInfo->u8ProtocolType = DT_UNPROTOCOL;//Ĭ��DT��Э��
	
	pProtocolInfo->u8ErrorCode = APP_NO_ERROR;//Ĭ���޴���
	
	pQueueInfo = AppConfigure(APPCOMMUNICATE_PORT);

	/*�޸Ļ���------�豸��*/
	memcpy((char *)(szInquery1Cmd+3), szDeviceName, ARRAY_SIZEOF(szDeviceName));
	memcpy((char *)(szConfigureInfo1+3), szDeviceName, ARRAY_SIZEOF(szDeviceName));
	memcpy((char *)(szConfigureInfo2+3), szDeviceName, ARRAY_SIZEOF(szDeviceName));
	memcpy((char *)(szCmd1+3), szDeviceName, ARRAY_SIZEOF(szDeviceName));
	memcpy((char *)(szCmd2+3), szDeviceName, ARRAY_SIZEOF(szDeviceName));
	memcpy((char *)(szResponse+4), szDeviceName, ARRAY_SIZEOF(szDeviceName));
		if(GetQueueLength(pQueueInfo))
		{ 
			if(finishflag == 1)//һ����������
			{
				finishflag = 0;
				ReadUsartData(APPCOMMUNICATE_PORT, szData, &u16Length);//��ȡ����
				//UsartSend(APPCOMMUNICATE_PORT, szData, u16Length);
				u8Ret = HandleAppCmdProc(szData, u16Length, pProtocolInfo);//������Ӧ������
				switch(u8Ret)
				{
					case ERROR_CMD:
						HandleErrorCmdProc();
						break;
					
					case OTHER_CMD:
						HandleOtherCmdProc(szData, u16Length);
						break;
					default:
						break;
				}
				
				if(pProtocolInfo->u8ErrorCode != APP_NO_ERROR)
				{
					HandleErrorFlowProc(pProtocolInfo);
					pProtocolInfo->u8ErrorCode = APP_NO_ERROR;
				}
			}
		}
		
		free(pProtocolInfo);
		free(szData);
}