
/***************************************************************
Copyright(c) 2013  AITON. All rights reserved.
Author:     AITON
FileName:   WirelessCoord.cpp
Date:       2013-1-1
Description:�źŻ�����Э��ģʽ�������ļ������ڴ����źŻ�����Э������
Version:    V1.0
History:
***************************************************************/
#include "Coordinate.h"
#include "ManaKernel.h"

#ifndef WINDOWS
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <assert.h>
#include <sys/time.h>
#endif

const int MAX_PLUS_SCALE   = 30;  //һ���������ӵ������� 
const int MAX_MINU_SCALE   = 20;  //ÿ�����ڼ��ٵ�������
const int PLUS_LINE        = 50;  //���ӵĽ���
/**************************************************************
Function:        CWirelessCoord::IsMasterMachine
Description:     �ж��Ƿ�������Э������		
Input:          ��           
Output:         ��
Return:         0
***************************************************************/
Uint CWirelessCoord::IsMasterMachine()
{
	return 0;
}

/**************************************************************
Function:        CWirelessCoord::SyncSubMachine
Description:     �ж��Ƿ�������Э���ӻ�		
Input:          ��           
Output:         ��
Return:         0
***************************************************************/
void CWirelessCoord::SyncSubMachine()
{
	return ;
}


/**************************************************************
Function:        CWirelessCoord::CWirelessCoord
Description:     ����Э���๹�캯������ʼ�������			
Input:          ��           
Output:         ��
Return:         ��
***************************************************************/
CWirelessCoord::CWirelessCoord()
{
	//m_bForceAssort = false;
	m_bPlus       = true;            
	m_bMaster = false;
	m_iUtsCycle   = 0;      
	m_iUtscOffset = 0;    
	m_tLastTi     = 0;
	m_iCycle      = 0;                              
	m_iOffset     = 0;         
	m_iAdjustCnt  = 0;     

	ACE_DEBUG((LM_DEBUG,"create CWirelessCoord\n"));

}

/**************************************************************
Function:        CWirelessCoord::~CWirelessCoord
Description:     ����Э������������		
Input:          ��           
Output:         ��
Return:         ��
***************************************************************/
CWirelessCoord::~CWirelessCoord()
{
	ACE_DEBUG((LM_DEBUG,"delete CWirelessCoord\n"));
}


/**************************************************************
Function:        CWirelessCoord::CreateInstance
Description:     ����CWirelessCoord�������̬����		
Input:          ��           
Output:         ��
Return:         ��̬����ָ��
***************************************************************/
CWirelessCoord* CWirelessCoord::CreateInstance()
{
	static CWirelessCoord cCableless;

	return &cCableless;
}


/**************************************************************
Function:        CWirelessCoord::SetStepInfo
Description:     ��������Э������ģʽ��Ϣ		
Input:          bCenter - �Ƿ�Ϊ����Э��
*        		iStepNum  - ��������
*        		iCycle    - ����ʱ��
*       		iOffset   - Э����λ��
*        		iStepLen - ���������ĳ��� 
				iMaster   ���ڵ���         
Output:         ��
Return:         ��
***************************************************************/
void CWirelessCoord::SetStepInfo(bool bCenter
							 , int iStepNum 
							 , int iCycle 
							 , int iOffset 
							 , int* iStepLen )//,int iMaster
{
	if ( ( !bCenter && ( (iCycle != m_iCycle) || ( iOffset != m_iOffset) ) ) 
		|| ( bCenter && ( (iCycle != m_iUtsCycle) || ( iOffset != m_iUtscOffset) ) ) )  
		                                                       //����ʱ�� ������λ�� �ı���Ҫ����Э��
	{
		m_tLastTi = 0;
	}

	m_iStepNum = iStepNum;
	//ACE_OS::memset(m_iStepLen , 0 , MAX_STEP);
	//for (int i=0; i<iStepNum; i++ )
	//{
		//m_iStepLen[i] = iStepLen[i];
	//}
	ACE_OS::memcpy(m_iStepLen,iStepLen,MAX_STEP);
	if ( bCenter )
	{
		m_bUtcs       = true;
		m_iUtsCycle   = iCycle;
		m_iUtscOffset = iOffset;
	}
	else
	{
		m_bUtcs    = false;
		m_iCycle   = iCycle;
		m_iOffset  = iOffset;
	}
	//ACE_DEBUG((LM_DEBUG,"%s,%d m_iUtsCycle:%d m_iUtscOffset:%d\n",__FILE__,__LINE__,m_iUtsCycle,m_iUtscOffset));

}


