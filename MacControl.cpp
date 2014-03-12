/***************************************************************
Copyright(c) 2013  AITON. All rights reserved.
Author:     AITON
FileName:   MacControl.cpp
Date:       2013-1-1
Description:�źŻ�����ģ��������ļ�.
Version:    V1.0
History:	20130910 ��ӿ�����LCD״̬��ʾ��Ϣ
***************************************************************/
#include "ace/Date_Time.h"
#include "MacControl.h"
#include "Can.h"
#include "TscMsgQueue.h"
#include "ManaKernel.h"
#include "ComFunc.h"  		//ADD:201309081048
#include "PowerBoard.h"     //ADD:201309091530
#include "GbtMsgQueue.h"
/*
��������������ö��
*/
enum
{
	CTRLBOARD_HEAD_ENVSTS   	 = 0x02 , //��ȡ��ǰ��������������
	CTRLBOARD_HEAD_CFGSET1 		 = 0x03 , //���ÿ�����1
	CTRLBOARD_HEAD_CFGSET2 	 	 = 0x04 , //���ÿ�����2
	CTRLBOARD_HEAD_TIMESHOW 	 = 0x05 , //��������LCD����ʱ����ʾ��Ϣ
	CTRLBOARD_HEAD_MAINCTRL		 = 0x06 , //���ذ�ֱ�ӿ���ģ�����豸�����������ɢ����,���ر������������豸��Զ��IO���ƿ�
	CTRLBOARD_HEAD_CTRLSTATUS 	 = 0x07 , //��������LCD���͵�ǰ�źż�����ģʽ��״̬
	CTRLBOARD_HEAD_VOLTAGE 	 	 = 0x08 , //��������LCD���͵�ǰǿ��ѹֵ
	CTRLBOARD_HEAD_IP            = 0x09   //�������������źŻ�IP��ַ
};

/*
����������LCD�źŻ����Ʒ�ʽ��״̬��ʾ 201309281540
*/
enum
{
	CTRLBOARD_LCD_LAST  		 = 0x0  , //������LCD��ʾ���ֲ���
	CTRLBOARD_LCD_LAMPOFF  		 = 0x01 , //������LCD��ʾ���
	CTRLBOARD_LCD_LAMPFLASH  	 = 0x02 , //������LCD��ʾ����
	CTRLBOARD_LCD_LAMPRED	     = 0x03 , //������LCD��ʾ�����
	CTRLBOARD_LCD_CTRLPANEL	     = 0x04 , //������LCD��ʾ�źŻ������ֶ�����
	CTRLBOARD_LCD_SCHEDULE	     = 0x05 , //������LCD��ʾ�źŻ����ڶ�ʱ�ο���
	CTRLBOARD_LCD_VECHE	    	 = 0x06 , //������LCD��ʾ�źŻ����ڸ�Ӧ����
	CTRLBOARD_LCD_ADAPTIVE	     = 0x07 , //������LCD��ʾ�źŻ���������Ӧ����
	CTRLBOARD_LCD_UTCS	     	 = 0x08  //������LCD��ʾ�źŻ�����Э������
	
};

CMacControl::CMacControl()
{
	LcdCtrlMod 	 = CTRLBOARD_LCD_LAST;
	bSendCtrlOk =  false ;
	ACE_DEBUG((LM_DEBUG,"%s:%d Init MacControl object ok !\n",__FILE__,__LINE__));
}

CMacControl::~CMacControl()
{
	ACE_DEBUG((LM_DEBUG,"%s:%d Destruct MacControl object ok !\n",__FILE__,__LINE__));
}

CMacControl* CMacControl::CreateInstance()
{
	static CMacControl cMacControl;
	return &cMacControl;
}

