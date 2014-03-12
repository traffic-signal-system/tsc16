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
	int iReWriteCnt     = 0;    //重复写的次数
	int iHaveWriteCnt   = 0;    //已经写的个数
	int iWriteCntOnePer = 0;    //一次写的个数

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
	int iReReadCnt   = 0;    //重复读取的次数
	int iHaveReadCnt = 0;    //已经读的个数
	int iReadCntOnePer = 0;  //一次读取的个数

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
