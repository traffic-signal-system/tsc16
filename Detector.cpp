
/***************************************************************
Copyright(c) 2013  AITON. All rights reserved.
Author:     AITON
FileName:   Detetctor.cpp
Date:       2013-1-1
Description:信号机检测器处理类文件。
Version:    V1.0
History:
***************************************************************/
#include "ace/Synch.h"
#include <ace/Log_Msg.h>
#include "Define.h"
#include "Detector.h"
#include "ManaKernel.h"
#include "IoOperate.h"
#include "TscMsgQueue.h"
#include "Configure.h"
#include "Can.h"
#include "ComFunc.h"
#include "DbInstance.h"

#ifndef WINDOWS
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>

#endif

/*
	检测器线圈工作状态
*/
enum
{
	DET_NORMAL = 0,  //正常
	DET_CARVE     ,  //开路
	DET_SHORT     ,  //短路
	DET_STOP         //停振
};

/**************************************************************
Function:        CDetector::CDetector
Description:     检测器CDetector类构造函数，初始化类			
Input:          无           
Output:         无
Return:         无
***************************************************************/
CDetector::CDetector()
{
#ifdef CHECK_MEMERY
	TestMem(__FILE__,__LINE__);//MOD:201310081445
#endif

	//m_iDevFd   = Serial1::CreateInstance()->GetSerial1Fd();
	m_pTscCfg  = CManaKernel::CreateInstance()->m_pTscConfig; 

	m_iTotalDistance = 5 * 60;    //5min
	m_iChkType = 0 ;
	ACE_OS::memset(m_iBoardErr  , 0 , 4);
		
	ACE_OS::memset(m_iDetStatus       , 0 , sizeof(int)  * MAX_DETECTOR);
	ACE_OS::memset(m_ucDetError       , 0 , sizeof(Byte) * MAX_DETECTOR);
	ACE_OS::memset(m_ucLastDetError   , 0 , sizeof(Byte) * MAX_DETECTOR);
	ACE_OS::memset(m_ucDetErrTime     , 0 , sizeof(Byte) * MAX_DETECTOR);
	ACE_OS::memset(m_iDetTimeLen      , 0 , sizeof(int)  * MAX_DETECTOR);
	ACE_OS::memset(m_iDetOccupy       , 0 , sizeof(int)  * MAX_DETECTOR);
	ACE_OS::memset(m_ucTotalStat      , 0 , sizeof(Byte) * MAX_DETECTOR);
	ACE_OS::memset(m_iAdapDetTimeLen  , 0 , sizeof(int)  * MAX_DETECTOR);
	ACE_OS::memset(m_iAdapTotalStat   , 0 , sizeof(int)  * MAX_DETECTOR);
	ACE_OS::memset(m_iLastDetSts      , 0 , sizeof(int)  * MAX_DETECTOR);
	ACE_OS::memset(m_ucRoadSpeed      , 0 , sizeof(Byte) * MAX_DET_BOARD * 8);
	ACE_OS::memset(m_ucSetRoadDis     , 0 , sizeof(Byte) * MAX_DET_BOARD * 8);
	ACE_OS::memset(m_ucGetRoadDis     , 0 , sizeof(Byte) * MAX_DET_BOARD * 8);
	ACE_OS::memset(m_ucSetDetDelicacy , 0 , sizeof(Byte) * MAX_DETECTOR);
	ACE_OS::memset(m_ucGetDetDelicacy , 0 , sizeof(Byte) * MAX_DETECTOR);
	ACE_OS::memset(m_sSetLookLink     , 0 ,  MAX_DET_BOARD * 8);
	ACE_OS::memset(m_sGetLookLink     , 0 ,  MAX_DET_BOARD * 8);
	ACE_OS::memset(m_ucDetSts         , 0 , sizeof(Byte) * MAX_DET_BOARD*MAX_DETECTOR_PER_BOARD); //ADD: 20130710 10 12
	ACE_OS::memset(m_ucNoCnt          , 0 , MAX_DET_BOARD);
	ACE_OS::memset(m_ucErrAddrCnt     , 0 , MAX_DET_BOARD);
	ACE_OS::memset(m_ucErrCheckCnt    , 0 , MAX_DET_BOARD);
	ACE_OS::memset(m_ucRightCnt       , 0 , MAX_DET_BOARD);
	
	ACE_OS::memset(m_ucGetFrency   , 0 , sizeof(Byte) * MAX_DET_BOARD*MAX_DETECTOR_PER_BOARD); //ADD: 20130805 1720
	ACE_OS::memset(m_ucSetFrency   , 0 , sizeof(Byte) * MAX_DET_BOARD*MAX_DETECTOR_PER_BOARD); //ADD: 20130805 1720
	ACE_OS::memset(m_ucSetSensibility   , 0 , sizeof(Byte) * MAX_DET_BOARD*MAX_DETECTOR_PER_BOARD); //ADD: 20130816 1420
	ACE_OS::memset(m_ucGetSensibility   , 0 , sizeof(Byte) * MAX_DET_BOARD*MAX_DETECTOR_PER_BOARD); //ADD: 20130816 1420
		
	for ( int i=0; i<MAX_DETECTOR; i++ )
	{
		m_bErrFlag[i]	= true;
		//m_ucDetError[i] = 2;   //非正常状态
		m_ucDetError[i] = DET_CARVE ; //MOD: 2013 0709 15 56
	}
	ACE_DEBUG((LM_DEBUG,"%s:%d Init Detector object ok !\n",__FILE__,__LINE__));

}

/**************************************************************
Function:        CDetector::~CDetector
Description:     检测器CDetector析构函数		
Input:          无           
Output:         无
Return:         无
***************************************************************/
CDetector::~CDetector()
{
	if ( m_iDevFd > 0 )
	{
		#ifndef WINDOWS
		close(m_iDevFd);
		#endif
	}

	ACE_DEBUG((LM_DEBUG,"%s:%d Destruct Detector object ok !\n",__FILE__,__LINE__));
}

/**************************************************************
Function:        CDetector::CreateInstance
Description:     创建CreateInstance检测器静态对象		
Input:          无           
Output:         无
Return:         静态对象指针
***************************************************************/
CDetector* CDetector::CreateInstance()
{
	static CDetector cDetector; 
	return &cDetector;
}


/**************************************************************
Function:        CDetector::SelectDetectorBoardCfg
Description:     获取检测板的配置情况，获取活动检测器板		
Input:          pDetCfg   检测器配置结构体指针           
Output:         无
Return:         无
***************************************************************/
void CDetector::SelectDetectorBoardCfg(int *pDetCfg)
{
#ifdef NTCIP
ACE_TString sVal;
ACE_INT32 iVal = 0;
	Configure::CreateInstance()->GetInteger(ACE_TEXT("ntcip"), ACE_TEXT("appname"), iVal);
#endif
	ACE_OS::memset(m_iBoardErr,0,4);
	ACE_OS::memcpy(m_iDetCfg,pDetCfg,sizeof(int)*MAX_DET_BOARD);
	/*****Cut from CDECTOR()**/  //20130709 14 35
		for ( int i=0; i<MAX_DET_BOARD; i++ )
		{
			if ( m_iDetCfg[i] != 0 )
			{
				m_iBoardErr[i] = DEV_IS_CONNECTED;
			}
			else
			{
				m_iBoardErr[i] = DEV_IS_DISCONNECTED;
			}
			m_bRecordSts[i] = true;
		}
		
	//ACE_DEBUG((LM_DEBUG,"%s:%d m_iBoardErr[0] = %d m_iBoardErr[1] = %d m_iBoardErr[2] = %d m_iBoardErr[3] = %d\n",__FILE__,__LINE__,m_iBoardErr[0],m_iBoardErr[1],m_iBoardErr[2],m_iBoardErr[3] ));
	/*****Cut from CDECTOR()**/ // 20130709 14 35

	for ( int iIndex=0; iIndex<MAX_DET_BOARD; iIndex++ )
	{
		//ACE_DEBUG((LM_DEBUG,"%s:%d m_iDetCfg[%d] = %d\n",__FILE__,__LINE__,iIndex,m_iDetCfg[iIndex] ));
		if ( 1 == m_iDetCfg[iIndex] )
		{
			m_ucActiveBoard1 = iIndex;     // 1-16对应的检测器板m_ucActiveBoard1 = 0  0-15
		}
		else if ( 17 == m_iDetCfg[iIndex] )
		{
			m_ucActiveBoard2 = iIndex;    // 17 - 32 对应的检测器板  m_ucActiveBoard2=1 16-31
		}		
	}	
	ACE_DEBUG((LM_DEBUG,"%s:%d DetBoard1 index = %d ,DetBoard2 index = %d \n",__FILE__,__LINE__,m_ucActiveBoard1,m_ucActiveBoard2));
}



/**************************************************************
Function:        CDetector::PrintDetInfo
Description:     打印接收到的灯泡数据信息		
Input:          pFileName:文件名 			iFileLine:文件行数  
				ucBoardIndex:板的下标		sErrSrc:错误原因 
			   iPrintCnt:打印的个数 			ucRecvBuf：打印的字符串          
Output:         无
Return:         无
***************************************************************/
void CDetector::PrintDetInfo(char* pFileName,int iFileLine,Byte ucBoardIndex, char* sErrSrc,int iPrintCnt,Byte* ucRecvBuf)
{
	if ( ( (m_pTscCfg->sSpecFun[FUN_PRINT_FLAG].ucValue) & 1 )  != 0 )
	{
		ACE_DEBUG((LM_DEBUG,"%s:%d iIndex:%d %s:",pFileName,iFileLine,ucBoardIndex,sErrSrc));
		for ( int iTmp = 0; iTmp<iPrintCnt; iTmp++ )
		{
			ACE_DEBUG((LM_DEBUG,"  %02x",ucRecvBuf[iTmp]));
		}
		ACE_DEBUG((LM_DEBUG,"\n"));
	}
}