/**************************************************************
Function:        CWirelessCoord::GetDeflection
Description:     ��ȡ�ܵ���ʱ��		
Input:          ��  
Output:         ��
Return:         ��
***************************************************************/
void CWirelessCoord::GetDeflection()
{
	int iDeflection = 0;
	int iCycle      = 0;
	int iOffset     = 0;

	if ( m_bUtcs )
	{
		iCycle  = m_iUtsCycle;		
		iOffset = m_iUtscOffset;
	}
	else
	{
		iCycle  = m_iCycle;
		iOffset = m_iOffset;
	}

#ifndef WINDOWS

	time_t tTi, tTi0;
	struct tm *tNow;
	
	tTi  = time(NULL);	
	tNow = localtime(&tTi);

	tNow->tm_hour = 0;
	tNow->tm_min  = 0;
	tNow->tm_sec  = 0;
	tTi0 = mktime(tNow);

	if ( 0 == iCycle )
	{
		m_iAdjustCnt = 0;
		return;
	}

	iDeflection = (tTi - iOffset - tTi0) % iCycle;	
	ACE_DEBUG((LM_DEBUG, "%s:%d iDeflection:%d iCycle:%d iOffset:%d tTi:%d  tTi0:%d\n"
			, __FILE__ , __LINE__  , iDeflection , iCycle , iOffset , tTi , tTi0 ));
	
	if ( 0 == iDeflection )
	{		
		m_iAdjustCnt = 0;
		return;
	}
	/*
	else if ( ( tTi - m_tLastTi ) < ( 12 * 60 * 60 ) ) //12СʱЭ��һ��
	{
		m_iAdjustCnt = 0;
		return;
	}
	*/
	
#else
	iDeflection = 10;
#endif

	m_iAdjustCnt = (iCycle - iDeflection) % iCycle;

	if ( ( m_iAdjustCnt * 100 / iCycle ) > PLUS_LINE )
	{
		m_bPlus      = false;
		m_iAdjustCnt = iCycle - m_iAdjustCnt;
	}
	else
	{
		m_bPlus = true;
	}
	
}

/**************************************************************
Function:        CWirelessCoord::GetAdjust
Description:     ��ȡ���в����ĵ���ʱ��		
Input:          ��  
Output:         ��
Return:         ��
***************************************************************/
void CWirelessCoord::GetAdjust()
{
	int i              = 0;
	int iCycle         = 0;
	int iAdjustPerStep = 0;
	int iMaxAdjustCnt  = 0; 
	int iMinGreen      = 0;
	int iMaxGreen      = 0;
	int iAdjustCnt     = m_iAdjustCnt;
	CManaKernel* pWorkParaManager = CManaKernel::CreateInstance();
	Byte ucStageCnt    = pWorkParaManager->m_pRunData->ucStageCount;

	if ( m_bUtcs )
	{
		iCycle  = m_iUtsCycle;
	}
	else
	{
		iCycle = m_iCycle;
	}

	//for ( i = 0; i < MAX_STEP; i++ )
	//{
	//	m_iAdjustSecond[i] = 0;
	//}

	if ( m_bPlus )
	{
		iMaxAdjustCnt = iCycle * MAX_PLUS_SCALE / 100;   //ÿ�������������ӵĵ�����
	}
	else
	{
		iMaxAdjustCnt = iCycle * MAX_MINU_SCALE / 100;   //ÿ���������ļ��ٵĵ�����
	}

	if ( iMaxAdjustCnt > iAdjustCnt )  //�������������������
	{
		iMaxAdjustCnt = iAdjustCnt;
	}

	for ( i = 0; i < MAX_STEP && i < m_iStepNum; i++ ) //ƽ��
	{
		if ( !pWorkParaManager->IsLongStep(i) )
		{
			continue;
		}
		m_iAdjustSecond[i] = iMaxAdjustCnt / ucStageCnt;  //����ȷ���ĵ���
	}
	iMaxAdjustCnt = iMaxAdjustCnt - ( iMaxAdjustCnt / ucStageCnt ) * ucStageCnt;

	//���ܳ�����С�̻�������̵ķ�Χ������ÿ������С������̷�Χ�������µ���
	for ( i = 0; i < MAX_STEP && i < m_iStepNum; i++ ) 
	{
		if ( !pWorkParaManager->IsLongStep(i) )
		{
			continue;
		}

		iAdjustPerStep = 0;
		iMinGreen      = pWorkParaManager->GetCurStepMinGreen(i,&iMaxGreen,NULL);
		//printf("%s:%d :iMinGreen ==%d iMaxGreen == %d \n",__FILE__,__LINE__,iMinGreen,iMaxGreen);
		if ( m_bPlus )
		{
			iAdjustPerStep = iMaxGreen - m_iStepLen[i];
			//printf("%s:%d m_bPlus == 1, i==%d iAdjustPerStep == %d m_iStepLen[%d]==%d\n",__FILE__,__LINE__,i,iAdjustPerStep,i,m_iStepLen[i]);
		}
		else
		{
			iAdjustPerStep = m_iStepLen[i] - iMinGreen;
			//printf("%s:%d m_bPlus == 0, i==%d iAdjustPerStep == %d m_iStepLen[%d]==%d\n",__FILE__,__LINE__,i,iAdjustPerStep,i,m_iStepLen[i]);
		}

		if ( iAdjustPerStep < 0 )  //Ԥ����С�̻�����̵����ô���
		{
			iAdjustPerStep = m_iStepLen[i] / MAX_ADJUST_CYCLE;
		}

		if ( m_iAdjustSecond[i] > iAdjustPerStep )
		{
			iMaxAdjustCnt      = iMaxAdjustCnt + ( m_iAdjustSecond[i] - iAdjustPerStep );
			m_iAdjustSecond[i] = iAdjustPerStep;
			//printf("%s:%d iMaxAdjustCnt ==%d ,m_iAdjustSecond[%d]==%d\n",__FILE__,__LINE__,iMaxAdjustCnt,i,m_iAdjustSecond[i]);
		}
		else
		{
			iAdjustPerStep = iAdjustPerStep - m_iAdjustSecond[i];
			//printf("%s:%d iAdjustPerStep ==%d \n",__FILE__,__LINE__,iAdjustPerStep);
		}

		if ( iAdjustPerStep > iMaxAdjustCnt )  //�������
		{
			iAdjustPerStep = iMaxAdjustCnt;
		}

		m_iAdjustSecond[i]  = m_iAdjustSecond[i] + iAdjustPerStep;
		iMaxAdjustCnt       = iMaxAdjustCnt - iAdjustPerStep;

		ACE_DEBUG((LM_DEBUG,"%s:%d m_iAdjustSecond[%d]:%d \n",__FILE__,__LINE__,i,m_iAdjustSecond[i]));
	}
}


