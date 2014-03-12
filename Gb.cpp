/***************************************************************
Copyright(c) 2013  AITON. All rights reserved.
Author:     AITON
FileName:   Gb.cpp
Date:       2013-1-1
Description:信号机主程序启动文件.
Version:    V1.0
History:
***************************************************************/

#include "ace/Process_Mutex.h"
#include "ace/SOCK_Dgram_Bcast.h"

#include "TimerManager.h"
#include "TscMsgQueue.h"

#include "ManaKernel.h"
#include "GbtMsgQueue.h"
#include "DbInstance.h"

#include "MainBoardLed.h"
#include "Detector.h"

#include "Gps.h"
#include "Gb.h"
#include "Can.h"
#include "ComFunc.h"


/**************************************************************
Function:        main
Description:    系统入口函数。				
Input:          argc - 参数个数 argv - 参数数组             
Output:         无
Return:         -1  程序启动失败
				0   程序启动成功
***************************************************************/
int main(int argc, char *argv[]) 
{
	ACE_Process_Mutex ationMutex("ation"); //进程互斥量
	int iRetAcquire = ationMutex.tryacquire();
	if ( 0 != iRetAcquire )
	{
		ACE_DEBUG((LM_DEBUG, "\n Error!!!Reason: TSC has already been runned.iRetAcquire = %d\n", iRetAcquire));
		ationMutex.release();
		return -1;
	}
	//CheckSystemTime(); //检查修正系统时间	
	RunGb();	       //系统核心入口函数
	ationMutex.release();
	return 0; 
}


/**************************************************************
Function:        SignalMsgQueue
Description:    信号机控制消息队列线程函数。				
Input:          arg - 线程函数参数        
Output:         无
Return:         0
***************************************************************/
static void* SignalMsgQueue(void *arg)
{
	ACE_DEBUG((LM_DEBUG,"%s:%d  开启信号机控制消息队列线程!\n",__FILE__,__LINE__));	
	CTscMsgQueue::CreateInstance()->DealData();
	return 0;
}


/**************************************************************
Function:        GbtMsgQueue
Description:    gbt消息处理队列线程函数。				
Input:          arg - 线程函数参数        
Output:         无
Return:         0
***************************************************************/
static void* GbtMsgQueue(void* arg)
{
	ACE_DEBUG((LM_DEBUG,"%s:%d  开启gbt消息处理队列线程!\n",__FILE__,__LINE__));
	CGbtMsgQueue::CreateInstance()->DealData();
	return 0;
}


/**************************************************************
Function:        BroadCast
Description:    广播线程函数，回送广播消息，包括IP地址，系统端口，
				系统版本信息。				
Input:          arg - 线程函数参数        
Output:         无
Return:         0
***************************************************************/
static void* BroadCast(void* arg)
{
	ACE_DEBUG((LM_DEBUG,"%s:%d  开启广播线程!\n",__FILE__,__LINE__));
	ACE_INET_Addr addrBroadcast(DEFAULT_BROADCAST_PORT),addrRemote;
	ACE_SOCK_Dgram_Bcast udpBcast(addrBroadcast);
	char buf[10];
	Byte pHwEther[6] = {0};
	Byte pIp[4]      = {0};
	Byte pMask[4]    = {0};
	Byte pGateway[4] = {0};
	Byte sBroadcastMessage[64] = {0};
	Byte ucSendCount = 0;
	CGbtMsgQueue *pGbtMsgQueue = CGbtMsgQueue::CreateInstance();
	pGbtMsgQueue->GetNetPara(pHwEther , pIp , pMask , pGateway);
	Uint iPort = pGbtMsgQueue->iPort ;    //ADD:201309250900 
	ACE_DEBUG((LM_DEBUG,"\nMAC=%02x:%02x:%02x:%02x:%02x:%02x IP= %d.%d.%d.%d MASK=%d.%d.%d.%d GateWay=%d.%d.%d.%d PortNum = %d\n",pHwEther[0],pHwEther[1],pHwEther[2],pHwEther[3],pHwEther[4],pHwEther[5],pIp[0],pIp[1],pIp[2],pIp[3],pMask[0],pMask[1],pMask[2],pMask[3],pGateway[0], pGateway[1],pGateway[2], pGateway[3] ,iPort ));

	for(;;)
	{
		int size = udpBcast.recv(buf,10,addrRemote);

		if ( size > 0 )
		{
			//信号机ip
			pGbtMsgQueue->GetNetPara(pHwEther , pIp , pMask , pGateway);
			ACE_OS::memcpy(sBroadcastMessage , pIp , 4);
			sBroadcastMessage[0] = pIp[0];
			sBroadcastMessage[1] = pIp[1];
			sBroadcastMessage[2] = pIp[2];
			sBroadcastMessage[3] = pIp[3];
			ucSendCount += 4;
			//信号机端口
			(sBroadcastMessage+ucSendCount)[0] = (Byte)((iPort>>24)&0xFF);
			(sBroadcastMessage+ucSendCount)[1] = (Byte)((iPort>>16)&0xFF);
			(sBroadcastMessage+ucSendCount)[2] = (Byte)((iPort>>8)&0xFF);
			(sBroadcastMessage+ucSendCount)[3] = (Byte)(iPort&0xFF);
			ucSendCount += 4;
			//信号机版本
			(sBroadcastMessage+ucSendCount)[0] = 0;
			(sBroadcastMessage+ucSendCount)[1] = 1;
			(sBroadcastMessage+ucSendCount)[2] = 0;
			 ucSendCount += 3;

			udpBcast.send(sBroadcastMessage , ucSendCount , addrRemote);
			ucSendCount = 0;
		}
	}
}