/**************************************************************
Function:        CDetector::SelectBrekonCardStatus
Description:     查询某块板的车辆状态及故障状态，100ms一次	
Input:         iBoardIndex:板块下标  iAddress:获取板信息的地址(协议规定)          
Output:         无，AACCVVCXXBBB
Return:         false:该板块不存在
***************************************************************/
bool CDetector::SelectBrekonCardStatus(Byte ucBoardIndex,Byte ucAddress)
{
	Byte ucSum         = 0;
	Byte ucTmp         = 0;
	Byte ucDetIndex    = 0;
	Byte ucDetCfgIndex = 0;
	Byte ucData[16]    = {0};
	int iPreReadCnt    = 0;
	int iReadCnt       = 0;
	int iIndex         = 0;

#ifndef WINDOWS
	fd_set rfds;
	struct timeval 	tv;

	//usleep(USLEEP_TIME);
	ioctl(m_iDevFd, com_9bit_enable, 0);
	//usleep(USLEEP_TIME);

	if ( !CIoOprate::TscWrite(m_iDevFd , &ucAddress , sizeof(ucAddress)) )
	{
		//ACE_DEBUG((LM_DEBUG,"%s:%d write error",__FILE__,__LINE__));
		return false;
	}

	//usleep(USLEEP_TIME);
	ioctl(m_iDevFd, com_9bit_disable, 0);
	//usleep(USLEEP_TIME);

	ucSum = ~ucAddress;
	if ( !CIoOprate::TscWrite(m_iDevFd , &ucSum , sizeof(ucSum)) )
	{
		//ACE_DEBUG((LM_DEBUG,"%s:%d write error",__FILE__,__LINE__));
		return false;
	}

	FD_ZERO(&rfds);
	FD_SET(m_iDevFd, &rfds);
    tv.tv_sec  = 0;
	tv.tv_usec = 20000;
	iReadCnt = select(m_iDevFd + 1, &rfds, NULL, NULL, &tv);
#endif
									 
    if ( -1 == iReadCnt )  /*select error*/
	{
#ifndef WINDOWS
		//usleep(USLEEP_TIME);
		ioctl(m_iDevFd, com_9bit_disable, 0);
		//usleep(USLEEP_TIME);
		SendRecordBoardMsg(ucBoardIndex,3);
	}
	else if ( 0 == iReadCnt )  /*timeout*/
	{
		if( m_iBoardErr[ucBoardIndex] != DEV_IS_DISCONNECTED )
		{
			m_iBoardErr[ucBoardIndex] = DEV_IS_DISCONNECTED;
		}
		//usleep(USLEEP_TIME);
		ioctl(m_iDevFd, com_9bit_disable, 0);
		//usleep(USLEEP_TIME);

		//ACE_DEBUG((LM_DEBUG,"%s:%d timeout(%d)\n",__FILE__,__LINE__,ucBoardIndex));
		SendRecordBoardMsg(ucBoardIndex,3);
		PrintDetInfo((char*)"CDetector.cpp",__LINE__,ucBoardIndex,(char*)"Timeout",0,NULL);
#endif
	}
	else  /*ÊÕµœÊýŸÝ*/
	{
		m_iBoardErr[ucBoardIndex] = DEV_IS_CONNECTED;

#ifndef WINDOWS
		//ACE_DEBUG((LM_DEBUG,"%s:%d ucBoardIndex:%d value:%d\n"
			//,__FILE__,__LINE__,ucBoardIndex,m_iBoardErr[ucBoardIndex]));
		
		//usleep(USLEEP_TIME);
		ioctl(m_iDevFd, com_9bit_disable, 0);
		//usleep(USLEEP_TIME);
#endif

		if ( 0 == ucBoardIndex || 1 == ucBoardIndex ) 
		{
			iPreReadCnt = 5;
		}
		else
		{
			iPreReadCnt = 9;
		}

#ifndef WINDOWS
		//iReadCnt = read(m_iDevFd, ucData ,iPreReadCnt);
		if ( !CIoOprate::TscRead(m_iDevFd , ucData , iPreReadCnt) )
		{
			//ACE_DEBUG((LM_DEBUG,"%s:%d write error\n",__FILE__,__LINE__));
			return false;
		}
#endif

#ifdef WINDOWS
		 if ( 2 == ucBoardIndex )
		{			
			static bool bFlag = true;

			if ( bFlag )
			{
				ucData[0] = 0x12; 
				ucData[1] = 0x06;
				ucData[2] = 0x04;
				ucData[3] = 0;
				ucData[4] = 0;
				ucData[5] = 0x55;
				ucData[6] = 0x55;
				ucData[7] = 0x45;
				ucData[8] = 0xf4;
			}
			else
			{
				ucData[0] = 0x12; 
				ucData[1] = 6;
				ucData[2] = 0;
				ucData[3] = 0;
				ucData[4] = 0;
				ucData[5] = 0;
				ucData[6] = 0;
				ucData[7] = 0;
				ucData[8] = 0xE7;
			}
			bFlag = !bFlag;
		}
		else
		{
			return false;
		}
#endif
		iReadCnt = iPreReadCnt;
		
		if ( ucData[0] != ( ucAddress + 16 ) )  //地址错
		{			
			SendRecordBoardMsg(ucBoardIndex,2);
			PrintDetInfo((char*)"Detector.cpp",__LINE__,ucBoardIndex,(char*)"Addr Error",iReadCnt,ucData);
			return false;
		}
		else
		{
			iIndex = 0;
			ucSum = 0;
			while ( iIndex < (iReadCnt-1) )
			{
				ucSum +=  ucData[iIndex];
				iIndex++;
			}
			ucSum = ~ucSum;

			if ( ucSum != ucData[iReadCnt-1] )
			{
				SendRecordBoardMsg(ucBoardIndex,1);
				PrintDetInfo((char*)"Detector.cpp",__LINE__,ucBoardIndex,(char*)"Check Sum Error",iReadCnt,ucData);
				return false;
			}

			SendRecordBoardMsg(ucBoardIndex,0);
			PrintDetInfo((char*)"Detector.cpp",__LINE__,ucBoardIndex,(char*)"Right",iReadCnt,ucData);

			if ( (0x10+ucBoardIndex) == ucData[0] )
			{
				//车辆状态
				ucTmp = ucData[2];
				iIndex = 0;
				while ( iIndex < 8 )
				{
					m_iDetStatus[16*ucBoardIndex+iIndex]  = 0; 

					if ( 1 == ( (ucTmp>>iIndex) & 1 ) )
					{
						m_iDetTimeLen[16*ucBoardIndex+iIndex] += 1;     //计算占有率
						m_iDetStatus[16*ucBoardIndex+iIndex]  = 1;      //车道上有车

						m_iAdapDetTimeLen[16*ucBoardIndex+iIndex] += 1;
						
						if ( 0 == m_iLastDetSts[16*ucBoardIndex+iIndex] )
						{
							m_ucTotalStat[16*ucBoardIndex+iIndex] += 1;    //计算车流量 上次无车此次有车才为有车 
							m_iAdapTotalStat[16*ucBoardIndex+iIndex] += 1; 
						}
					}
					m_iLastDetSts[16*ucBoardIndex+iIndex] = (ucTmp>>iIndex) & 1;
					iIndex++;
				}

				ucTmp = ucData[3];
				iIndex = 0;
				while ( iIndex < 8 )
				{
					m_iDetStatus[16*ucBoardIndex+8+iIndex]  = 0;  

					if ( 1 == ( (ucTmp>>iIndex) & 1 ) )
					{
						m_iDetTimeLen[16*ucBoardIndex+8+iIndex]     += 1;     //计算占有率
						m_iDetStatus[16*ucBoardIndex+8+iIndex]       = 1;     //车道上有车
						m_iAdapDetTimeLen[16*ucBoardIndex+8+iIndex] += 1;   

						if ( 0 == m_iLastDetSts[16*ucBoardIndex+8+iIndex] )
						{
							m_ucTotalStat[16*ucBoardIndex+8+iIndex]     += 1;    //计算车流量 上次无车此次有车才为有车 
							m_iAdapTotalStat[16*ucBoardIndex+8+iIndex] += 1;
						}
					}
					m_iLastDetSts[16*ucBoardIndex+8+iIndex] = (ucTmp>>iIndex) & 1;
					iIndex++;
				}
				
				if ( 0 == ucBoardIndex || 1 == ucBoardIndex )  //检测器接口板没有故障状态信息
				{
					for ( int i=0; i<16; i++ )
					{
						m_ucDetError[16*ucBoardIndex+i] = DET_NORMAL;
					}
					return true;
				}
				//故障状态
				for (int i = 0; i< 4; i++ )
				{
					ucTmp = ucData[4+i];
					for (int j=0; j<4; j++)
					{
						ucDetIndex = 16 * ucBoardIndex + i * 4 + j;
						m_ucDetError[ucDetIndex] = (Byte)((ucTmp>>(2*j)) & 3);
						
						if ( m_ucDetError[ucDetIndex] == m_ucLastDetError[ucDetIndex] )
						{
							if ( m_ucDetErrTime[ucDetIndex] > 15 )
							{
								if ( ( m_bErrFlag[ucDetIndex] && m_ucDetError[ucDetIndex] != DET_NORMAL)
								  || (!m_bErrFlag[ucDetIndex] && m_ucDetError[ucDetIndex] == DET_NORMAL) )
								{
									m_bErrFlag[ucDetIndex] = !m_bErrFlag[ucDetIndex];

									ucDetCfgIndex = m_pTscCfg->iDetCfg[ucDetIndex/MAX_DETECTOR_PER_BOARD] - 1
													+ ucDetIndex % MAX_DETECTOR_PER_BOARD;
									if ( ucDetCfgIndex < MAX_DETECTOR 
										&& m_pTscCfg->sDetector[ucDetCfgIndex].ucPhaseId != 0 )  //存在配置的检测器才记录
									{
										//发送记录检测器损坏情况
										//等待检测器故障检测稳定开放该功能
										SThreadMsg sTscMsg;
										sTscMsg.ulType       = TSC_MSG_LOG_WRITE;
										sTscMsg.ucMsgOpt     = LOG_TYPE_DETECTOR;
										sTscMsg.uiMsgDataLen = 4;
										sTscMsg.pDataBuf     = ACE_OS::malloc(4);
										((Byte*)(sTscMsg.pDataBuf))[0] = 0;
										((Byte*)(sTscMsg.pDataBuf))[1] = 0;
										((Byte*)(sTscMsg.pDataBuf))[2] = m_ucDetError[ucDetIndex];
										((Byte*)(sTscMsg.pDataBuf))[3] = ucDetCfgIndex + 1; 
										CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));
									}
								}
							}
							else
							{
								m_ucDetErrTime[ucDetIndex]++;
							}
						}
						else
						{
							m_ucLastDetError[ucDetIndex] = m_ucDetError[ucDetIndex];
							m_ucDetErrTime[ucDetIndex]   = 0;
						}
						 
					}
				}
			}
		}
	
		return true;
	}

	return false;
}


