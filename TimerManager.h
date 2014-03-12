#ifndef __TIMERMANAGER_H_
#define __TIMERMANAGER_H_

#include "ComStruct.h"
#include "TscTimer.h"
#include "GbtTimer.h"
//#include "ComDef.h"

const int MAX_TIMER = 2;   //���Ķ�ʱ������ 
/*
*��������Ŀ�Ķ�ʱ���Ĺ���������ʼ��������ɾ��
*����ʵ������
*/
class CTimerManager
{
public:
	static CTimerManager* CreateInstance();
	void CreateAllTimer();
	void CloseAllTimer(); 
	void StartAllTimer(); 
	void CloseTscTimer();
	void StartTscTimer();
private:
	CTimerManager();
	~CTimerManager();

	long m_lTimerId[MAX_TIMER];
	ActiveTimer m_tActiveTimer;
	CTscTimer* m_pTscTimer;
	CGbtTimer* m_pGbtTimer;
};

#endif //_TIMERMANAGER_H_

