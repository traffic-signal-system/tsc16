#ifndef _CRS485_H_
#define _CRS485_H_

#include "ComStruct.h"
#define RS485_CPP "CRs485.cpp"

/*
*´®¿Ú1²Ù×÷Àà
*/
class CRs485
{
public:
	static CRs485* CreateInstance();
	static int SetOpt(int fd,int nSpeed,int nBits, char nEvent, int nStop);
	void OpenRs485();
	bool Send(Byte* pBuffer, int iSize);
	bool Recvice(Byte* pBuffer , int iSize);
	void Reopen();

private:
	CRs485();
	~CRs485();

	int  m_iRs485Ctrlfd;
	int  m_iRs485DataFd;
	int  m_iRs485Led;
};

#endif //_CRS485_H_
