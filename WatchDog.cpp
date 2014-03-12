
/***************************************************************
Copyright(c) 2013  AITON. All rights reserved.
Author:     AITON
FileName:   WatchDog.cpp
Date:       2013-1-1
Description:���Ź��豸�ļ������ļ�
Version:    V1.0
History:    
***************************************************************/
#include "WatchDog.h"

/**************************************************************
Function:       WatchDog::WatchDog
Description:    WatchDog�๹�캯������ʼ�����Ź�����				
Input:          ��              
Output:         ��
Return:         ��
***************************************************************/
WatchDog::WatchDog()
{
	m_watchdogFd = -1;
	ACE_DEBUG((LM_DEBUG,"%s:%d Init WatchDog object ok !\n",__FILE__,__LINE__));
}

/**************************************************************
Function:       WatchDog::~WatchDog
Description:    WatchDog����������	
Input:          ��              
Output:         ��
Return:         ��
***************************************************************/
WatchDog::~WatchDog()
{
	ACE_DEBUG((LM_DEBUG,"%s:%d Destruct WatchDog object ok !\n",__FILE__,__LINE__));
}

/**************************************************************
Function:       WatchDog::CreateInstance
Description:    ����	WatchDog��̬����
Input:          ��              
Output:         ��
Return:         ��̬����ָ��
***************************************************************/
WatchDog* WatchDog::CreateInstance()
{
	static WatchDog cWatchDog; 

	return &cWatchDog;
}


/**************************************************************
Function:       WatchDog::OpenWatchdog
Description:    �򿪿��Ź��豸�ļ�
Input:          ��              
Output:         ��
Return:         ��
***************************************************************/
void WatchDog::OpenWatchdog()
{
#ifndef WINDOWS
	m_watchdogFd = open(DEV_WATCHDOG, O_WRONLY, 0);
#endif
}


/**************************************************************
Function:       WatchDog::CloseWatchdog
Description:    �رտ��Ź��豸�ļ�
Input:          ��              
Output:         ��
Return:         ��
***************************************************************/
void WatchDog::CloseWatchdog()
{
#ifndef WINDOWS
	if(m_watchdogFd >= 0)
	{
		close(m_watchdogFd);
	}
#endif
}


/**************************************************************
Function:       WatchDog::FillWatchdog
Description:    ι����������������ַ���V�������Թرտ��Ź�����չ��
Input:          ��              
Output:         ��
Return:         ��
***************************************************************/
void WatchDog::FillWatchdog(char cData)
{
#ifndef WINDOWS
	if(m_watchdogFd > 0)
	{
		write(m_watchdogFd, &cData, sizeof(cData));
		//ACE_DEBUG((LM_DEBUG,"%s:%d filldog\n",__FILE__,__LINE__));
	}
#endif

}