/********************************************************
*���ذ��������ģ�鷢�ͻ�������
*���ֿ����ݣ��¶ȣ�ʪ�ȣ��ſ���״̬��
*�����豸״̬�����ر�����״̬��Զ��IO״̬��
*������״̬��ɢ����״̬���𶯴�����״̬
*********************************************************/
void CMacControl::GetEnvSts()
{
	SCanFrame sSendFrameTmp;
	ACE_OS::memset(&sSendFrameTmp , 0 , sizeof(SCanFrame));
	Can::BuildCanId(CAN_MSG_TYPE_100 , BOARD_ADDR_MAIN  , FRAME_MODE_P2P   , BOARD_ADDR_HARD_CONTROL  , &(sSendFrameTmp.ulCanId));
	sSendFrameTmp.pCanData[0] = ( DATA_HEAD_RESEND << 6 ) | CTRLBOARD_HEAD_ENVSTS;
	sSendFrameTmp.ucCanDataLen = 1;
	Can::CreateInstance()->Send(sSendFrameTmp);
	//Can::PrintInfo(__FILE__,__LINE__,0,sSendFrameTmp);	

}

/*******************************************************************
*���ذ巢�͸���Դģ����������
*�������Ⱥ�ɢ�ȵĿ����¶ȣ���ѹ�ߵ�ѹԤ����ѹֵ��ʪ��Ԥ����ֵ
********************************************************************/
void CMacControl::SetCfg1()
{
	SCanFrame sSendFrameTmp;
	ACE_OS::memset(&sSendFrameTmp , 0 , sizeof(SCanFrame));
	Can::BuildCanId(CAN_MSG_TYPE_100 , BOARD_ADDR_MAIN , FRAME_MODE_P2P , BOARD_ADDR_HARD_CONTROL , &(sSendFrameTmp.ulCanId));
	sSendFrameTmp.pCanData[0] = ( DATA_HEAD_CHECK << 6 ) | CTRLBOARD_HEAD_CFGSET1;
	sSendFrameTmp.pCanData[1] = (Byte)m_iSetAddHotTemp;
	sSendFrameTmp.pCanData[2] = (Byte)m_iSetReduHotTemp;
	sSendFrameTmp.pCanData[3] = m_ucSetHumWarn;
	sSendFrameTmp.pCanData[4] |= m_ucSetAddHotPlan;
	sSendFrameTmp.pCanData[4] |= m_ucSetReduHotPlan << 2;
	sSendFrameTmp.pCanData[4] |= m_ucSetCabinetWarn << 4;
	sSendFrameTmp.pCanData[5] = m_ucSetHumPlan;	
	sSendFrameTmp.ucCanDataLen = 6;
	Can::CreateInstance()->Send(sSendFrameTmp);

	//Can::PrintInfo(__FILE__,__LINE__,0,sSendFrameTmp);

}

/***********************���ذ巢�͸�����ģ����������***********************/
void CMacControl::SetCfg2()
{
	SCanFrame sSendFrameTmp;	
	ACE_OS::memset(&sSendFrameTmp , 0 , sizeof(SCanFrame));	

	Can::BuildCanId(CAN_MSG_TYPE_100 , BOARD_ADDR_MAIN  , FRAME_MODE_P2P   , BOARD_ADDR_HARD_CONTROL  , &(sSendFrameTmp.ulCanId));
	sSendFrameTmp.pCanData[0] = ( DATA_HEAD_CHECK << 6 ) | CTRLBOARD_HEAD_CFGSET2;
	sSendFrameTmp.pCanData[1] |= m_ucSetLightPlan;
	sSendFrameTmp.pCanData[1] |= m_ucSetDoorPlan << 2;
	sSendFrameTmp.pCanData[1] |= m_ucSetLedLight << 4;
	sSendFrameTmp.pCanData[1] |= m_ucSetLedDisplay << 6;
	ACE_Date_Time tvTime(GetCurTime());
	sSendFrameTmp.pCanData[2] = (Byte)(tvTime.year() & 0xff);
	sSendFrameTmp.pCanData[3] = (Byte)(tvTime.year() >> 8 & 0xff);
	sSendFrameTmp.pCanData[4] = (Byte)tvTime.month();
	sSendFrameTmp.pCanData[5] = (Byte)tvTime.day();
	sSendFrameTmp.pCanData[6] = (Byte)tvTime.hour();
	sSendFrameTmp.pCanData[7] = (Byte)tvTime.minute();
	sSendFrameTmp.pCanData[8] = (Byte)tvTime.second();
	
	sSendFrameTmp.ucCanDataLen = 8;

	Can::CreateInstance()->Send(sSendFrameTmp);

	//Can::PrintInfo(__FILE__,__LINE__,0,sSendFrameTmp);
}

