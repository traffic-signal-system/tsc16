/***************************************************************
Copyright(c) 2013  AITON. All rights reserved.
Author:     AITON
FileName:   SerialCtrl.cpp
Date:       2013-9-13
Description:串口处理操作类
Version:    V1.0
History:    
***************************************************************/
#include "SerialCtrl.h"
#include "ace/Synch.h"
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

#define SERIAL0  "/dev/ttySAC0"    //串口0
#define SERIAL1  "/dev/ttySAC1"    //串口1
#define SERIAL2  "/dev/ttySAC2"    //串口2
#define SERIAL3  "/dev/ttySAC3"    //串口3

#define SERIALNUM1  1
#define SERIALNUM2  2
#define SERIALNUM3  3
/**************************************************************
Function:       CSerialCtrl::CSerialCtrl
Description:    CSerialCtrl类构造函数，用于类初始化串口				
Input:          无              
Output:         无
Return:         无
***************************************************************/
CSerialCtrl::CSerialCtrl()
{
	m_iSerial1fd = -1;
	m_iSerial2fd = -1;
	m_iSerial3fd = -1;
	ACE_DEBUG((LM_DEBUG,"%s:%d Init SerialCom object ok !\n",__FILE__,__LINE__));
}

/**************************************************************
Function:       CSerialCtrl::~CSerialCtrl
Description:    CSerialCtrl类析构函数	
Input:          无              
Output:         无
Return:         无
***************************************************************/
CSerialCtrl::~CSerialCtrl()
{
	#ifndef WINDOWS
	if( m_iSerial2fd > 0 )
	{
		close(m_iSerial2fd);
	}
	else if( m_iSerial1fd > 0 )
	{
		close(m_iSerial1fd);
	}
	else if( m_iSerial3fd > 0 )
	{
		close(m_iSerial3fd);
	}
	#endif
	ACE_DEBUG((LM_DEBUG,"%s:%d Destruct SerialCom object ok !\n",__FILE__,__LINE__));
}

/**************************************************************
Function:       CSerialCtrl::CreateInstance
Description:    创建	CSerialCtrl静态对象
Input:          无              
Output:         无
Return:         静态对象指针
***************************************************************/
CSerialCtrl* CSerialCtrl::CreateInstance()
{
	static CSerialCtrl cSerialOperate;

	return &cSerialOperate;
}

/**************************************************************
Function:       CSerialCtrl::OpenSerial
Description:    打开串口设备文件
Input:          无              
Output:         设置按钮文件句柄
Return:         无
***************************************************************/
void CSerialCtrl::OpenSerial(short iSerNum)
{
	int iTmpSerFd = -1 ;
#ifndef WINDOWS
	struct termios terminfo;
	if(iSerNum == SERIALNUM1)
	{
		m_iSerial1fd = open(SERIAL1, O_RDWR | O_NOCTTY | O_NONBLOCK);
		iTmpSerFd = m_iSerial1fd ;
	}
	else if(iSerNum == SERIALNUM2)
	{
		m_iSerial2fd = open(SERIAL2, O_RDWR | O_NOCTTY | O_NONBLOCK);
		iTmpSerFd = m_iSerial2fd ;
	}
	else if(iSerNum == SERIALNUM3)
	{
		m_iSerial3fd = open(SERIAL3, O_RDWR | O_NOCTTY | O_NONBLOCK);
		iTmpSerFd = m_iSerial3fd ;
	}
	
	cfmakeraw(&terminfo);

	terminfo.c_iflag |= IGNBRK | IGNPAR | IXANY;
	terminfo.c_cflag = CREAD | CS8 | CLOCAL | B38400;
	if (tcsetattr(iTmpSerFd, TCSANOW, &terminfo) == -1)
	{
		ACE_DEBUG((LM_DEBUG,"tcsetattr error\n"));
		close(iTmpSerFd);
		if(iSerNum == SERIALNUM1)
		{
			m_iSerial1fd = -1 ;
		}
		else if(iSerNum == SERIALNUM2)
		{
			m_iSerial2fd = -1 ;		
		}
		else if(iSerNum == SERIALNUM3)
		{
			m_iSerial3fd = -1 ;		
		}
	}
#endif
}

/**************************************************************
Function:       CSerialCtrl::GetSerialFd
Description:    获取串口设备句柄
Input:          无              
Output:         无
Return:         串口设备句柄
***************************************************************/
int CSerialCtrl::GetSerialFd(short iSerNum)
{
	int iTmpSerFd = -1 ;
	if(iSerNum == SERIALNUM1)
	{
		iTmpSerFd = m_iSerial1fd  ;
	}
	else if(iSerNum == SERIALNUM2)
	{
		iTmpSerFd = m_iSerial2fd  ;	
	}
	else if(iSerNum == SERIALNUM3)
	{
		iTmpSerFd = m_iSerial3fd  ;	
	}
	if ( iTmpSerFd < 0 )
	{
#ifndef WINDOWS
		close(iTmpSerFd);
#endif
		OpenSerial(iSerNum);
		if(iSerNum == SERIALNUM1)
		{
			iTmpSerFd = m_iSerial1fd  ;
		}
		else if(iSerNum == SERIALNUM2)
		{
			iTmpSerFd = m_iSerial2fd  ;	
		}
		else if(iSerNum == SERIALNUM3)
		{
			iTmpSerFd = m_iSerial3fd  ;	
		}

	}
	return iTmpSerFd;
}