/**************************************************************
Function:       RunGb
Description:    信号机系统核心入口函数，包含7个主要工作线程			
Input:          无        
Output:         无
Return:         无
***************************************************************/
void RunGb()
{
	ACE_thread_t  tThreadId[6];
	ACE_hthread_t hThreadHandle[6];

	(CDbInstance::m_cGbtTscDb).InitDb(DB_NAME);  //数据库类初始化
	
	CManaKernel::CreateInstance()->InitWorkPara();  //初始化信号参数
	RecordTscStartTime();   //记录系统开启时间
	
	/********************************************************************************/
	if ( ACE_Thread::spawn((ACE_THR_FUNC)SignalMsgQueue,  //开启信号核心控制队列
							0,
							THR_NEW_LWP | THR_JOINABLE,
							&tThreadId[0],
							&hThreadHandle[0],
							ACE_DEFAULT_THREAD_PRIORITY,
							0,
							ACE_DEFAULT_THREAD_STACKSIZE,
							0) == -1 )
	{
		TscAceDebug((LM_DEBUG,"Error: SignalMsgQueue thread faild\n"));
	}
	
	/********************************************************************************/
	if ( ACE_Thread::spawn((ACE_THR_FUNC)GbtMsgQueue, //开启通信gbt队列
							0,
							THR_NEW_LWP | THR_JOINABLE,
							&tThreadId[1],
							&hThreadHandle[1],
							ACE_DEFAULT_THREAD_PRIORITY,
							0,
							ACE_DEFAULT_THREAD_STACKSIZE,
							0) == -1 )
	{
		TscAceDebug((LM_DEBUG,"Error: GbtMsgQueue thread faild\n"));
	}

	/********************************************************************************/
	if ( ACE_Thread::spawn((ACE_THR_FUNC)CGbtMsgQueue::RunGbtRecv, //开启Gbt的udp等待数据线程
							0,
							THR_NEW_LWP | THR_JOINABLE,
							&tThreadId[2],
							&hThreadHandle[2],
							ACE_DEFAULT_THREAD_PRIORITY,
							0,
							ACE_DEFAULT_THREAD_STACKSIZE,
							0) == -1 )
	{
		TscAceDebug((LM_DEBUG,"Error: RunGbtRecv thread faild\n"));
	}
	
	/********************************************************************************/
	if ( ACE_Thread::spawn((ACE_THR_FUNC)Can::RunCanRecv, //开启CAN总线数据接收线程
							0,
							THR_NEW_LWP | THR_JOINABLE,
							&tThreadId[3],
							&hThreadHandle[3],
							ACE_DEFAULT_THREAD_PRIORITY,
							0,
							ACE_DEFAULT_THREAD_STACKSIZE,
							0) == -1 )
	{
		TscAceDebug((LM_DEBUG,"Error: RunCanRecv thread faild\n"));
	}
	
	/********************************************************************************/
	if ( ACE_Thread::spawn((ACE_THR_FUNC)Can::DealCanData, //处理CAN接收队列数据
							0,
							THR_NEW_LWP | THR_JOINABLE,
							&tThreadId[4],
							&hThreadHandle[4],
							ACE_DEFAULT_THREAD_PRIORITY,
							0,
							ACE_DEFAULT_THREAD_STACKSIZE,
							0) == -1 )
	{
		TscAceDebug((LM_DEBUG,"Error: RunCanRecv thread faild\n"));
	}

	/********************************************************************************/
	if ( ACE_Thread::spawn((ACE_THR_FUNC)BroadCast, //开启广播响应线程
		0,
		THR_NEW_LWP | THR_JOINABLE,
		&tThreadId[5],
		&hThreadHandle[5],
		ACE_DEFAULT_THREAD_PRIORITY,
		0,
		ACE_DEFAULT_THREAD_STACKSIZE,
		0) == -1 )
	{
		TscAceDebug((LM_DEBUG,"Error: BroadCast thread faild\n"));
	}	
	
	/********************************************************************************/
	CTimerManager::CreateInstance()->CreateAllTimer();   //开启所有的定时器
	
	/********************************************************************************/
	if ( 0 != CManaKernel::CreateInstance()->m_pTscConfig->sSpecFun[FUN_GPS].ucValue )
	{
		if ( ACE_Thread::spawn((ACE_THR_FUNC)CGps::RunGpsData, //开启gps校时线程
								0,
								THR_NEW_LWP | THR_JOINABLE,
								&tThreadId[6],
								&hThreadHandle[6],
								ACE_DEFAULT_THREAD_PRIORITY,
								0,
								ACE_DEFAULT_THREAD_STACKSIZE,
								0) == -1 )
		{
			TscAceDebug((LM_DEBUG,"Error: CGps thread faild\n"));
		}
	}
	
	
	ACE_Thread::join(hThreadHandle[0]);   //回收线程资源
	ACE_Thread::join(hThreadHandle[1]);
	ACE_Thread::join(hThreadHandle[2]);
	ACE_Thread::join(hThreadHandle[3]);
	ACE_Thread::join(hThreadHandle[4]);
	ACE_Thread::join(hThreadHandle[5]);
	ACE_Thread::join(hThreadHandle[6]);
	//ACE_Thread::join(hThreadHandle[7]);

	if ( 0 != CManaKernel::CreateInstance()->m_pTscConfig->sSpecFun[FUN_GPS].ucValue )
	{
		ACE_Thread::join(hThreadHandle[6]);
	}		
	
	CDbInstance::m_cGbtTscDb.CloseDb();  //关闭数据库	

}