/**************************************************************
Function:        CDetector::SendRecordBoardMsg
Description:     发送记录灯控板通信状态	
Input:          ucType  - 0正确 1校验错误 2地址错误 3没有数据        
Output:         无
Return:         无
***************************************************************/
void CDetector::SendRecordBoardMsg(Byte ucDetIndex,Byte ucType)
{
	Byte ucCnt   = 0;
	Byte ucByte0 = 0;
	Byte ucByte1 = 0;

	if ( ( 0 == ucType && m_bRecordSts[ucDetIndex] ) || ( ucType != 0 && !m_bRecordSts[ucDetIndex] ) )
	{
		switch ( ucType )
		{
			case 0:  //正确	
				m_ucErrCheckCnt[ucDetIndex] = 0;
				m_ucNoCnt[ucDetIndex]       = 0;          
				m_ucErrAddrCnt[ucDetIndex]  = 0; 
				return;
			case 1:   //校验错误
				m_ucNoCnt[ucDetIndex]       = 0;          
				m_ucErrAddrCnt[ucDetIndex]  = 0;     
				m_ucRightCnt[ucDetIndex]    = 0;
				return;
			case 2:   //地址错误
				m_ucErrCheckCnt[ucDetIndex] = 0;
				m_ucNoCnt[ucDetIndex]       = 0;            
				m_ucRightCnt[ucDetIndex]    = 0;
				return;
			case 3:  //没有数据
				m_ucErrCheckCnt[ucDetIndex] = 0;        
				m_ucErrAddrCnt[ucDetIndex]  = 0;     
				m_ucRightCnt[ucDetIndex]    = 0;
				return;
			default:
				return;
		}
	}

	switch ( ucType )
	{
		case 0:
			m_ucRightCnt[ucDetIndex]++;
			ucCnt = m_ucRightCnt[ucDetIndex];
			break;
		case 1:
			m_ucErrCheckCnt[ucDetIndex]++;
			ucCnt =  m_ucErrCheckCnt[ucDetIndex];
			break;
		case 2:
			m_ucErrAddrCnt[ucDetIndex]++;
			ucCnt = m_ucErrAddrCnt[ucDetIndex];
			break;
		case 3:
			m_ucNoCnt[ucDetIndex]++;
			ucCnt = m_ucNoCnt[ucDetIndex];
			break;
		default:
			return;
	}

	if ( ucCnt > BOARD_REPEART_TIME )
	{
		switch ( ucType )
		{
			case 0:  //正确
				m_bRecordSts[ucDetIndex] = true;
				m_ucErrCheckCnt[ucDetIndex] = 0;
				m_ucNoCnt[ucDetIndex]       = 0;          
				m_ucErrAddrCnt[ucDetIndex]  = 0; 
				ucByte0 = 0;
				ucByte1 = 0;
				break;
			case 1:  //校验错误
				m_bRecordSts[ucDetIndex] = false;
				m_ucNoCnt[ucDetIndex]       = 0;          
				m_ucErrAddrCnt[ucDetIndex]  = 0;     
				m_ucRightCnt[ucDetIndex]    = 0;
				ucByte0 = 1;
				ucByte1 = 2;
				break;
			case 2:  //地址错误
				m_bRecordSts[ucDetIndex] = false;
				m_ucErrCheckCnt[ucDetIndex] = 0;
				m_ucNoCnt[ucDetIndex]       = 0;            
				m_ucRightCnt[ucDetIndex]    = 0;
				ucByte0 = 1;
				ucByte1 = 1;
				break;
			case 3:  //没有数据
				m_bRecordSts[ucDetIndex] = false;
				m_ucErrCheckCnt[ucDetIndex] = 0;        
				m_ucErrAddrCnt[ucDetIndex]  = 0;     
				m_ucRightCnt[ucDetIndex]    = 0;
				ucByte0 = 1;
				ucByte1 = 0;
				break;
			default:
				return;
		}
	}
	else
	{
		return;
	}

	SThreadMsg sTscMsg;
	sTscMsg.ulType       = TSC_MSG_LOG_WRITE;
	sTscMsg.ucMsgOpt     = LOG_TYPE_DETBOARD;
	sTscMsg.uiMsgDataLen = 4;
	sTscMsg.pDataBuf     = ACE_OS::malloc(4);
	((Byte*)(sTscMsg.pDataBuf))[0] = 0;
	((Byte*)(sTscMsg.pDataBuf))[1] = ucDetIndex;
	((Byte*)(sTscMsg.pDataBuf))[2] = ucByte1;
	((Byte*)(sTscMsg.pDataBuf))[3] = ucByte0; 
	CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));

	//ACE_DEBUG((LM_DEBUG,"%s:%d ucDetIndex:%d ucByte1:%d ucByte0:%d\n"
	//	,__FILE__,__LINE__,ucDetIndex,ucByte1,ucByte0));
}


/**************************************************************
Function:        CDetector::SearchAllStatus
Description:     获取检测器板车辆状态及故障状态，100ms一次	
Input:          无        
Output:         无
Return:         无
***************************************************************/
 void CDetector::SearchAllStatus()
 {
	Byte ucIndex = 0;
	static int iTick = 0;
	unsigned int uiTclCtl = CManaKernel::CreateInstance()->m_pRunData->uiCtrl;

	while ( ucIndex < MAX_DET_BOARD )
	{
		if ( ( DEV_IS_CONNECTED == m_iBoardErr[ucIndex] )	&& ( m_iDetCfg[ucIndex] != 0 ) && iTick%5 == 0)
		{
			
			//SelectBrekonCardStatus(ucIndex, ucIndex);  //第iIndex片检测器板车辆状态及故障状态 MOD:20130723 1620
			if(CManaKernel::CreateInstance()->IsVehile()) //ADD:2013 0724 1010
			{	
				GetAllVehSts(DET_HEAD_VEHSTS,ucIndex); //查询车检板有车无车通过
			}
		 
			GetAllVehSts(DET_HEAD_STS,ucIndex);    //查询检测器通道状态  开路 正常 短路等情况。								
		}		
		ucIndex++;
	}
		
		iTick++;
		if ( iTick >= MAX_REGET_TIME )  // 1S一次检查车检板状态和有无车情况
		{
			iTick = 0;
		}
	
	if ( (CTRL_VEHACTUATED == uiTclCtl) || (CTRL_MAIN_PRIORITY == uiTclCtl) || (CTRL_SECOND_PRIORITY == uiTclCtl) ) 
	{
		IsVehileHaveCar(); //如果有车则增加长步放行相位的绿灯时间 最大为最大绿时间
	}

}

/**************************************************************

Function:        CDetector::GetAllWorkSts
Description:     获取所有检测器板的工作状态，一个周期一次	
Input:          无        
Output:         无
Return:         无
***************************************************************/
void CDetector::GetAllWorkSts()
{
	Byte ucIndex = 0;	

	while ( ucIndex < MAX_DET_BOARD )
	{
		if ( ( DEV_IS_CONNECTED == m_iBoardErr[ucIndex] )&& (m_iDetCfg[ucIndex] != 0 ))
		{	 
			GetAllVehSts(DET_HEAD_STS,ucIndex);   //查询检测器通道状态  开路 正常 短路等情况。								
		}		
		ucIndex++;
	}


}

/**************************************************************
Function:        CDetector::SendDetLink
Description:     设置检测器对应关系 线圈对应关系	
Input:          ucBoardIndex - 0    1    2    3   检测板下标  
				SetType  - DET_HEAD_COIL0104_SET 
						   DET_HEAD_COIL0508_SET 设置组类型      
Output:         无
Return:         无
***************************************************************/
void CDetector::SendDetLink(Byte ucBoardIndex,Byte SetType)
{
	
		Byte iNdex = 0 ;
		Byte BoardAddr = 0;
		SCanFrame sSendFrameTmp;
		ACE_OS::memset(&sSendFrameTmp , 0 , sizeof(SCanFrame));	
		
		if(SetType != DET_HEAD_COIL0104_SET && SetType != DET_HEAD_COIL0508_SET)
		{
			ACE_DEBUG((LM_DEBUG,"%s:%d, settype:%02x \n", __FILE__, __LINE__, SetType));
			return ;

		}
		BoardAddr = GetDecAddr(ucBoardIndex);			

		Can::BuildCanId(CAN_MSG_TYPE_101 , BOARD_ADDR_MAIN  , FRAME_MODE_P2P , BoardAddr  , &(sSendFrameTmp.ulCanId));

		sSendFrameTmp.pCanData[0] = ( DATA_HEAD_RESEND<< 6) | SetType;
		
		 if(SetType == DET_HEAD_COIL0508_SET)
			iNdex = 4 ;
		sSendFrameTmp.pCanData[1] = m_sSetLookLink[ucBoardIndex][0+iNdex]; 		
		sSendFrameTmp.pCanData[2] = m_sSetLookLink[ucBoardIndex][1+iNdex];		
		sSendFrameTmp.pCanData[3] = m_sSetLookLink[ucBoardIndex][2+iNdex]; 		
		sSendFrameTmp.pCanData[4] = m_sSetLookLink[ucBoardIndex][3+iNdex];  	
		
		sSendFrameTmp.ucCanDataLen = 5;	
		Can::CreateInstance()->Send(sSendFrameTmp);
		ACE_DEBUG((LM_DEBUG,"\n%s:%d SetType:%02x  %02x  %02x  %02x  %02x\n ",__FILE__,__LINE__,SetType,sSendFrameTmp.pCanData		  [1],sSendFrameTmp.pCanData[2],sSendFrameTmp.pCanData[3],sSendFrameTmp.pCanData[4]));  

}


/**************************************************************
Function:        CDetector::GetDecAddr
Description:     通过检测板下标获取检测板CAN地址	
Input:          ucBoardIndex - 0    1    2    3   检测板下标   
Output:         无
Return:         检测器板地址
***************************************************************/
Byte CDetector::GetDecAddr(Byte ucBoardIndex)
{
		Byte BoardAddr = 0 ;
		switch(ucBoardIndex)
		{
			case 0:
				BoardAddr = BOARD_ADDR_DETECTOR1 ;
				break ;
			case 1:
				BoardAddr = BOARD_ADDR_DETECTOR2 ;
				break ;
			case 2:
				BoardAddr = BOARD_ADDR_INTEDET1 ;
				break ;
			case 3:
				BoardAddr = BOARD_ADDR_INTEDET2 ;
				break ;
			default:
				return 0;
		}	

		return BoardAddr ;

}



