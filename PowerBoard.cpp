#include "Can.h"
#include "PowerBoard.h"

#ifndef WINDOWS
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#endif

/*
电源模块功能类型枚举
*/
enum
{
	POWER_HEAD_ENV      = 0x02 ,  //电压数据，电压状态
	POWER_HEAD_CFG_GET  = 0x03 ,  //请求电源模块配置数据，包括电压高低压预警电压值以及强弱电控制预案
    POWER_HEAD_CFG_SET  = 0x04 ,  //发送电源模块配置数据
	POWER_HEAD_HEARBEAT = 0x05 , //心跳包
};

/*
电源模块预案功能类型枚举
*/
enum
{
	PLAN_KEEP  = 0 ,  //预案方式不变
	PLAN_ON    = 1 ,  //保护开启
	PLAN_OFF   = 2 ,  //保护关闭
	PLAN_OTHER = 3    //备用
};

/**************************************************************
Function:       CPowerBoard::CPowerBoard
Description:    CPowerBoard类构造函数		
Input:          无              
Output:         无
Return:         无
***************************************************************/
CPowerBoard::CPowerBoard()
{
	m_iStongVoltage     = 220;
	m_iWeakVoltage = 5;
	m_iBusVoltage  = 0;
	ACE_DEBUG((LM_DEBUG,"%s:%d Init PowerBoard object ok !\n",__FILE__,__LINE__));
}

Byte CPowerBoard::iHeartBeat = 0;

/**************************************************************
Function:       CPowerBoard::~CPowerBoard
Description:    CPowerBoard类析构函数		
Input:          无              
Output:         无
Return:         无
***************************************************************/
CPowerBoard::~CPowerBoard()
{	
	ACE_DEBUG((LM_DEBUG,"%s:%d Destruct PowerBoard object ok !\n",__FILE__,__LINE__));
}


/**************************************************************
Function:       CPowerBoard::CreateInstance
Description:    创建电源板类静态对象		
Input:          无              
Output:         无
Return:         CPowerBoard静态对象指针
***************************************************************/
CPowerBoard* CPowerBoard::CreateInstance()
{
	static CPowerBoard cPowerBoard; 
	return &cPowerBoard;
}


/**************************************************************
Function:       CPowerBoard::GetPowerBoardCfg
Description:    主控板请求电源模块发送电压数据，电压状态		
Input:          无              
Output:         无
Return:         0
***************************************************************/
void CPowerBoard::CheckVoltage()
{
	SCanFrame sSendFrameTmp;
	ACE_OS::memset(&sSendFrameTmp , 0 , sizeof(SCanFrame));
	Can::BuildCanId(CAN_MSG_TYPE_100 , BOARD_ADDR_MAIN , FRAME_MODE_P2P  , BOARD_ADDR_POWER , &(sSendFrameTmp.ulCanId));
	sSendFrameTmp.pCanData[0] = ( DATA_HEAD_RESEND << 6 ) | POWER_HEAD_ENV;
	sSendFrameTmp.ucCanDataLen = 1;

	Can::CreateInstance()->Send(sSendFrameTmp);

	//Can::PrintInfo(__FILE__,__LINE__,0,sSendFrameTmp);

}


