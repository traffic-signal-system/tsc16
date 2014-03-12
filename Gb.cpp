/***************************************************************
Copyright(c) 2013  AITON. All rights reserved.
Author:     AITON
FileName:   Gb.cpp
Date:       2013-1-1
Description:�źŻ������������ļ�.
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
Description:    ϵͳ��ں�����				
Input:          argc - �������� argv - ��������             
Output:         ��
Return:         -1  ��������ʧ��
				0   ���������ɹ�
***************************************************************/
int main(int argc, char *argv[]) 
{
	ACE_Process_Mutex ationMutex("ation"); //���̻�����
	int iRetAcquire = ationMutex.tryacquire();
	if ( 0 != iRetAcquire )
	{
		ACE_DEBUG((LM_DEBUG, "\n Error!!!Reason: TSC has already been runned.iRetAcquire = %d\n", iRetAcquire));
		ationMutex.release();
		return -1;
	}
	//CheckSystemTime(); //�������ϵͳʱ��	
	RunGb();	       //ϵͳ������ں���
	ationMutex.release();
	return 0; 
}


/**************************************************************
Function:        SignalMsgQueue
Description:    �źŻ�������Ϣ�����̺߳�����				
Input:          arg - �̺߳�������        
Output:         ��
Return:         0
***************************************************************/
static void* SignalMsgQueue(void *arg)
{
	ACE_DEBUG((LM_DEBUG,"%s:%d  �����źŻ�������Ϣ�����߳�!\n",__FILE__,__LINE__));	
	CTscMsgQueue::CreateInstance()->DealData();
	return 0;
}


/**************************************************************
Function:        GbtMsgQueue
Description:    gbt��Ϣ��������̺߳�����				
Input:          arg - �̺߳�������        
Output:         ��
Return:         0
***************************************************************/
static void* GbtMsgQueue(void* arg)
{
	ACE_DEBUG((LM_DEBUG,"%s:%d  ����gbt��Ϣ��������߳�!\n",__FILE__,__LINE__));
	CGbtMsgQueue::CreateInstance()->DealData();
	return 0;
}


/**************************************************************
Function:        BroadCast
Description:    �㲥�̺߳��������͹㲥��Ϣ������IP��ַ��ϵͳ�˿ڣ�
				ϵͳ�汾��Ϣ��				
Input:          arg - �̺߳�������        
Output:         ��
Return:         0
***************************************************************/
static void* BroadCast(void* arg)
{
	ACE_DEBUG((LM_DEBUG,"%s:%d  �����㲥�߳�!\n",__FILE__,__LINE__));
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
			//�źŻ�ip
			pGbtMsgQueue->GetNetPara(pHwEther , pIp , pMask , pGateway);
			ACE_OS::memcpy(sBroadcastMessage , pIp , 4);
			sBroadcastMessage[0] = pIp[0];
			sBroadcastMessage[1] = pIp[1];
			sBroadcastMessage[2] = pIp[2];
			sBroadcastMessage[3] = pIp[3];
			ucSendCount += 4;
			//�źŻ��˿�
			(sBroadcastMessage+ucSendCount)[0] = (Byte)((iPort>>24)&0xFF);
			(sBroadcastMessage+ucSendCount)[1] = (Byte)((iPort>>16)&0xFF);
			(sBroadcastMessage+ucSendCount)[2] = (Byte)((iPort>>8)&0xFF);
			(sBroadcastMessage+ucSendCount)[3] = (Byte)(iPort&0xFF);
			ucSendCount += 4;
			//�źŻ��汾
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
Description:    �źŻ�ϵͳ������ں���������7����Ҫ�����߳�			
Input:          ��        
Output:         ��
Return:         ��
***************************************************************/
void RunGb()
{
	ACE_thread_t  tThreadId[6];
	ACE_hthread_t hThreadHandle[6];

	(CDbInstance::m_cGbtTscDb).InitDb(DB_NAME);  //���ݿ����ʼ��
	
	CManaKernel::CreateInstance()->InitWorkPara();  //��ʼ���źŲ���
	RecordTscStartTime();   //��¼ϵͳ����ʱ��
	
	/********************************************************************************/
	if ( ACE_Thread::spawn((ACE_THR_FUNC)SignalMsgQueue,  //�����źź��Ŀ��ƶ���
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
	if ( ACE_Thread::spawn((ACE_THR_FUNC)GbtMsgQueue, //����ͨ��gbt����
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
	if ( ACE_Thread::spawn((ACE_THR_FUNC)CGbtMsgQueue::RunGbtRecv, //����Gbt��udp�ȴ������߳�
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
	if ( ACE_Thread::spawn((ACE_THR_FUNC)Can::RunCanRecv, //����CAN�������ݽ����߳�
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
	if ( ACE_Thread::spawn((ACE_THR_FUNC)Can::DealCanData, //����CAN���ն�������
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
	if ( ACE_Thread::spawn((ACE_THR_FUNC)BroadCast, //�����㲥��Ӧ�߳�
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
	CTimerManager::CreateInstance()->CreateAllTimer();   //�������еĶ�ʱ��
	
	/********************************************************************************/
	if ( 0 != CManaKernel::CreateInstance()->m_pTscConfig->sSpecFun[FUN_GPS].ucValue )
	{
		if ( ACE_Thread::spawn((ACE_THR_FUNC)CGps::RunGpsData, //����gpsУʱ�߳�
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
	
	
	ACE_Thread::join(hThreadHandle[0]);   //�����߳���Դ
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
	
	CDbInstance::m_cGbtTscDb.CloseDb();  //�ر����ݿ�	

}