/**************************************************************
Function:        CDetector::SendDelicacy
Description:     设置检测器板各个通道的灵敏度等级
Input:          ucBoardIndex - 0 1 2 3
			   SetType DET_HEAD_SEN0108_SET  
			   		   DET_HEAD_SEN0916_SET 通道组类型 
Output:         无
Return:         无
***************************************************************/
void CDetector::SendDelicacy(Byte ucBoardIndex,Byte SetType)
{
	Byte iNdex = 0 ;
	Byte BoardAddr = 0;
	SCanFrame sSendFrameTmp;
	ACE_OS::memset(&sSendFrameTmp , 0 , sizeof(SCanFrame));	
		
	if(SetType != DET_HEAD_SEN0108_SET && SetType != DET_HEAD_SEN0916_SET)
	{
		return ;
	}
	BoardAddr = GetDecAddr( ucBoardIndex);			

	Can::BuildCanId(CAN_MSG_TYPE_101 , BOARD_ADDR_MAIN  , FRAME_MODE_P2P , BoardAddr  , &(sSendFrameTmp.ulCanId));

	sSendFrameTmp.pCanData[0] = ( DATA_HEAD_RESEND<< 6) | SetType;

	 if(SetType == DET_HEAD_SEN0916_SET)
		iNdex = 8 ;
	sSendFrameTmp.pCanData[1] = m_ucSetDetDelicacy[ucBoardIndex][0+iNdex];  ; 
	sSendFrameTmp.pCanData[1] |=  m_ucSetDetDelicacy[ucBoardIndex][1+iNdex]<<4; 
		
	sSendFrameTmp.pCanData[2] = m_ucSetDetDelicacy[ucBoardIndex][2+iNdex];  ; 
	sSendFrameTmp.pCanData[2] |=  m_ucSetDetDelicacy[ucBoardIndex][3+iNdex]<<4; 
		
	sSendFrameTmp.pCanData[3] = m_ucSetDetDelicacy[ucBoardIndex][4+iNdex];  ; 
	sSendFrameTmp.pCanData[3] |=  m_ucSetDetDelicacy[ucBoardIndex][5+iNdex]<<4; 

	sSendFrameTmp.pCanData[4] = m_ucSetDetDelicacy[ucBoardIndex][6+iNdex];  ; 
	sSendFrameTmp.pCanData[4] |=  m_ucSetDetDelicacy[ucBoardIndex][7+iNdex]<<4; 
		
	sSendFrameTmp.ucCanDataLen = 5;	

	Can::CreateInstance()->Send(sSendFrameTmp);
	ACE_DEBUG((LM_DEBUG,"\nSetType:%02x %d  %d  %d  %d\n ",SetType,sSendFrameTmp.pCanData[1],sSendFrameTmp.pCanData  		[2],sSendFrameTmp.pCanData[3],sSendFrameTmp.pCanData[4]));

}



/**************************************************************
Function:        CDetector::GetDecVars
Description:    查询检测器各个状态类型的值
Input:          ucBoardIndex - 0 1 2 3
   				SetType  查询类型
Output:         无
Return:         无
***************************************************************/
void CDetector::GetDecVars(Byte ucBoardIndex,Byte GetType)
{		
	Byte BoardAddr = 0;
	SCanFrame sSendFrameTmp;
	ACE_OS::memset(&sSendFrameTmp , 0 , sizeof(SCanFrame));	
		
	if(ucBoardIndex<0 || ucBoardIndex >3)
		return ;
	BoardAddr = GetDecAddr( ucBoardIndex);	

	Can::BuildCanId(CAN_MSG_TYPE_101 , BOARD_ADDR_MAIN  , FRAME_MODE_P2P , BoardAddr  , &(sSendFrameTmp.ulCanId));

	sSendFrameTmp.pCanData[0] = ( DATA_HEAD_RESEND<< 6) | GetType;
	sSendFrameTmp.ucCanDataLen = 1;	

	Can::CreateInstance()->Send(sSendFrameTmp);

}



/**************************************************************
Function:        CDetector::SendRoadDistance
Description:    设置检测器组线圈距离 
Input:          ucBoardIndex - 0 1 2 3
   				SetType  设置类型
   						 DET_HEAD_DISTAN0104_SET DET_HEAD_DISTAN0508_SET
Output:         无
Return:         无
***************************************************************/
void CDetector::SendRoadDistance(Byte ucBoardIndex,Byte SetType)
{
	Byte iNdex = 0 ;
	Byte BoardAddr = 0;
	SCanFrame sSendFrameTmp;
	ACE_OS::memset(&sSendFrameTmp , 0 , sizeof(SCanFrame));	
		
	if(SetType != DET_HEAD_DISTAN0104_SET && SetType != DET_HEAD_DISTAN0508_SET)
	{
		ACE_DEBUG((LM_DEBUG,"%s:%d, settype:%02x \n", __FILE__, __LINE__, SetType));
		return ;
	}
	BoardAddr = GetDecAddr( ucBoardIndex);			

	Can::BuildCanId(CAN_MSG_TYPE_101 , BOARD_ADDR_MAIN  , FRAME_MODE_P2P , BoardAddr  , &(sSendFrameTmp.ulCanId));

	sSendFrameTmp.pCanData[0] = ( DATA_HEAD_RESEND<< 6) | SetType;
		
	if(SetType == DET_HEAD_DISTAN0508_SET)
		iNdex = 4 ;
	sSendFrameTmp.pCanData[1] = m_ucSetRoadDis[ucBoardIndex][0+iNdex]; 		
	sSendFrameTmp.pCanData[2] = m_ucSetRoadDis[ucBoardIndex][1+iNdex];		
	sSendFrameTmp.pCanData[3] = m_ucSetRoadDis[ucBoardIndex][2+iNdex]; 		
	sSendFrameTmp.pCanData[4] = m_ucSetRoadDis[ucBoardIndex][3+iNdex];  	
		
	sSendFrameTmp.ucCanDataLen = 5;	
	Can::CreateInstance()->Send(sSendFrameTmp);
	ACE_DEBUG((LM_DEBUG,"\nSetType:%02x %d  %d  %d  %d\n ",SetType,sSendFrameTmp.pCanData[1],sSendFrameTmp.pCanData[2],sSendFrameTmp.pCanData[3],sSendFrameTmp.pCanData[4]));
}


/**************************************************************
Function:        CDetector::SendDecWorkType
Description:    设置检测器工作方式  0 -- 脉冲型 1--存在型
Input:          ucBoardIndex - 0 1 2 3
   				
Output:         无
Return:         无
***************************************************************/
void CDetector::SendDecWorkType(Byte ucBoardIndex)
{	
	Byte BoardAddr = 0;
	SCanFrame sSendFrameTmp;
	ACE_OS::memset(&sSendFrameTmp , 0 , sizeof(SCanFrame));	

	BoardAddr = GetDecAddr( ucBoardIndex);			

	Can::BuildCanId(CAN_MSG_TYPE_101 , BOARD_ADDR_MAIN  , FRAME_MODE_P2P , BoardAddr  , &(sSendFrameTmp.ulCanId));
	sSendFrameTmp.pCanData[0] = ( DATA_HEAD_RESEND<< 6) | DET_HEAD_WORK_SET;		
	sSendFrameTmp.pCanData[1] =m_iChkType; 		
	sSendFrameTmp.ucCanDataLen = 2;	
	Can::CreateInstance()->Send(sSendFrameTmp);
}



/**************************************************************
Function:        CDetector::SendDecIsLink
Description:    设置检测器板是否允许绑定线圈组
Input:          ucBoardIndex - 0 1 2 3
   				IsAllowLink true-允许 false-不允许
Output:         无
Return:         无
***************************************************************/
void CDetector::SendDecIsLink(Byte ucBoardIndex,Byte IsAllowLink)
{
	Byte BoardAddr = 0;
	SCanFrame sSendFrameTmp;
	ACE_OS::memset(&sSendFrameTmp , 0 , sizeof(SCanFrame));	

	BoardAddr = GetDecAddr( ucBoardIndex);			
	Can::BuildCanId(CAN_MSG_TYPE_101 , BOARD_ADDR_MAIN  , FRAME_MODE_P2P , BoardAddr  , &(sSendFrameTmp.ulCanId));
	sSendFrameTmp.pCanData[0] = ( DATA_HEAD_RESEND<< 6) | DET_HEAD_COILALLOW_SET;		
	sSendFrameTmp.pCanData[1] =IsAllowLink; 		
	sSendFrameTmp.ucCanDataLen = 2;	
	Can::CreateInstance()->Send(sSendFrameTmp);
}



/**************************************************************
Function:        CDetector::SendDecFrency
Description:    设置检测器的振荡频率
Input:          ucBoardIndex - 0 1 2 3
   				IsAllowLink true-允许 false-不允许
Output:         无
Return:         无
***************************************************************/
void CDetector::SendDecFrency(Byte ucBoardIndex)
{		
	Byte BoardAddr = 0;
	SCanFrame sSendFrameTmp;
	ACE_OS::memset(&sSendFrameTmp , 0 , sizeof(SCanFrame));	
	
	BoardAddr = GetDecAddr( ucBoardIndex);			
	Can::BuildCanId(CAN_MSG_TYPE_101 , BOARD_ADDR_MAIN  , FRAME_MODE_P2P , BoardAddr  , &(sSendFrameTmp.ulCanId));
	sSendFrameTmp.pCanData[0] = ( DATA_HEAD_RESEND<< 6) | DET_HEAD_FRENCY_SET;		
	for(int i = 0 ; i < MAX_DETECTOR_PER_BOARD ; i++)			
		sSendFrameTmp.pCanData[i/4+1] |= m_ucSetFrency[ucBoardIndex][i] <<(i%4*2); 		
	sSendFrameTmp.ucCanDataLen = 5;	
	Can::CreateInstance()->Send(sSendFrameTmp);
}