/**************************************************************
Function:        CWirelessCoord::OverCycle
Description:     һ�����ڿ�ʼ��Ĳ���		
Input:          ��  
Output:         ��
Return:         ��
***************************************************************/
void CWirelessCoord::OverCycle(/*SStepInfo* pRunStepInfo*/)
{
	GetDeflection();
	ACE_DEBUG((LM_DEBUG,"%s:%d m_iAdjustCnt:%d m_bPlus:%d\n" , __FILE__ , __LINE__ , m_iAdjustCnt , m_bPlus));
	ACE_OS::memset(m_iAdjustSecond,0,MAX_STEP);
	if ( 0 == m_iAdjustCnt ) 
	{
		//for (int i = 0; i < MAX_STEP; i++)
		//{
			//m_iAdjustSecond[i] = 0;
		//}		
		return;
	}
	else
	{
		GetAdjust();
	}
	
}


/**************************************************************
Function:        CWirelessCoord::GetStepLength
Description:     ÿ�β�����ʼ����һ��		
Input:          iCurStepNo:��ǰ�Ĳ�����  
Output:         ��
Return:         ���ص�ǰ�����ĳ��� -1:����
***************************************************************/
int CWirelessCoord::GetStepLength(int iCurStepNo)
{
	int iCurSetpTime = 0;

	if ( iCurStepNo > m_iStepNum )  /*�쳣���*/
	{
		ACE_DEBUG((LM_DEBUG,"%s:%d,iCurStepNo:%d m_iStepNum:%d \n",__FILE__,__LINE__,iCurStepNo,m_iStepNum));
		return m_iStepLen[0];
	}

	//m_iAdjustCnt -= m_iAdjustSecond[iCurStepNo];

	if ( m_bPlus )
	{
		iCurSetpTime = m_iStepLen[iCurStepNo] + m_iAdjustSecond[iCurStepNo];
	}
	else
	{
		iCurSetpTime = m_iStepLen[iCurStepNo] - m_iAdjustSecond[iCurStepNo];
	}

	return iCurSetpTime;
}


/**************************************************************
Function:        CWirelessCoord::ForceAssort
Description:     ����ǿ��Э������	
Input:          iCurStepNo:��ǰ�Ĳ�����  
Output:         ��
Return:         ��
***************************************************************/
void CWirelessCoord::ForceAssort()
{
	m_tLastTi = 0;
}

/**************************************************************
Function:        CWirelessCoord::ForceAssort
Description:     ���ý���ʱ�������Ͳ�����������	
Input:          �� 
Output:         ��
Return:         ��
***************************************************************/
 void CWirelessCoord::SetDegrade()
{
	 m_iStepNum = 0 ;                //��������
	 ACE_OS::memset(m_iStepLen,0,MAX_STEP);      //������

}