/********************************************************************
*���ذ�ֱ�ӿ���ģ�����豸
*���������ɢ����,���ر������������豸��Զ��IO���ƿ�
*********************************************************************/
void CMacControl::MainBoardCtrl()
{
	SCanFrame sSendFrameTmp;
	ACE_OS::memset(&sSendFrameTmp , 0 , sizeof(SCanFrame));

	Can::BuildCanId(CAN_MSG_TYPE_100 , BOARD_ADDR_MAIN  , FRAME_MODE_P2P  , BOARD_ADDR_HARD_CONTROL , &(sSendFrameTmp.ulCanId));
	sSendFrameTmp.pCanData[0] = ( DATA_HEAD_CHECK << 6 ) | CTRLBOARD_HEAD_MAINCTRL;
	sSendFrameTmp.pCanData[1] |= m_ucSetMainAddHot;
	sSendFrameTmp.pCanData[1] |= m_ucSetMainReduHot << 2;
	sSendFrameTmp.pCanData[1] |= m_ucSetMainWarn << 4;
	sSendFrameTmp.pCanData[1] |= m_ucSetMainLight << 6;
	sSendFrameTmp.pCanData[2] |= m_ucSetFarIo1;
	sSendFrameTmp.pCanData[2] |= m_ucSetFarIo2 << 2;
	sSendFrameTmp.ucCanDataLen = 3;

	Can::CreateInstance()->Send(sSendFrameTmp);

	//Can::PrintInfo(__FILE__,__LINE__,0,sSendFrameTmp);
	/*
	fd_set rfds;
	struct timeval 	tv;
	int iCanHandle = Can::CreateInstance()->GetHandle();
	Ulong u1CanMsgType;
	Ulong u1ModuleAddr;
	Ulong u1FrameMode;
	Ulong u1RemodeAddr;
	Ulong ulProtocolVersion;
	
	FD_ZERO(&rfds);
	FD_SET(iCanHandle, &rfds);
	tv.tv_sec  = 0;
	tv.tv_usec = 20000;
	int iRetCnt = select(iCanHandle + 1, &rfds, NULL, NULL, &tv);
	if ( -1 == iRetCnt )  //select error
	{
		ACE_DEBUG((LM_DEBUG,"%s:%d select error\n",__FILE__,__LINE__));
		return;
	}
	else if ( 0 == iRetCnt )  //timeout
	{
		ACE_DEBUG((LM_DEBUG,"%s:%d timeout\n",__FILE__,__LINE__));
		return;
	}
	else
	{
		Can::CreateInstance()->Recv(sRecvFrameTmp);

		//Can::PrintInfo(__FILE__,__LINE__,0,sRecvFrameTmp);

		Can::CreateInstance()->ExtractCanId(u1CanMsgType
		            , u1ModuleAddr
				    , u1FrameMode
				    , u1RemodeAddr
					, ulProtocolVersion
				    , sRecvFrameTmp.ulCanId);
		if ( BOARD_ADDR_HARD_CONTROL != u1ModuleAddr )
		{
			ACE_DEBUG((LM_DEBUG,"%s:%d u1ModuleAddr:%d error\n",__FILE__,__LINE__,u1ModuleAddr));
			return;
		}
		if ( BOARD_ADDR_MAIN != u1RemodeAddr )
		{
			ACE_DEBUG((LM_DEBUG,"%s:%d u1RemodeAddr:%d error\n",__FILE__,__LINE__,u1RemodeAddr));
			return;
		}
		if ( CTRLBOARD_HEAD_MAINCTRL != (sRecvFrameTmp.pCanData[0] & 0x3F) )
		{
			ACE_DEBUG((LM_DEBUG,"%s:%d Not CTRLBOARD_HEAD_MAINCTRL\n",__FILE__,__LINE__));
			return;
		}

		m_ucGetMainAddHot  = sRecvFrameTmp.pCanData[1] & 0x3;
		m_ucGetMainReduHot = (sRecvFrameTmp.pCanData[1] >> 2) & 0x3;
		m_ucGetMainWarn    = (sRecvFrameTmp.pCanData[1] >> 4) & 0x3;
		m_ucGetMainLight   = (sRecvFrameTmp.pCanData[1] >> 6) & 0x3;
		m_ucGetFarIo1 = sRecvFrameTmp.pCanData[2] & 0x3;
		m_ucGetFarIo2 = (sRecvFrameTmp.pCanData[2] >> 2) & 0x3;

		if ( m_ucGetMainAddHot != m_ucSetMainAddHot 
		 || m_ucGetMainReduHot != m_ucSetMainReduHot 
		 || m_ucGetMainWarn != m_ucSetMainWarn 
		 || m_ucGetMainLight != m_ucSetMainLight 
		 || m_ucGetFarIo1 != m_ucSetFarIo1 
		 || m_ucGetFarIo2 != m_ucSetFarIo2 
		)
		{
			ACE_DEBUG((LM_DEBUG,"%s:%d set and recv not same\n",__FILE__,__LINE__));
			return;
		}
	}
	*/
}