/**************************************************************
Function:        CDetector::SendDecSenData
Description:    设置检测器板灵敏度数值
Input:          ucBoardIndex - 0 1 2 3
   				ucSetType 灵敏度数组设置组
Output:         无
Return:         无
***************************************************************/
void CDetector::SendDecSenData(Byte ucBoardIndex,Byte ucSetType)
{	
	Byte BoardAddr = 0;
	SCanFrame sSendFrameTmp;
	ACE_OS::memset(&sSendFrameTmp , 0 , sizeof(SCanFrame));	

	if(ucSetType != DET_HEAD_SENDATA0107_SET && ucSetType != DET_HEAD_SENDATA0814_SET  && ucSetType != DET_HEAD_SENDATA1516_SET)
	{
		ACE_DEBUG((LM_DEBUG,"%s:%d, settype:%02x \n", __FILE__, __LINE__, ucSetType));
		return ;
	}
	BoardAddr = GetDecAddr( ucBoardIndex);			

	Can::BuildCanId(CAN_MSG_TYPE_101 , BOARD_ADDR_MAIN  , FRAME_MODE_P2P , BoardAddr  , &(sSendFrameTmp.ulCanId));

	sSendFrameTmp.pCanData[0] = ( DATA_HEAD_RESEND<< 6) | ucSetType;		
		
	 if(ucSetType == DET_HEAD_SENDATA0107_SET)
	{			
		for(int ucIndex = 0 ; ucIndex < 7 ;ucIndex++)
		{
			sSendFrameTmp.pCanData[1+ucIndex] = m_ucSetSensibility[ucBoardIndex][ucIndex]; 
			//printf(" %02x + ",m_ucSetSensibility[ucBoardIndex][ucIndex]);
		}
		sSendFrameTmp.ucCanDataLen = 8;	
	}
	else if(ucSetType == DET_HEAD_SENDATA0814_SET)
	{
		for(int ucIndex = 0 ; ucIndex < 7 ;ucIndex++)
		{
			sSendFrameTmp.pCanData[1+ucIndex] = m_ucSetSensibility[ucBoardIndex][7+ucIndex]; 
			//printf(" %02x - ",sSendFrameTmp.pCanData[1+ucIndex]);
		}
			sSendFrameTmp.ucCanDataLen = 8;
		}
	else if(ucSetType == DET_HEAD_SENDATA1516_SET)
	{
		sSendFrameTmp.pCanData[1] = m_ucSetSensibility[ucBoardIndex][14];
		sSendFrameTmp.pCanData[2] = m_ucSetSensibility[ucBoardIndex][15];
		sSendFrameTmp.ucCanDataLen = 3;
		//printf(" %02x * %02x ",sSendFrameTmp.pCanData[2],sSendFrameTmp.pCanData[2]);
	}				
	Can::CreateInstance()->Send(sSendFrameTmp);
	
}


/**************************************************************
Function:        CDetector::GetOccupy
Description:    获取占有率 1s per  5min统计一次
Input:          无
Output:         无
Return:         无
***************************************************************/
void CDetector::GetOccupy()
{
	static int iTick = 0;
	Ushort iIndex       = 0;

	iTick++;

	if ( iTick < m_iTotalDistance )  //5min 5*60
	{
		return;
	}
	iTick = 0;
	//ACE_Guard<ACE_Thread_Mutex> guard(m_sMutex);
	//ACE_OS::memcpy( m_iLastDetTimeLen , m_iDetTimeLen , sizeof(int)*MAX_DETECTOR );

	iIndex = MAX_DETECTOR;
	CManaKernel *pManaKernel = CManaKernel::CreateInstance();
	while ( iIndex-- > 0 )
	{
		//ACE_DEBUG((LM_DEBUG,"******%d\n",m_iDetTimeLen[iIndex]));
		if(pManaKernel->m_pTscConfig->sDetector[iIndex].ucPhaseId>0)
		{
			m_iDetOccupy[iIndex] = m_iDetTimeLen[iIndex] *100 / (m_iTotalDistance*10) ;  
	//1s 10ŽÎÍ³ŒÆ m_iDetTimeLen 1min=60s 60*10*5
			(CDbInstance::m_cGbtTscDb).AddVehicleStat(iIndex+1 , m_ucTotalStat[iIndex] ,  m_iDetOccupy[iIndex]);
		}
	}
	
	ACE_OS::memset(m_iDetTimeLen , 0 , sizeof(int)*MAX_DETECTOR);
	ACE_OS::memset(m_ucTotalStat  , 0 , sizeof(Byte)*MAX_DETECTOR);
}


/**************************************************************
Function:        CDetector::GetActiveDetSum
Description:    获取活动检测器总数
Input:          无
Output:         无
Return:         无
***************************************************************/
int CDetector::GetActiveDetSum()
{
	int iSum   = 0;
	int iIndex = 0;
	int jIndex = 0;
	//ACE_Guard<ACE_Thread_Mutex> guard(m_sMutex);
	while ( iIndex < 4 )
	{
		if ( DEV_IS_CONNECTED == m_iBoardErr[iIndex] )
		{
			jIndex = 0;
			while ( jIndex < 16)
			{
				if ( DET_NORMAL == m_ucDetError[iIndex*16+jIndex] )
				{
					iSum += 1;
				}
				jIndex++;
			}
			
		}
		iIndex++;
	}
	//ACE_DEBUG((LM_DEBUG,"%s:%d iSum:%d\n",__FILE__,__LINE__,iSum));
	return iSum;
}



/**************************************************************
Function:        CDetector::GetDetBoardType
Description:    获取检测器板的类型
Input:          无
Output:         无
Return:         0:检测器 1:检测器接口板
***************************************************************/
int CDetector::GetDetBoardType()
{
	if ( (DEV_IS_CONNECTED == m_iBoardErr[0]) || (DEV_IS_CONNECTED == m_iBoardErr[1]) )
	{
		return 1;
	}

	return 0;
}

/**************************************************************
Function:        CDetector::GetDetStatus
Description:    获取检测器板的类型
Input:          无
Output:         pDetStatus 检测器状态结构体数组指针
Return:         无
***************************************************************/
void CDetector::GetDetStatus(SDetectorSts* pDetStatus)
{
	int iIndex    = 0;
	int jIndex    = 0;
	int iDetId    = 0;
	Byte ucStutas = 0;
	Byte ucAlarm  = 0;
	Byte ucBoardIndex      = m_ucActiveBoard1;
	
	while ( iIndex < 8 )
	{
		(pDetStatus+iIndex)->ucId = iIndex + 1;
		iIndex++;
	}

	iIndex = 0;
	while ( iIndex < 4 )
	{
		ucStutas = 0;
		ucAlarm  = 0;
		jIndex   = 0;

		if ( (2 == iIndex) || (3 == iIndex) )  
		{
			ucBoardIndex = m_ucActiveBoard2;
		}

		while ( jIndex < 8 )
		{
			iDetId    = MAX_DETECTOR_PER_BOARD * ucBoardIndex + ( iIndex % 2 ) * 8 + jIndex;
			ucStutas |= (m_iDetStatus[iDetId]<<jIndex);

			if ( ( m_ucDetError[iDetId] != DET_NORMAL )  && ( m_pTscCfg->sDetector[iIndex*8+jIndex].ucPhaseId != 0 ) )
			{
				ucAlarm  |= (1<<jIndex);
			}
			jIndex++;
		}
		(pDetStatus+iIndex)->ucStatus = ucStutas;
		(pDetStatus+iIndex)->ucAlarm  = ucAlarm;
		iIndex++;
	}

#if 0
	iIndex = 0;
	while ( iIndex < 8 )
	{
		ACE_DEBUG((LM_DEBUG,"id:%d status:%d alarm:%d\n"
			            ,(pDetStatus+iIndex)->ucId
						,(pDetStatus+iIndex)->ucStatus
						,(pDetStatus+iIndex)->ucAlarm));
		iIndex++;
	}
	ACE_DEBUG((LM_DEBUG,"\n"));
#endif
}


/**************************************************************
Function:        CDetector::GetDetData
Description:    获取交通检测器数据表
Input:          无
Output:         pDetData 交通检测数据结构体指针
Return:         无
***************************************************************/
void CDetector::GetDetData(SDetectorData* pDetData)
{
	Byte ucBoardIndex = m_ucActiveBoard1;
	int iIndex    = 0;
	int iDetId    = 0;

	while ( iIndex < 48 )
	{
		(pDetData+iIndex)->ucId = iIndex + 1;
		iIndex++;
	}

	iIndex    = 0;
	while ( iIndex < 32 )
	{
		if ( iIndex >= MAX_DETECTOR_PER_BOARD )
		{
			ucBoardIndex = m_ucActiveBoard2;  //16 - 31
		}
		iDetId = ucBoardIndex * MAX_DETECTOR_PER_BOARD + iIndex % 16;
		//(pDetData+iIndex)->ucId        = iIndex + 1;
		(pDetData+iIndex)->ucVolume      = m_ucTotalStat[iDetId];
		(pDetData+iIndex)->ucLongVolume  = 0;
		(pDetData+iIndex)->ucSmallVolume = 0;
		
		//上一个周期的占有率
		(pDetData+iIndex)->ucOccupancy = m_iDetOccupy[iDetId] * 2; //单位0.5

		//当前的占有率
		//(pDetData+iIndex)->ucOccupancy   = m_iDetTimeLen[iDetId] * 100 * 2 / ( m_iTotalDistance * 10 ) ; //单位0.5
		
		//(pDetData+iIndex)->ucVelocity  = m_iDetSpeedAvg[iDetId];
		(pDetData+iIndex)->ucVehLen      = 0;
		iIndex++;
	}

#if 0
	iIndex = 0;
	while ( iIndex < 48 )
	{
		ACE_DEBUG((LM_DEBUG,"id:%d Volume:%d LongVolume:%d SmallVolume:%d Occupancy:%d Velocity:%d VehLen:%d\n"
			,(pDetData+iIndex)->ucId
			,(pDetData+iIndex)->ucVolume
			,(pDetData+iIndex)->ucLongVolume
			,(pDetData+iIndex)->ucSmallVolume
			,(pDetData+iIndex)->ucOccupancy
			,(pDetData+iIndex)->ucVelocity
			,(pDetData+iIndex)->ucVehLen));
		iIndex++;
	}
	ACE_DEBUG((LM_DEBUG,"\n"));
#endif
}


