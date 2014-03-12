#include "TimerManager.h"
#include "ComFunc.h"

//��ʱ���±�
enum 
{
	TIMER_TSC_INDEX    = 0,  //�źŻ�����ģ��Ķ�ʱ��
	TIMER_GBT_INDEX    = 1,  //ͨ��Э��ģ��Ķ�ʱ��
};


CTimerManager::CTimerManager()
{
	for (int iIndex=0; iIndex<MAX_TIMER; iIndex++)
	{
		m_lTimerId[iIndex] = 0;
	}

	m_pTscTimer = NULL;
	m_pGbtTimer = NULL;
	
	m_tActiveTimer.timer_queue()->gettimeofday(getCurrTime);
	m_tActiveTimer.activate();
	ACE_DEBUG((LM_DEBUG,"%s:%d Init TimerManager object ok !\n",__FILE__,__LINE__));
}

CTimerManager::~CTimerManager()
{
	CloseAllTimer();

	if ( m_pTscTimer != NULL )
	{
		ACE_OS::free(m_pTscTimer);
	}

	ACE_DEBUG((LM_DEBUG,"%s:%d Destruct TimerManager object ok !\n",__FILE__,__LINE__));
}

CTimerManager* CTimerManager::CreateInstance()
{
	static CTimerManager tTimerManager;

	return &tTimerManager;   //�Զ�����
}

/*
*�������еĶ�ʱ��
*/
void CTimerManager::CreateAllTimer()
{

	ACE_DEBUG((LM_DEBUG,"%s : %d ��ʱ����TSC GBT ��ʱ��!\n",__FILE__,__LINE__));
	m_pTscTimer = new CTscTimer(10);
	m_pGbtTimer = CGbtTimer::CreateInstance();
	
    const ACE_Time_Value curr_tv = getCurrTime();
	//100���붨ʱ��,�ӳ�3������
	m_lTimerId[TIMER_TSC_INDEX] = m_tActiveTimer.schedule(m_pTscTimer,NULL,curr_tv+ACE_Time_Value(1),ACE_Time_Value(0,100*1000)); 

	//100���뾫ȷ��ʱ��������os���ʱ��
	//m_lTimerId[TIMER_TSC_INDEX ] =  m_tActiveTimer.schedule(m_pTscTimer,  NULL,GetCurTime(),ACE_Time_Value(0,100*1000)); 

	m_lTimerId[TIMER_GBT_INDEX] = m_tActiveTimer.schedule(m_pGbtTimer,NULL,curr_tv+ACE_Time_Value(1),ACE_Time_Value(0,100*1000)); 
}

/*
*�ر����еĶ�ʱ��
*/
void CTimerManager::CloseAllTimer()
{
	for (int iIndex=0; iIndex<MAX_TIMER; iIndex++)
	{
		if ( m_lTimerId[iIndex] > 0 )
		{
			m_tActiveTimer.cancel(m_lTimerId[iIndex]);
		}
	}
}

/*
*�������еĶ�ʱ��
*/
void CTimerManager::StartAllTimer()
{
	//100���붨ʱ��,,��������
	m_lTimerId[TIMER_TSC_INDEX] = m_tActiveTimer.schedule(m_pTscTimer,NULL,getCurrTime(),ACE_Time_Value(0,100*1000)); 

	m_lTimerId[TIMER_GBT_INDEX] = m_tActiveTimer.schedule(m_pGbtTimer,NULL,getCurrTime(),ACE_Time_Value(0,100*1000)); 
}

/*
*����Tscģ��Ķ�ʱ��
*/
void CTimerManager::StartTscTimer()
{
	m_lTimerId[TIMER_TSC_INDEX] =   //100���붨ʱ��,��������
		m_tActiveTimer.schedule(m_pTscTimer,NULL,getCurrTime(),ACE_Time_Value(0,100*1000)); 
}

/*
*�ر�Tscģ��Ķ�ʱ��
*/
void CTimerManager::CloseTscTimer()
{
	if ( m_lTimerId[TIMER_TSC_INDEX] > 0 )
	{
		m_tActiveTimer.cancel(m_lTimerId[TIMER_TSC_INDEX]);
	}
}


