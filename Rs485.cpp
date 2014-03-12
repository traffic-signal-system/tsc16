
/***************************************************************
Copyright(c) 2013  AITON. All rights reserved.
Author:     AITON
FileName:   Rs485.cpp
Date:       2013-1-1
Description:Rs485���ڴ����ļ�
Version:    V1.0
History:
***************************************************************/
#include "Rs485.h"
#include "ManaKernel.h"
#include "SerialCtrl.h"
#include "IoOperate.h"
#include "MainBoardLed.h"

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

/**************************************************************
Function:       CRs485::CRs485
Description:    CRs485�๹�캯������ʼ������
Input:          ��                 
Output:         ��
Return:         ��
***************************************************************/
CRs485::CRs485()
{
	m_iRs485Led = LED_RS485_OFF;
	OpenRs485();
	ACE_DEBUG((LM_DEBUG,"%s:%d Init RS485 object ok !\n",__FILE__,__LINE__));
}

/**************************************************************
Function:       CRs485::~CRs485
Description:    CRs485�������������ͷ��������Դ
Input:          ��               
Output:         ��
Return:         ��
***************************************************************/
CRs485::~CRs485()
{
	if ( m_iRs485Ctrlfd > 0 )
	{
#ifndef WINDOWS
		close(m_iRs485Ctrlfd);
#endif
	}
	ACE_DEBUG((LM_DEBUG,"%s:%d Destruct Rs485 object ok !\n",__FILE__,__LINE__));
}

/**************************************************************
Function:       CRs485::CreateInstance
Description:    ����	CRs485��̬����
Input:          ��              
Output:         ��
Return:         ��̬����ָ��
***************************************************************/
CRs485* CRs485::CreateInstance()
{
	static CRs485 oRs485;

	return &oRs485;
}

/**************************************************************
Function:       CRs485::OpenRs485
Description:    ��R485����
Input:          ��              
Output:         ��
Return:         ��̬����ָ��
***************************************************************/
void CRs485::OpenRs485()
{
#ifndef WINDOWS 
	m_iRs485Ctrlfd = open(RS485, 0);
#endif
	 m_iRs485DataFd = CSerialCtrl::CreateInstance()->GetSerialFd(3);
	//m_iRs485DataFd = Serial2::CreateInstance()->GetSerial2Fd();

	if( m_iRs485Ctrlfd < 0 || m_iRs485DataFd < 0 )
	{
		ACE_DEBUG((LM_DEBUG,"%s:%d open dev error\n",__FILE__,__LINE__));
		return;
	}

	#ifdef GA_COUNT_DOWN
	SetOpt(m_iRs485DataFd, 9600, 8, 'N', 1);
	#else
	SetOpt(m_iRs485DataFd, 9600, 8, 'N', 1);
	#endif
		
#ifndef WINDOWS 
	//ioctl(m_iRs485DataFd, com_9bit_save_old, 0);
#endif
}



/*
*��485��������
*/
bool CRs485::Send(Byte* pBuffer, int iSize)
{
	//STscConfig* pTscCfg = CManaKernel::CreateInstance()->m_pTscConfig;

#ifndef WINDOWS
	ioctl(m_iRs485Ctrlfd, 1,5);
#endif

	/*
	if ( ( (pTscCfg->sSpecFun[FUN_PRINT_FLAGII].ucValue) & 1 ) != 0 )
	{
		Byte ucIndex = 0;
		ACE_DEBUG((LM_DEBUG,"%s:%d send %d Bytes to rs485: ",RS485_CPP, __LINE__, iSize));
		while ( ucIndex < iSize )
		{
			ACE_DEBUG((LM_DEBUG,"%02x  ",pBuffer[ucIndex]));
			ucIndex++;
		}
		ACE_DEBUG((LM_DEBUG,"\n") );
	}
	*/

	if ( !CIoOprate::TscWrite(m_iRs485DataFd, pBuffer, iSize) )	 
	{
		ACE_DEBUG((LM_DEBUG,"%s:%d write error\n",__FILE__,__LINE__));
		return false;
	}

	if ( LED_RS485_ON == m_iRs485Led  )
	{
		m_iRs485Led	= LED_RS485_OFF;
	}
	else
	{
		m_iRs485Led	= LED_RS485_ON;
	}
//	CMainBoardLed::CreateInstance()->OperateLed(m_iRs485Led);

	return true;

}