/**************************************************************
Function:        CDetector::GetDetAlarm
Description:    获取车辆检测器告警表
Input:          无
Output:         pDetAlarm 车辆告警数据结构体指针
Return:         无
***************************************************************/
void CDetector::GetDetAlarm(SDetectorAlarm* pDetAlarm)
{
	Byte ucBoardIndex = m_ucActiveBoard1;
	int iIndex        = 0;
	int iDetId        = 0;
	//STscConfig* pTscConfig = CManaKernel::CreateInstance()->m_pTscConfig;

	while ( iIndex < 48 )
	{
		(pDetAlarm+iIndex)->ucId = iIndex + 1;
		iIndex++;
	}

	iIndex = 0;
	while ( iIndex < 32 ) 
	{
		if ( iIndex >=  MAX_DETECTOR_PER_BOARD )
		{
			ucBoardIndex = m_ucActiveBoard2;  // 16 - 31
		}
		
		if ( m_iBoardErr[ucBoardIndex] == DEV_IS_DISCONNECTED )
		{
			(pDetAlarm+iIndex)->ucDetAlarm |= (1<<3);  /*通信故障*/
			iIndex++;
			continue;
		}

		iDetId = ucBoardIndex * MAX_DETECTOR_PER_BOARD + iIndex % 16;

		if ( ( m_ucDetError[iDetId] != DET_NORMAL )
		  /*&& ( pTscConfig->sDetector[iIndex].ucPhaseId != 0 ) */)
		{
			(pDetAlarm+iIndex)->ucDetAlarm |= (1<<7); /*未知故障*/
		}

		switch ( m_ucDetError[iDetId] )
		{
			case DET_CARVE:  //开路
				(pDetAlarm+iIndex)->ucCoilAlarm |= (1<<2);    //开路
				break;
			case DET_SHORT:   //短路
				(pDetAlarm+iIndex)->ucCoilAlarm |= (1<<4);  //感应变化超限 
				break;
			case DET_STOP:    //停振
				(pDetAlarm+iIndex)->ucCoilAlarm |= (1<<3);  //电感不足
				break;
			default:
				break;
		}

		iIndex++;
	}

#if 0
	iIndex = 0;
	while ( iIndex < 48 )
	{
		ACE_DEBUG((LM_DEBUG,"id:%d ucDetAlarm:%d ucCoilAlarm:%d \n"
			,(pDetAlarm+iIndex)->ucId
			,(pDetAlarm+iIndex)->ucDetAlarm
			,(pDetAlarm+iIndex)->ucCoilAlarm));
		iIndex++;
	}
	ACE_DEBUG((LM_DEBUG,"\n"));
#endif

}


/**************************************************************
Function:        CDetector::IsDetError
Description:    判断是否存在检测器故障 检测器板存在+检测器故障
Input:          无
Output:         无
Return:         false:不存在板或者没有检测器故障 true:存在检测器故障
***************************************************************/
bool CDetector::IsDetError()
{
	int iIndex = 0;

	while ( iIndex < MAX_DETECTOR )
	{
		if ( DEV_IS_CONNECTED == m_iBoardErr[iIndex/16] )
		{
			if ( m_ucDetError[iIndex] != DET_NORMAL ) 
			{
				return true;
			}
		}
		iIndex++;
	}

	return false;
}

/***************************************************************

获取有车的时间记录1s获取一次
#ifndef WINDOWS
void CDetector::GetHaveCarTime(time_t* pTime)
{
	ACE_Guard<ACE_Thread_Mutex> guard(m_sMutex);

	ACE_OS::memcpy(pTime,m_tHaveCarTime,MAX_DETECTOR);
}
#endif

*****************************************************************/

/**************************************************************
Function:        CDetector::HaveDetBoard
Description:    是否存在检测器板 且 存在检测器是好的   两块检测板
				任意一块有任意一个检测器则返回真 这个主要是从配置
				表来判断是否有检测器
Input:          无
Output:         无
Return:         true:存在  false:不存在
***************************************************************/
bool CDetector::HaveDetBoard()
{	
	int iIndex = 0;
	int iDetId = 0;
	int iBoardIndex = 0;
	STscConfig* pTscConfig = CManaKernel::CreateInstance()->m_pTscConfig;

	/*判断当前是否有配置检测器板*/
	for (  iBoardIndex=0; iBoardIndex<MAX_DET_BOARD; iBoardIndex++ )
	{
		if ( pTscConfig->iDetCfg[iBoardIndex] != 0 )
		
			break;		
	}
	if ( iBoardIndex >= MAX_DET_BOARD )
	{
		ACE_DEBUG((LM_DEBUG,"%s:%d  All DecBoards error!:%d \n",__FILE__,__LINE__));
		return false;
	}
	//往下运行说明至少配置了一块检测器板
	//ACE_Guard<ACE_Thread_Mutex> guard(m_sMutex);
	//ACE_DEBUG((LM_DEBUG,"%s:%d  There are at list %d DET\n",__FILE__,__LINE__,iBoardIndex+1));
#ifdef WINDOWS
	return true;
	
#else	
	while ( iIndex < MAX_DETECTOR )//32
	{
		if ( iIndex / MAX_DETECTOR_PER_BOARD != 0 )  //16 - 31
		{
			iDetId = (iIndex % MAX_DETECTOR_PER_BOARD) + m_ucActiveBoard2 * MAX_DETECTOR_PER_BOARD; // 16-31
		}
		else  //0 - 15
		{
			iDetId = (iIndex % MAX_DETECTOR_PER_BOARD) + m_ucActiveBoard1 * MAX_DETECTOR_PER_BOARD; // 0--15
		}
		//ACE_DEBUG((LM_DEBUG,"%s:%d  pTscConfig->sDetector[%d].ucPhaseId =  %d m_iBoardErr[%d]=%d,m_ucDetError[%d] =%d\n",__FILE__,__LINE__,iIndex,pTscConfig->sDetector[iIndex].ucPhaseId,iIndex/16, m_iBoardErr[iIndex/16],iIndex,m_ucDetError[iIndex]));
		if ( (DEV_IS_CONNECTED == m_iBoardErr[iDetId/MAX_DETECTOR_PER_BOARD] )  && (pTscConfig->sDetector[iIndex].ucPhaseId != 0)   && (DET_NORMAL == m_ucDetError[iDetId]) )
		{
			//ACE_DEBUG((LM_DEBUG,"%s:%d  index:%d detId:%d\n",__FILE__,__LINE__,iIndex,iDetId+1));
			return true; //两块板32个检测器位置任何一个检测器存在 则返回真.
		}
		
		iIndex++;
	}
#endif
	
	return false;
}


/**************************************************************
Function:        CDetector::IsVehileHaveCar
Description:    感应控制,判断是否有车 如果有车则加绿灯延长时间
Input:          无
Output:         无
Return:         无
***************************************************************/
void CDetector::IsVehileHaveCar()
{
	static bool bVehile      = false;
	static bool bDefStep     = false;
	static bool bOtherWayCar = false;   //另一个车道是否有车
	bool bOtherTmp           = false;
	bool bHaveCar            = false;
	bool bCurMainDrive       = false;
	Byte ucPhaseIndex        = 0;
	static int  iAdjustTime  = 0;
	static Uint uiCurPhase   = 0;
	static Uint uiNextPhase  = 0;
	Uint uiTscCtrl           = 0;
	static SPhaseDetector sPhaseDet[MAX_PHASE];

	CManaKernel::CreateInstance()->GetVehilePara( &bVehile , &bDefStep , &iAdjustTime 
		                                           , &uiCurPhase , &uiNextPhase , sPhaseDet);
	if ( !bVehile ) //如果不是阶段初始步并且是绿长步，则直接返回 不会增加绿步长
	{
		return;
	}

	if ( bDefStep )  //新的步伐
	{
		bDefStep     = false;
		bOtherWayCar = false;
		
	}

	uiTscCtrl     = CManaKernel::CreateInstance()->m_pRunData->uiCtrl;
	if ( uiTscCtrl != CTRL_VEHACTUATED )
	{
		bCurMainDrive = CManaKernel::CreateInstance()->IsMainPhaseGrp(uiCurPhase);  //只有是阶段第一步并且是绿长步的情况下有感应才有意义
	}

	switch ( uiTscCtrl )
	{
		case CTRL_VEHACTUATED:
			if ( IsHaveCarPhaseGrp( uiCurPhase, ucPhaseIndex , sPhaseDet) ) //uiCurPhase当前阶段放行相位 每个周期会获取一次各个阶段的放行相位
			{
				bHaveCar = true; //放行相位任何一个相位的任何一个检测器有车 则 结果有车	 sPhaseDet 相位与检测器对应关系 
			}
			break;
		case CTRL_MAIN_PRIORITY:
			if ( bCurMainDrive )  //当前为主相位
			{
				if ( IsHaveCarPhaseGrp( uiCurPhase, ucPhaseIndex , sPhaseDet) ) //主车道有车
				{
					bHaveCar = true;
				}
			}
			else  if ( !bOtherWayCar ) //当前为次相位 + 主车道在该步伐一直没有车
			{
				bOtherTmp = IsHaveCarPhaseGrp(uiNextPhase, ucPhaseIndex , sPhaseDet);   //当前主车道是否有车
				if ( bOtherTmp )
				{
					bOtherWayCar = true;
				}

				if ( !bOtherWayCar && IsHaveCarPhaseGrp(uiCurPhase, ucPhaseIndex ,sPhaseDet) )  //主车道在该步伐一直没有车 + 次车道有车 
					                          
				{
					bHaveCar = true;    
				}
			}
			break;
		case CTRL_SECOND_PRIORITY:
			if ( !bCurMainDrive ) //当前为次相位
			{
				if ( IsHaveCarPhaseGrp( uiCurPhase, ucPhaseIndex , sPhaseDet) )   //次车道有车
				{
					bHaveCar = true;
				}
			}
			else  if ( !bOtherWayCar ) //当前主相位 + 次车道在该步伐一直没有车
			{
				bOtherTmp = IsHaveCarPhaseGrp(uiNextPhase, ucPhaseIndex , sPhaseDet);   //当前次车道是否有车
				if ( bOtherTmp )
				{
					bOtherWayCar = true;
				}

				if ( !bOtherWayCar && IsHaveCarPhaseGrp(uiCurPhase, ucPhaseIndex , sPhaseDet) )  //次车道在该步伐一直没有车 + 主车道有车 
					                    
				{
					bHaveCar = true;    
				}
			}
			break;
		default:
			break;
	}

	if ( bHaveCar )
	{
		//ACE_DEBUG((LM_DEBUG,"\n\n%s:%d now add green time!\n\n",__FILE__,__LINE__));
		CManaKernel::CreateInstance()->AddRunTime(iAdjustTime,ucPhaseIndex);
	}
}


