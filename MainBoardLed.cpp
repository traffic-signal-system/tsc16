/***************************************************************
Copyright(c) 2013  AITON. All rights reserved.
Author:     AITON
FileName:   MainBoardLed.cpp
Date:       2013-1-1
Description:信号机主板指示灯和LED显示板灯文件
Version:    V1.0
History:
***************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include "MainBoardLed.h"
#include "ManaKernel.h"
#include "Can.h"
#include "FlashMac.h"
#include "Gps.h"

#define MAIN_BOARD_LED "/dev/leds" 
CMainBoardLed::CMainBoardLed()
{
	m_iLedFd = -1;
	STscConfig* pSTscConfig = NULL ;
	pSTscConfig =  CManaKernel::CreateInstance()->m_pTscConfig ;
	
	for(int iNumFun = 0 ; iNumFun < 12 ;iNumFun++)
		LedBoardStaus[iNumFun] = 0x1 ;
	if ( 0 !=pSTscConfig->sSpecFun[FUN_GPS].ucValue )
		LedBoardStaus[8] = 0x2;
	if ( 0 != pSTscConfig->sSpecFun[FUN_3G].ucValue )
		LedBoardStaus[4] = 0x2;
	if ( 0 != pSTscConfig->sSpecFun[FUN_MSG_ALARM].ucValue )
		LedBoardStaus[5] = 0x2;
	if ( 0 != pSTscConfig->sSpecFun[FUN_CAM].ucValue )
		LedBoardStaus[6] = 0x2;
	if ( 0 != pSTscConfig->sSpecFun[FUN_WLAN].ucValue )
		LedBoardStaus[7] = 0x2;
	LedBoardStaus[9] = 0x2;//黄闪器
	
	
	OpenDev();
	ACE_DEBUG((LM_DEBUG,"%s:%d Init MainBoardLed object ok !\n",__FILE__,__LINE__));
}

CMainBoardLed::~CMainBoardLed()
{
	CloseDev();
	ACE_DEBUG((LM_DEBUG,"%s:%d Destruct MainBoardLed object ok !\n",__FILE__,__LINE__));
}

CMainBoardLed* CMainBoardLed::CreateInstance()
{
	static CMainBoardLed cMainBoardLed;
	return &cMainBoardLed;
}

/****************Žò¿ªµÆ¿ØÉè±ž**********************************/
void CMainBoardLed::OpenDev()
{
#ifndef WINDOWS
	m_iLedFd = ::open(MAIN_BOARD_LED, O_RDONLY); 
	 
#endif

	if ( m_iLedFd < 0 )
	{
		ACE_DEBUG((LM_DEBUG,"open device (%s) failed.\n", MAIN_BOARD_LED));
	}
}



/***************************************************************
			关闭LED设备文件
***************************************************************/
void CMainBoardLed::CloseDev()
{
	if( m_iLedFd >= 0 )
	{
#ifndef WINDOWS
		close(m_iLedFd);
#endif
		m_iLedFd = -1;
	}
}

/*****************设置MODE指示灯显示状态, 使用NLED3  NLED4 IO口 
		修正:	  绿：正常     0 1
                  黄：降级黄闪 1 1
                  红：无法工作 1 0
***************************************************************/
void CMainBoardLed::DoModeLed(bool bLed3Value,bool bLed4Value)
{
#ifndef WINDOWS
	ioctl(m_iLedFd, bLed3Value, 2);
	ioctl(m_iLedFd, bLed4Value, 3);
#endif
}

/************设置TSC/PSC指示灯显示状态, 使用GPK0 IO口***********/
void CMainBoardLed::DoTscPscLed()
{
	
	static bool bValue = true; 
	#ifndef WINDOWS
	ioctl(m_iLedFd, bValue, 4);
	#endif
	bValue = !bValue;
}

/************设置Auto指示灯显示状态, 使用GPK2 IO口**************/
void CMainBoardLed::DoAutoLed(bool bValue)
{
	//Žý²¹  ÊÖ¶¯ÃðµÆ
	//static bool bValue = true; 
	#ifndef WINDOWS
	ioctl(m_iLedFd, bValue, 5);
	#endif
	//bValue = !bValue;
}