/**************************************************************
Function:       CPowerBoard::GetPowerBoardCfg
Description:    主控板请求电源模块发送配置数据，包括电压高低压预
				警电压值以及强弱点控制预案			
Input:          无              
Output:         无
Return:         0
***************************************************************/
void CPowerBoard::GetPowerBoardCfg()
{
	SCanFrame sSendFrameTmp;
	SCanFrame sRecvFrameTmp;

	ACE_OS::memset(&sSendFrameTmp , 0 , sizeof(SCanFrame));
	ACE_OS::memset(&sRecvFrameTmp , 0 , sizeof(SCanFrame));

	Can::BuildCanId(CAN_MSG_TYPE_100 , BOARD_ADDR_MAIN
				  , FRAME_MODE_P2P   , BOARD_ADDR_POWER
				  , &(sSendFrameTmp.ulCanId));
	sSendFrameTmp.pCanData[0] = ( DATA_HEAD_CHECK << 6 ) | POWER_HEAD_CFG_GET;
	sSendFrameTmp.ucCanDataLen = 1;

	Can::CreateInstance()->Send(sSendFrameTmp);

	//Can::PrintInfo(__FILE__,__LINE__,0,sSendFrameTmp);


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
		if ( BOARD_ADDR_POWER != u1ModuleAddr )
		{
			ACE_DEBUG((LM_DEBUG,"%s:%d u1ModuleAddr:%d error\n",__FILE__,__LINE__,u1ModuleAddr));
			return;
		}
		if ( BOARD_ADDR_MAIN != u1RemodeAddr )
		{
			ACE_DEBUG((LM_DEBUG,"%s:%d u1RemodeAddr:%d error\n",__FILE__,__LINE__,u1RemodeAddr));
			return;
		}
		if ( POWER_HEAD_CFG_GET != (sRecvFrameTmp.pCanData[0] & 0x3F) )
		{
			ACE_DEBUG((LM_DEBUG,"%s:%d Not POWER_HEAD_CFG_GET\n",__FILE__,__LINE__));
			return;
		}

		m_iGetWarnHighVol = sRecvFrameTmp.pCanData[1] + 150;

		m_iGetWarnLowVol  = sRecvFrameTmp.pCanData[2] + 150;

		m_ucGetStongHighVolPlan = sRecvFrameTmp.pCanData[3] & 0x3;
		m_ucGetStongLowVolPlan  = (sRecvFrameTmp.pCanData[3] >> 2 )& 0x3;
		m_ucGetWeakHighVolPlan  = (sRecvFrameTmp.pCanData[3] >> 4 )& 0x3; 
		m_ucGetWeakLowVolPlan   = (sRecvFrameTmp.pCanData[3] >> 6 )& 0x3; 
	}
}


/**************************************************************
Function:       CPowerBoard::SetPowerBoardCfg
Description:    主控板设置电源板配置数据				
Input:          无              
Output:         无
Return:         0
***************************************************************/
void CPowerBoard::SetPowerBoardCfg()
{
	SCanFrame sSendFrameTmp;
	SCanFrame sRecvFrameTmp;

	ACE_OS::memset(&sSendFrameTmp , 0 , sizeof(SCanFrame));
	ACE_OS::memset(&sRecvFrameTmp , 0 , sizeof(SCanFrame));

	Can::BuildCanId(CAN_MSG_TYPE_100 , BOARD_ADDR_MAIN
				  , FRAME_MODE_P2P   , BOARD_ADDR_POWER
				  , &(sSendFrameTmp.ulCanId));
	sSendFrameTmp.pCanData[0] = ( DATA_HEAD_CHECK << 6 ) | POWER_HEAD_CFG_SET;
	sRecvFrameTmp.pCanData[1] = m_iSetWarnHighVol - 150;
	sRecvFrameTmp.pCanData[2] = m_iSetWarnLowVol - 150;
	sRecvFrameTmp.pCanData[3] |= m_ucSetStongHighVolPlan;
	sRecvFrameTmp.pCanData[3] |= m_ucSetStongLowVolPlan << 2;
	sRecvFrameTmp.pCanData[3] |= m_ucSetWeakHighVolPlan << 4;
	sRecvFrameTmp.pCanData[3] |= m_ucSetWeakLowVolPlan << 6;
	sSendFrameTmp.ucCanDataLen = 4;

	Can::CreateInstance()->Send(sSendFrameTmp);

	//Can::PrintInfo(__FILE__,__LINE__,0,sSendFrameTmp);

	//œÓÊÜµçÔŽ°åµÄ»ØžŽ  µÃµœÏàÓŠµÄ»·Ÿ³ÊýŸÝ
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
		if ( BOARD_ADDR_POWER != u1ModuleAddr )
		{
			ACE_DEBUG((LM_DEBUG,"%s:%d u1ModuleAddr:%d error\n",__FILE__,__LINE__,u1ModuleAddr));
			return;
		}
		if ( BOARD_ADDR_MAIN != u1RemodeAddr )
		{
			ACE_DEBUG((LM_DEBUG,"%s:%d u1RemodeAddr:%d error\n",__FILE__,__LINE__,u1RemodeAddr));
			return;
		}
		if ( POWER_HEAD_CFG_SET != (sRecvFrameTmp.pCanData[0] & 0x3F) )
		{
			ACE_DEBUG((LM_DEBUG,"%s:%d Not POWER_HEAD_CFG_SET\n",__FILE__,__LINE__));
			return;
		}

		m_iGetWarnHighVol = sRecvFrameTmp.pCanData[1] + 150;

		m_iGetWarnLowVol  = sRecvFrameTmp.pCanData[2] + 150;

		m_ucGetStongHighVolPlan = sRecvFrameTmp.pCanData[3] & 0x3;
		m_ucGetStongLowVolPlan  = (sRecvFrameTmp.pCanData[3] >> 2 )& 0x3;
		m_ucGetWeakHighVolPlan  = (sRecvFrameTmp.pCanData[3] >> 4 )& 0x3; 
		m_ucGetWeakLowVolPlan   = (sRecvFrameTmp.pCanData[3] >> 6 )& 0x3; 
	}
}


