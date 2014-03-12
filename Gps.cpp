
/***************************************************************
Copyright(c) 2013  AITON. All rights reserved.
Author:     AITON
FileName:   MainBoardLed.cpp
Date:       2013-1-1
Description:信号机GPS校时处理
Version:    V1.0
History:
***************************************************************/
#include "Gps.h"
//#include "Serial2.h"
#include "SerialCtrl.h"
#include "TscMsgQueue.h"
#include "GbtMsgQueue.h"
#include "ManaKernel.h"

#ifndef WINDOWS
#include <strings.h>
#endif 

bool   CGps::m_bNeedGps     = false;
time_t CGps::m_tLastTi      = 0;
int    CGps::m_iGpsFd  	    = CSerialCtrl::CreateInstance()->GetSerialFd(1);
char   CGps::m_cBuf[128]    = {0};
bool   CGps::m_bGpsTime = false ;

/**************************************************************
Function:        CGps::CGps
Description:     CGps类构造函数，初始化类			
Input:          无           
Output:         无
Return:         无
***************************************************************/
CGps::CGps()
{
	
#ifdef TSC_DEBUG
	ACE_DEBUG((LM_DEBUG,"create CGps\n"));
#endif
}

/**************************************************************
Function:       CGps::~CGps
Description:    CGps类析构函数		
Input:          无              
Output:         无
Return:         无
***************************************************************/
CGps::~CGps()
{
#ifdef TSC_DEBUG
	ACE_DEBUG((LM_DEBUG,"delete CGps\n"));
#endif
}

/**************************************************************
Function:       CGps::CGps
Description:    创建CGps类静态对象		
Input:          无              
Output:         无
Return:         CGps静态对象指针
***************************************************************/
CGps* CGps::CreateInstance()
{
	static CGps cGps;
	return &cGps;
}

/**************************************************************
Function:       CGps::RunGpsData
Description:    GPS线程入口函数		
Input:          arg  默认NULL              
Output:         无
Return:         0
***************************************************************/
void* CGps::RunGpsData(void* arg)
{
	char *pBuf   = m_cBuf;
	int iFlagGps = 0;
	
	if ( m_iGpsFd < 0 )
	{
		ACE_DEBUG((LM_DEBUG,"\n%s:%d Open Serial1 erroe!\n",__FILE__,__LINE__));
		return NULL;
	}	
	ACE_DEBUG((LM_DEBUG,"\n%s:%d Begin to adjust GPS time!\n",__FILE__,__LINE__));
	m_bNeedGps = true;
#ifndef WINDOWS
	while ( m_bNeedGps ) 
	{
		
		if ( read(m_iGpsFd, pBuf,1) <= 0 )
		{
			ACE_OS::sleep(ACE_Time_Value(0,100*1000));
			continue;
		}		
		if ( '$' == *pBuf )
		{
			pBuf++;
			iFlagGps = 1;
		}
		else if ( 1 == iFlagGps )
		{
			if ( ('*' == *(pBuf-2) ) || ( (pBuf - m_cBuf) >= (int)sizeof(m_cBuf) - 1) )
			{
#ifdef TSC_DEBUG
				ACE_DEBUG((LM_DEBUG,"\n%s:%d %c\n",__FILE__,__LINE__,*pBuf));
#endif
				*(pBuf+1) = '\n';
				if ( 0 == strncmp(GPRMC, m_cBuf, strlen(GPRMC)) )
				{
					//ACE_DEBUG((LM_DEBUG,"\n%s,%d gps_read %s \n", __FILE__, __LINE__, m_cBuf));					
					if ( CheckSum(m_cBuf) )
					{
						Extract();
					}
					else
					{
						ACE_DEBUG((LM_DEBUG,"\n%s,%d gps_read checksum error! \n", __FILE__, __LINE__));	
					}
				}
				pBuf     = m_cBuf;
				iFlagGps = 0;
				ACE_OS::memset(m_cBuf,0,128);
			}
			else
			{
				pBuf++;
			}
		}
		else
		{
			pBuf = m_cBuf;
		}
	}
	#endif

	return NULL;
}