/**************************************************************
Function:        CDetector::IsHaveCarPhaseGrp
Description:    判断某个相位组是否有车
Input:          uiPhase-相位组  pPhaseDet-单个相位与检测器的关系
Output:         ucPhaseIndex - 有车的相位 0 - 15
Return:         true-有车   false-无车
***************************************************************/
bool CDetector::IsHaveCarPhaseGrp(Uint uiPhase,Byte& ucPhaseIndex , SPhaseDetector* pPhaseDet)//uiPhase 阶段长步绿灯阶段放行相位组 取值来源于周期overcycle中的函数
{
	bool bHaveCar  = false;
	int  iIndex    = 0;
	int  iDetCnt   = 0;
	int  iDetIndex = 0;
	int  iDetId    = 0;

	while ( iIndex <  MAX_PHASE  )
	{
		if ( (uiPhase>>iIndex) & 1 )
		{
			ucPhaseIndex = iIndex;  //放行相位
			iDetCnt      = pPhaseDet[iIndex].iRoadwayCnt;//判断该相位有几个检测器

			iDetIndex = 0;
			while ( iDetIndex < iDetCnt )//该相位所有的检测器循环一遍
			{
				iDetId = pPhaseDet[iIndex].iDetectorId[iDetIndex] - 1; //检测器数组下标
				
				if ( iDetId / MAX_DETECTOR_PER_BOARD != 0 )  // 16 - 31 // 即检测器号大于15  16 - 31
				{
					iDetId = iDetId % MAX_DETECTOR_PER_BOARD + m_ucActiveBoard2 * MAX_DETECTOR_PER_BOARD;
				}
				else  // 0 - 15
				{
					iDetId = iDetId % MAX_DETECTOR_PER_BOARD + m_ucActiveBoard1 * MAX_DETECTOR_PER_BOARD;
				}

				//ACE_DEBUG((LM_DEBUG,"%s:%d iDetId:%d \n",__FILE__,__LINE__,iDetId+1));

				if ( (1==m_iDetStatus[iDetId]) && (DET_NORMAL==m_ucDetError[iDetId]) && (m_iDetOccupy[iDetId] < 99 ) )  //有车且检测器没有损坏且占有率小于99
				{
					//ACE_DEBUG((LM_DEBUG,"\n%s:%d Dector No.:%d\n\n",__FILE__,__LINE__,iDetId+1));

					bHaveCar = true; //如果放行相位绿灯长步时候的任何一个检测器有车则中断退出循环 并且返回有车 true 
					break;
				}
				iDetIndex++;
			}
		}

		if ( bHaveCar )
		{
			return true;
		}
		iIndex++;
	}

	return false;
}


/**************************************************************
Function:        CDetector::GetAdaptInfo
Description:   自适应控制
Input:           pDetTimeLen - 检测器有车的时间
Output:         pTotalStat  - 车流量
Return:         无
***************************************************************/
void CDetector::GetAdaptInfo(int* pDetTimeLen , int* pTotalStat)
{
	if ( (NULL == pDetTimeLen) || (NULL == pTotalStat) )
	{
		return;
	}

	ACE_OS::memcpy(pDetTimeLen , m_iAdapDetTimeLen , sizeof(int)*MAX_DETECTOR);   //ÓÐ³µÊ±Œä ËãÕŒÓÐÂÊ
	ACE_OS::memcpy(pTotalStat  , m_iAdapTotalStat  , sizeof(int)*MAX_DETECTOR);   //³µÁ÷Á¿

	ACE_OS::memset(m_iAdapDetTimeLen , 0 , sizeof(int)*MAX_DETECTOR);
	ACE_OS::memset(m_iAdapTotalStat  , 0 , sizeof(int)*MAX_DETECTOR);
}


/**************************************************************
Function:        CDetector::SetStatCycle
Description:    设置采集周期
Input:           ucCycle - 周期
Output:         无
Return:         无
***************************************************************/
void CDetector::SetStatCycle(Byte ucCycle)
{
	m_iTotalDistance = ucCycle;
}


