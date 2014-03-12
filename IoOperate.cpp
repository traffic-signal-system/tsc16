#include "IoOperate.h"

CIoOprate::CIoOprate()
{
	ACE_DEBUG((LM_DEBUG,"create CIoOprate\n"));
}

CIoOprate::~CIoOprate()
{
	ACE_DEBUG((LM_DEBUG,"delete CIoOprate\n"));
}

bool CIoOprate::TscWrite(int iDevFd , Byte* pWriteData , int iWriteCnt)
{
	int iReWriteCnt     = 0;    //�ظ�д�Ĵ���
	int iHaveWriteCnt   = 0;    //�Ѿ�д�ĸ���
	int iWriteCntOnePer = 0;    //һ��д�ĸ���

	while ( iHaveWriteCnt < iWriteCnt )
	{
#ifdef LINUX
		iWriteCntOnePer = write(iDevFd, pWriteData+iHaveWriteCnt ,iWriteCnt-iHaveWriteCnt);
#endif
		if ( iWriteCntOnePer > 0 )
		{
			iHaveWriteCnt += iWriteCntOnePer;
		}
		else
		{
			if ( iReWriteCnt++ > 5 )
			{
				ACE_DEBUG((LM_DEBUG,"%s:%d read error %d-%d\n",
					__FILE__, __LINE__,iWriteCnt,iHaveWriteCnt));
				return false;
			}
#ifdef LINUX
			usleep(USLEEP_TIME);
#endif
		}
	}

	return true;
}

bool CIoOprate::TscRead(int iDevFd , Byte* pReadData , int iReadCnt )
{
	int iReReadCnt   = 0;    //�ظ���ȡ�Ĵ���
	int iHaveReadCnt = 0;    //�Ѿ����ĸ���
	int iReadCntOnePer = 0;  //һ�ζ�ȡ�ĸ���

	while ( iHaveReadCnt < iReadCnt )
	{
#ifdef LINUX
		iReadCntOnePer = read(iDevFd, pReadData+iHaveReadCnt ,iReadCnt-iHaveReadCnt);
#endif
		if ( iReadCntOnePer > 0 )
		{
			iHaveReadCnt += iReadCntOnePer;
		}
		else
		{
			if ( iReReadCnt++ > 5 )
			{
				ACE_DEBUG((LM_DEBUG,"%s:%d read error %d-%d\n",
									__FILE__, __LINE__,iReadCnt,iHaveReadCnt));
				return false;
			}

			ACE_DEBUG((LM_DEBUG,"%s:%d read error iReReadCnt:%d %d-%d\n",
				__FILE__, __LINE__,iReReadCnt , iReadCnt,iHaveReadCnt));

#ifdef LINUX
			usleep(USLEEP_TIME);
#endif
		}
	}

	return true;
}