/**************************************************************
Function:       CGps::Extract
Description:    解析GPS数据		
Input:          无              
Output:         无
Return:         0-失败 1-成功
***************************************************************/
int CGps::Extract()
{
	char  cValid = 'V';
	char* pBuf   = m_cBuf;
	int   iYear  = 0;
	int   iMon   = 0;
	int   iDay   = 0;
	int   iHour  = -1;
	int   iMin   = -1;
	int   iSec   = -1;
	int   iMsec  = -1;
	int   iIndex = 0;
	//static int iGpsLed = LED_GPS_OFF;	
	
#ifndef WINDOWS
	sscanf(pBuf, GPRMC "," "%02d%02d%02d.%03d,%c,"  "%*s\r\n", &iHour, &iMin, &iSec, &iMsec, &cValid);

	for ( iIndex = 0; pBuf && iIndex < 9; iIndex++ ) 
	{
		pBuf = index(pBuf, ',');
		if ( pBuf == NULL )
		{
			return 0;
		}
		pBuf++;
	}

	if ( pBuf )
	{
		sscanf(pBuf, "%02d%02d%02d,%*s\r\n", &iDay, &iMon, &iYear);
	}

	if (! (iYear >= 0 && iYear <= 99 && iMon >= 1  && iMon <= 12 && iDay  >= 1  && iDay <= 31 && iHour >= 0 && iHour <= 23 && iMin >= 0  &&
		 iMin <= 59 && iSec  >= 0  && iSec <= 59 ))
	
	{
		return 0;
	}

	//ACE_DEBUG((LM_DEBUG,"GPS get time %d-%d-%d %d:%02d:%02d %c\n\n", 2000 + iYear, iMon, iDay, iHour, iMin, iSec, cValid) );

	if ( cValid != 'A' )
	{
		return 0;
	}

	if ( CManaKernel::CreateInstance()->m_bFinishBoot )  //过度步后才需要校时
	{
		SetTime(2000 + iYear , iMon , iDay , iHour , iMin , iSec );
	}
	
#endif	
	return 1;
}


/**************************************************************
Function:       CGps::CheckSum
Description:    校验GPS数据		
Input:          cMsg  GPS数据指针              
Output:         无
Return:         false-失败 true-成功
***************************************************************/
bool CGps::CheckSum(char *cMsg)
{
	char *pBuf         = NULL;
	int iChecksum      = 0;
	Byte ucSum         = 0;

#ifndef WINDOWS
	if ( NULL == cMsg )
	{
		return false;
	}

	if ( NULL == (pBuf = index(cMsg, '*')) )
	{
		return false;
	}

	if ( sscanf(pBuf, "*%X\r\n", &iChecksum) != 1 )	
	{		
		return false;
	}	
	if ( '$' == *cMsg )
	{
		cMsg++;
	}
	while ( cMsg && *cMsg != '*' ) 
	{
		ucSum ^= *cMsg;
		cMsg++;
	}
	//ACE_DEBUG((LM_DEBUG,"receive checksum is %02X and the calculation is %02X\n", iChecksum, ucSum ));
#endif
	
	if ( ucSum == iChecksum ) 
	{
		return true;
	} 
	else
	{
		return false;
	}
}


/**************************************************************
Function:       CGps::SetTime
Description:    设置系统时间		
Input:          iYear-年    iMon-月      iDay-日  
				iHour-时    iMin-分      iSec-秒
Output:         无
Return:         无
***************************************************************/
void CGps::SetTime(int iYear , int iMon , int iDay, int iHour, int iMin, int iSec)
{
#ifndef WINDOWS
	time_t Ttime;
	struct tm *pTheTime, *pLocalTime;
	FILE *fpGps = NULL;
	SThreadMsg sTscMsg;

	Ttime = time(NULL);
	pTheTime = localtime(&Ttime);
	pTheTime->tm_year = iYear - 1900;
	pTheTime->tm_mon  = iMon - 1;
	pTheTime->tm_mday = iDay;
	pTheTime->tm_hour = iHour;
	pTheTime->tm_min  = iMin;
	pTheTime->tm_sec  = iSec;
	Ttime = mktime(pTheTime);

	Ttime += 8 * 60 * 60;
	if ( (Ttime > m_tLastTi) && (Ttime - m_tLastTi < 14400 ) )  
	{	
		m_bGpsTime = false ;
		return;
	}
	m_bGpsTime = true ;
	m_tLastTi = Ttime ;

	pLocalTime = localtime(&Ttime);	
	fpGps = fopen(GPSFILE,"a+");
	if ( fpGps != NULL )
	{
		fprintf(fpGps, "gps time: %d-%d-%d %d:%02d:%02d\n", pLocalTime->tm_year+1900, pLocalTime->tm_mon+1,                               						   pLocalTime->tm_mday,pLocalTime->tm_hour, pLocalTime->tm_min, pLocalTime->tm_sec);
		fclose(fpGps);
	}
	Ttime += 8 * 60 * 60;
	sTscMsg.ulType       = TSC_MSG_CORRECT_TIME;
	sTscMsg.ucMsgOpt     = OBJECT_UTC_TIME;
	sTscMsg.uiMsgDataLen = 4;
	sTscMsg.pDataBuf     = ACE_OS::malloc(4);
	*((Byte*)sTscMsg.pDataBuf+3)  = Ttime & 0xff;  
	*((Byte*)sTscMsg.pDataBuf+2)  = (Ttime>>8) & 0xff;
	*((Byte*)sTscMsg.pDataBuf+1)  = (Ttime>>16) & 0xff;
	*((Byte*)sTscMsg.pDataBuf)    = (Ttime>>24) & 0xff;
	CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));
#endif
}

/**************************************************************
Function:       CGps::ForceAdjust
Description:    设置强制校时	属性
Input:          无
Output:         无
Return:         无
***************************************************************/
void CGps::ForceAdjust()
{
	m_tLastTi = 0;
}