/*
*��485��������
*/
bool CRs485::Recvice(Byte* pBuffer , int iSize)
{
	//STscConfig* pTscCfg = CManaKernel::CreateInstance()->m_pTscConfig;

#ifndef WINDOWS
	ioctl(m_iRs485Ctrlfd, 0,5);
#endif

	if ( !CIoOprate::TscRead(m_iRs485DataFd, pBuffer, iSize) )	// ���rs485��������,���´��豸1��
	{
		//ACE_DEBUG((LM_DEBUG,"%s:%d read error\n",__FILE__,__LINE__));
		return false;
	}

	//ACE_DEBUG((LM_DEBUG,"%s:%d recevice %d Bytes from rs485: %02x %02x %02x %02x \n\n",
		//__FILE__, __LINE__, iSize, pBuffer[0] , pBuffer[1] , pBuffer[2] , pBuffer[3]) );

	/*
	if ( ( (pTscCfg->sSpecFun[FUN_PRINT_FLAGII].ucValue) & 1 ) != 0 )
	{
		ACE_DEBUG((LM_DEBUG,"%s:%d recevice from rs485(%d): %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
			__FILE__, __LINE__, iSize, 
			pBuffer[0] , pBuffer[1] , pBuffer[2] , pBuffer[3] ,  pBuffer[4] 
		,  pBuffer[5] , pBuffer[6] , pBuffer[7] , pBuffer[8] ,  pBuffer[9] 
		,  pBuffer[10], pBuffer[11], pBuffer[12], pBuffer[13],  pBuffer[14]
		,  pBuffer[15], pBuffer[16]) );
	}
	*/

	return true;
}

void CRs485::Reopen()
{
#ifndef WINDOWS
	close(m_iRs485Ctrlfd);
#endif

	OpenRs485();
}

// fd: device handle;
// nSpeed: baudrate
// nBits: data bit;
// nEvent: parity;
// nStop: stop bit;
/*
*�����豸����
*
*input: fd-�豸id        nSpeed-������ nBits-����λ
*       nEvent-��żУ��λ nStop-ֹͣλ
*/
int CRs485::SetOpt(int fd,int nSpeed,int nBits, char nEvent, int nStop)
{
#ifndef WINDOWS
	struct termios newtio,oldtio;

	if(tcgetattr(fd,&oldtio) != 0)
	{
		printf("SetupSerial 1\n");
		return -1;
	}

	bzero(&newtio,sizeof(newtio));
	newtio.c_cflag |= CLOCAL | CREAD;
	newtio.c_cflag &= ~CSIZE;

	switch(nBits)
	{
	case 7:
		newtio.c_cflag |= CS7;
		break;

	case 8:
		newtio.c_cflag |= CS8;
		break;
	}

	switch(nEvent)
	{
	case 'o':
	case 'O':
		newtio.c_cflag |= PARENB;
		newtio.c_cflag |= PARODD;
		newtio.c_iflag |= (INPCK | ISTRIP);
		break;

	case 'e':
	case 'E':
		newtio.c_cflag |= PARENB;
		newtio.c_cflag &= ~PARODD;
		newtio.c_iflag |= (INPCK | ISTRIP);
		break;

	case 'n':
	case 'N':
		newtio.c_cflag &= ~PARENB;
		break;
	}

	switch(nSpeed)
	{
	case 2400:
		cfsetispeed(&newtio,B2400);
		cfsetospeed(&newtio,B2400);
		break;

	case 4800:
		cfsetispeed(&newtio,B4800);
		cfsetospeed(&newtio,B4800);
		break;

	case 9600:
		cfsetispeed(&newtio,B9600);
		cfsetospeed(&newtio,B9600);
		break;

	case 1152000:
		cfsetispeed(&newtio,B115200);
		cfsetospeed(&newtio,B115200);
		break;

	default:
		cfsetispeed(&newtio,B19200);
		cfsetospeed(&newtio,B19200);
		break;
	}


	if(nStop == 2)
	{
		newtio.c_cflag |= CSTOPB;
	}
	else
	{
		newtio.c_cflag &= ~CSTOPB;
	}

	newtio.c_lflag  &= ~(ICANON | ECHO | ECHOE | ISIG);  /*Input*/
	newtio.c_oflag  &= ~OPOST;   /*Output*/

	tcflush(fd,TCIFLUSH);

	if((tcsetattr(fd,TCSANOW, &newtio)) != 0)
	{
		printf("SetupSerial 2\n");
		return -1;
	}
#endif

	return 0;
}