/**************************************************************
Function:       CPowerBoard::HeartBeat
Description:    电源板发送广播心跳Can数据包，用于保持主板和部件的
				连接状态
Input:          无              
Output:         无
Return:         0
***************************************************************/
void CPowerBoard::HeartBeat()
{
	SCanFrame sSendFrameTmp;
	ACE_OS::memset(&sSendFrameTmp , 0 , sizeof(SCanFrame));
	
	Can::BuildCanId(CAN_MSG_TYPE_100 , BOARD_ADDR_MAIN , FRAME_MODE_HEART_BEAT  , BOARD_ADDR_POWER , &(sSendFrameTmp.ulCanId));
	sSendFrameTmp.pCanData[0] = ( DATA_HEAD_NOREPLY << 6 ) | POWER_HEAD_HEARBEAT;
	sSendFrameTmp.ucCanDataLen = 1;
	
	Can::CreateInstance()->Send(sSendFrameTmp);
	
}

/**************************************************************
Function:        CPowerBoard::RecvPowerCan
Description:    主控板接收电源板返回Can数据包，解析并处理。				
Input:          ucBoardAddr  电源板地址
				sRecvCanTmp  Can数据包              
Output:         无
Return:         0
***************************************************************/
void CPowerBoard::RecvPowerCan(Byte ucBoardAddr,SCanFrame sRecvCanTmp)
{
		Byte ucType = sRecvCanTmp.pCanData[0] & 0x3F ;
		ACE_Guard<ACE_Thread_Mutex>  guard(m_mutexVoltage); 
		switch(ucType)
		{
		case POWER_HEAD_ENV :	
			m_iStongVoltage = sRecvCanTmp.pCanData[1] + 150;  //强电电压		
			m_iWeakVoltage  = 0;  //弱电电压
			m_iWeakVoltage  = (sRecvCanTmp.pCanData[2] & 0xf)  << 8;
			m_iWeakVoltage  = m_iWeakVoltage | sRecvCanTmp.pCanData[3];
				
			m_iWeakVoltage  = m_iWeakVoltage / 500;		
			m_iBusVoltage   = 0;  //总线电压
			m_iBusVoltage   = (sRecvCanTmp.pCanData[4] & 0xf)  << 8;
			m_iBusVoltage  = m_iBusVoltage | sRecvCanTmp.pCanData[5];
			m_iBusVoltage  = m_iBusVoltage / 500;
		
			//ACE_DEBUG((LM_DEBUG,"%s:%d StongVol:%d WeakVol:%d BusVol:%d			\n",__FILE__,__LINE__,m_iStongVoltage,m_iWeakVoltage,m_iBusVoltage)); //MOD:0604 1738
			break ;
		case POWER_HEAD_HEARBEAT :
			 iHeartBeat = 0;
			 //ACE_DEBUG((LM_DEBUG,"%s:%d Get from PowerBoard,iHeartBeat = %d		\n",__FILE__,__LINE__,iHeartBeat)); //MOD:0604 1738
			 
			
			break ;			
			default :				
				break ;
		}

}


/**************************************************************
Function:        CPowerBoard::GetStongVoltage
Description:    获取强电电压属性值，用于控制器LCD电压显示。				
Input:          无
Output:         无
Return:         强电电压
***************************************************************/
int  CPowerBoard::GetStongVoltage()
{
	return m_iStongVoltage ;
}