/**************************************************************
Function:        CDetector::RecvDetCan
Description:     主控板处理从检测器板发送过来的CAN数据包
Input:           ucBoardAddr-检测器板地址 
				 sRecvCanTmp-接收到CAN数据帧
Output:         无
Return:         无
***************************************************************/
void CDetector::RecvDetCan(Byte ucBoardAddr,SCanFrame sRecvCanTmp)
{
		
	Byte ucDetBoardIndex = 0;
	Byte ucValueTmp = 0;
	Byte RecvType = sRecvCanTmp.pCanData[0] & 0x3F ;		 
	
	//ACE_DEBUG((LM_DEBUG,"%s:%d Get from DetBoard:%x!\n",__FILE__,__LINE__,ucBoardAddr));
	switch ( ucBoardAddr )
	{
		case BOARD_ADDR_DETECTOR1:    //dector1 			
			ucDetBoardIndex = 0;
			break;
		case BOARD_ADDR_DETECTOR2:
			ucDetBoardIndex = 1;
			break;
		case BOARD_ADDR_INTEDET1:
			ucDetBoardIndex = 2;
			break;
		case BOARD_ADDR_INTEDET2:
			ucDetBoardIndex = 3;
			break;
		default:
			ACE_DEBUG((LM_DEBUG,"%s:%d error\n",__FILE__,__LINE__));
			return;
		}
		
		if ( DET_HEAD_VEHSTS ==RecvType )
		{			
				//数据字节只使用到两个字节
			for ( int i=1; i<sRecvCanTmp.ucCanDataLen; i++ )
			{				
				ucValueTmp = sRecvCanTmp.pCanData[i];			
				Byte iDetId = 0 ;
				for ( int j=0; j<8; j++ )
				{
					iDetId = ucDetBoardIndex*16+(i-1)*8+j ;
					m_iDetStatus[iDetId] = (ucValueTmp >> j) & 0x01;
					if((ucValueTmp >> j) & 0x1)
					{
						m_iDetTimeLen[iDetId] += 1;    //计算占有率
						m_iDetStatus[iDetId]  = 1;     //车道上有车

						m_iAdapDetTimeLen[iDetId] += 1;
						
						if ( 0 == m_iLastDetSts[iDetId] )
						{
							ACE_DEBUG((LM_DEBUG,"%s:%d Detcotr No.:%d has car,%02x!\n",__FILE__,__LINE__,ucDetBoardIndex*16+(i-1)*8+j+1,sRecvCanTmp.pCanData[i]));
							m_ucTotalStat[iDetId] += 1;   //计算车流量 上次无车此次有车才为有车 
							m_iAdapTotalStat[iDetId] += 1; 
						}
					}
					m_iLastDetSts[iDetId] = (ucValueTmp >> j) & 0x01;
				}
			}
	
			
		}
		else if(DET_HEAD_STS == RecvType)
		{			
			Byte ucDecId = 0 ;
			CManaKernel * pManakernel = CManaKernel::CreateInstance() ;
			for ( int i=1; i<sRecvCanTmp.ucCanDataLen; i++ )
			{
				ucValueTmp = sRecvCanTmp.pCanData[i];
			
				for ( int j=0; j<8; j++ )
				{
					if(j%2 == 0)
					{
						ucDecId = 16*ucDetBoardIndex+(i-1)*4+j/2 ;
						if (pManakernel->m_pTscConfig->sDetector[ucDecId].ucDetectorId == 0 || 
							pManakernel->m_pTscConfig->sDetector[ucDecId].ucPhaseId == 0)     
							continue ;
						m_ucDetError[ucDecId] = (ucValueTmp >> j) & 0x3;//
						if (m_ucLastDetError[ucDecId] == m_ucDetError[ucDecId] )
							continue ;
						m_ucLastDetError[ucDecId] = m_ucDetError[ucDecId] ;
						//ACE_DEBUG((LM_DEBUG,"%s:%d m_ucDetError[%d] = %d!\n",__FILE__,__LINE__,ucDecId,m_ucDetError[ucDecId]));
						if((ucValueTmp >> j) & 0x3)
						{
							
							CManaKernel::CreateInstance()->SndMsgLog(LOG_TYPE_DETECTOR, ucDecId+1,m_ucDetError[ucDecId],0,0);			
							ACE_DEBUG((LM_DEBUG,"%s:%d m_ucDetError[%d] bad:%d!\n",__FILE__,__LINE__,ucDecId,(ucValueTmp >> j) & 0x3));
						}
					}
				}
			}	


		}
	else if(DET_HEAD_SEN0108_GET == RecvType|| DET_HEAD_SEN0916_GET == RecvType )
			{
			int iNdex = 0 ;
			if(DET_HEAD_SEN0916_GET == RecvType)
				iNdex =8 ;

			m_ucGetDetDelicacy[ucDetBoardIndex][0+iNdex]= sRecvCanTmp.pCanData[1] &0xf ;
			m_ucGetDetDelicacy[ucDetBoardIndex][1+iNdex]= sRecvCanTmp.pCanData[1]>>4 &0xf ;
			m_ucGetDetDelicacy[ucDetBoardIndex][2+iNdex]= sRecvCanTmp.pCanData[2] &0xf ;
			m_ucGetDetDelicacy[ucDetBoardIndex][3+iNdex]= sRecvCanTmp.pCanData[2]>>4 &0xf ;
			m_ucGetDetDelicacy[ucDetBoardIndex][4+iNdex]= sRecvCanTmp.pCanData[3] &0xf ;
			m_ucGetDetDelicacy[ucDetBoardIndex][5+iNdex]= sRecvCanTmp.pCanData[3]>>4 &0xf ;
			m_ucGetDetDelicacy[ucDetBoardIndex][6+iNdex]= sRecvCanTmp. pCanData[4] &0xf ;
			m_ucGetDetDelicacy[ucDetBoardIndex][7+iNdex]= sRecvCanTmp.pCanData[4]>>4 &0xf ;
			ACE_DEBUG((LM_DEBUG,"%s:%d  DecBoard:%d Senso-Grade-> %d:%x %d:%x %d:%x %d:%x %d:%x %d:%x %d:%x %d:%x\n",__FILE__,__LINE__,ucDetBoardIndex+1,1+iNdex, sRecvCanTmp.pCanData[1] &0xf ,2+iNdex,\
			sRecvCanTmp.pCanData[1]>>4 &0xf,3+iNdex,sRecvCanTmp.pCanData[2] &0xf,4+iNdex,sRecvCanTmp.pCanData[2]>>4 &0xf ,5+iNdex ,sRecvCanTmp.pCanData[3] &0xf,6+iNdex,sRecvCanTmp.pCanData[3]>>4 &0xf,7+iNdex,\
			sRecvCanTmp.pCanData[4] &0xf,8+iNdex, sRecvCanTmp.pCanData[4]>>4 &0xf));

		}
		else if(DET_HEAD_SPEED0104 == RecvType || DET_HEAD_SPEED0508 == RecvType)
		{
			int iNdex = 0 ;
			if(DET_HEAD_SPEED0508 == RecvType)
				iNdex = 4 ;
			m_ucRoadSpeed[ucDetBoardIndex][0+iNdex] = sRecvCanTmp.pCanData[1] ;
			m_ucRoadSpeed[ucDetBoardIndex][1+iNdex] = sRecvCanTmp.pCanData[2] ;
			m_ucRoadSpeed[ucDetBoardIndex][2+iNdex] = sRecvCanTmp.pCanData[3] ;
			m_ucRoadSpeed[ucDetBoardIndex][3+iNdex] = sRecvCanTmp.pCanData[4] ;
			ACE_DEBUG((LM_DEBUG,"%s:%d DecRoadSpeed : m_ucRoadSpeed[%d][%d]:%d m_ucRoadSpeed[%d][%d]:%d m_ucRoadSpeed[%d][%d]:%d m_ucRoadSpeed[%d][%d]:%d\n",__FILE__,__LINE__,ucDetBoardIndex,1+iNdex,sRecvCanTmp.pCanData[1] ,ucDetBoardIndex,2+iNdex,sRecvCanTmp.pCanData[2] ,ucDetBoardIndex,3+iNdex,sRecvCanTmp.pCanData[3], ucDetBoardIndex,4+iNdex,sRecvCanTmp.pCanData[4]));
		}
		else if(DET_HEAD_COIL0104_GET == RecvType || DET_HEAD_COIL0508_GET == RecvType)
		{
			int iNdex = 0 ;
			if(DET_HEAD_COIL0508_GET == RecvType)
				iNdex = 4 ;
			m_sGetLookLink[ucDetBoardIndex][0+iNdex] = sRecvCanTmp.pCanData[1] ;
			m_sGetLookLink[ucDetBoardIndex][1+iNdex] = sRecvCanTmp.pCanData[2] ;
			m_sGetLookLink[ucDetBoardIndex][2+iNdex] = sRecvCanTmp.pCanData[3] ;
			m_sGetLookLink[ucDetBoardIndex][3+iNdex] = sRecvCanTmp.pCanData[4] ;
			ACE_DEBUG((LM_DEBUG,"%s:%d DecLookLink : m_sGetLookLink[%d][%d]:%2x m_sGetLookLink[%d][%d]:%2x m_sGetLookLink[%d][%d]:%2x m_sGetLookLink[%d][%d]:%2x\n",__FILE__,__LINE__,ucDetBoardIndex,1+iNdex,sRecvCanTmp.pCanData[1] ,ucDetBoardIndex,2+iNdex,sRecvCanTmp.pCanData[2] ,ucDetBoardIndex,3+iNdex,sRecvCanTmp.pCanData[3], ucDetBoardIndex,4+iNdex,sRecvCanTmp.pCanData[4]));
		}
		else if(DET_HEAD_DISTAN0104_GET== RecvType || DET_HEAD_DISTAN0508_GET == RecvType)
		{
			int iNdex = 0 ;
			if(DET_HEAD_DISTAN0508_GET == RecvType)
				iNdex = 4 ;
			
			m_ucGetRoadDis[ucDetBoardIndex][0+iNdex] = sRecvCanTmp.pCanData[1] ;
			m_ucGetRoadDis[ucDetBoardIndex][1+iNdex] = sRecvCanTmp.pCanData[2] ;
			m_ucGetRoadDis[ucDetBoardIndex][2+iNdex] = sRecvCanTmp.pCanData[3] ;
			m_ucGetRoadDis[ucDetBoardIndex][3+iNdex] = sRecvCanTmp.pCanData[4] ;
			ACE_DEBUG((LM_DEBUG,"%s:%d DecRoadDis : m_ucGetRoadDis[%d][%d]:%d m_ucGetRoadDis[%d][%d]:%d m_ucGetRoadDis[%d][%d]:%d m_ucGetRoadDis[%d][%d]:%d\n",__FILE__,__LINE__,ucDetBoardIndex,1+iNdex,sRecvCanTmp.pCanData[1] ,ucDetBoardIndex,2+iNdex,sRecvCanTmp.pCanData[2] ,ucDetBoardIndex,3+iNdex,sRecvCanTmp.pCanData[3],ucDetBoardIndex,4+iNdex,sRecvCanTmp.pCanData[4]));
			
		}
		else if(DET_HEAD_FRENCY_GET== RecvType)
		{
			for(int i = 0 ; i < MAX_DETECTOR_PER_BOARD ; i++)			
			{
				 m_ucGetFrency[ucDetBoardIndex][i] = ((sRecvCanTmp.pCanData[i/4+1])>>(i%4)*2) &0x3 ;
				 ACE_DEBUG((LM_DEBUG,"\n%s:%d m_ucGetFrency[%d][%d]:%d\n",__FILE__,__LINE__,ucDetBoardIndex,i,m_ucGetFrency[ucDetBoardIndex][i]));

			}
			
		}
		else if(DET_HEAD_SENDATA0107_GET ==RecvType || DET_HEAD_SENDATA0814_GET ==RecvType || DET_HEAD_SENDATA1516_GET ==RecvType )
		{
			if(DET_HEAD_SENDATA0107_GET ==RecvType)
			{
				for(int ucIndex = 0 ; ucIndex< 7 ;ucIndex++)
				{
					m_ucGetSensibility[ucDetBoardIndex][ucIndex] = sRecvCanTmp.pCanData[ucIndex+1] ;
					printf(" %d  ",m_ucGetSensibility[ucDetBoardIndex][ucIndex]);
				}
				printf("\n");
			}
			else if(DET_HEAD_SENDATA0814_GET ==RecvType )
			{
				for(int ucIndex = 0 ; ucIndex< 7 ;ucIndex++)
				{
					m_ucGetSensibility[ucDetBoardIndex][7+ucIndex] = sRecvCanTmp.pCanData[ucIndex+1] ;
					printf(" %d  ",m_ucGetSensibility[ucDetBoardIndex][7+ucIndex]);
				}
				printf("\n");
			}
			else if(DET_HEAD_SENDATA1516_GET ==RecvType )
			{
				m_ucGetSensibility[ucDetBoardIndex][14] = sRecvCanTmp.pCanData[1];
				m_ucGetSensibility[ucDetBoardIndex][15] = sRecvCanTmp.pCanData[2];
				printf("\n%d   %d \n",m_ucGetSensibility[ucDetBoardIndex][14],m_ucGetSensibility[ucDetBoardIndex][15]);
			}
			
		}

		
}



/**************************************************************
Function:        CDetector::GetAllVehSts
Description:     获取检测器板状态 ADD :201307101054
Input:           QueryType-查询状态类型 有车无车还是线圈状态 
				 ucBdindex-0 1 2 3 检测器板下标
Output:         无
Return:         无
***************************************************************/
void CDetector::GetAllVehSts(Byte QueryType,Byte ucBdindex)
{	
	//ACE_DEBUG((LM_DEBUG,"%s:%d send CAN data to BOARD:%d\n",__FILE__,__LINE__,iNdex));
	switch(ucBdindex)
	{
		case 2 :
			GetVehSts(BOARD_ADDR_INTEDET1,QueryType);			
			break ;
		case 3 :
			GetVehSts(BOARD_ADDR_INTEDET2,QueryType);				
			break ;
		case 0 :
			GetVehSts(BOARD_ADDR_DETECTOR1,QueryType);				
			break ;
		case 1 :
			GetVehSts(BOARD_ADDR_DETECTOR2,QueryType);				
			break ;
		default :
				 ;

	}	
		
	
}


/**************************************************************
Function:        CDetector::GetVehSts
Description:     主控板请求检测器发送16个通道的车辆检测状态 
				 ADD:201307101054
Input:           QueryType-查询状态类型 有车无车还是线圈状态 
				 ucBoardAddr-检测器板地址
Output:         无
Return:         无
***************************************************************/
void CDetector::GetVehSts(Byte ucBoardAddr,Byte QueryType)
{
	SCanFrame sSendFrameTmp;
	ACE_OS::memset(&sSendFrameTmp , 0 , sizeof(SCanFrame));
	
	switch(QueryType)
	{
		case DET_HEAD_VEHSTS :
			Can::BuildCanId(CAN_MSG_TYPE_100 , BOARD_ADDR_MAIN  , FRAME_MODE_P2P  ,ucBoardAddr  , &(sSendFrameTmp.ulCanId));
			sSendFrameTmp.pCanData[0] = ( DATA_HEAD_RESEND << 6 ) | DET_HEAD_VEHSTS; //DATA_HEAD_CHECK ->>> DATA_HEAD_RESEND MOD?2013 0710 1645			
			sSendFrameTmp.ucCanDataLen = 1; //检测是否有车
			//ACE_DEBUG((LM_DEBUG,"%s:%d send CAN data to BOARD_ADDR:%x,QueryType=%d\n",__FILE__,__LINE__,ucBoardAddr,QueryType));
			break ;
		case DET_HEAD_STS :
			Can::BuildCanId(CAN_MSG_TYPE_100 , BOARD_ADDR_MAIN  , FRAME_MODE_P2P  ,ucBoardAddr  , &(sSendFrameTmp.ulCanId));

			sSendFrameTmp.pCanData[0] = ( DATA_HEAD_RESEND << 6 ) | DET_HEAD_STS; ////DATA_HEAD_CHECK ->>> DATA_HEAD_RESEND 需要回复检测器内部数据
			sSendFrameTmp.ucCanDataLen = 1; //检测每块板检测器工作状态
			//ACE_DEBUG((LM_DEBUG,"%s:%d send CAN data to BOARD_ADDR:%x,QueryType=%d\n",__FILE__,__LINE__,ucBoardAddr,QueryType));
			break ;
		default :
			return ;
	}
	
	Can::CreateInstance()->Send(sSendFrameTmp);
}