/****************************************************************
*�Դӿ��ư巵�������CAN���ݽ��д���
*
*****************************************************************/
void CMacControl::RecvMacCan(SCanFrame sRecvCanTmp)
{
	Byte ucType = sRecvCanTmp.pCanData[0] & 0x3F;
	switch(ucType)
	{
		case CTRLBOARD_HEAD_ENVSTS :
			m_ucDoorFront = (sRecvCanTmp.pCanData[1] >> 4) & 0x1;
			m_ucDoorBack  = (sRecvCanTmp.pCanData[1] >> 5) & 0x1;
			m_ucLightDev  = (sRecvCanTmp.pCanData[1] >> 6) & 0x1;
			m_ucWarnDev   = (sRecvCanTmp.pCanData[1] >> 7) & 0x1;

			m_ucTemp = sRecvCanTmp.pCanData[2];
			m_ucHum  = sRecvCanTmp.pCanData[3];

			m_ucFarOut1  = sRecvCanTmp.pCanData[4] & 0x1;
			m_ucFarOut2  = (sRecvCanTmp.pCanData[4] >> 1) & 0x1;
			m_ucFarIn1   = (sRecvCanTmp.pCanData[4] >> 2) & 0x1;
			m_ucFarIn2   = (sRecvCanTmp.pCanData[4] >> 3) & 0x1;
			m_ucAddHot   = (sRecvCanTmp.pCanData[4] >> 4) & 0x1;
			m_ucReduHot  = (sRecvCanTmp.pCanData[4] >> 5) & 0x1;
			m_ucCabinet  = (sRecvCanTmp.pCanData[4] >> 6) & 0x1;
			ACE_DEBUG((LM_DEBUG,"%s:%d DoorFront: %d  m_ucDoor: %d Back Temp:%d�� Hum:%d\n"	,__FILE__,__LINE__,m_ucDoorFront,m_ucDoorBack,m_ucTemp,m_ucHum)); //MOD:0604 1415
			
			break ;
		case CTRLBOARD_HEAD_CFGSET1 :
			break ;
		case CTRLBOARD_HEAD_CFGSET2 :
			break ;
		case CTRLBOARD_HEAD_MAINCTRL :
			break ;
		case CTRLBOARD_HEAD_CTRLSTATUS:
			bSendCtrlOk = true ;
			//ACE_DEBUG((LM_DEBUG,"%s:%d Get MacCtrol  SendCtrlOk\n"	,__FILE__,__LINE__));
			break ;
		default :
			break ;

	}

}