/*****设置Run指示灯显示状态, 1s内量灭各一次,使用NLED1 IO口******/
void CMainBoardLed::DoRunLed()
{
	static bool bValue = true;
	#ifndef WINDOWS
	ioctl(m_iLedFd, bValue, 0);
	#endif
	bValue = !bValue;
}


/*****设置Can指示灯显示状态, CAN总线收发时亮,使用NLED2 IO口******/

void CMainBoardLed::DoCanLed()
{
	static bool bValue = true; 
	#ifndef WINDOWS
	ioctl(m_iLedFd, bValue, 1);
	#endif
	bValue = !bValue;
}


bool CMainBoardLed::IsEthLinkUp()
{
	Byte rusult = 0 ;
	FILE *fpsys = NULL ;             //定义管道符句柄
	fpsys = popen("ifconfig eth0|grep -c RUNNING", "r");  //打开管道,执行脚本或命令
	fread(&rusult, 1 , 1,fpsys) ;            //读取管道符,获得执行结果
	pclose(fpsys);                     //关闭管道符句柄
	fpsys = NULL;
	if(rusult ==(Byte)'1')
	{
		//printf("\nEth0 link up!\n");
		//LedBoardStaus[3] = 0x2 ;
		return true ;
	}
	else
	
	{
		//printf("\nEth0 link down!\n");
		//LedBoardStaus[3] = 0x1 ;
		return false ;
	}
	

}

void CMainBoardLed::DoLedBoardShow()
{
	if(IsEthLinkUp())
		LedBoardStaus[3] = 0x2 ;
	else
		LedBoardStaus[3] = 0x1 ;
	if(CFlashMac::CreateInstance()->GetHardwareFlash())
		LedBoardStaus[9] = 0x3 ;
	if(CGps::CreateInstance()->m_bGpsTime)
		LedBoardStaus[8] = 0x3 ;
	SetLedBoardShow();
}



void CMainBoardLed::SetLedBoardShow()
{
		
		SCanFrame sSendFrameTmp;
		ACE_OS::memset(&sSendFrameTmp , 0 , sizeof(SCanFrame));	
		Can::BuildCanId(CAN_MSG_TYPE_100 , BOARD_ADDR_MAIN  , FRAME_MODE_P2P , BOARD_ADDR_LED  , &(sSendFrameTmp.ulCanId));
		//Can::BuildCanId(CAN_MSG_TYPE_100 , BOARD_ADDR_MAIN  , FRAME_MODE_P2P , BOARD_ADDR_LED  , &(sSendFrameTmp.ulCanId));
		//ACE_DEBUG((LM_DEBUG,"%s:%d LEDCANID:%x \n",__FILE__,__LINE__,sSendFrameTmp.ulCanId));
		sSendFrameTmp.ulCanId = 0x91032ff0 ;
		sSendFrameTmp.pCanData[0] = ( DATA_HEAD_RESEND<< 6) | LED_BOARD_SHOW;
				
		sSendFrameTmp.pCanData[1] = LedBoardStaus[0]&0x3 ;
		sSendFrameTmp.pCanData[1] |= (LedBoardStaus[1]&0x3)<<2 ;
		sSendFrameTmp.pCanData[1] |= (LedBoardStaus[2]&0x3)<<4 ;
		sSendFrameTmp.pCanData[1] |= (LedBoardStaus[3]&0x3)<<6 ;

		sSendFrameTmp.pCanData[2] = LedBoardStaus[4]&0x3 ;
		sSendFrameTmp.pCanData[2] |= (LedBoardStaus[5]&0x3)<<2 ;
		sSendFrameTmp.pCanData[2] |= (LedBoardStaus[6]&0x3)<<4 ;
		sSendFrameTmp.pCanData[2] |= (LedBoardStaus[7]&0x3)<<6 ;

		
		sSendFrameTmp.pCanData[3] = LedBoardStaus[8]&0x3 ;
		sSendFrameTmp.pCanData[3] |= (LedBoardStaus[9]&0x3)<<2 ;		

		sSendFrameTmp.ucCanDataLen = 4;	

		Can::CreateInstance()->Send(sSendFrameTmp);
		//ACE_DEBUG((LM_DEBUG,"%s:%d pCanData[1]= %02x , pCanData[2]= %02x ,pCanData[3]= %02x \n ",__FILE__,__LINE__,sSendFrameTmp.pCanData[1],sSendFrameTmp.pCanData[2],sSendFrameTmp.pCanData[3]));

}