/**************************************************************
Function:        CMacControl::SetCtrlStaus
Description:     ���ÿ������źŻ�����ģʽLCD��ʾ����			
Input:          ucCtrlStas   ����ģʽ״ֵ̬           
Output:         ��
Return:         ������LCD����ģʽ״ֵ̬
***************************************************************/
Byte CMacControl::GetCtrlStaus()
{
	Byte uiCtrl ,uiWorkStaus ,uiLcdCtrl;
	CManaKernel * pManaKernel = CManaKernel::CreateInstance();
	uiWorkStaus = pManaKernel->m_pRunData->uiWorkStatus ;
	uiCtrl =pManaKernel->m_pRunData->uiCtrl;
	switch(uiWorkStaus)
	{
	case SIGNALOFF :
		uiLcdCtrl = CTRLBOARD_LCD_LAMPOFF ;
		break ;
	case ALLRED :
		uiLcdCtrl = CTRLBOARD_LCD_LAMPRED ;
		break ;
	case FLASH :
		uiLcdCtrl = CTRLBOARD_LCD_LAMPFLASH ;
		break ;
	case STANDARD :
			switch(uiCtrl)
			{
			case CTRL_SCHEDULE :
				uiLcdCtrl = CTRLBOARD_LCD_SCHEDULE ;
				break ;
			case CTRL_VEHACTUATED :
				uiLcdCtrl = CTRLBOARD_LCD_VECHE ;
				break ;
			case CTRL_PANEL :
			case CTRL_MANUAL:
				uiLcdCtrl = CTRLBOARD_LCD_CTRLPANEL ;
				break ;
			case CTRL_ACTIVATE :
				uiLcdCtrl = CTRLBOARD_LCD_ADAPTIVE ;
				break ;
			case CTRL_UTCS :
			case CTRL_WIRELESS:
			case CTRL_LINE:
				uiLcdCtrl = CTRLBOARD_LCD_UTCS ;
				break ;
			default:
				break ;

			}
		break ;
	
	default :
		break;	
	}
	return uiLcdCtrl ;

}

/**************************************************************
Function:        CMacControl::SndLcdShow
Description:     ������ʾ��Ϣ��������LCD��ʾ��1s����һ��			
Input:          ��           
Output:         ��
Return:         ��
***************************************************************/
void CMacControl::SndLcdShow()
{
	static Byte ucTimeCnt = 0 ;
	if(ucTimeCnt++ >=30)   //30s��ʾ������ʾʱ��͵�ѹֵ
	{
		ucTimeCnt = 0 ;
		ACE_Date_Time tvTime(ACE_OS::gettimeofday()); 
		SCanFrame sSendFrameTmp;
		
		/****���͵�ǰʱ��****/
		ACE_OS::memset(&sSendFrameTmp , 0 , sizeof(SCanFrame));
		Can::BuildCanId(CAN_MSG_TYPE_100 , BOARD_ADDR_MAIN  , FRAME_MODE_P2P  , BOARD_ADDR_HARD_CONTROL , &(sSendFrameTmp.ulCanId));
		sSendFrameTmp.pCanData[0] = ( DATA_HEAD_CHECK << 6 ) | CTRLBOARD_HEAD_TIMESHOW;
		sSendFrameTmp.pCanData[1] = (tvTime.year()>>8)&0xff;
		sSendFrameTmp.pCanData[2] = tvTime.year()&0xff;
		sSendFrameTmp.pCanData[3] = tvTime.month();
		sSendFrameTmp.pCanData[4] = tvTime.day();
		sSendFrameTmp.pCanData[5] = tvTime.hour();
		sSendFrameTmp.pCanData[6] = tvTime.minute();
		sSendFrameTmp.ucCanDataLen = 7;			
		Can::CreateInstance()->Send(sSendFrameTmp);
		
		/****����ǿ��ѹֵ****/
		ACE_OS::memset(&sSendFrameTmp , 0 , sizeof(SCanFrame));
		Can::BuildCanId(CAN_MSG_TYPE_100 , BOARD_ADDR_MAIN  , FRAME_MODE_P2P  , BOARD_ADDR_HARD_CONTROL , &(sSendFrameTmp.ulCanId));
		sSendFrameTmp.pCanData[0] = ( DATA_HEAD_CHECK << 6 ) | CTRLBOARD_HEAD_VOLTAGE;
		sSendFrameTmp.pCanData[1] = CPowerBoard::CreateInstance()->GetStongVoltage()-150 ;
		sSendFrameTmp.ucCanDataLen = 2;			
		Can::CreateInstance()->Send(sSendFrameTmp);	
		
		/****�źŻ�IP��ַ****/
		ACE_OS::memset(&sSendFrameTmp , 0 , sizeof(SCanFrame));
		Byte pHwEther[6] = {0};
		Byte pIp[4]      = {0};
		Byte pMask[4]    = {0};
		Byte pGateway[4] = {0};
		CGbtMsgQueue::CreateInstance()->GetNetPara(pHwEther ,pIp , pMask , pGateway);
		
		Can::BuildCanId(CAN_MSG_TYPE_100 , BOARD_ADDR_MAIN  , FRAME_MODE_P2P  , BOARD_ADDR_HARD_CONTROL , &(sSendFrameTmp.ulCanId));
		sSendFrameTmp.pCanData[0] = ( DATA_HEAD_CHECK << 6 ) | CTRLBOARD_HEAD_IP;
		sSendFrameTmp.pCanData[1] = pIp[0];
		sSendFrameTmp.pCanData[2] = pIp[1];
		sSendFrameTmp.pCanData[3] = pIp[2];
		sSendFrameTmp.pCanData[4] = pIp[3];
		sSendFrameTmp.ucCanDataLen = 5;			
		Can::CreateInstance()->Send(sSendFrameTmp);		
	}
	else 
	{		
		static Byte uiLcdCtrl = 0 ;
		static Byte uiPatternNo = 0 ;
		LcdCtrlMod = GetCtrlStaus();
		
		if(uiLcdCtrl !=  LcdCtrlMod || uiPatternNo != CManaKernel::CreateInstance()->m_pRunData->ucTimePatternId || bSendCtrlOk ==false)
		{
			if(bSendCtrlOk != false)
				bSendCtrlOk = false ;
			uiLcdCtrl =  LcdCtrlMod ;
			uiPatternNo = CManaKernel::CreateInstance()->m_pRunData->ucTimePatternId ;
			/****���͵�ǰ����״̬��Ϣ****/
			SCanFrame sSendFrameTmp;		
			ACE_OS::memset(&sSendFrameTmp , 0 , sizeof(SCanFrame));
			Can::BuildCanId(CAN_MSG_TYPE_100 , BOARD_ADDR_MAIN  , FRAME_MODE_P2P  , BOARD_ADDR_HARD_CONTROL , &(sSendFrameTmp.ulCanId));
			sSendFrameTmp.pCanData[0] = ( DATA_HEAD_CHECK << 6 ) | CTRLBOARD_HEAD_CTRLSTATUS;
			sSendFrameTmp.pCanData[1] = LcdCtrlMod;
			sSendFrameTmp.pCanData[2] = CManaKernel::CreateInstance()->m_pRunData->ucTimePatternId;
			sSendFrameTmp.pCanData[3] =  CManaKernel::CreateInstance()->bDegrade?1:0 ;
			sSendFrameTmp.ucCanDataLen = 4;				
			Can::CreateInstance()->Send(sSendFrameTmp);
		}
	}

}
