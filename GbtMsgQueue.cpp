/***************************************************************
Copyright(c) 2013  AITON. All rights reserved.
Author:     AITON
FileName:   GbtMsgQueue.cpp
Date:       2013-1-1
Description:�źŻ�GBTЭ����Ϣ������
Version:    V1.0
History:
***************************************************************/
#include "ace/Synch.h"
#include "ace/Timer_Heap.h"
#include "ace/OS.h"
#include "ace/Guard_T.h" 
#include "ace/Date_Time.h"

#include "DbInstance.h"
#include "GbtMsgQueue.h"
#include "TscMsgQueue.h"
#include "ManaKernel.h"
#include "GbtDb.h"
#include "FlashMac.h"
#include "PowerBoard.h"
#include "Detector.h"
#include "LampBoard.h"
#include "GbtTimer.h"
#include "Gps.h"
#include "Can.h"
#include "PscMode.h"
#include "MainBoardLed.h"
#include "ComFunc.h"

/**************************************************************
Function:        CGbtMsgQueue::CGbtMsgQueue
Description:     GBT CGbtMsgQueue�๹�캯������ʼ����			
Input:          ��           
Output:         ��
Return:         ��
***************************************************************/
CGbtMsgQueue::CGbtMsgQueue()
{

#ifdef NTCIP
	int iLen = 0;
	timeval tTmp;
	ACE_Message_Block *mb = NULL;
	SThreadMsg sMsg; 
	ACE_Time_Value nowait(GetCurTime());
#endif

	iPort     = 0;
	m_pMsgQue = ACE_Message_Queue_Factory<ACE_MT_SYNCH>::create_static_message_queue();
	
	STscConfig* pTscCfg = CManaKernel::CreateInstance()->m_pTscConfig;

	for ( Byte i=0; i<MAX_CLIENT_NUM; i++ )
	{
		m_sGbtDealData[i].bIsDeal = false;
	}
	
	iPort |= pTscCfg->sSpecFun[FUN_PORT_LOW].ucValue;
	iPort |= pTscCfg->sSpecFun[FUN_PORT_HIGH].ucValue << 8;
	
	if ( iPort > MAX_GBT_PORT || iPort < MIN_GBT_PORT )
	{
		iPort = DEFAULT_GBT_PORT; //UDP����ͨ�Ŷ˿�
	}
	m_addrLocal.set_port_number(iPort);

#ifdef GBT_TCP
	if ( -1 == m_acceptor.open(m_addrLocal,1) )   //�󶨶˿�
	{
		ACE_DEBUG((LM_DEBUG,"%s:%d bind port fail",__FILE__,__LINE__));
	}
#else
	if ( -1 == m_sockLocal.open(m_addrLocal) )
	{
		ACE_DEBUG((LM_DEBUG,"%s:%d bind port fail",__FILE__,__LINE__));
	}
#endif
ACE_DEBUG((LM_DEBUG,"%s:%d Init GbtMsgQuete object ok !\n",__FILE__,__LINE__));
}


/**************************************************************
Function:       CGbtMsgQueue::~CGbtMsgQueue
Description:    CGbtMsgQueue����������		
Input:          ��              
Output:         ��
Return:         ��
***************************************************************/
CGbtMsgQueue::~CGbtMsgQueue()
{
	if ( m_pMsgQue != NULL )
	{
		delete m_pMsgQue;
		m_pMsgQue = NULL;
	}	
	ACE_DEBUG((LM_DEBUG,"%s:%d Destruct GbtMsgQuete object ok !\n",__FILE__,__LINE__));
}

/**************************************************************
Function:       CGbtMsgQueue::CreateInstance
Description:    ����CGbtMsgQueue�ྲ̬����		
Input:          ��              
Output:         ��
Return:         CGbtMsgQueue��̬����ָ��
***************************************************************/
CGbtMsgQueue* CGbtMsgQueue::CreateInstance()
{
	static CGbtMsgQueue cGbt;
	return &cGbt;
}


/**************************************************************
Function:       CGbtMsgQueue::SendGbtMsg
Description:    ��gbt��Ϣ���з���gbt��Ϣ		
Input:          pMsg  ��Ϣ���ݽṹ��ָ��   
				iLength  ��Ϣ����         
Output:         ��
Return:         0-����ʧ��  1-���ͳɹ�
***************************************************************/
int CGbtMsgQueue::SendGbtMsg(SThreadMsg* pMsg,int iLength)
{
	if ( pMsg==NULL || m_pMsgQue==NULL )
	{
#ifdef TSC_DEBUG
		ACE_DEBUG((LM_DEBUG,"pMsg or m_pMsgQue is NULL"));
#endif
		return 0;
	}
	
	ACE_Message_Block *mb = new ACE_Message_Block(iLength);  //������Ϣ��
	mb->copy((char*)pMsg, iLength); // �����ݿ�������Ϣ��

	ACE_Time_Value nowait(GetCurTime()+ACE_Time_Value(1));
	m_pMsgQue->enqueue_tail(mb, &nowait);  //�� ACE_Message_Queue����������ݿ�

	return 1;
}


/**************************************************************
Function:       CGbtMsgQueue::DealData
Description:    ����gbt��Ϣ��������		
Input:          ��   
Output:         ��
Return:         ��
***************************************************************/
void CGbtMsgQueue::DealData()
{
	int iLen = 0;
	timeval tTmp;
	ACE_Message_Block *mb = NULL;
	SThreadMsg sMsg; 
	ACE_Time_Value nowait(GetCurTime());

	tTmp.tv_sec = 0;
	tTmp.tv_usec = 10 * 1000;

	while ( m_pMsgQue != NULL )
	{
		if(m_pMsgQue->dequeue_head(mb, &nowait) != -1) //�� ACE_Message_Queue �е�����Ϣ��
		{   
			iLen = (int)mb->length();
			memcpy((char*)&sMsg, mb->base(), iLen);   //����Ϣ���ж�����
			mb->release();
		}
		else
		{
			ACE_OS::sleep(ACE_Time_Value(tTmp));   //��ͣ10����
			continue;
		}		
		//ACE_DEBUG((LM_DEBUG,"\n%s:%d  type:%d opt:%d dataLen:%d\n",__FILE__,__LINE__,sMsg.ulType,sMsg.ucMsgOpt,sMsg.uiMsgDataLen));//MOD:0515 17:05

		switch ( sMsg.ulType )   //��Ϣ����
		{
		case GBT_MSG_FIRST_RECV:   //�������յ������ݣ���һ�δ���λ�����յ���UDPЭ������ ������gbttimer������ʱ�����͹���������
			if ( 1 == CheckMsg(sMsg.ucMsgOpt,sMsg.uiMsgDataLen,(Byte*)sMsg.pDataBuf) ) //�����յ�GBTЭ�����ݺϷ���,�������ʹ���Ӧ��!
			{
				FirstRecv(sMsg.ucMsgOpt,sMsg.uiMsgDataLen,(Byte*)sMsg.pDataBuf);//sMsg.ucMsgOpt ������ʾ��Ϣ�����ĸ����ջ����
			}
			break;

		case GBT_MSG_DEAL_RECVBUF:  //�������յ���BUF
			DealRecvBuf(sMsg.ucMsgOpt);
			break;
			
		case GBT_MSG_SEND_HOST:   //���͸���λ��
			SendToHost(sMsg.ucMsgOpt);
			break;
			
		case GBT_MSG_TSC_STATUS:  //��ȡ�źŻ�״̬  //������źŻ�TSC�̷߳��͹�����GBT��Ϣ������������λ����
			PackTscStatus(sMsg.ucMsgOpt,sMsg.pDataBuf);
			break;

		case GBT_MSG_TSC_EXSTATUS: //��ȡ�źŻ���չ���� F8
			PackTscExStatus(sMsg.ucMsgOpt,sMsg.pDataBuf);
			break;

		case GBT_MSG_OTHER_OBJECT: //gbt���������
			PackOtherObject(sMsg.ucMsgOpt);
			break;

		case GBT_MSG_EXTEND:
			PackExtendObject(sMsg.ucMsgOpt);
			break;
			
		case GBT_MSG_SELF_REPORT:  //�����ϱ�
			SelfReport(sMsg.uiMsgDataLen,(Byte*)sMsg.pDataBuf);
			break;
			
		default:
			break;
		}

		if ( sMsg.pDataBuf != NULL )
		{
			ACE_OS::free(sMsg.pDataBuf);  //ɾ��������ڴ�
			sMsg.pDataBuf = NULL;
		}
	}
}


/**************************************************************
Function:       CGbtMsgQueue::PackOtherObject
Description:    ��������������͵Ķ���		
Input:          ucDealDataIndex   ��Ϣ���������±�   
Output:         ��
Return:         ��
***************************************************************/
void CGbtMsgQueue::PackOtherObject(Byte ucDealDataIndex)
{
	Byte ucRecvOptType = ( m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[0]) & 0xf;   //�0�8�0�9�0�8�0�4�0�0�0�3�0�8�0�2�0�5�0�2���0�8���0�4�0�1
	int iRecvIndex     = m_sGbtDealData[ucDealDataIndex].sRecvFrame.iIndex;          
	int iRecvBufLen    = m_sGbtDealData[ucDealDataIndex].sRecvFrame.iBufLen;
	int iSendIndex     = m_sGbtDealData[ucDealDataIndex].sSendFrame.iIndex;          
	//int iSendBufLen    = m_sGbtDealData[ucDealDataIndex].sSendFrame.iBufLen;
	Byte ucIndexCnt    = 0;  //��������
	Byte ucErrorSts    = 0;  //����״̬
	Byte ucErrorIdx    = 0;  //��������
	Byte ucObjId       = 0;  //������(����)
	Byte ucIdxFst      = 0;  //��һ������(id1)
	Byte ucIdxSnd      = 0;  //�ڶ�������(id2)
	Byte ucSubId       = 0;  //�Ӷ���(�ֶ��±�)
	Byte ucIndex       = 0;

	if ( iRecvIndex >= iRecvBufLen )
	{
#ifdef TSC_DEBUG
		ACE_DEBUG((LM_DEBUG,"%s:%d,GBT_ERROR_OTHER\n",__FILE__,__LINE__));
#endif
		ucErrorSts = GBT_ERROR_OTHER;
		GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
		return;
	}

	/************�����ʶ*************/
	ucObjId = m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex];
	m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex] = ucObjId;  
	iRecvIndex++;
	iSendIndex++;

	/***********�����������Ӷ���*******/
	if ( iRecvIndex >= iRecvBufLen )
	{
#ifdef TSC_DEBUG
		//ACE_DEBUG((LM_DEBUG,"%s:%d,GBT_ERROR_OTHER\n",__FILE__,__LINE__));
#endif
		ucErrorSts = GBT_ERROR_OTHER;
		GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
		return;
	}
	m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex] =
					m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex]; 			//�����������Ӷ���
	ucIndexCnt = (m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex]>>6) & 0x3;   //��������
	ucSubId    = m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex] & 0x3F;       //�Ӷ����ֶ��±�
	iRecvIndex++;
	iSendIndex++;

	/***********����*************/
	if ( ucIndexCnt > 0 )  
	{
		if ( iRecvIndex >= iRecvBufLen )
		{
#ifdef TSC_DEBUG
			//ACE_DEBUG((LM_DEBUG,"%s:%d,GBT_ERROR_OTHER\n",__FILE__,__LINE__));
#endif
			ucErrorSts = GBT_ERROR_OTHER;
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
		ucIdxFst = m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex];
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex] = ucIdxFst;
		iRecvIndex++;
		iSendIndex++;
		ucIndexCnt--;
	}
	if ( ucIndexCnt > 0 ) 
	{
		if ( iRecvIndex >= iRecvBufLen )
		{
#ifdef TSC_DEBUG
			//ACE_DEBUG((LM_DEBUG,"%s:%d,GBT_ERROR_OTHER\n",__FILE__,__LINE__));
#endif
			ucErrorSts = GBT_ERROR_OTHER;
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
		ucIdxSnd = m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex];
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex] = ucIdxSnd;
		iRecvIndex++;
		iSendIndex++;
		ucIndexCnt--;
	}
	if ( ucIndexCnt > 0 )  
	{
		if ( iRecvIndex >= iRecvBufLen )
		{
#ifdef TSC_DEBUG
			//ACE_DEBUG((LM_DEBUG,"%s:%d,GBT_ERROR_OTHER\n",__FILE__,__LINE__));
#endif
			ucErrorSts = GBT_ERROR_OTHER;
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
		iRecvIndex++;
		iSendIndex++;
		ucIndexCnt--;
	}

	/************ֵ��**************/
	//m_sGbtDealData[ucDealDataIndex].sSendFrame.iBufLen = iSendIndex;
	switch ( ucObjId )
	{
	case OBJECT_UTC_TIME:
		if ( GBT_SEEK_REQ == ucRecvOptType )  /*�0�5���0�5�0�4*/
		{
			ACE_Time_Value tvCurTime = GetCurTime();
			unsigned int iTotalSec   = (unsigned int)tvCurTime.sec();    //utc time
			iTotalSec = iTotalSec +8*3600 ;

			//ACE_DEBUG((LM_DEBUG,"******************iTotalSec = %d return:\n",iTotalSec+8 * 60 * 60,iTotalSec));
			/*��ȡ�����ֽ��򣬸��ֽ��ڵ�λ*/
			while ( ucIndex < 4 )
			{
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = ( iTotalSec>>(8*(3-ucIndex)) & 0xFF );
				ucIndex++;
			}
			//ACE_DEBUG((LM_DEBUG,"%s:%d Reple UTC seconds: %d,ucDealDataIndex=%d!\n",__FILE__,__LINE__,iTotalSec,ucDealDataIndex));
		}
		else if ( (GBT_SET_REQ == ucRecvOptType) || (GBT_SET_REQ_NOACK == ucRecvOptType) ) 
		{
			int iIndex = 0;
			SThreadMsg sTscMsg;
			Ulong iTotalSec = 0;
			ACE_Time_Value tvCurTime;
			ACE_Date_Time  tvDateTime;

			while ( iIndex < 4 )
			{
				iTotalSec |=  
					((m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iIndex+iRecvIndex]&0xff)<<(8*(3-iIndex)));
				iIndex++;
			}

			tvCurTime.sec(iTotalSec);
			tvDateTime.update(tvCurTime);

			//CDate cCorrDate((Uint)tvDateTime.year(),(Byte)tvDateTime.month(),(Byte)tvDateTime.day());
			//if ( !cCorrDate.IsRightDate() ) MOD:201309281025
			if (!IsRightDate((Uint)tvDateTime.year(),(Byte)tvDateTime.month(),(Byte)tvDateTime.day()))
			{
				//ACE_DEBUG((LM_DEBUG,"%s:%d %d-%d-%d\n",
				//__FILE__,__LINE__,tvDateTime.year(),tvDateTime.month(),tvDateTime.day()));
				GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
				return;
			}

			sTscMsg.ulType       = TSC_MSG_CORRECT_TIME;
			sTscMsg.ucMsgOpt     = OBJECT_UTC_TIME;
			sTscMsg.uiMsgDataLen = 4;
			sTscMsg.pDataBuf     = ACE_OS::malloc(4);
			ACE_OS::memcpy(sTscMsg.pDataBuf,m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf+iRecvIndex,4);
			iRecvIndex += 4;
			CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));

			if ( CManaKernel::CreateInstance()->m_pTscConfig->sSpecFun[FUN_GPS].ucValue != 0 )
			{
				CGps::CreateInstance()->ForceAdjust();
			}
		}
		break;
	case OBJECT_LOCAL_TIME:
		if ( GBT_SEEK_REQ == ucRecvOptType )  
		{
			ACE_Time_Value tvCurTime = GetCurTime();
			unsigned int iTotalSec = (unsigned int)tvCurTime.sec() + 8 * 60 * 60; 
			
			/*��ȡ�����ֽ��򣬸��ֽ��ڵ�λ*/
			while ( ucIndex < 4 )
			{
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = ( iTotalSec>>(8*(3-ucIndex)) & 0xFF );
				ucIndex++;
			}
		}
		else if ( (GBT_SET_REQ == ucRecvOptType) || (GBT_SET_REQ_NOACK == ucRecvOptType) ) 
		{
			int iIndex = 0;
			SThreadMsg sTscMsg;
			Ulong iTotalSec = 0;
			ACE_Time_Value tvCurTime;
			ACE_Date_Time  tvDateTime;

			while ( iIndex < 4 )
			{
				iTotalSec |=  
					((m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iIndex+iRecvIndex]&0xff)<<(8*(3-iIndex)));
				iIndex++;
			}
			iTotalSec = iTotalSec + 8 * 60 * 60;

			tvCurTime.sec(iTotalSec);
			tvDateTime.update(tvCurTime);

			//CDate cCorrDate((Uint)tvDateTime.year(),(Byte)tvDateTime.month(),(Byte)tvDateTime.day());
			//if ( !cCorrDate.IsRightDate() )MOD:201309281017
			if (!IsRightDate((Uint)tvDateTime.year(),(Byte)tvDateTime.month(),(Byte)tvDateTime.day()))
			{
				//ACE_DEBUG((LM_DEBUG,"%s:%d error date:%d-%d-%d\n"
					//,__FILE__,__LINE__,tvDateTime.year(),tvDateTime.month(),tvDateTime.day()));
				GotoMsgError(ucDealDataIndex,5,ucErrorIdx);
				return;
			}

			sTscMsg.ulType       = TSC_MSG_CORRECT_TIME;
			sTscMsg.ucMsgOpt     = OBJECT_LOCAL_TIME;
			sTscMsg.uiMsgDataLen = 4;
			sTscMsg.pDataBuf     = ACE_OS::malloc(4);
			ACE_OS::memcpy(sTscMsg.pDataBuf,m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf+iRecvIndex,4);
			iRecvIndex += 4;
			CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));
			if ( CManaKernel::CreateInstance()->m_pTscConfig->sSpecFun[FUN_GPS].ucValue != 0 )
			{
				CGps::CreateInstance()->ForceAdjust();
			}
		}
		break;
	default:
		GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
		return;
		break;
	}

	m_sGbtDealData[ucDealDataIndex].sRecvFrame.iIndex = iRecvIndex;
	m_sGbtDealData[ucDealDataIndex].sSendFrame.iIndex = iSendIndex;
	m_sGbtDealData[ucDealDataIndex].iObjNum--;
	if ( iRecvIndex == iRecvBufLen ) 		//�ö��������
	{
		if ( 0 == m_sGbtDealData[ucDealDataIndex].iObjNum )   //���ж��󶼴������
		{
			m_sGbtDealData[ucDealDataIndex].sSendFrame.iBufLen = iSendIndex;
			ACE_DEBUG((LM_DEBUG,"%s:%d Send datetime to host,ucDealDataIndex=%d ,sendlen = %d!\n",__FILE__,__LINE__,ucDealDataIndex,iSendIndex));
			GotoSendToHost(ucDealDataIndex);
			return;
		}
		else
		{
			ucErrorSts = 5;
			ucErrorIdx = 0;
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
	}
	else
	{
		GotoDealRecvbuf(ucDealDataIndex);
		return;
	}
}


/**************************************************************
Function:       CGbtMsgQueue::GotoMsgError
Description:    ����λ�����ʹ��������Ϣ		
Input:          ucDealDataIndex   ��Ϣ���������±�
				ucErrorSts  ����״ֵ̬   
				ucErrorIdx  ��������
Output:         ��
Return:         ��
***************************************************************/
void CGbtMsgQueue::GotoMsgError(Byte ucDealDataIndex,Byte ucErrorSts,Byte ucErrorIdx)
{
	m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[0] = 
		(m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[0] & 0xf0) | GBT_ERR_ACK;
	m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[1] = ucErrorSts;
	m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[2] = ucErrorIdx;
	m_sGbtDealData[ucDealDataIndex].sSendFrame.iBufLen = 3;
	GotoSendToHost(ucDealDataIndex);
}


/**************************************************************
Function:       CGbtMsgQueue::GotoSendToHost
Description:    ֡������ϻ��߳��ִ��� ִ�з��ظ���λ����Ϣ		
Input:          ucDealDataIndex   ��Ϣ���������±�
Output:         ��
Return:         ��
***************************************************************/
void CGbtMsgQueue::GotoSendToHost(Byte ucDealDataIndex)
{
	SThreadMsg sGbtMsg;
	sGbtMsg.ulType       = GBT_MSG_SEND_HOST;  
	sGbtMsg.ucMsgOpt     = ucDealDataIndex;
	sGbtMsg.uiMsgDataLen = 0;
	sGbtMsg.pDataBuf     = NULL;	
	SendGbtMsg(&sGbtMsg,sizeof(sGbtMsg));
}


/**************************************************************
Function:       CGbtMsgQueue::GotoDealRecvbuf
Description:    ֡�������δȫ����� ��������֡		
Input:          ucDealDataIndex   ��Ϣ���������±�
Output:         ��
Return:         ��
***************************************************************/
void CGbtMsgQueue::GotoDealRecvbuf(Byte ucDealDataIndex)
{
	SThreadMsg sGbtMsg;
	sGbtMsg.ulType       = GBT_MSG_DEAL_RECVBUF;  
	sGbtMsg.ucMsgOpt     = ucDealDataIndex;
	sGbtMsg.uiMsgDataLen = 0;
	sGbtMsg.pDataBuf     = NULL;	
	SendGbtMsg(&sGbtMsg,sizeof(sGbtMsg));
}


/**************************************************************
Function:       CGbtMsgQueue::PackExtendObject
Description:    �����չЭ��Ķ���		
Input:          ucDealDataIndex   ��Ϣ���������±�
Output:         ��
Return:         ��
***************************************************************/
void CGbtMsgQueue::PackExtendObject(Byte ucDealDataIndex)
{
	Byte ucRecvOptType = ( m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[0]) & 0xf;   //�0�8�0�9�0�8�0�4�0�0�0�3�0�8�0�2�0�5�0�2���0�8���0�4�0�1
	int iRecvIndex     = m_sGbtDealData[ucDealDataIndex].sRecvFrame.iIndex;          
	int iRecvBufLen    = m_sGbtDealData[ucDealDataIndex].sRecvFrame.iBufLen;
	int iSendIndex     = m_sGbtDealData[ucDealDataIndex].sSendFrame.iIndex;          
	//int iSendBufLen    = m_sGbtDealData[ucDealDataIndex].sSendFrame.iBufLen;
	Byte ucIndexCnt    = 0;  //��������
	Byte ucErrorSts    = 0;  //����״̬
	Byte ucErrorIdx    = 0;  //��������
	Byte ucObjId       = 0;  //������(����)
	Byte ucIdxFst      = 0;  //��һ������(id1)
	Byte ucIdxSnd      = 0;  //�ڶ�������(id2)
	Byte ucSubId       = 0;  //�Ӷ���(�ֶ��±�)

	if ( iRecvIndex >= iRecvBufLen )
	{
		ucErrorSts = GBT_ERROR_OTHER;
		GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
		return;
	}
	
	ucObjId = m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex];
	m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex] = ucObjId;  
	iRecvIndex++;
	iSendIndex++;
	
	if ( iRecvIndex >= iRecvBufLen )
	{
		ucErrorSts = GBT_ERROR_OTHER;
		GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
		return;
	}
	m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex] =
		m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex]; 
	ucIndexCnt = (m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex]>>6) & 0x3;  
	ucSubId    = m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex] & 0x3F;     
	iRecvIndex++;
	iSendIndex++;
	
	if ( ucIndexCnt > 0 ) 
	{
		if ( iRecvIndex >= iRecvBufLen )
		{
			ucErrorSts = GBT_ERROR_OTHER;
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
		ucIdxFst = m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex];
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex] = ucIdxFst;
		iRecvIndex++;
		iSendIndex++;
		ucIndexCnt--;
	}
	if ( ucIndexCnt > 0 ) 
	{
		if ( iRecvIndex >= iRecvBufLen )
		{
			ucErrorSts = GBT_ERROR_OTHER;
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
		ucIdxSnd = m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex];
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex] = ucIdxSnd;
		iRecvIndex++;
		iSendIndex++;
		ucIndexCnt--;
	}
	if ( ucIndexCnt > 0 ) 
	{
		if ( iRecvIndex >= iRecvBufLen )
		{
			ucErrorSts = GBT_ERROR_OTHER;
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
		iRecvIndex++;
		iSendIndex++;
		ucIndexCnt--;
	}
	
	//m_sGbtDealData[ucDealDataIndex].sSendFrame.iBufLen = iSendIndex;
	switch ( ucObjId )
	{
	case OBJECT_EXT_TSC_STATUS:   //�źŻ���չ״̬
		if ( GBT_SEEK_REQ == ucRecvOptType )  
		{
			SThreadMsg sTscMsg;
			sTscMsg.ulType       = TSC_MSG_EXSTATUS_READ;
			sTscMsg.ucMsgOpt     = ucDealDataIndex;
			sTscMsg.uiMsgDataLen = 0;
			sTscMsg.pDataBuf     = NULL;

			CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(SThreadMsg));
			
			return;
		}
		else
		{
			ucErrorSts = GBT_ERROR_OTHER;
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
		break;
	case OBJECT_IP:
		if ( GBT_SEEK_REQ == ucRecvOptType )  
		{
			if ( ucSubId != 0 )
			{
				GetCmuAndCtrl( m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf,iSendIndex , ucSubId);
			}
			else
			{
				GetCmuAndCtrl( m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf,iSendIndex);
			}
		}
		else if ( (GBT_SET_REQ == ucRecvOptType) || (GBT_SET_REQ_NOACK == ucRecvOptType) ) 
		{
			if ( ucSubId != 0 )
			{
				SetCmuAndCtrl(m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf,iRecvIndex,ucSubId);
			}
			else
			{
				SetCmuAndCtrl(m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf,iRecvIndex);
			}
			
			SThreadMsg sTscMsg;
			sTscMsg.ulType       = TSC_MSG_UPDATE_PARA;  
			sTscMsg.ucMsgOpt     = 0;
			sTscMsg.uiMsgDataLen = 0;
			sTscMsg.pDataBuf     = NULL;
			CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));
		}
		break;
	case OBJECT_WATCH_PARA:    //������Ͳ��� �¶� ��ѹ ��
		if ( GBT_SEEK_REQ == ucRecvOptType )  
		{
			GetWatchPara(m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf,&iSendIndex);     
		}
		else
		{
			ucErrorSts = GBT_ERROR_OTHER;
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
		break;
	case OBJECT_MODULE_STATUS:  
		if (  GBT_SEEK_REQ == ucRecvOptType )
		{
			GetModuleStatus(m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf,&iSendIndex); 
		}
		else
		{
			ucErrorSts = GBT_ERROR_OTHER;
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
		break;
		case OBJECT_YWFLASH_CFG:    //��������չ����
		if ( GBT_SEEK_REQ == ucRecvOptType )  //��ѯ
		{

			GetFlashCfg(m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf,&iSendIndex);  

		}
		else if((GBT_SET_REQ == ucRecvOptType) || (GBT_SET_REQ_NOACK == ucRecvOptType)) //����
		{
			SetFlashCtrl(m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf,iRecvIndex);

		}
		break ;
	case OBJECT_DET_EXTCFG :    //�������չ���ö���
		if ( GBT_SEEK_REQ == ucRecvOptType )  //��ѯ
		{
			Byte ucQueryType =( m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf)[iRecvIndex++] ;
			Byte ucBdIndx= (m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf)[iRecvIndex++] ;
			//ACE_DEBUG((LM_DEBUG,"%s:%d,ucBdindex= %d ,ucQueryType = %d\n",__FILE__,__LINE__,ucBdIndx,ucQueryType));
			GetDetCfg(m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf,ucBdIndx,ucQueryType,&iSendIndex);  

		}
		else if((GBT_SET_REQ == ucRecvOptType) || (GBT_SET_REQ_NOACK == ucRecvOptType)) //����
		{
			
			SetDetCfg(m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf,iRecvIndex) ;
		}
		break ;

	case OBJECT_LAMPBOARD_CFG :    //�ƿذ���ݼ��ͺ��̳�ͻ�������
		if ( GBT_SEEK_REQ == ucRecvOptType )  
		{
			;//�Ժ��ٸ���ʵ�ʲ���

		}
		else if((GBT_SET_REQ == ucRecvOptType) || (GBT_SET_REQ_NOACK == ucRecvOptType)) //����
		{		 
			
			SetLampBdtCfg(m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf,iRecvIndex); 
		}
		break ;
	case OBJECT_PSCBTN_NUM :    //ģ���������˰�ťֵ
		if ( GBT_SEEK_REQ == ucRecvOptType )  
		{
			;//�Ժ��ٸ���ʵ�ʲ���

		}
		else if((GBT_SET_REQ == ucRecvOptType) || (GBT_SET_REQ_NOACK == ucRecvOptType)) //����
		{		 
			SetPscNum(m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf,iRecvIndex); 
		}
		break ;
	case OBJECT_TMPPATTERN_CFG :    //��ʱ12���������ϣ�����60��Ĭ��
		//ACE_DEBUG((LM_DEBUG,"%s:%d,ObjectId == OBJECT_TMPPATTERN_CFG\n",__FILE__,__LINE__));
		if ( GBT_SEEK_REQ == ucRecvOptType )  
		{
			;//�Ժ��ٸ�����Ҫ����

		}
		else if((GBT_SET_REQ == ucRecvOptType) || (GBT_SET_REQ_NOACK == ucRecvOptType)) //����
		{		 
			if(CManaKernel::CreateInstance()->bTmpPattern==true)
				return ;
			SetTmpPattern(m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf,iRecvIndex); 
		}
		break ;
	case OBJECT_SYSFUNC_CFG :    //ϵͳ������������
		if ( GBT_SEEK_REQ == ucRecvOptType )  
		{
			;//�Ժ��ٸ�����Ҫ����

		}
		else if((GBT_SET_REQ == ucRecvOptType) || (GBT_SET_REQ_NOACK == ucRecvOptType)) //����
		{		 
			
			SetSysFunc(m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf,iRecvIndex); 
		}
		break ;	
		
	default:
		ACE_DEBUG((LM_DEBUG,"%s:%d,ObjectId error objectId:%d\n",__FILE__,__LINE__,ucObjId));
		GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
		return;
	}	
	m_sGbtDealData[ucDealDataIndex].sRecvFrame.iIndex = iRecvIndex;
	m_sGbtDealData[ucDealDataIndex].sSendFrame.iIndex = iSendIndex;
	m_sGbtDealData[ucDealDataIndex].iObjNum--;
	
	ACE_DEBUG((LM_DEBUG,"%s:%d iRecvIndex = %d iRecvBufLen = %d\n",__FILE__,__LINE__,iRecvIndex,iRecvBufLen));
	if ( iRecvIndex == iRecvBufLen )  
	{
		if ( 0 == m_sGbtDealData[ucDealDataIndex].iObjNum ) 
		{
			m_sGbtDealData[ucDealDataIndex].sSendFrame.iBufLen = iSendIndex;
			GotoSendToHost(ucDealDataIndex);
			return;
		}
		else
		{
			ucErrorSts = 5;
			ucErrorIdx = 0;
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
	}
	else
	{
		GotoDealRecvbuf(ucDealDataIndex);
		return;
	}
}


/**************************************************************
Function:       CGbtMsgQueue::GetCmuAndCtrl
Description:    ͨ�ż����Ʒ�ʽ������չ����(0xf6)ȫ���Ӷ����ѯ		
Input:          iSendIndex   �������ַ����
Output:         pBuf  ��ѯ�������ָ��
Return:         ��
***************************************************************/
void CGbtMsgQueue::GetCmuAndCtrl(Byte* pBuf,int& iSendIndex)
{
	Byte ucTmp       = 0;
	Byte pHwEther[6] = {0};
	Byte pIp[4]      = {0};
	Byte pMask[4]    = {0};
	Byte pGateway[4] = {0};
	STscConfig* pTscCfg = CManaKernel::CreateInstance()->m_pTscConfig;

	GetNetPara(pHwEther , pIp , pMask , pGateway);

	//�豸����ȡֵ
	pBuf[iSendIndex++] = 0;
	pBuf[iSendIndex++] = 0;

	ucTmp = 0;
	if ( pTscCfg->sSpecFun[FUN_DOOR].ucValue > 0 )
	{
		ucTmp |= 1<<5;
	}
	if ( pTscCfg->sSpecFun[FUN_VOLTAGE].ucValue > 0 )
	{
		ucTmp |= 1<<6;
	}
	if ( pTscCfg->sSpecFun[FUN_TEMPERATURE].ucValue > 0 )
	{
		ucTmp |= 1<<7;
	}
	pBuf[iSendIndex++] = ucTmp;

	ucTmp = 0;
	if ( pTscCfg->sSpecFun[FUN_MSG_ALARM].ucValue > 0 )
	{
		ucTmp |= 1<<5;
	}
	if ( pTscCfg->sSpecFun[FUN_GPS].ucValue > 0 )
	{
		ucTmp |= 1<<6;
	}
	if ( pTscCfg->sSpecFun[FUN_COUNT_DOWN].ucValue > 0 )
	{
		ucTmp |= 1<<7;
	}
	pBuf[iSendIndex++] = ucTmp;
	
	ucTmp = pTscCfg->sSpecFun[FUN_PRINT_FLAG].ucValue;
	pBuf[iSendIndex++] = ucTmp;

	ucTmp = 0;
	ucTmp = pTscCfg->sSpecFun[FUN_PRINT_FLAGII].ucValue;
	if ( pTscCfg->sSpecFun[FUN_SERIOUS_FLASH].ucValue > 0 )
	{
		ucTmp |= 1<<7;
	}
	pBuf[iSendIndex++] = ucTmp;

	//�0�1����¡��0�4�0�8�0�4
	ucTmp = pTscCfg->sSpecFun[FUN_CROSS_TYPE].ucValue;
	pBuf[iSendIndex++] = ucTmp;
	ucTmp = pTscCfg->sSpecFun[FUN_STAND_STAGEID].ucValue;
	pBuf[iSendIndex++] = ucTmp;
	ucTmp = pTscCfg->sSpecFun[FUN_CORSS1_STAGEID].ucValue;
	pBuf[iSendIndex++] = ucTmp;
	ucTmp = pTscCfg->sSpecFun[FUN_CROSS2_STAGEID].ucValue;
	pBuf[iSendIndex++] = ucTmp;

	//�0�1�0�8�0�4�0�3�0�4�0�7�0�7�0�3
	ucTmp = pTscCfg->sSpecFun[FUN_COMMU_PARA].ucValue;
	pBuf[iSendIndex++] = ucTmp;

	//�0�9�0�9�0�7�0�3�0�2�0�3
	ucTmp = pTscCfg->sSpecFun[FUN_PORT_HIGH].ucValue;
	pBuf[iSendIndex++] = ucTmp;
	ucTmp = pTscCfg->sSpecFun[FUN_PORT_LOW].ucValue;
	pBuf[iSendIndex++] = ucTmp;

	//ip
	pBuf[iSendIndex]   = pIp[0];
	pBuf[iSendIndex+1] = pIp[1];
	pBuf[iSendIndex+2] = pIp[2];
	pBuf[iSendIndex+3] = pIp[3];
	iSendIndex += 16;

	//net mask
	pBuf[iSendIndex]   = pMask[0];
	pBuf[iSendIndex+1] = pMask[1];
	pBuf[iSendIndex+2] = pMask[2];
	pBuf[iSendIndex+3] = pMask[3];
	iSendIndex += 16;

	//gate way
	pBuf[iSendIndex]   = pGateway[0];
	pBuf[iSendIndex+1] = pGateway[1];
	pBuf[iSendIndex+2] = pGateway[2];
	pBuf[iSendIndex+3] = pGateway[3];
	iSendIndex += 16;
	
	/*
	pBuf[iSendIndex++] = pHwEther[0];
	pBuf[iSendIndex++] = pHwEther[1];
	pBuf[iSendIndex++] = pHwEther[2];
	pBuf[iSendIndex++] = pHwEther[3];
	pBuf[iSendIndex++] = pHwEther[4];
	pBuf[iSendIndex++] = pHwEther[5];
	*/
}


/**************************************************************
Function:       CGbtMsgQueue::GetCmuAndCtrl
Description:    ͨ�ż����Ʒ�ʽ������չ����(0xf6)���Ӷ����ѯ		
Input:          iSendIndex   �������ַ����
				ucSubId      �Ӷ���id
Output:         pBuf  ��ѯ�������ָ��
Return:         ��
***************************************************************/
void CGbtMsgQueue::GetCmuAndCtrl(Byte* pBuf,int& iSendIndex , Byte ucSubId)
{
	Byte ucTmp       = 0;
	Byte pHwEther[6] = {0};
	Byte pIp[4]      = {0};
	Byte pMask[4]    = {0};
	Byte pGateway[4] = {0};
	STscConfig* pTscCfg = CManaKernel::CreateInstance()->m_pTscConfig;

	switch ( ucSubId )
	{
		case 1:   //�0�7�����0�6�0�9�0�4���0�6�0�3�0�0�0�8
			pBuf[iSendIndex++] = 0;
			pBuf[iSendIndex++] = 0;
			break;
		case 2: //?�0�7�����0�6�0�4�0�0�0�7�0�1���0�0
			ucTmp = 0;
			if ( pTscCfg->sSpecFun[FUN_DOOR].ucValue > 0 )
			{
				ucTmp |= 1<<5;
			}
			if ( pTscCfg->sSpecFun[FUN_VOLTAGE].ucValue > 0 )
			{
				ucTmp |= 1<<6;
			}
			if ( pTscCfg->sSpecFun[FUN_TEMPERATURE].ucValue > 0 )
			{
				ucTmp |= 1<<7;
			}
			pBuf[iSendIndex++] = ucTmp;

			ucTmp = 0;
			if ( pTscCfg->sSpecFun[FUN_MSG_ALARM].ucValue > 0 )
			{
				ucTmp |= 1<<5;
			}
			if ( pTscCfg->sSpecFun[FUN_GPS].ucValue > 0 )
			{
				ucTmp |= 1<<6;
			}
			if ( pTscCfg->sSpecFun[FUN_COUNT_DOWN].ucValue > 0 )
			{
				ucTmp |= 1<<7;
			}
			pBuf[iSendIndex++] = ucTmp;

			ucTmp = pTscCfg->sSpecFun[FUN_PRINT_FLAG].ucValue;
			pBuf[iSendIndex++] = ucTmp;

			ucTmp = 0;
			ucTmp = pTscCfg->sSpecFun[FUN_PRINT_FLAGII].ucValue;
			if ( pTscCfg->sSpecFun[FUN_SERIOUS_FLASH].ucValue > 0 )
			{
				ucTmp |= 1<<7;
			}
			pBuf[iSendIndex++] = ucTmp;
			break;
		case 3:  //�0�1����¡��0�4�0�8�0�4
			ucTmp = pTscCfg->sSpecFun[FUN_CROSS_TYPE].ucValue;
			pBuf[iSendIndex++] = ucTmp;
			ucTmp = pTscCfg->sSpecFun[FUN_STAND_STAGEID].ucValue;
			pBuf[iSendIndex++] = ucTmp;
			ucTmp = pTscCfg->sSpecFun[FUN_CORSS1_STAGEID].ucValue;
			pBuf[iSendIndex++] = ucTmp;
			ucTmp = pTscCfg->sSpecFun[FUN_CROSS2_STAGEID].ucValue;
			pBuf[iSendIndex++] = ucTmp;
			break;
		case 4:  //�0�1�0�8�0�4�0�3�0�4�0�7�0�7�0�3
			ucTmp = pTscCfg->sSpecFun[FUN_COMMU_PARA].ucValue;
			pBuf[iSendIndex++] = ucTmp;
			break;
		case 5:  //�0�9�0�9�0�7�0�3�0�2�0�3
			ucTmp = pTscCfg->sSpecFun[FUN_PORT_HIGH].ucValue;
			pBuf[iSendIndex++] = ucTmp;
			ucTmp = pTscCfg->sSpecFun[FUN_PORT_LOW].ucValue;
			pBuf[iSendIndex++] = ucTmp;
			break;
		case 6:  //ip
			GetNetPara(pHwEther , pIp , pMask , pGateway);
			pBuf[iSendIndex]   = pIp[0];
			pBuf[iSendIndex+1] = pIp[1];
			pBuf[iSendIndex+2] = pIp[2];
			pBuf[iSendIndex+3] = pIp[3];
			iSendIndex += 16;
			break;
		case 7: //net mask
			GetNetPara(pHwEther , pIp , pMask , pGateway);
			pBuf[iSendIndex]   = pMask[0];
			pBuf[iSendIndex+1] = pMask[1];
			pBuf[iSendIndex+2] = pMask[2];
			pBuf[iSendIndex+3] = pMask[3];
			iSendIndex += 16;
			break;
		case 8: //gate way
			GetNetPara(pHwEther , pIp , pMask , pGateway);
			pBuf[iSendIndex]   = pGateway[0];
			pBuf[iSendIndex+1] = pGateway[1];
			pBuf[iSendIndex+2] = pGateway[2];
			pBuf[iSendIndex+3] = pGateway[3];
			iSendIndex += 16;
			break;
		default:
			break;

	}
}


/**************************************************************
Function:       CGbtMsgQueue::SetCmuAndCtrl
Description:    ͨ�ż����Ʒ�ʽ������չ����(0xf6)��ȫ���Ӷ���ֵ����		
Input:          pBuf   ����ֵ�����ַָ��
				iRecvIndex     ��ǰ����ֵ��ַ
Output:        
Return:         ��
***************************************************************/
void CGbtMsgQueue::SetCmuAndCtrl(Byte* pBuf,int& iRecvIndex)
{
	Byte ucTmp             = 0;
	Byte cIp[4]            = {0};
	Byte cMask[4]          = {0};
	Byte cGateWay[4]       = {0};
	
	STscConfig* pTscCfg = CManaKernel::CreateInstance()->m_pTscConfig;

	/*************�0�7�����0�6�0�9�0�4���0�6�0�3�0�0�0�8*********/
	ucTmp       = pBuf[iRecvIndex++];
	ucTmp       = pBuf[iRecvIndex++];
	//bit0  �0�0�0�1�0�4�0�0�0�3�0�8�0�1�0�6
	//bit1  �0�4�0�3�0�6�0�5���0�8�0�3��
	//bit2  �0�4�0�3�0�6�0�5�0�7�0�5�0�3�0�9
	//bit3  �0�0�0�1�0�4�0�0�0�6�0�0�0�4��
	//bit4  �0�5�0�2�0�6�0�5�0�5�0�3�0�0�0�1�0�8�0�0�0�3�0�6
	//bit5  �0�5�0�2�0�6�0�5�0�8�0�0�0�3�0�6
	//bit6  �0�7�0�0�0�6�0�5�0�1�0�6�0�3�0�4�0�8�0�5�0�0�0�6
	//bit7  �0�7�0�0�0�6�0�5�0�6�0�9�0�0�0�0
	//bit8  �0�7���0�0�0�1�0�1�0�8�0�4�0�3�0�5�0�2�0�8�0�5
	//bit15 �0�9�0�8�0�4�0�2�0�6���0�8�0�8�0�9�0�1

	/***********�0�7�����0�6�0�4�0�0�0�7�0�1���0�0************/
	/*        bit0   bit1     bit2     bit3    bit4    bit5    bit6    bit7
	* Byte0                                    �0�1�0�3�0�7�0�9�0�1�0�1   �0�8�0�4�0�5�0�1     �0�2�0�0�0�9�0�6�0�3�0�4
	* Byte1                                    �0�9�0�0�0�4�0�3     gps     �0�8�0�1�0�3�0�4�0�8��
	* Byte2  �0�3���0�5�0�9�0�4��  ���0�4�0�8�0�4�0�8�0�5��0�2   �0�8�0�9�0�8�0�4�0�8�0�5��0�2  ���0�4�0�8�0�4�0�7�0�1��0�2 �0�8�0�9�0�8�0�4�0�7�0�1��0�2 ���0�4�0�3�0�4�0�7�0�9�0�4�� �0�8�0�9�0�3�0�4�0�7�0�9�0�4�� �0�5�0�4���0�6�0�4�0�3�0�3�0�4
	* Byte3  �0�8�0�1�0�3�0�4�0�8��  StartTime costTime                                 �0�5�0�3�0�0�0�1�0�5���0�2���0�3�0�4�0�7�0�9 
	*/
	ucTmp = pBuf[iRecvIndex++];
	pTscCfg->sSpecFun[FUN_DOOR].ucValue        = (ucTmp>>5) & 1;
	pTscCfg->sSpecFun[FUN_VOLTAGE].ucValue     = (ucTmp>>6) & 1;
	pTscCfg->sSpecFun[FUN_TEMPERATURE].ucValue = (ucTmp>>7) & 1;
	(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_DOOR+1        , (ucTmp>>5) & 1 );
	(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_VOLTAGE+1     , (ucTmp>>6) & 1 );
	(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_TEMPERATURE+1 , (ucTmp>>7) & 1 );

	ucTmp = pBuf[iRecvIndex++];
	pTscCfg->sSpecFun[FUN_MSG_ALARM].ucValue  = (ucTmp>>5) & 1;
	pTscCfg->sSpecFun[FUN_GPS].ucValue        = (ucTmp>>6) & 1;
	pTscCfg->sSpecFun[FUN_COUNT_DOWN].ucValue = (ucTmp>>7) & 1;
	(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_MSG_ALARM+1  , (ucTmp>>5) & 1 );
	(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_GPS+1        , (ucTmp>>6) & 1 );
	(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_COUNT_DOWN+1 , (ucTmp>>7) & 1 );

	ucTmp = pBuf[iRecvIndex++];
	pTscCfg->sSpecFun[FUN_PRINT_FLAG].ucValue  = ucTmp;
	(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_PRINT_FLAG+1 , ucTmp);

	ucTmp = pBuf[iRecvIndex++];
	pTscCfg->sSpecFun[FUN_PRINT_FLAGII].ucValue  = ucTmp & 0x7F;
	pTscCfg->sSpecFun[FUN_SERIOUS_FLASH].ucValue = (ucTmp>>7) & 1; 
	(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_PRINT_FLAGII+1  , ucTmp & 0x7F );
	(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_SERIOUS_FLASH+1 , (ucTmp>>7) & 1 );
	
	/***********�0�1����¡��0�4�0�8�0�4************/
	ucTmp = pBuf[iRecvIndex++];
	//pTscCfg->sSpecFun[FUN_CROSS_TYPE].ucValue  = ucTmp;
	(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_CROSS_TYPE+1    , ucTmp);
	ucTmp = pBuf[iRecvIndex++];
	//pTscCfg->sSpecFun[FUN_STAND_STAGEID].ucValue  = ucTmp;
	(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_STAND_STAGEID+1  , ucTmp);
	ucTmp = pBuf[iRecvIndex++];
	//pTscCfg->sSpecFun[FUN_CORSS1_STAGEID].ucValue  = ucTmp;
	(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_CORSS1_STAGEID+1 , ucTmp);
	ucTmp = pBuf[iRecvIndex++];
	//pTscCfg->sSpecFun[FUN_CROSS2_STAGEID].ucValue  = ucTmp;
	(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_CROSS2_STAGEID+1 , ucTmp);

	/***********�0�1�0�8�0�4�0�3�0�4�0�7�0�7�0�3***********/
	ucTmp       = pBuf[iRecvIndex++];
	pTscCfg->sSpecFun[FUN_COMMU_PARA].ucValue  = ucTmp;
	(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_COMMU_PARA+1 , ucTmp);
	//bit0 �0�1�0�8�0�4�0�3�0�8�0�1�0�7�0�1null�0�7�����0�6
	//bit1 1-TCP  0-UDP
	//bit2 1-IPV6 0-IPV4

	//�0�9�0�9�0�7�0�3�0�2�0�3 �0�9�0�4���0�0�0�4�0�3 �0�6�0�3�0�0�0�82048-16768
	ucTmp       = pBuf[iRecvIndex++];
	pTscCfg->sSpecFun[FUN_PORT_HIGH].ucValue  = ucTmp;
	(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_PORT_HIGH+1 , ucTmp);
	ucTmp       = pBuf[iRecvIndex++];
	pTscCfg->sSpecFun[FUN_PORT_LOW].ucValue  = ucTmp;
	(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_PORT_LOW+1 , ucTmp);

	CManaKernel::CreateInstance()->SetUpdateBit();

	//ip
	cIp[0] = *(pBuf+iRecvIndex);
	cIp[1] = *(pBuf+iRecvIndex + 1);
	cIp[2] = *(pBuf+iRecvIndex + 2);
	cIp[3] = *(pBuf+iRecvIndex + 3);
	iRecvIndex += 16;

	//�0�1�0�3�0�0�0�4�0�5�0�3�0�0�0�5
	cMask[0] = *(pBuf+iRecvIndex);
	cMask[1] = *(pBuf+iRecvIndex + 1);
	cMask[2] = *(pBuf+iRecvIndex + 2);
	cMask[3] = *(pBuf+iRecvIndex + 3);
	iRecvIndex += 16;

	//�0�1�0�3�0�1�0�1IP�0�8�0�1�0�0��
	cGateWay[0] = *(pBuf+iRecvIndex);
	cGateWay[1] = *(pBuf+iRecvIndex + 1);
	cGateWay[2] = *(pBuf+iRecvIndex + 2);
	cGateWay[3] = *(pBuf+iRecvIndex + 3);
	iRecvIndex += 16;

	//�0�2�0�7�0�8���0�8�0�1�0�0�� �0�5�0�3�0�6�0�1�0�4�0�7�0�6�0�2
	/*
	cHwEther[0] = *(pBuf+iRecvIndex);
	cHwEther[1] = *(pBuf+iRecvIndex + 1);
	cHwEther[2] = *(pBuf+iRecvIndex + 2);
	cHwEther[3] = *(pBuf+iRecvIndex + 3);
	cHwEther[4] = *(pBuf+iRecvIndex + 4);
	cHwEther[5] = *(pBuf+iRecvIndex + 5);
	iRecvIndex += 6;
	*/

	ReworkNetPara(cIp,cMask,cGateWay);

}


/**************************************************************
Function:       CGbtMsgQueue::SetCmuAndCtrl
Description:    ͨ�ż����Ʒ�ʽ������չ����(0xf6)���Ӷ���ֵ����		
Input:          pBuf   ����ֵ�����ַָ��
				iRecvIndex     ��ǰ����ֵ��ַ
				ucSubId        �Ӷ���ID
Output:        
Return:         ��
***************************************************************/
void CGbtMsgQueue::SetCmuAndCtrl(Byte* pBuf,int& iRecvIndex , Byte ucSubId)
{
	Byte ucTmp       = 0;
	Byte cIp[4]      = {0};
	Byte cMask[4]    = {0};
	Byte cGateWay[4] = {0};
	STscConfig* pTscCfg = CManaKernel::CreateInstance()->m_pTscConfig;

	switch ( ucSubId )
	{
		case 1:  //�0�7�����0�6�0�9�0�4���0�6�0�3�0�0�0�8
			ucTmp = pBuf[iRecvIndex++];
			ucTmp = pBuf[iRecvIndex++];			
			break;
		case 2:  //�0�7�����0�6�0�4�0�0�0�7�0�1���0�0
			ucTmp = pBuf[iRecvIndex++];
			pTscCfg->sSpecFun[FUN_DOOR].ucValue         = (ucTmp>>5) & 1;
			pTscCfg->sSpecFun[FUN_VOLTAGE].ucValue      = (ucTmp>>6) & 1;
			pTscCfg->sSpecFun[FUN_TEMPERATURE].ucValue  = (ucTmp>>7) & 1;
			(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_DOOR+1       , (ucTmp>>5) & 1 );
			(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_VOLTAGE+1    , (ucTmp>>6) & 1 );
			(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_TEMPERATURE+1, (ucTmp>>7) & 1 );

			ucTmp = pBuf[iRecvIndex++];
			pTscCfg->sSpecFun[FUN_MSG_ALARM].ucValue  = (ucTmp>>5) & 1;
			pTscCfg->sSpecFun[FUN_GPS].ucValue        = (ucTmp>>6) & 1;
			pTscCfg->sSpecFun[FUN_COUNT_DOWN].ucValue = (ucTmp>>7) & 1;
			(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_MSG_ALARM+1  , (ucTmp>>5) & 1 );
			(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_GPS+1        , (ucTmp>>6) & 1 );
			(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_COUNT_DOWN+1 , (ucTmp>>7) & 1 );

			ucTmp = pBuf[iRecvIndex++];
			pTscCfg->sSpecFun[FUN_PRINT_FLAG].ucValue = ucTmp;
			(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_PRINT_FLAG+1 , ucTmp);

			ucTmp = pBuf[iRecvIndex++];
			pTscCfg->sSpecFun[FUN_PRINT_FLAGII].ucValue = ucTmp & 0x7F;
			pTscCfg->sSpecFun[FUN_SERIOUS_FLASH].ucValue = (ucTmp>>7)&1;
			(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_PRINT_FLAGII+1  , ucTmp & 0x7F );
			(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_SERIOUS_FLASH+1 , (ucTmp>>7)&1 );
			break;
		case 3:  //�0�1����¡��0�4�0�8�0�4
			ucTmp = pBuf[iRecvIndex++];
			//pTscCfg->sSpecFun[FUN_CROSS_TYPE].ucValue = ucTmp ;
			(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_CROSS_TYPE+1     , ucTmp);
			ucTmp = pBuf[iRecvIndex++];
			//pTscCfg->sSpecFun[FUN_STAND_STAGEID].ucValue = ucTmp;
			(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_STAND_STAGEID+1  , ucTmp);
			ucTmp = pBuf[iRecvIndex++];
			//pTscCfg->sSpecFun[FUN_CORSS1_STAGEID].ucValue = ucTmp;
			(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_CORSS1_STAGEID+1 , ucTmp);
			ucTmp = pBuf[iRecvIndex++];
			//pTscCfg->sSpecFun[FUN_CROSS2_STAGEID].ucValue = ucTmp;
			(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_CROSS2_STAGEID+1 , ucTmp);
			CManaKernel::CreateInstance()->SetUpdateBit();
			break;
		case 4:  //�0�1�0�8�0�4�0�3�0�4�0�7�0�7�0�3
			ucTmp       = pBuf[iRecvIndex++];
			(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_COMMU_PARA+1 , ucTmp);
			break;
		case 5:  //�0�9�0�9�0�7�0�3�0�2�0�3
			ucTmp       = pBuf[iRecvIndex++];
			pTscCfg->sSpecFun[FUN_PORT_HIGH].ucValue = ucTmp;
			(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_PORT_HIGH+1 , ucTmp);
			ucTmp       = pBuf[iRecvIndex++];
			pTscCfg->sSpecFun[FUN_PORT_LOW].ucValue = ucTmp;
			(CDbInstance::m_cGbtTscDb).ModSpecFun(FUN_PORT_LOW+1 , ucTmp);
			break;
		case 6:  //ip
			cIp[0] = *(pBuf+iRecvIndex);
			cIp[1] = *(pBuf+iRecvIndex + 1);
			cIp[2] = *(pBuf+iRecvIndex + 2);
			cIp[3] = *(pBuf+iRecvIndex + 3);
			iRecvIndex += 16;
			ReworkNetPara(cIp,NULL,NULL);
			break;
		case 7: //�0�5�0�3�0�0�0�5
			cMask[0] = *(pBuf+iRecvIndex);
			cMask[1] = *(pBuf+iRecvIndex + 1);
			cMask[2] = *(pBuf+iRecvIndex + 2);
			cMask[3] = *(pBuf+iRecvIndex + 3);
			iRecvIndex += 16;
			ReworkNetPara(NULL,cMask,NULL);
			break;
		case 8:  //�0�1�0�3�0�1�0�1
			cGateWay[0] = *(pBuf+iRecvIndex);
			cGateWay[1] = *(pBuf+iRecvIndex + 1);
			cGateWay[2] = *(pBuf+iRecvIndex + 2);
			cGateWay[3] = *(pBuf+iRecvIndex + 3);
			iRecvIndex += 16;
			ReworkNetPara(NULL,NULL,cGateWay);
			break;
		default:
			break;
	}
}


/**************************************************************
Function:       CGbtMsgQueue::GetWatchPara
Description:    ��ȡ��ز�����չ����ֵ		
Input:          iSendIndex  ��ǰ����֡�ֽڵ�ַ
Output:         pBuf  ����֡��ַ
Return:         ��
***************************************************************/
void CGbtMsgQueue::GetWatchPara(Byte* pBuf,int *iSendIndex)
{
	CFlashMac* pFlashMac = CFlashMac::CreateInstance();

	bool bForDoor        = pFlashMac->m_bGetForDoor;   //�0�5��0�1�0�3�0�5���0�7�0�9
	bool bPowerType      = pFlashMac->m_bPowerType;    //�0�1�0�8�0�8�0�4�0�8���0�4�0�1 true:�0�4�0�3�0�9�0�8�0�4 false:�0�0�0�0�0�5�0�0�0�2�0�5
	bool bAlarmStatus    = false;//CPowerBoard::CreateInstance()->m_bGetAlarmStatus;  //���0�8�0�0�0�4�0�4�¡��0�5�0�0�0�1
	Byte ucDoorValue     = 0;
	int  iTemperature    = pFlashMac->m_iTemperature;  //�0�2�0�0�0�9�0�6
	int  iVoltage        = pFlashMac->m_iVoltage;      //�0�8�0�4�0�5�0�1

	//�0�2�0�0�0�9�0�6�0�0�0�8
	if ( iTemperature > -65 && iTemperature < 120 )
	{
		iTemperature += 65;
	}
	else if ( iTemperature < -65 )
	{
		iTemperature = 0;
	}
	else if ( iTemperature > 120 )
	{
		iTemperature = 185;
	}

	pBuf[*iSendIndex] = (iTemperature >> 8) & 0xff;  //�0�6�0�8���0�0�0�4�0�3�0�8�0�3�0�8�0�1�0�2�0�3
	*iSendIndex += 1;

	pBuf[*iSendIndex] = iTemperature & 0xff;
	*iSendIndex += 1;

	//�0�1�0�3�0�7�0�9�0�1�0�1�0�0�0�8  �0�4�0�5�0�8���0�7�����0�6.....
	if ( bForDoor && bAlarmStatus )  //�0�1�0�3�0�5���0�7�0�9�0�5�0�6�0�8���0�5��0�5�0�7�0�7�0�3���0�8�0�0�0�4���0�5�0�0�0�1
	{
		ucDoorValue = 0;
	}
	else if ( bForDoor && !bAlarmStatus ) 
	{
		ucDoorValue = 0x03;
	}
	else
	{
		ucDoorValue = 0x02;
	}
	pBuf[*iSendIndex] = ucDoorValue;
	*iSendIndex += 1;

	//�0�8�0�4�0�5�0�1�0�0�0�8
	pBuf[*iSendIndex] = (iVoltage >> 8) & 0xff;  //�0�6�0�8���0�0�0�4�0�3�0�8�0�3�0�8�0�1�0�2�0�3
	*iSendIndex += 1;

	pBuf[*iSendIndex] = iVoltage & 0xff;
	*iSendIndex += 1;

	//�0�1�0�8�0�8�0�4�0�8���0�4�0�1
	if ( bPowerType )
	{
		pBuf[*iSendIndex] = 0;
		*iSendIndex += 1;
	}
	else
	{
		pBuf[*iSendIndex] = 2;
		*iSendIndex += 1;
	}
}


/**************************************************************
Function:       CGbtMsgQueue::GetModuleStatus
Description:    ��ȡģ��״̬����ֵ		
Input:          iSendIndex  ��ǰ����֡�ֽڵ�ַ
Output:         pBuf  ����֡��ַ
Return:         ��
***************************************************************/
void CGbtMsgQueue::GetModuleStatus(Byte* pBuf,int *iSendIndex)
{
	Byte ucTmp = 0;
	CDetector* pDet          = CDetector::CreateInstance();
	CLampBoard* pLamp        = CLampBoard::CreateInstance();
	CFlashMac* pFlashMac     = CFlashMac::CreateInstance();
	
	if ( pLamp->m_bRecordSts[0] != true )
	{
		ucTmp |= 1;
	}
	if (  pLamp->m_bRecordSts[1] != true )
	{
		ucTmp |= 1<<1;
	}
	if ( pLamp->m_bRecordSts[2] != true )
	{
		ucTmp |= 1<<2;
	}
	if ( pLamp->m_bRecordSts[3] != true )
	{
		ucTmp |= 1<<3;
	}

	//�0�3���0�5�0�9�0�4�¡�0�2
	if ( pDet->m_bRecordSts[0] != true )
	{
		ucTmp |= 1<<4;
	}
	if ( pDet->m_bRecordSts[1] != true )
	{
		ucTmp |= 1<<5;
	}
	if ( pDet->m_bRecordSts[2] != true)
	{
		ucTmp |= 1<<6;
	}
	if ( pDet->m_bRecordSts[3] != true )
	{
		ucTmp |= 1<<7;
	}
	pBuf[*iSendIndex] = ucTmp;   
	*iSendIndex      += 1;
	ucTmp             = 0;

	//�0�8�0�4�0�8�0�5��0�2
	if ( false )//if ( pPowerBoard->m_bRecordSts != true )
	{
		ucTmp |= 1;
	}
	
	//�0�3���0�7�0�1��0�2 �0�3�0�4�0�7�0�9�0�4�¡�0�2
	if ( pFlashMac->m_bRecordSts != true )
	{
		ucTmp |= 1<<1;
	}
	pBuf[*iSendIndex] = ucTmp;   
	*iSendIndex      += 1;

	pBuf[*iSendIndex] = 0;   
	*iSendIndex      += 1;

	pBuf[*iSendIndex] = 0;   
	*iSendIndex      += 1;
}

/**************************************************************
Function:       CGbtMsgQueue::SetPscNum
Description:    �������˰�ťֵ		
Input:          pBuf  ����֡�����ַ
Output:         iRecvIndex  ���ջ��浱ǰ��ȡ��ַ
Return:         ��
***************************************************************/
void CGbtMsgQueue::SetPscNum(Byte* pBuf,int& iRecvIndex)
{
	CPscMode::CreateInstance()->m_ucBtnNum = pBuf[iRecvIndex++];
}

/**************************************************************
Function:       CGbtMsgQueue::SetTmpPattern
Description:    ������ʱ������λ���У�Ĭ��60��
Input:          pBuf  ����֡�����ַ
Output:         iRecvIndex  ���ջ��浱ǰ��ȡ��ַ
Return:         ��
***************************************************************/
void CGbtMsgQueue::SetTmpPattern(Byte* pBuf,int& iRecvIndex)
{	
	CManaKernel *pManaKernel = CManaKernel::CreateInstance();
	
	
	if(pManaKernel->m_iTimePatternId == 0)
	{	
		pManaKernel->bTmpPattern = true ;
		Ushort TmpPhase = pBuf[iRecvIndex++];
		GBT_DB::StagePattern TmpStgPatterb ;
		TmpStgPatterb.usAllowPhase = TmpPhase ;
		TmpStgPatterb.ucGreenTime = 60 ;
		TmpStgPatterb.ucYellowTime = 0 ;
		TmpStgPatterb.ucRedTime = 0 ;
		TmpStgPatterb.ucOption = 0 ;
		(CDbInstance::m_cGbtTscDb).ModStagePattern(17, 1, TmpStgPatterb);		
		ACE_DEBUG((LM_DEBUG,"%s:%d usAllowPhase ==%d \n",__FILE__,__LINE__,TmpStgPatterb.usAllowPhase));
		pManaKernel->UpdateConfig();
		
		SThreadMsg sTscMsg ;
		pManaKernel->m_iTimePatternId = 251;
		sTscMsg.ulType       = TSC_MSG_PATTER_RECOVER;  //�����ⷽ������ԭ��״̬
		sTscMsg.ucMsgOpt     = 0;
		sTscMsg.uiMsgDataLen = 1;
		sTscMsg.pDataBuf     = NULL;
		CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));
		ACE_DEBUG((LM_DEBUG,"%s:%d TSC_MSG_TIMEPATTERN == 251 \n",__FILE__,__LINE__));
	 }
	 else
	 	return ;

}

/**************************************************************
Function:       CGbtMsgQueue::SetLampBdtCfg
Description:    ���õƿذ���ݼ��ͺ��̳�ͻ�������		
Input:          pBuf   �������û���ָ��
				iRecvIndex     ��ǰ����ȡֵ��ַ
Output:         ��
Return:         ��
***************************************************************/
void CGbtMsgQueue::SetLampBdtCfg(Byte* pBuf,int& iRecvIndex)
{
	
	Byte ucSetType = pBuf[iRecvIndex++];
	if(ucSetType != 0x03)
		return ;
	CLampBoard *pLampBd = CLampBoard::CreateInstance();
	Byte ucBdIndex = pBuf[iRecvIndex++];
	pLampBd->m_ucCheckCfg[ucBdIndex] = pBuf[iRecvIndex++];
	if(pLampBd->m_ucCheckCfg[ucBdIndex] == 0xa)
		CManaKernel::CreateInstance()->m_pRunData->bIsChkLght = true ;
	else if(pLampBd->m_ucCheckCfg[ucBdIndex] == 0x5)
		CManaKernel::CreateInstance()->m_pRunData->bIsChkLght = false ;
	pLampBd->SendSingleCfg(ucBdIndex);
}


/**************************************************************
Function:       CGbtMsgQueue::SetSysFunc
Description:    ��������ϵͳ��������	
Input:          pBuf   �������û���ָ��
				iRecvIndex     ��ǰ����ȡֵ��ַ
Output:         ��
Return:         ��
***************************************************************/
void CGbtMsgQueue::SetSysFunc(Byte* pBuf,int& iRecvIndex)
{
	CManaKernel *pManakernel = CManaKernel::CreateInstance();
	Byte Tmp = pBuf[iRecvIndex++];	
	//char SysEypSerial[32]={0};	
	switch(Tmp)
	{
		case 0x01 :			
			pManakernel->m_pRunData->uiUtcsHeartBeat = 0; //���յ��������ۻ���0
			if(pManakernel->bUTS == false)
			{
				pManakernel->bUTS = true ;
			}
			if(pManakernel->m_pRunData->uiCtrl != CTRL_UTCS && pManakernel->m_pRunData->uiCtrl != CTRL_PANEL)
			{
				pManakernel->m_iTimePatternId = 0;    //ADD:201311111530
				pManakernel->bTmpPattern = false ;     //ADD:201311111530
				if(pManakernel->bDegrade == true)
					pManakernel->bDegrade = false ;
				//SendTscCommand(OBJECT_SWITCH_CONTROL,12);
				pManakernel->SwitchCtrl(CTRL_UTCS);
				if(pManakernel->m_pRunData->uiWorkStatus != STANDARD) //�ӽ������� Ϩ�� ȫ�췵��
					pManakernel->SwitchStatus(STANDARD);
				CMainBoardLed::CreateInstance()->DoModeLed(false,true);  //MODָʾ������
			}
		
			break ;
		case 0x02 :			
			(CDbInstance::m_cGbtTscDb).SetEypSerial();
			 //(CDbInstance::m_cGbtTscDb).GetEypSerial(SysEypSerial);
			 //ACE_DEBUG((LM_DEBUG,"%s:%d Get SysEypSerial = %s \n",__FILE__,__LINE__,SysEypSerial));
			break ;
		case 0x03 :	
			{ 
		   	 	Byte uEvtTypeId = 0 ;
				Uint uStartTime = 0 ;
				Uint uEndTime = 0 ;
				uEvtTypeId = pBuf[iRecvIndex++] ;
				ACE_DEBUG((LM_DEBUG,"%s:%d uEvtTypeId = %d \n",__FILE__,__LINE__,uEvtTypeId));
				for(Byte uNum = 0 ;uNum <4 ;uNum++)
				{
					uStartTime |= (pBuf[iRecvIndex++])<<(8*uNum);
				}
				for(Byte uNum = 0 ;uNum <4 ;uNum++)
				{
					uEndTime |= (pBuf[iRecvIndex++])<<(8*uNum);
				}
				ACE_DEBUG((LM_DEBUG,"%s:%d StartTime = %0X \n",__FILE__,__LINE__,uStartTime));
				ACE_DEBUG((LM_DEBUG,"%s:%d EndTime = %0X \n",__FILE__,__LINE__,uEndTime));
				 (CDbInstance::m_cGbtTscDb).DelEventLog(uEvtTypeId , uStartTime , uEndTime) ;
				
			break ;
			}
		case 0x04 :
			{
			Byte updateType = 0xff ;
			updateType= pBuf[iRecvIndex++] ;
			if(updateType == 0x1)
			{
				ACE_OS::system("mv Gb.aiton /userdata/backup/Gb.aiton.`date +%Y%m%d`");			
			}
			else if(updateType == 0x2)
			{
				ACE_OS::system("chmod 777 /Gb*");
				ACE_OS::system("chmod 777 /Gb*");
				ACE_OS::system("reboot");
			}
			else if(updateType == 0x3)		
			{
				ACE_OS::system("cp Gb.aiton /userdata/backup/Gb.aiton.`date +%Y%m%d`.bak");	
				ACE_OS::system("cp GbAitonTsc.db /userdata/backup/GbAitonTsc.db.`date +%Y%m%d`.bak");	
			}
			
			break ;
			}
		case 0x05:
			;
			break ;
		default:
			break ;
	}
	return ;

}



/**************************************************************
Function:       CGbtMsgQueue::GetFlashCfg
Description:    ��ȡ������������Ϣ		
Input:         	iSendIndex     ����֡��ǰд��ַ
Output:         pBuf   ����֡��ַָ��
Return:         ��
***************************************************************/
void CGbtMsgQueue::GetFlashCfg(Byte* pBuf,int *iSendIndex)
{
	CFlashMac* pFlashMac = CFlashMac::CreateInstance();
	pFlashMac->FlashCfgGet();
	ACE_OS::sleep(ACE_Time_Value(0, 30000)); 
	
	pBuf[*iSendIndex] = pFlashMac->m_ucGetFlashRate & 0x0f;
	pBuf[*iSendIndex]|=  (pFlashMac->m_ucGetDutyCycle<<4) &0xf0 ;
	*iSendIndex += 1;

	pBuf[*iSendIndex] = (pFlashMac->m_ucGetSyType)&0xff ;
	*iSendIndex += 1;
	pBuf[*iSendIndex] = (pFlashMac->m_ucFlashStatus)&0xff;
	*iSendIndex += 1;

}


/**************************************************************
Function:       CGbtMsgQueue::SetFlashCtrl
Description:    ������������������Ϣ����	
Input:         	pBuf     ����֡��ַָ��
				iRecvIndex ����֡��ǰ��ȡ��ַ
Output:        	��
Return:         ��
***************************************************************/
void  CGbtMsgQueue::SetFlashCtrl(Byte* pBuf,int& iRecvIndex)
{
	Byte Tmp = pBuf[iRecvIndex++];
	CFlashMac* pFlashMac = CFlashMac::CreateInstance();
	pFlashMac->FlashForceEnd();
	
	switch(Tmp)
	{
		case 0x05 :
			pFlashMac->FlashForceEnd();
			break ;
		case 0x04 :			
			pFlashMac->FlashForceStart(pBuf[iRecvIndex++]);
			break ;
		case 0x03 :			
			pFlashMac->m_ucSetFlashRate = pBuf[iRecvIndex] & 0x0f;
			pFlashMac->m_ucSetDutyCycle = (pBuf[iRecvIndex++]>>4) & 0x0f;
			
			pFlashMac->m_ucSetSyType    = pBuf[iRecvIndex++] ;
				
			pFlashMac->FlashCfgSet();
			break ;
		default:
			break ;
	}
	return ;
}

/**************************************************************
Function:       CGbtMsgQueue::GetDetCfg
Description:    ��ȡ�������Ϣ	
Input:         	pBuf     ������֡��ַָ��
				iSendIndex ����֡��ǰд���ַ
				ucBdIndex   �����������
				ucQueryType ���ѯ�ļ������Ŀ
Output:        	��
Return:         ��
***************************************************************/
void CGbtMsgQueue::GetDetCfg(Byte* pBuf,Byte ucBdIndex,Byte ucQueryType,int *iSendIndex)
{
	CDetector* pDetctor = CDetector::CreateInstance();
	Byte Tmp = ucQueryType;
	Byte ucBoardIndex = ucBdIndex ;
	ACE_Time_Value tv = ACE_Time_Value(0,150000);
	switch(Tmp)
	{
		case 0x03 :
			pDetctor->GetDecVars(ucBoardIndex,0x03);
			ACE_OS::sleep(tv);
			for(int ucIndex = 0 ;ucIndex < 4 ;ucIndex++)
				pBuf[(*iSendIndex)++] = pDetctor->m_ucRoadSpeed[ucBoardIndex][ucIndex];
			break ;
		case 0x04 :
			pDetctor->GetDecVars(ucBoardIndex,0x04);
			ACE_OS::sleep(tv);
			for(int ucIndex = 4 ;ucIndex < 8 ;ucIndex++)
				pBuf[(*iSendIndex)++] = pDetctor->m_ucRoadSpeed[ucBoardIndex][ucIndex];
			break ;
		case 0x05 :
			pDetctor->GetDecVars(ucBoardIndex,0x05);
			ACE_OS::sleep(tv);
			for(int ucIndex = 0 ; ucIndex <8 ; ucIndex++)
				pBuf[(*iSendIndex)++] = pDetctor->m_ucGetDetDelicacy[ucBoardIndex][ucIndex];
			break ;
		case 0x06 :
			pDetctor->GetDecVars(ucBoardIndex,0x06);
			ACE_OS::sleep(tv);
			for(int ucIndex = 8 ; ucIndex <16 ; ucIndex++)
				pBuf[(*iSendIndex)++] = pDetctor->m_ucGetDetDelicacy[ucBoardIndex][ucIndex];
			break ;
		case 0x07 :
			pDetctor->GetDecVars(ucBoardIndex,0x07);
			ACE_OS::sleep(tv);
			for(int ucIndex = 0 ;ucIndex < 4 ;ucIndex++)
				pBuf[(*iSendIndex)++] = pDetctor->m_sGetLookLink[ucBoardIndex][ucIndex];
			break ;
		case 0x08 :
			pDetctor->GetDecVars(ucBoardIndex,0x08);
			ACE_OS::sleep(tv);
			for(int ucIndex = 4 ;ucIndex < 8 ;ucIndex++)
				pBuf[(*iSendIndex)++] = pDetctor->m_sGetLookLink[ucBoardIndex][ucIndex];
			break ;
		case 0x09 :
			pDetctor->GetDecVars(ucBoardIndex,0x09);
			ACE_OS::sleep(tv);
			for(int ucIndex = 0 ;ucIndex < 4 ;ucIndex++)
				pBuf[(*iSendIndex)++] = pDetctor->m_ucGetRoadDis[ucBoardIndex][ucIndex];
			break ;
		case 0x0a :
			pDetctor->GetDecVars(ucBoardIndex,0x0a);
			ACE_OS::sleep(tv);
			for(int ucIndex = 4 ;ucIndex < 8 ;ucIndex++)
				pBuf[(*iSendIndex)++] = pDetctor->m_ucGetRoadDis[ucBoardIndex][ucIndex];
			break ;
		case 0x12 :
			pDetctor->GetDecVars(ucBoardIndex,0x12);
			ACE_OS::sleep(tv);
			for(int ucIndex = 0 ;ucIndex < 7 ;ucIndex++)
				pBuf[(*iSendIndex)++] = pDetctor->m_ucGetSensibility[ucBoardIndex][ucIndex];
			break ;
		case 0x13 :
			pDetctor->GetDecVars(ucBoardIndex,0x13);
			ACE_OS::sleep(tv);
			for(int ucIndex = 7 ;ucIndex < 14 ;ucIndex++)
				pBuf[(*iSendIndex)++] = pDetctor->m_ucGetSensibility[ucBoardIndex][ucIndex];
			break ;
		case 0x14 :
			pDetctor->GetDecVars(ucBoardIndex,0x14);
			ACE_OS::sleep(tv);
			for(int ucIndex = 14 ;ucIndex < 16 ;ucIndex++)
				pBuf[(*iSendIndex)++] = pDetctor->m_ucGetSensibility[ucBoardIndex][ucIndex];
			break ;
		case 0x18 :
			pDetctor->GetDecVars(ucBoardIndex,0x18);
			ACE_OS::sleep(tv);
			for(int ucIndex = 0 ;ucIndex < 16 ;ucIndex++)
				pBuf[(*iSendIndex)++] = pDetctor->m_ucGetFrency[ucBoardIndex][ucIndex];
			break ;
		default :
			return ;
	}
}


/**************************************************************
Function:       CGbtMsgQueue::SetDetCfg
Description:    ���ü����������Ϣ	
Input:         	pBuf     ����֡��ַָ��
				iRecvIndex  ����֡��ǰ��ȡ��ַ
Output:        	��
Return:         ��
***************************************************************/
void CGbtMsgQueue::SetDetCfg(Byte* pBuf,int& iRecvIndex)
{
	Byte ucSetType = pBuf[iRecvIndex++];
	Byte ucBdIndex = pBuf[iRecvIndex++];
	CDetector* pDetctor = CDetector::CreateInstance();
	
	switch(ucSetType)
	{
		case 0x0b :
			for(int ucIndex = 0 ;ucIndex <4 ; ucIndex++)
			{
				pDetctor->m_ucSetDetDelicacy[ucBdIndex][2*ucIndex] = pBuf[iRecvIndex] &0x0f;
				pDetctor->m_ucSetDetDelicacy[ucBdIndex][2*ucIndex+1] = (pBuf[iRecvIndex]>>4) &0x0f;
				iRecvIndex++;
			}	
			pDetctor->SendDelicacy(ucBdIndex, 0x0b);
			break ;
		case 0x0c :
			for(int ucIndex = 4 ;ucIndex <8 ; ucIndex++)
			{
				pDetctor->m_ucSetDetDelicacy[ucBdIndex][2*ucIndex] = pBuf[iRecvIndex] &0x0f;
				pDetctor->m_ucSetDetDelicacy[ucBdIndex][2*ucIndex+1] = (pBuf[iRecvIndex]>>4) &0x0f;
				iRecvIndex++;
			}	
			pDetctor->SendDelicacy(ucBdIndex, 0x0c);
			break ;
		case 0x0d :
			for(int ucIndex = 0 ;ucIndex <4 ; ucIndex++)
			{
				pDetctor->m_sSetLookLink[ucBdIndex][ucIndex] = pBuf[iRecvIndex++];		
				
			}	
			pDetctor->SendDecIsLink(ucBdIndex,0);				
			pDetctor->SendDetLink(ucBdIndex,DET_HEAD_COIL0104_SET);
					
			break ;
		case 0x0e :
			for(int ucIndex = 4 ;ucIndex <8 ; ucIndex++)
			{
				pDetctor->m_sSetLookLink[ucBdIndex][ucIndex] = pBuf[iRecvIndex++];
				
			}	
			pDetctor->SendDecIsLink(ucBdIndex,0);				
			pDetctor->SendDetLink(ucBdIndex,DET_HEAD_COIL0508_SET);
			break ;
		case 0x0f :
			for(int ucIndex = 0 ;ucIndex <4 ; ucIndex++)
			{
				pDetctor->m_ucSetRoadDis[ucBdIndex][ucIndex] = pBuf[iRecvIndex++];		
				
			}	
			pDetctor->SendRoadDistance(ucBdIndex,DET_HEAD_DISTAN0104_SET);
					
			break ;
		case 0x10 :
			for(int ucIndex = 4 ;ucIndex <8 ; ucIndex++)
			{
				pDetctor->m_ucSetRoadDis[ucBdIndex][ucIndex] = pBuf[iRecvIndex++];
				
			}	
			pDetctor->SendRoadDistance(ucBdIndex,DET_HEAD_DISTAN0508_SET);
			break ;
		case 0x19 :
			for(int ucIndex = 0 ;ucIndex < 4 ; ucIndex++)
			{
				pDetctor->m_ucSetFrency[ucBdIndex][4*ucIndex] = pBuf[iRecvIndex] & 0x03;
				pDetctor->m_ucSetFrency[ucBdIndex][4*ucIndex+1] = (pBuf[iRecvIndex]>>2) & 0x03;
				pDetctor->m_ucSetFrency[ucBdIndex][4*ucIndex+2] = (pBuf[iRecvIndex] >>4)& 0x03;
				pDetctor->m_ucSetFrency[ucBdIndex][4*ucIndex+3] = (pBuf[iRecvIndex]>>6) & 0x03;
				iRecvIndex++;
			}	
			pDetctor->SendDecFrency(ucBdIndex);
			break ;
		case 0x1a :
			pDetctor->m_iChkType = pBuf[iRecvIndex++] ;
			pDetctor->SendDecWorkType(ucBdIndex);
		case 0x15 :			
					
			for(int ucIndex = 0 ;ucIndex < 7 ;ucIndex++)
			   pDetctor->m_ucSetSensibility[ucBdIndex][ucIndex]=pBuf[iRecvIndex++] ;
							
			pDetctor->SendDecSenData(ucBdIndex,DET_HEAD_SENDATA0107_SET);
			break ;
		case 0x16 :
			
			for(int ucIndex = 7 ;ucIndex < 14 ;ucIndex++)
				pDetctor->m_ucSetSensibility[ucBdIndex][ucIndex]=pBuf[iRecvIndex++];
			pDetctor->SendDecSenData(ucBdIndex,DET_HEAD_SENDATA0814_SET);
			break ;
		case 0x17 :
			pDetctor->GetDecVars(ucBdIndex,0x14);
			for(int ucIndex = 14 ;ucIndex < 16 ;ucIndex++)
				pDetctor->m_ucSetSensibility[ucBdIndex][ucIndex]=pBuf[iRecvIndex++];
			pDetctor->SendDecSenData(ucBdIndex,DET_HEAD_SENDATA1516_SET);
			break ;
					
		default :
			return ;


	}

	printf("\n%s:%d ucSetType= %d ucBdIndex = %d \n",__FILE__,__LINE__,ucSetType,ucBdIndex);

}


/**************************************************************
Function:       CGbtMsgQueue::PackTscStatus
Description:    �źŻ�״̬��Ϣ���	
Input:         	pValue  ��Ϣ��״̬�ṹ��ָ��
Output:        	��
Return:         ��
***************************************************************/
void CGbtMsgQueue::PackTscStatus(Byte ucDealDataIndex,void* pValue)
{
	//Byte ucRecvOptType = ( m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[0]) & 0xf;   //�0�8�0�9�0�8�0�4�0�0�0�3�0�8�0�2�0�5�0�2���0�8���0�4�0�1
	int iRecvIndex     = m_sGbtDealData[ucDealDataIndex].sRecvFrame.iIndex;          
	int iRecvBufLen    = m_sGbtDealData[ucDealDataIndex].sRecvFrame.iBufLen;
	int iSendIndex     = m_sGbtDealData[ucDealDataIndex].sSendFrame.iIndex;          
	//int iSendBufLen    = m_sGbtDealData[ucDealDataIndex].sSendFrame.iBufLen;
	Byte ucIndexCnt    = 0;  //�0�9�0�6�0�5�0�6�0�2�0�8�0�5
	Byte ucErrorSts    = 0;  //�0�5���0�2�����0�5�0�0�0�1
	Byte ucErrorIdx    = 0;  //�0�5���0�2���0�9�0�6�0�5
	Byte ucObjId       = 0;  //�0�9�0�8�0�3���0�1�0�4(�����0�1�0�4)
	Byte ucIdxFst      = 0;  //�0�8�0�3�0�6�0�3�0�6�0�2�0�9�0�6�0�5(id1)
	Byte ucIdxSnd      = 0;  //�0�8�0�3�0�9�0�6�0�6�0�2�0�9�0�6�0�5(id2)
	Byte ucSubId       = 0;  //���0�7�0�9�0�8�0�3��(���0�0�0�9�0�2�0�3�0�0����)
	Byte ucIndex       = 0;
	Byte ucRecordCnt   = 0;
	STscStatus* pTscStatus = (STscStatus*)pValue;

	if ( iRecvIndex >= iRecvBufLen )
	{
		ucErrorSts = GBT_ERROR_OTHER;
		GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
		return;
	}

	/************�0�9�0�8�0�3�������0�8�0�9*************/
	ucObjId = m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex];
	m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex] = ucObjId;   //�0�9�0�8�0�3�������0�8�0�9
	iRecvIndex++;
	iSendIndex++;

	/***********�0�9�0�6�0�5�0�6�0�2�0�8�0�5�0�7�0�5���0�7�0�9�0�8�0�3��*******/
	if ( iRecvIndex >= iRecvBufLen )
	{
		ucErrorSts = GBT_ERROR_OTHER;
		GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
		return;
	}
	m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex] =
		m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex]; //�0�9�0�6�0�5�0�6�0�2�0�8�0�5�0�7�0�5���0�7�0�9�0�8�0�3��
	ucIndexCnt = (m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex]>>6) & 0x3;  //�0�9�0�6�0�5�0�6�0�2�0�8�0�5
	ucSubId    = m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex] & 0x3F;      //���0�7�0�9�0�8�0�3���0�5�0�1���0�0�0�9�0�2�0�3�0�0����
	iRecvIndex++;
	iSendIndex++;

	/***********�0�9�0�6�0�5*************/
	if ( ucIndexCnt > 0 )  /*�0�9�0�6�0�51*/
	{
		if ( iRecvIndex >= iRecvBufLen )
		{
			ucErrorSts = GBT_ERROR_OTHER;
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
		ucIdxFst = m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex];
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex] = ucIdxFst;
		iRecvIndex++;
		iSendIndex++;
		ucIndexCnt--;
	}
	if ( ucIndexCnt > 0 ) /*�0�9�0�6�0�52*/
	{
		if ( iRecvIndex >= iRecvBufLen )
		{
			ucErrorSts = GBT_ERROR_OTHER;
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
		ucIdxSnd = m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex];
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex] = ucIdxSnd;
		iRecvIndex++;
		iSendIndex++;
		ucIndexCnt--;
	}
	if ( ucIndexCnt > 0 )  /*�0�9�0�6�0�53�0�2�0�2�0�0�0�8*/
	{
		if ( iRecvIndex >= iRecvBufLen )
		{
			ucErrorSts = GBT_ERROR_OTHER;
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
		iRecvIndex++;
		iSendIndex++;
		ucIndexCnt--;
	}

	/************�0�0�0�8�0�7��**************/
	switch ( ucObjId )
	{
	case OBJECT_CURTSC_CTRL:          /*�0�8���0�5��0�4�0�3�0�2�0�3�0�3���0�8�0�2�0�7�0�1�0�0�0�4���0�5�0�0�0�1*/
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = ToObjectCurTscCtrl(pTscStatus->uiCtrl);
		break;
	/*
	case OBJECT_SWITCH_CONTROL:
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 
			ToObjectControlSwitch(pTscStatus->uiWorkStatus,pTscStatus->uiCtrl);
		break;
	
	case OBJECT_SWITCH_STAGE:        //�0�4���0�9�0�2���0�5�0�0�0�1
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = pTscStatus->ucStageNo;
		break;
	*/
	case OBJECT_ACTIVESCHEDULE_NO:  /*�0�8���0�5��0�3�0�6�0�9�0�4�0�8���0�9�0�2�����0�2�0�3*/
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = pTscStatus->ucActiveSchNo;
		break;
	case OBJECT_TSC_WARN2:  /*�0�4�0�3�0�2�0�3�0�3�����0�8�0�0�0�42*/
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = pTscStatus->ucTscAlarm2;
		break;
	case OBJECT_TSC_WARN1: /*�0�4�0�3�0�2�0�3�0�3�����0�8�0�0�0�41*/
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = pTscStatus->ucTscAlarm1;
		break;
	case OBJECT_TSC_WARN_SUMMARY:  /*�0�4�0�3�0�2�0�3�0�3�����0�8�0�0�0�4�0�9�0�9�0�6�0�9*/
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = pTscStatus->ucTscAlarmSummary;
		break;
	case OBJECT_ACTIVEDETECTOR_NUM: /*�0�3�0�6�0�9�0�4�0�3���0�5�0�9�0�4�¡��0�5�0�8�0�5*/
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = pTscStatus->ucActiveDetCnt;
		break;
	case OBJECT_SWITCH_MANUALCONTROL: /*�0�8�0�0�0�9�0�4�0�7�0�1�0�0�0�4���0�4��0�6*/
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] 
			= GetManualCtrlStatus(pTscStatus->uiWorkStatus,pTscStatus->uiCtrl);
		break;
	case OBJECT_SWITCH_SYSTEMCONTROL: /*�0�3�0�8�0�1�0�6�0�7�0�1�0�0�0�4���0�4��0�6*/
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] 
			= GetSysCtrlStatus(pTscStatus->uiWorkStatus,pTscStatus->uiCtrl);
		break;
	case OBJECT_SWITCH_CONTROL: /*�0�7�0�1�0�0�0�4���0�4�0�8�0�4*/
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] 
			= GetCtrlStatus(pTscStatus->uiWorkStatus,pTscStatus->uiCtrl);
		break;
	case OBJECT_SWITCH_STAGE:  /*�0�4���0�9�0�2���0�5�0�0�0�1*/
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = pTscStatus->ucStageNo;
		break;
	case OBJECT_GOSTEP: /*�0�5�0�4�0�4�0�3�0�0�0�6�0�9�0�6*/
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = pTscStatus->ucStepNo;
		break;
	case OBJECT_CURPATTERN_SCHTIMES:   /*�0�8���0�5�㡤�0�4��0�6�0�6�0�4���0�9�0�2�0�8���0�6��*/
		ACE_OS::memcpy(m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf+iSendIndex,
			                  pTscStatus->ucCurStageLen,16);
	    iSendIndex += 16;
		break;
	case OBJECT_CURPATTERN_GREENTIMES: /*�0�8���0�5�㡤�0�4��0�6�0�6�0�1�0�1�0�3���0�3���0�2�0�3�0�0�0�0�0�8�0�4�0�8���0�6��*/
		ACE_OS::memcpy(m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf+iSendIndex,
			pTscStatus->ucCurKeyGreen,16);
		iSendIndex += 16;
		break;
	case OBJECT_DETECTORSTS_TABLE: 
		if ( (0==ucIdxFst) && (0==ucIdxSnd) && (0==ucSubId) ) 
		{
			ucIndex     = 0;
			ucRecordCnt = 8;
			m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = ucRecordCnt;
			while ( ucIndex < ucRecordCnt )
			{
				/*
				ACE_OS::memcpy(m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf+iSendIndex
					            ,(pTscStatus->sDetSts)+ucIndex,sizeof(SDetectorStsPara));
				iSendIndex += sizeof(SDetectorStsPara);
				*/
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] 
												= ((pTscStatus->sDetSts)+ucIndex)->ucId;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] 
												= ((pTscStatus->sDetSts)+ucIndex)->ucStatus;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] 
												= ((pTscStatus->sDetSts)+ucIndex)->ucAlarm;
											
				//ACE_DEBUG((LM_DEBUG,"ID:%d\n",((pTscStatus->sDetSts)+ucIndex)->ucId));

				ucIndex++;
			}
		}
		else if ( (ucIdxFst>0) && (ucIdxFst<9) )
		{
			switch ( ucSubId )
			{
			case 0:
				ucIndex = ucIdxFst-1;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 1;
				/*
				ACE_OS::memcpy(m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf+iSendIndex,
					(pTscStatus->sDetSts)+ucIdxFst-1,sizeof(SDetectorStsPara));
				iSendIndex += sizeof(SDetectorStsPara);
				*/
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] 
												= ((pTscStatus->sDetSts)+ucIndex)->ucId;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] 
												= ((pTscStatus->sDetSts)+ucIndex)->ucStatus;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] 
												= ((pTscStatus->sDetSts)+ucIndex)->ucAlarm;
				break;
			case 1:
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 
					((pTscStatus->sDetSts)+ucIdxFst-1)->ucId;
				break;
			case 2:
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 
					((pTscStatus->sDetSts)+ucIdxFst-1)->ucStatus;
				break;
			case 3:
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 
					((pTscStatus->sDetSts)+ucIdxFst-1)->ucAlarm;
				break;
			default:
				GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
				return;
			}
		}
		else
		{
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
		break;
	case OBJECT_DETECTORDATA_TABLE: /*�0�4�0�3�0�1�0�8�0�3���0�5�0�9�0�8�0�5�0�0�0�6����*/
		if ( (0==ucIdxFst) && (0==ucIdxSnd) && (0==ucSubId) )  //�0�9�0�4�0�9�0�3����
		{
			ucIndex = 0;
			ucRecordCnt = 48;
			m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = ucRecordCnt;

			while ( ucIndex < ucRecordCnt )
			{
				/*
				ACE_OS::memcpy(m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf+iSendIndex
								,(pTscStatus->sDetData)+ucIndex,sizeof(SDetectorDataPara));
				iSendIndex += sizeof(SDetectorDataPara);
				*/
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= ((pTscStatus->sDetData)+ucIndex)->ucId;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= ((pTscStatus->sDetData)+ucIndex)->ucVolume;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= ((pTscStatus->sDetData)+ucIndex)->ucLongVolume;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= ((pTscStatus->sDetData)+ucIndex)->ucSmallVolume;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= ((pTscStatus->sDetData)+ucIndex)->ucOccupancy;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= ((pTscStatus->sDetData)+ucIndex)->ucVelocity;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= ((pTscStatus->sDetData)+ucIndex)->ucVehLen;

				ucIndex++;
			}
		}
		else if ( (ucIdxFst>0) && (ucIdxFst<49) )
		{
			switch ( ucSubId )
			{
			case 0:
				ucIndex = ucIdxFst - 1;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 1;
				/*
				ACE_OS::memcpy(m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf+iSendIndex
					,(pTscStatus->sDetData)+ucIdxFst-1,sizeof(SDetectorDataPara));
				iSendIndex += sizeof(SDetectorDataPara);
				*/
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= ((pTscStatus->sDetData)+ucIndex)->ucId;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= ((pTscStatus->sDetData)+ucIndex)->ucVolume;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= ((pTscStatus->sDetData)+ucIndex)->ucLongVolume;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= ((pTscStatus->sDetData)+ucIndex)->ucSmallVolume;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= ((pTscStatus->sDetData)+ucIndex)->ucOccupancy;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= ((pTscStatus->sDetData)+ucIndex)->ucVelocity;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= ((pTscStatus->sDetData)+ucIndex)->ucVehLen;
				break;
			case 1:
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 
					((pTscStatus->sDetData)+ucIdxFst-1)->ucId;
				break;
			case 2:
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 
					((pTscStatus->sDetData)+ucIdxFst-1)->ucVolume;
				break;
			case 3:
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 
					((pTscStatus->sDetData)+ucIdxFst-1)->ucLongVolume;
				break;
			case 4:
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 
					((pTscStatus->sDetData)+ucIdxFst-1)->ucSmallVolume;
				break;
			case 5:
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 
					((pTscStatus->sDetData)+ucIdxFst-1)->ucOccupancy;
				break;
			case 6:
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 
					((pTscStatus->sDetData)+ucIdxFst-1)->ucVelocity;
				break;
			case 7:
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 
					((pTscStatus->sDetData)+ucIdxFst-1)->ucVehLen;
				break;
			default:
				GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
				return;
			}
		}
		else
		{
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
		break;
	case OBJECT_DETECTORWARN_TABLE: /*�0�6�0�8�0�9�0�0�0�3���0�5�0�9�0�4�0�6�0�3�0�0�0�4*/
		if ( (0==ucIdxFst) && (0==ucIdxSnd) && (0==ucSubId) )  //�0�9�0�4�0�9�0�3����
		{
			ucIndex = 0;
			ucRecordCnt = 48;
			m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = ucRecordCnt;
			
			while ( ucIndex < ucRecordCnt )
			{
				/*
				ACE_OS::memcpy(m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf+iSendIndex
					,(pTscStatus->sDetAlarm)+ucIndex,sizeof(SDetectorAlarm));
				iSendIndex += sizeof(SDetectorAlarm);
				*/
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
							= ((pTscStatus->sDetAlarm)+ucIndex)->ucId;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
							= ((pTscStatus->sDetAlarm)+ucIndex)->ucDetAlarm;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
							= ((pTscStatus->sDetAlarm)+ucIndex)->ucCoilAlarm;
				ucIndex++;
			}
		}
		else if ( (ucIdxFst>0) && (ucIdxFst<49) )
		{
			switch ( ucSubId )
			{
			case 0:
				ucIndex = ucIdxFst - 1;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 1;
				/*
				ACE_OS::memcpy(m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf+iSendIndex
					,(pTscStatus->sDetAlarm)+ucIdxFst-1,sizeof(SDetectorAlarm));
				iSendIndex += sizeof(SDetectorAlarm);
				*/
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
							= ((pTscStatus->sDetAlarm)+ucIndex)->ucId;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
							= ((pTscStatus->sDetAlarm)+ucIndex)->ucDetAlarm;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
							= ((pTscStatus->sDetAlarm)+ucIndex)->ucCoilAlarm;

				break;
			case 1:
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 
					((pTscStatus->sDetAlarm)+ucIdxFst-1)->ucId;
				break;
			case 2:
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 
					((pTscStatus->sDetAlarm)+ucIdxFst-1)->ucDetAlarm;
				break;
			case 3:
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 
					((pTscStatus->sDetAlarm)+ucIdxFst-1)->ucCoilAlarm;
				break;
			default:
				GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
				return;
			}
		}
		else
		{
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
		break;
		break;
	case OBJECT_PHASESTATUS_TABLE:   /*�0�3���0�2�0�3���0�5�0�0�0�1*/
		if ( (0==ucIdxFst) && (0==ucIdxSnd) && (0==ucSubId) )  //�0�9�0�4�0�9�0�3����
		{
			ucIndex = 0;
			m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 2;
			while ( ucIndex < 2 )
			{
				/*
				ACE_OS::memcpy(m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf+iSendIndex
					,(pTscStatus->sPhaseSts)+ucIndex,sizeof(SPhaseStsPara));
				iSendIndex += sizeof(SPhaseStsPara);
				*/
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= ((pTscStatus->sPhaseSts)+ucIndex)->ucId;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= ((pTscStatus->sPhaseSts)+ucIndex)->ucRed;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= ((pTscStatus->sPhaseSts)+ucIndex)->ucYellow;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= ((pTscStatus->sPhaseSts)+ucIndex)->ucGreen;

				ucIndex++;
			}
		}
		else if ( (1 == ucIdxFst) || (2 == ucIdxFst) )
		{
			switch ( ucSubId )
			{
				case 0:
					ucIndex = ucIdxFst - 1;
					m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 1;
					/*
					ACE_OS::memcpy(m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf+iSendIndex
						,(pTscStatus->sPhaseSts)+ucIdxFst-1,sizeof(SPhaseStsPara));
					iSendIndex += sizeof(SPhaseStsPara);
					*/
					m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
							= ((pTscStatus->sPhaseSts)+ucIndex)->ucId;
					m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
							= ((pTscStatus->sPhaseSts)+ucIndex)->ucRed;
					m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
							= ((pTscStatus->sPhaseSts)+ucIndex)->ucYellow;
					m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
							= ((pTscStatus->sPhaseSts)+ucIndex)->ucGreen;
					break;
				case 1:
					m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 
						((pTscStatus->sPhaseSts)+ucIdxFst-1)->ucId;
					break;
				case 2:
					m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 
						((pTscStatus->sPhaseSts)+ucIdxFst-1)->ucRed;
					break;
				case 3:
					m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 
						((pTscStatus->sPhaseSts)+ucIdxFst-1)->ucYellow;
					break;
				case 4:
					m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 
						((pTscStatus->sPhaseSts)+ucIdxFst-1)->ucGreen;
					break;
				default:
					GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
					return;
			}
		}
		else
		{
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
		break;
	case OBJECT_OVERLAPPHASE_STATUS: /*�0�6���0�9�0�3�0�3���0�2�0�3���0�5�0�0�0�1*/
		if ( (0==ucIdxFst) && (0==ucIdxSnd) && (0==ucSubId) )  //�0�9�0�4�0�9�0�3����
		{
			m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 1;
			
			/*
			ACE_OS::memcpy(m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf+iSendIndex
				,&(pTscStatus->sOverlapPhaseSts),sizeof(SOverlapPhaseStsPara));
			iSendIndex += sizeof(SOverlapPhaseStsPara);
			*/

			m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= (pTscStatus->sOverlapPhaseSts).ucId;
			m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= (pTscStatus->sOverlapPhaseSts).ucRed;
			m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= (pTscStatus->sOverlapPhaseSts).ucYellow;
			m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= (pTscStatus->sOverlapPhaseSts).ucGreen;
		}
		else if ( 1 == ucIdxFst )
		{
			switch ( ucSubId )
			{
			case 0:
				/*
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 1;
				ACE_OS::memcpy(m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf+iSendIndex
					,&(pTscStatus->sOverlapPhaseSts),sizeof(SOverlapPhaseStsPara));
				iSendIndex += sizeof(SOverlapPhaseStsPara);
				*/
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= (pTscStatus->sOverlapPhaseSts).ucId;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= (pTscStatus->sOverlapPhaseSts).ucRed;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= (pTscStatus->sOverlapPhaseSts).ucYellow;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= (pTscStatus->sOverlapPhaseSts).ucGreen;
				break;
			case 1:
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] =
					(pTscStatus->sOverlapPhaseSts).ucId;
				break;
			case 2:
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] =
					(pTscStatus->sOverlapPhaseSts).ucRed;
				break;
			case 3:
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 
					(pTscStatus->sOverlapPhaseSts).ucYellow;
				break;
			case 4:
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 
					(pTscStatus->sOverlapPhaseSts).ucGreen;
				break;
			default:
				GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
				return;
			}
		}
		else
		{
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}

		break;
	case OBJECT_CHANNELSTATUS_TABLE:      /*�0�1�0�8�0�8�0�8���0�5�0�0�0�1*/
		if ( (0==ucIdxFst) && (0==ucIdxSnd) && (0==ucSubId) )  //�0�9�0�4�0�9�0�3����
		{
			ucIndex = 0;
			m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 2;

			while ( ucIndex < 2 )
			{
				/*
				ACE_OS::memcpy(m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf+iSendIndex
					,pTscStatus->sChannelSts,sizeof(SChannelStsPara));
				iSendIndex += sizeof(SChannelStsPara);
				*/
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= ((pTscStatus->sChannelSts)+ucIndex)->ucId;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= ((pTscStatus->sChannelSts)+ucIndex)->ucRed;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= ((pTscStatus->sChannelSts)+ucIndex)->ucYellow;
				m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
						= ((pTscStatus->sChannelSts)+ucIndex)->ucGreen;
				ucIndex++;
			}
		}
		else if ( 1 == ucIdxFst || 2 == ucIdxFst )
		{
			switch ( ucSubId )
			{
				case 0:
					m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 1;
					/*
					ACE_OS::memcpy(m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf+iSendIndex
						,(pTscStatus->sChannelSts)+ucIdxFst-1,sizeof(SChannelStsPara));
					iSendIndex += sizeof(SChannelStsPara);
					*/
					m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
							= ((pTscStatus->sChannelSts)+ucIndex)->ucId;
					m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
							= ((pTscStatus->sChannelSts)+ucIndex)->ucRed;
					m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
							= ((pTscStatus->sChannelSts)+ucIndex)->ucYellow;
					m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++]
							= ((pTscStatus->sChannelSts)+ucIndex)->ucGreen;
				break;
				case 1:
					m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 
						((pTscStatus->sChannelSts)+ucIdxFst-1)->ucId;
					break;
				case 2:
					m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 
						((pTscStatus->sChannelSts)+ucIdxFst-1)->ucRed;
					break;
				case 3:
					m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] =
						((pTscStatus->sChannelSts)+ucIdxFst-1)->ucYellow;
					break;
				case 4:
					m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] =
						((pTscStatus->sChannelSts)+ucIdxFst-1)->ucGreen;
					break;
				default:
					GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
					return;
			}
		}
		else
		{
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
		break;
	case OBJECT_CURTSC_FLASHCTRL:  /*�0�8���0�5��0�7�0�9�0�1�0�9�0�7�0�1�0�0�0�4�0�6�0�5�0�4�0�8�0�8�0�2�0�8�0�2�0�6��*/
		if ( pTscStatus->uiWorkStatus != FLASH )   //���0�5�0�3�0�4�0�7�0�9
		{
			m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 2;
		}
		else if ( CTRL_MANUAL == pTscStatus->uiCtrl ) //�0�8�0�0�0�9�0�4�0�3�0�4�0�7�0�9
		{
			m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 4;
		}
		else if ( true == pTscStatus->bStartFlash ) //�0�4�0�0�0�9�0�4�0�8���0�3�0�4�0�7�0�9
		{
			m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 7;
		}
		else
		{
			m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex++] = 3;
		}
		break;
	default:
#ifdef TSC_DEBUG
		ACE_DEBUG((LM_DEBUG,"%s:%d,objectId error objectid:%d\n",__FILE__,__LINE__,ucObjId));
#endif
		GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
		return;
	}

	m_sGbtDealData[ucDealDataIndex].sRecvFrame.iIndex = iRecvIndex;
	m_sGbtDealData[ucDealDataIndex].sSendFrame.iIndex = iSendIndex;
	m_sGbtDealData[ucDealDataIndex].iObjNum--;
	if ( iRecvIndex == iRecvBufLen )  //�0�5�0�7�0�8���0�1�����0�3
	{
		if ( 0 == m_sGbtDealData[ucDealDataIndex].iObjNum )  //�0�9�0�8�0�3���0�8�0�5�0�6�0�5�0�6�0�9�0�2�0�1�0�5�0�7�0�8���0�1�����0�3
		{
			m_sGbtDealData[ucDealDataIndex].sSendFrame.iBufLen = iSendIndex;
			GotoSendToHost(ucDealDataIndex);
			return;
		}
		else
		{
#ifdef TSC_DEBUG
			ACE_DEBUG((LM_DEBUG,"%s:%d,ObjNum error ObjNum:%d\n",
				               __FILE__,__LINE__,m_sGbtDealData[ucDealDataIndex].iObjNum));
#endif
			ucErrorSts = 5;
			ucErrorIdx = 0;
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
	}
	else
	{
		GotoDealRecvbuf(ucDealDataIndex);
		return;
	}
}


/**************************************************************
Function:       CGbtMsgQueue::PackTscExStatus
Description:    �źŻ���չ״̬��Ϣ���	
Input:         	ucDealDataIndex  ��������Ϣ�±�
				pValue  ��Ϣ����չ״̬�ṹ��ָ��
Output:        	��
Return:         ��
***************************************************************/
void CGbtMsgQueue::PackTscExStatus(Byte ucDealDataIndex,void* pValue)
{
	//Byte ucRecvOptType = ( m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[0]) & 0xf;   //�0�8�0�9�0�8�0�4�0�0�0�3�0�8�0�2�0�5�0�2���0�8���0�4�0�1
	int iRecvIndex     = m_sGbtDealData[ucDealDataIndex].sRecvFrame.iIndex;          
	int iRecvBufLen    = m_sGbtDealData[ucDealDataIndex].sRecvFrame.iBufLen;
	int iSendIndex     = m_sGbtDealData[ucDealDataIndex].sSendFrame.iIndex;          
	//int iSendBufLen    = m_sGbtDealData[ucDealDataIndex].sSendFrame.iBufLen;
	Byte ucIndexCnt    = 0;  //�0�9�0�6�0�5�0�6�0�2�0�8�0�5
	Byte ucErrorSts    = 0;  //�0�5���0�2�����0�5�0�0�0�1
	Byte ucErrorIdx    = 0;  //�0�5���0�2���0�9�0�6�0�5
	Byte ucObjId       = 0;  //�0�9�0�8�0�3���0�1�0�4(�����0�1�0�4)
	Byte ucIdxFst      = 0;  //�0�8�0�3�0�6�0�3�0�6�0�2�0�9�0�6�0�5(id1)
	Byte ucIdxSnd      = 0;  //�0�8�0�3�0�9�0�6�0�6�0�2�0�9�0�6�0�5(id2)
	Byte ucSubId       = 0;  //���0�7�0�9�0�8�0�3��(���0�0�0�9�0�2�0�3�0�0����)

	if ( iRecvIndex >= iRecvBufLen )
	{
		ucErrorSts = GBT_ERROR_OTHER;
		GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
		return;
	}

	/************�0�9�0�8�0�3�������0�8�0�9*************/
	ucObjId = m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex];
	m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex] = ucObjId;   //�0�9�0�8�0�3�������0�8�0�9
	iRecvIndex++;
	iSendIndex++;

	/***********�0�9�0�6�0�5�0�6�0�2�0�8�0�5�0�7�0�5���0�7�0�9�0�8�0�3��*******/
	if ( iRecvIndex >= iRecvBufLen )
	{
		ucErrorSts = GBT_ERROR_OTHER;
		GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
		return;
	}
	m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex] =
		m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex]; //�0�9�0�6�0�5�0�6�0�2�0�8�0�5�0�7�0�5���0�7�0�9�0�8�0�3��
	ucIndexCnt = (m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex]>>6) & 0x3;  //�0�9�0�6�0�5�0�6�0�2�0�8�0�5
	ucSubId    = m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex] & 0x3F;      //���0�7�0�9�0�8�0�3���0�5�0�1���0�0�0�9�0�2�0�3�0�0����
	iRecvIndex++;
	iSendIndex++;

	/***********�0�9�0�6�0�5*************/
	if ( ucIndexCnt > 0 )  /*�0�9�0�6�0�51*/
	{
		if ( iRecvIndex >= iRecvBufLen )
		{
			ucErrorSts = GBT_ERROR_OTHER;
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
		ucIdxFst = m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex];
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex] = ucIdxFst;
		iRecvIndex++;
		iSendIndex++;
		ucIndexCnt--;
	}
	if ( ucIndexCnt > 0 ) /*�0�9�0�6�0�52*/
	{
		if ( iRecvIndex >= iRecvBufLen )
		{
			ucErrorSts = GBT_ERROR_OTHER;
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
		ucIdxSnd = m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex];
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex] = ucIdxSnd;
		iRecvIndex++;
		iSendIndex++;
		ucIndexCnt--;
	}
	if ( ucIndexCnt > 0 )  /*�0�9�0�6�0�53�0�2�0�2�0�0�0�8*/
	{
		if ( iRecvIndex >= iRecvBufLen )
		{
			ucErrorSts = GBT_ERROR_OTHER;
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
		iRecvIndex++;
		iSendIndex++;
		ucIndexCnt--;
	}

	/************�0�0�0�8�0�7��**************/
	switch ( ucObjId )
	{
	case OBJECT_EXT_TSC_STATUS:  /*�0�8���0�5��0�4�0�3�0�2�0�3�0�3���0�8�0�2�0�7�0�1�0�0�0�4���0�5�0�0�0�1*/
		ACE_OS::memcpy(m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf+iSendIndex,(Byte*)pValue,25);
		iSendIndex += 25;
		break;
	default:
		GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
		return;
	}

	m_sGbtDealData[ucDealDataIndex].sRecvFrame.iIndex = iRecvIndex;
	m_sGbtDealData[ucDealDataIndex].sSendFrame.iIndex = iSendIndex;
	m_sGbtDealData[ucDealDataIndex].iObjNum--;
	if ( iRecvIndex == iRecvBufLen )  //�0�5�0�7�0�8���0�1�����0�3
	{
		if ( 0 == m_sGbtDealData[ucDealDataIndex].iObjNum )  //�0�9�0�8�0�3���0�8�0�5�0�6�0�5�0�6�0�9�0�2�0�1�0�5�0�7�0�8���0�1�����0�3
		{
			m_sGbtDealData[ucDealDataIndex].sSendFrame.iBufLen = iSendIndex;
			GotoSendToHost(ucDealDataIndex);
			return;
		}
		else
		{
			ucErrorSts = 5;
			ucErrorIdx = 0;
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
	}
	else
	{
		GotoDealRecvbuf(ucDealDataIndex);
		return;
	}
}


/**************************************************************
Function:       CGbtMsgQueue::GetCtrlStatus
Description:    ��ȡ�źŻ���ǰ����״̬
Input:         	uiWorkStatus  ����״̬
				uiCtrl  	  ���Ʒ�ʽ
Output:        	��
Return:         ���Ʒ�ʽ����ֵ
***************************************************************/
int CGbtMsgQueue::GetCtrlStatus(unsigned int uiWorkStatus,unsigned int uiCtrl)
{
	if ( SIGNALOFF == uiWorkStatus )
	{
		return 1;
	}
	else if ( FLASH == uiWorkStatus )
	{
		return 2;
	}
	else if ( ALLRED == uiWorkStatus )
	{
		return 3;
	}
	else
	{
		switch ( uiCtrl )
		{
		case CTRL_VEHACTUATED:
		case CTRL_MAIN_PRIORITY:
		case CTRL_SECOND_PRIORITY:
			return 6;
		case CTRL_ACTIVATE:
			return 8;
		case CTRL_HOST:
			return 11;
		case CTRL_UTCS:
			return 12;
		case CTRL_WIRELESS:
		case CTRL_LINE:
			return 13;
		default:
			return 5;
		}
	}
}


/**************************************************************
Function:       CGbtMsgQueue::GetSysCtrlStatus
Description:    ��ȡ�źŻ�ϵͳ����״̬
Input:         	uiWorkStatus  ����״̬
				uiCtrl  	  ���Ʒ�ʽ
Output:        	��
Return:         ϵͳ����״̬����ֵ
***************************************************************/
int CGbtMsgQueue::GetSysCtrlStatus(unsigned int uiWorkStatus,unsigned int uiCtrl)
{
	if ( CTRL_MANUAL != uiCtrl )
	{
		return m_iSystemCtrlNo;
	}
	else 
	{
		switch ( uiWorkStatus )
		{
		case ALLRED:
			return 253;
		case FLASH:
			return 254;
		case SIGNALOFF:
			return 255;
		default:
			return m_iSystemCtrlNo;
		}
	}
}


/**************************************************************
Function:       CGbtMsgQueue::GetManualCtrlStatus
Description:    ��ȡ�źŻ��ֶ�����״̬
Input:         	uiWorkStatus  ����״̬
				uiCtrl  	  ���Ʒ�ʽ
Output:        	��
Return:         �ֶ�����״̬����ֵ
***************************************************************/
int CGbtMsgQueue::GetManualCtrlStatus(unsigned int uiWorkStatus,unsigned int uiCtrl)
{
	if ( CTRL_MANUAL != uiCtrl )
	{
		return 0;
	}
	else 
	{
		switch ( uiWorkStatus )
		{
		case ALLRED:
			return 253;
		case FLASH:
			return 254;
		case SIGNALOFF:
			return 255;
		default:
			return m_iManualCtrlNo;
		}
	}
}


/**************************************************************
Function:       CGbtMsgQueue::DealRecvBuf
Description:    �Խ��յ�udp���ݽ��д���
Input:         	ucDealDataIndex  ������Ϣ��������±�
Output:        	��
Return:         ��
***************************************************************/
void CGbtMsgQueue::DealRecvBuf(Byte ucDealDataIndex) 
{
	Byte ucRecvOptType = ( m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[0] ) & 0xf;  //�0�8�0�9�0�8�0�4�0�0�0�3�0�8�0�2�0�5�0�2���0�8���0�4�0�1
	Byte ucSendOptType = 0;                                                              //���0�4�0�9�0�1�0�0�0�3�0�8�0�2�0�5�0�2���0�8���0�4�0�1
	int iRecvIndex     = m_sGbtDealData[ucDealDataIndex].sRecvFrame.iIndex;          
	int iRecvBufLen    = m_sGbtDealData[ucDealDataIndex].sRecvFrame.iBufLen;
	int iSendIndex     = m_sGbtDealData[ucDealDataIndex].sSendFrame.iIndex;          
	int iSendBufLen    = m_sGbtDealData[ucDealDataIndex].sSendFrame.iBufLen;
	Byte ucIndexCnt    = 0;    //�0�9�0�6�0�5�0�6�0�2�0�8�0�5
	Byte ucErrorSts    = 0;    //�0�5���0�2�����0�5�0�0�0�1
	Byte ucErrorIdx    = 0;    //�0�5���0�2���0�9�0�6�0�5
	Byte ucObjId       = 0;    //�0�9�0�8�0�3���0�1�0�4(�����0�1�0�4)
	Byte ucIdxFst      = 255;  //�0�8�0�3�0�6�0�3�0�6�0�2�0�9�0�6�0�5(id1)
	Byte ucIdxSnd      = 255;  //�0�8�0�3�0�9�0�6�0�6�0�2�0�9�0�6�0�5(id2)
	Byte ucSubId       = 0;    //���0�7�0�9�0�8�0�3��(���0�0�0�9�0�2�0�3�0�0����)
	int  iFunRet       = -1;   //�0�2�0�4�0�8�0�5���0�8�0�3�0�1�0�0�0�8

	/**********�0�8���0�5�0�2�0�5�0�7�0�8���0�6�0�1�0�0�0�3************/
	if ( 0 == iRecvIndex ) 
	{
		ucSendOptType = GetSendOperateType(ucRecvOptType);   
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[0] = 
			(m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[0] & 0xf0) | ucSendOptType;  //�0�1�����0�0�0�4�0�3
		m_sGbtDealData[ucDealDataIndex].iObjNum = 
			 ( (m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[0]>>4) & 7 ) + 1;  //�0�9�0�8�0�3���0�6�0�2�0�8�0�5

		iRecvIndex = 1;
		iSendIndex = 1;
	}
	
	while ( 1 )
	{
		if ( iRecvIndex >= iRecvBufLen )
		{
			ucErrorSts = GBT_ERROR_OTHER;
#ifdef TSC_DEBUG
			ACE_DEBUG((LM_DEBUG,"%s:%d,GBT_ERROR_OTHER\n",__FILE__,__LINE__));
#endif
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
		
		/************�0�9�0�8�0�3�������0�8�0�9*************/
		ucObjId = m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex];
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex] = ucObjId;   //�0�9�0�8�0�3�������0�8�0�9

		if ( ( IsSendTscCommand(ucObjId) && (GBT_SEEK_REQ == ucRecvOptType) ) 
			|| IsGetTscStatusObject(ucObjId) )  //�0�3�0�9�0�6�0�3�0�4�0�3�0�2�0�3�0�3�����0�5�0�0�0�1�0�9�0�8�0�3��
		{
			if ( GBT_SEEK_REQ != ucRecvOptType )
			{
				ucErrorSts = GBT_ERROR_OTHER;
#ifdef TSC_DEBUG
				ACE_DEBUG((LM_DEBUG,"%s:%d,GetTscStatusObject && !GBT_SEEK_REQ\n",__FILE__,__LINE__));
#endif
				GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
				return;
			}
			m_sGbtDealData[ucDealDataIndex].sRecvFrame.iIndex  = iRecvIndex; 
			m_sGbtDealData[ucDealDataIndex].sSendFrame.iIndex  = iSendIndex;          
			m_sGbtDealData[ucDealDataIndex].sSendFrame.iBufLen = iSendBufLen;

			SThreadMsg sTscMsg;
			sTscMsg.ulType       = TSC_MSG_STATUS_READ;
			sTscMsg.ucMsgOpt     = ucDealDataIndex;
			sTscMsg.uiMsgDataLen = 0;
			sTscMsg.pDataBuf     = NULL;

			CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(SThreadMsg));
			return;
		}
		else if ( ucObjId == OBJECT_SET_REPORTSELF )  //�0�0�0�9�0�4�0�7�0�3���0�8
		{
#ifdef GBT_TCP
			CGbtTimer::CreateInstance()->TailorReport(ucDealDataIndex 
				                                 , iRecvBufLen 
												 , m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf);
#else
			CGbtTimer::CreateInstance()->TailorReport(m_sGbtDealData[ucDealDataIndex].AddrClient
				                                   ,iRecvBufLen
												   ,m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf);
#endif
			CleanDealData(ucDealDataIndex);
			return;
		}
		else if ( IsOtherObject(ucObjId) )  
		{
			m_sGbtDealData[ucDealDataIndex].sRecvFrame.iIndex  = iRecvIndex; 
			m_sGbtDealData[ucDealDataIndex].sSendFrame.iIndex  = iSendIndex;          
			m_sGbtDealData[ucDealDataIndex].sSendFrame.iBufLen = iSendBufLen;

			SThreadMsg sTscMsg;
			sTscMsg.ulType       = GBT_MSG_OTHER_OBJECT;
			sTscMsg.ucMsgOpt     = ucDealDataIndex;
			sTscMsg.uiMsgDataLen = 0;
			sTscMsg.pDataBuf     = NULL;

			SendGbtMsg(&sTscMsg,sizeof(SThreadMsg));
			return;
		}
		else if ( IsExtendObject(ucObjId) )  //�0�8�0�8�0�9�0�1�0�8���0�4�0�1�0�9�0�8�0�3��
		{
			m_sGbtDealData[ucDealDataIndex].sRecvFrame.iIndex  = iRecvIndex; 
			m_sGbtDealData[ucDealDataIndex].sSendFrame.iIndex  = iSendIndex;          
			m_sGbtDealData[ucDealDataIndex].sSendFrame.iBufLen = iSendBufLen;

			SThreadMsg sTscMsg;
			sTscMsg.ulType       = GBT_MSG_EXTEND;
			sTscMsg.ucMsgOpt     = ucDealDataIndex;
			sTscMsg.uiMsgDataLen = 0;
			sTscMsg.pDataBuf     = NULL;

			SendGbtMsg(&sTscMsg,sizeof(SThreadMsg));
			return;
		}
		
		iRecvIndex++;
		iSendIndex++;

		/***********�0�9�0�6�0�5�0�6�0�2�0�8�0�5�0�7�0�5���0�7�0�9�0�8�0�3��*******/
		if ( iRecvIndex >= iRecvBufLen )
		{
			ucErrorSts = GBT_ERROR_OTHER;
#ifdef TSC_DEBUG
			ACE_DEBUG((LM_DEBUG,"%s:%d,GBT_ERROR_OTHER\n",__FILE__,__LINE__));
#endif
			GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
			return;
		}
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex] =
			               m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex]; //�0�9�0�6�0�5�0�6�0�2�0�8�0�5�0�7�0�5���0�7�0�9�0�8�0�3��
		ucIndexCnt = (m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex]>>6) & 0x3;  //�0�9�0�6�0�5�0�6�0�2�0�8�0�5
		ucSubId    = m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex] & 0x3F;      //���0�7�0�9�0�8�0�3���0�5�0�1���0�0�0�9�0�2�0�3�0�0����
		iRecvIndex++;
		iSendIndex++;

		/***********�0�9�0�6�0�5**************/
		if ( ucIndexCnt > 0 )  /*�0�9�0�6�0�51*/
		{
			if ( iRecvIndex >= iRecvBufLen )
			{
				ucErrorSts = GBT_ERROR_OTHER;
#ifdef TSC_DEBUG
				ACE_DEBUG((LM_DEBUG,"%s:%d,GBT_ERROR_OTHER\n",__FILE__,__LINE__));
#endif
				GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
				return;
			}
			ucIdxFst = m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex];
			m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex] = ucIdxFst;
			iRecvIndex++;
			iSendIndex++;
			ucIndexCnt--;
		}
		if ( ucIndexCnt > 0 ) /*�0�9�0�6�0�52*/
		{
			if ( iRecvIndex >= iRecvBufLen )
			{
				ucErrorSts = GBT_ERROR_OTHER;
#ifdef TSC_DEBUG
				ACE_DEBUG((LM_DEBUG,"%s:%d,GBT_ERROR_OTHER\n",__FILE__,__LINE__));
#endif
				GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
				return;
			}
			ucIdxSnd = m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex];
			m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[iSendIndex] = ucIdxSnd;
			iRecvIndex++;
			iSendIndex++;
			ucIndexCnt--;
		}
		if ( ucIndexCnt > 0 )  /*�0�9�0�6�0�53�0�2�0�2�0�0�0�8*/
		{
			if ( iRecvIndex >= iRecvBufLen )
			{
				ucErrorSts = GBT_ERROR_OTHER;
#ifdef TSC_DEBUG
				ACE_DEBUG((LM_DEBUG,"%s:%d,GBT_ERROR_OTHER\n",__FILE__,__LINE__));
#endif
				GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
				return;
			}
			iRecvIndex++;
			iSendIndex++;
			ucIndexCnt--;
		}
		
		/************�0�0�0�8�0�7��**************/
		if ( IsSendTscCommand(ucObjId) )  //���0�4�0�9�0�1�0�4�0�3�0�2�0�3�0�3���0�7�0�1�0�0�0�4�0�9�0�8�0�3��
		{
			 if ( ! ( GBT_SEEK_REQ != ucRecvOptType ) )
			 {
				 ucErrorSts = GBT_ERROR_OTHER;
#ifdef TSC_DEBUG
				 ACE_DEBUG((LM_DEBUG,"%s:%d,IsSendTscCommand && !SET\n",__FILE__,__LINE__));
#endif
				 GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
				 return;
			 }
			//�0�3���0�4�0�3�0�2�0�3�0�3���0�4�0�3�0�6�0�0���0�4�0�9�0�1�0�7�0�1�0�0�0�4�0�1���0�9�0�6
			if ( !SendTscCommand(ucObjId,m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[iRecvIndex]) )   //���0�4�0�9�0�1�0�8���0�5
			{
				ucErrorSts = GBT_ERROR_OTHER;
#ifdef TSC_DEBUG
				ACE_DEBUG((LM_DEBUG,"%s:%d,SendTscCommand error\n",__FILE__,__LINE__));
#endif
				GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
				return;
			}
			iRecvIndex++;
		}
		else  //�0�8�0�5�0�0�0�6�0�7�0�9�0�5�0�2���0�9�0�8�0�3��
		{
			if ( GBT_SEEK_REQ == ucRecvOptType )  //�0�5���0�5�0�4
			{
				iFunRet = GBT_DB::ExchangeData(1,
									    ucObjId,      //�����0�1�0�4
										ucIdxFst,     //�0�8�0�3�0�6�0�3�0�6�0�2�0�9�0�6�0�5(id1)
										ucIdxSnd,     //�0�8�0�3�0�9�0�6�0�6�0�2�0�9�0�6�0�5(id2)
										ucSubId,      //���0�7�0�9�0�8�0�3��
					                    m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf+iSendIndex,      //�0�0�0�8
					                    MAX_BUF_LEN-iSendIndex,    //���0�4�0�9�0�1�0�0�0�3�0�8�0�2�0�8�0�5�0�7���0�3�0�2�0�5�0�3�0�6��0�9�0�6
										ucErrorSts,  //�0�5���0�2�����0�5�0�0�0�1
										ucErrorIdx); //�0�5���0�2���0�9�0�6�0�5
				//ACE_DEBUG((LM_DEBUG,"%s:%d,ucObjId:%02X  ucIdxFst:%d ucIdxSnd:%d ucSubId:%d	sizeleft:%d	 \n",__FILE__,__LINE__,ucObjId,ucIdxFst,ucIdxSnd,ucSubId,MAX_BUF_LEN-iSendIndex));
				if ( iFunRet < 0 )
				{
#ifdef TSC_DEBUG
					ACE_DEBUG((LM_DEBUG,"%s:%d,Database operate error iFunRet:%d\n",__FILE__,__LINE__,iFunRet));
#endif
					GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
					return;
				}
				else
				{
					iSendIndex += iFunRet;
				}
			}
			else if ( (GBT_SET_REQ == ucRecvOptType) || (GBT_SET_REQ_NOACK == ucRecvOptType) )  //�0�7���0�0�0�1
			{
				iFunRet = GBT_DB::ExchangeData(0,
									ucObjId,      //�����0�1�0�4
									ucIdxFst,     //�0�8�0�3�0�6�0�3�0�6�0�2�0�9�0�6�0�5(id1)
									ucIdxSnd,     //�0�8�0�3�0�9�0�6�0�6�0�2�0�9�0�6�0�5(id2)
									ucSubId,      //���0�7�0�9�0�8�0�3��
									m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf+iRecvIndex,      //�0�0�0�8
									m_sGbtDealData[ucDealDataIndex].sRecvFrame.iBufLen-iRecvIndex,    //�0�8�0�5�0�0�0�6�0�6��0�9�0�6
									ucErrorSts,  //�0�5���0�2�����0�5�0�0�0�1
									ucErrorIdx); //�0�5���0�2���0�9�0�6�0�5
				if ( iFunRet < 0 )
				{
#ifdef TSC_DEBUG
					ACE_DEBUG((LM_DEBUG,"%s:%d,Database operate error iFunRet:%d\n",__FILE__,__LINE__,iFunRet));
#endif
					GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
					return;
				}
				else
				{
					SThreadMsg sTscMsg;
					sTscMsg.ulType       = TSC_MSG_UPDATE_PARA;  
					sTscMsg.ucMsgOpt     = 0;
					sTscMsg.uiMsgDataLen = 0;
					sTscMsg.pDataBuf     = NULL;
					CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));
					
					iRecvIndex += iFunRet;
				}
			}
		}
		
		m_sGbtDealData[ucDealDataIndex].iObjNum--;
		if ( iRecvIndex == iRecvBufLen )  //�0�8�0�9�0�8�0�4�0�8�0�2���0�0�0�4�0�3�0�8�0�5�0�6�0�5�0�5�0�7�0�8���0�1�����0�3
		{
			if ( 0 == m_sGbtDealData[ucDealDataIndex].iObjNum )  //�0�9�0�8�0�3���0�8�0�5�0�6�0�5�0�6�0�9�0�2�0�1�0�5�0�7�0�8���0�1�����0�3
			{
				m_sGbtDealData[ucDealDataIndex].sSendFrame.iBufLen = iSendIndex;
				GotoSendToHost(ucDealDataIndex);
				return;
			}
			else
			{
#ifdef TSC_DEBUG
				ACE_DEBUG((LM_DEBUG,"%s:%d,ObjNum error num:%d\n"
					                ,__FILE__,__LINE__,m_sGbtDealData[ucDealDataIndex].iObjNum));
#endif
				ucErrorSts = 5;
				ucErrorIdx = 0;
				GotoMsgError(ucDealDataIndex,ucErrorSts,ucErrorIdx);
				return;
			}
			
		}		
	
	}
}


/**************************************************************
Function:       CGbtMsgQueue::GetSendOperateType
Description:    ��ȡ��udp���ݷ��صĲ�������
Input:         	ucRecvOptType  ���յ�udp��������
Output:        	��
Return:         Ҫ����udp��������
***************************************************************/
Byte CGbtMsgQueue::GetSendOperateType(Byte ucRecvOptType)
{
	if ( GBT_SEEK_REQ == ucRecvOptType )
	{
		return GBT_SEEK_ACK;
	}
	else if ( GBT_SET_REQ == ucRecvOptType )
	{
		return GBT_SET_ACK;
	}
	else 
	{
		return GBT_OTHER_ACK;
	}
}


/**************************************************************
Function:       CGbtMsgQueue::SelfReport
Description:    �Զ��ϱ�����
Input:         	uiDataLen  ���ݳ���
				pDataBuf   ���ݻ���ָ��
Output:        	��
Return:         ��
***************************************************************/
void CGbtMsgQueue::SelfReport(unsigned int uiDataLen,Byte* pDataBuf)
{
	Byte ucIndex = 0;
	Byte sendData[MAX_GBT_MSG_LEN];
	//�¼����� pDataBuf[0]
	//�����¼����ͱ�ţ���ȡ��Ϣ.......
	//�¼���־�� pDataBuf[1]
	//�����¼���־��ţ���ȡ��Ϣ.......
	//���������ϱ�����Ϣ��ģ��........
	sendData[ucIndex++] = 0xA3;
	
	sendData[ucIndex++] = OBJECT_EVENTTYPE_TABLE;
	sendData[ucIndex++] = 0x43;
	sendData[ucIndex++] = 0x01;
	sendData[ucIndex++] = 0x01;
	ACE_OS::memcpy(sendData+ucIndex,"CTRL_MANUAL",ACE_OS::strlen("CTRL_MANUAL"));
	ucIndex = ucIndex + (Byte)ACE_OS::strlen("CTRL_MANUAL");
	
	sendData[ucIndex++] = OBJECT_EVENTLOG_TEBLE;
	sendData[ucIndex++] = 0x43;
	sendData[ucIndex++] = 0x01;
	sendData[ucIndex++] = 0x01;
	sendData[ucIndex++] = 0x0f;
	sendData[ucIndex++] = 0xff;
	sendData[ucIndex++] = 0xff;
	sendData[ucIndex++] = 0xff;
	
}


/**************************************************************
Function:       CGbtMsgQueue::SendTscCommand
Description:    ����Tsc�Ŀ�������,�Լ����������λ������Ϣ
Input:         	ucObjType:�����ʾ  
				ucValue�� ����ֵ
Output:        	��
Return:         true�����ͳɹ�    false������ʧ��
***************************************************************/
bool CGbtMsgQueue::SendTscCommand(Byte ucObjType,Byte ucValue)
{
	//bool bRight = true;
	Uint uiTscCtrl    = CManaKernel::CreateInstance()->m_pRunData->uiCtrl;
	Uint uiWorkStatus = CManaKernel::CreateInstance()->m_pRunData->uiWorkStatus;

	switch ( ucObjType )
	{
		case OBJECT_CURTSC_CTRL:  /*�0�4�0�3�0�2�0�3�0�3���0�7�0�1�0�0�0�4���0�5�0�0�0�1*/
			if ( (ucValue < 1) || (ucValue > 6) )
			{
				return false;
			}
			SThreadMsg sTscMsg;
			sTscMsg.ulType       = TSC_MSG_SWITCH_CTRL;  
			sTscMsg.ucMsgOpt     = 0;
			sTscMsg.uiMsgDataLen = 1;
			sTscMsg.pDataBuf     = ACE_OS::malloc(1);
			*((Byte*)sTscMsg.pDataBuf) = GbtCtrl2TscCtrl(ucValue);  
			CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));
			break;
		case OBJECT_SWITCH_MANUALCONTROL:   
			if ( 0 == ucValue )
			{
				SThreadMsg sTscMsg;
				sTscMsg.ulType       = TSC_MSG_SWITCH_CTRL;  
				sTscMsg.ucMsgOpt     = 0;
				sTscMsg.uiMsgDataLen = 1;
				sTscMsg.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsg.pDataBuf) = CTRL_LAST_CTRL; 
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));

				SThreadMsg sTscMsgSts;
				sTscMsgSts.ulType       = TSC_MSG_SWITCH_STATUS;  
				sTscMsgSts.ucMsgOpt     = 0;
				sTscMsgSts.uiMsgDataLen = 1;
				sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsgSts.pDataBuf) = STANDARD; 
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
			}
			else if ( (ucValue>0) && (ucValue<33) )
			{
				SThreadMsg sTscMsg;
				sTscMsg.ulType       = TSC_MSG_SWITCH_CTRL;  
				sTscMsg.ucMsgOpt     = 0;
				sTscMsg.uiMsgDataLen = 1;
				sTscMsg.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsg.pDataBuf) = CTRL_PANEL;   
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));

				m_iManualCtrlNo = ucValue;
				SThreadMsg sTscMsgTm;
				sTscMsgTm.ulType       = TSC_MSG_TIMEPATTERN;  
				sTscMsgTm.ucMsgOpt     = 0;
				sTscMsgTm.uiMsgDataLen = 1;
				sTscMsgTm.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsgTm.pDataBuf) = ucValue; 
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgTm,sizeof(sTscMsgTm));
			}
			else if ( 253 == ucValue )
			{
				SThreadMsg sTscMsg;
				sTscMsg.ulType       = TSC_MSG_SWITCH_CTRL;  
				sTscMsg.ucMsgOpt     = 0;
				sTscMsg.uiMsgDataLen = 1;
				sTscMsg.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsg.pDataBuf) = CTRL_PANEL;   
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));

				SThreadMsg sTscMsgSts;
				sTscMsgSts.ulType       = TSC_MSG_SWITCH_STATUS;  
				sTscMsgSts.ucMsgOpt     = 0;
				sTscMsgSts.uiMsgDataLen = 1;
				sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsgSts.pDataBuf) = ALLRED;  
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
			}
			else if ( 254 == ucValue )
			{
				SThreadMsg sTscMsg;
				sTscMsg.ulType       = TSC_MSG_SWITCH_CTRL;  
				sTscMsg.ucMsgOpt     = 0;
				sTscMsg.uiMsgDataLen = 1;
				sTscMsg.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsg.pDataBuf) = CTRL_PANEL;   
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));

				SThreadMsg sTscMsgSts;
				sTscMsgSts.ulType       = TSC_MSG_SWITCH_STATUS;  
				sTscMsgSts.ucMsgOpt     = 0;
				sTscMsgSts.uiMsgDataLen = 1;
				sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsgSts.pDataBuf) = FLASH; 
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
			}
			else if ( 255 == ucValue )
			{
				SThreadMsg sTscMsg;
				sTscMsg.ulType       = TSC_MSG_SWITCH_CTRL;  
				sTscMsg.ucMsgOpt     = 0;
				sTscMsg.uiMsgDataLen = 1;
				sTscMsg.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsg.pDataBuf) = CTRL_PANEL;   
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));

				SThreadMsg sTscMsgSts;
				sTscMsgSts.ulType       = TSC_MSG_SWITCH_STATUS;  
				sTscMsgSts.ucMsgOpt     = 0;
				sTscMsgSts.uiMsgDataLen = 1;
				sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsgSts.pDataBuf) = SIGNALOFF;  
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
			}
			else
			{
				return false;
			}
			break;

		case OBJECT_SWITCH_SYSTEMCONTROL:
			if ( CTRL_PANEL == uiTscCtrl )
			{
				return false;
			}
			else if ( 0 == ucValue )
			{
				SThreadMsg sTscMsg;
				sTscMsg.ulType       = TSC_MSG_SWITCH_CTRL;  
				sTscMsg.ucMsgOpt     = 0;
				sTscMsg.uiMsgDataLen = 1;
				sTscMsg.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsg.pDataBuf) = CTRL_LAST_CTRL; 
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));

				SThreadMsg sTscMsgSts;
				sTscMsgSts.ulType       = TSC_MSG_SWITCH_STATUS;  
				sTscMsgSts.ucMsgOpt     = 0;
				sTscMsgSts.uiMsgDataLen = 1;
				sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsgSts.pDataBuf) = STANDARD;  
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
			}
			else if ( (ucValue>0) && (ucValue<33) )
			{
				m_iSystemCtrlNo = ucValue;

				SThreadMsg sTscMsg;
				sTscMsg.ulType       = TSC_MSG_SWITCH_CTRL;  
				sTscMsg.ucMsgOpt     = 0;
				sTscMsg.uiMsgDataLen = 1;
				sTscMsg.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsg.pDataBuf) = CTRL_UTCS;   
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));

				SThreadMsg sTscMsgTime;
				sTscMsgTime.ulType       = TSC_MSG_TIMEPATTERN;  
				sTscMsgTime.ucMsgOpt     = 0;
				sTscMsgTime.uiMsgDataLen = 1;
				sTscMsgTime.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsgTime.pDataBuf) = ucValue;  
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgTime,sizeof(sTscMsgTime));
			}
			else if ( 253 == ucValue )
		    {
				SThreadMsg sTscMsg;
				sTscMsg.ulType       = TSC_MSG_SWITCH_CTRL;  
				sTscMsg.ucMsgOpt     = 0;
				sTscMsg.uiMsgDataLen = 1;
				sTscMsg.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsg.pDataBuf) = CTRL_MANUAL;   
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));

		    	SThreadMsg sTscMsgSts;
				sTscMsgSts.ulType       = TSC_MSG_SWITCH_STATUS;  
				sTscMsgSts.ucMsgOpt     = 0;
				sTscMsgSts.uiMsgDataLen = 1;
				sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsgSts.pDataBuf) = ALLRED;  //�0�6�0�0�0�2��
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
			}
			else if ( 254 == ucValue )
		    {
				SThreadMsg sTscMsg;
				sTscMsg.ulType       = TSC_MSG_SWITCH_CTRL;  
				sTscMsg.ucMsgOpt     = 0;
				sTscMsg.uiMsgDataLen = 1;
				sTscMsg.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsg.pDataBuf) = CTRL_MANUAL;   
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));

		    	SThreadMsg sTscMsgSts;
				sTscMsgSts.ulType       = TSC_MSG_SWITCH_STATUS;  
				sTscMsgSts.ucMsgOpt     = 0;
				sTscMsgSts.uiMsgDataLen = 1;
				sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsgSts.pDataBuf) = FLASH;  //�0�7�0�9�0�1�0�9
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
			}
			else if ( 255 == ucValue )
			{
				SThreadMsg sTscMsg;
				sTscMsg.ulType       = TSC_MSG_SWITCH_CTRL;  
				sTscMsg.ucMsgOpt     = 0;
				sTscMsg.uiMsgDataLen = 1;
				sTscMsg.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsg.pDataBuf) = CTRL_MANUAL;   
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));

				SThreadMsg sTscMsgSts;
				sTscMsgSts.ulType       = TSC_MSG_SWITCH_STATUS;  
				sTscMsgSts.ucMsgOpt     = 0;
				sTscMsgSts.uiMsgDataLen = 1;
				sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsgSts.pDataBuf) = SIGNALOFF;  //�0�1�0�1�0�8�0�4
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
			}
			else
			{
				return false;
			}
			break;

		case OBJECT_SWITCH_CONTROL:  /***�0�7�0�1�0�0�0�4���0�4�0�8�0�4***/
			if ( uiTscCtrl == CTRL_PANEL || ucValue == 0 )   //�0�1�0�3��0�2�0�7�0�1�0�0�0�4
			{
				return false;
			}
			
		if(ucValue>=1 && ucValue<=3)  //ADD 201310211800
		{
				SThreadMsg sTscMsg;
				sTscMsg.ulType       = TSC_MSG_SWITCH_CTRL;  
				sTscMsg.ucMsgOpt     = 0;
				sTscMsg.uiMsgDataLen = 1;
				sTscMsg.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsg.pDataBuf) = CTRL_MANUAL;   
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));
			
			if ( 1 == ucValue ) 			
			{				
				SThreadMsg sTscMsgSts;
				sTscMsgSts.ulType       = TSC_MSG_SWITCH_STATUS;  
				sTscMsgSts.ucMsgOpt     = 0;
				sTscMsgSts.uiMsgDataLen = 1;
				sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsgSts.pDataBuf) = SIGNALOFF;  
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
			}
			else if ( 2 == ucValue )
			{
				
				SThreadMsg sTscMsgSts;
				sTscMsgSts.ulType       = TSC_MSG_SWITCH_STATUS;  
				sTscMsgSts.ucMsgOpt     = 0;
				sTscMsgSts.uiMsgDataLen = 1;
				sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsgSts.pDataBuf) = FLASH;  
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
			}
			else if ( 3 == ucValue )
			{
				
				SThreadMsg sTscMsgSts;
				sTscMsgSts.ulType       = TSC_MSG_SWITCH_STATUS;  
				sTscMsgSts.ucMsgOpt     = 0;
				sTscMsgSts.uiMsgDataLen = 1;
				sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsgSts.pDataBuf) = ALLRED;  
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
			}
			return true ;
		}
		 if ( 5 ==  ucValue )//ADD 20131021 2100 ��Ӷ�ʱ��
			{
				SThreadMsg sTscMsgSts;
				sTscMsgSts.ulType       = TSC_MSG_SWITCH_CTRL;  
				sTscMsgSts.ucMsgOpt     = 0;
				sTscMsgSts.uiMsgDataLen = 1;
				sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsgSts.pDataBuf) = CTRL_SCHEDULE;  
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
			}
			else if ( 6 ==  ucValue )
			{
				SThreadMsg sTscMsgSts;
				sTscMsgSts.ulType       = TSC_MSG_SWITCH_CTRL;  
				sTscMsgSts.ucMsgOpt     = 0;
				sTscMsgSts.uiMsgDataLen = 1;
				sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsgSts.pDataBuf) = CTRL_VEHACTUATED;  
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
			}
			else if ( 8 == ucValue )
			{
				SThreadMsg sTscMsgSts;
				sTscMsgSts.ulType       = TSC_MSG_SWITCH_CTRL;  
				sTscMsgSts.ucMsgOpt     = 0;
				sTscMsgSts.uiMsgDataLen = 1;
				sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsgSts.pDataBuf) = CTRL_ACTIVATE;  
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
			}
			else if ( 11 == ucValue )
			{
				SThreadMsg sTscMsgSts;
				sTscMsgSts.ulType       = TSC_MSG_SWITCH_CTRL;  
				sTscMsgSts.ucMsgOpt     = 0;
				sTscMsgSts.uiMsgDataLen = 1;
				sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsgSts.pDataBuf) = CTRL_HOST;  
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
			}
			else if ( 12 == ucValue )
			{
				SThreadMsg sTscMsgSts;
				sTscMsgSts.ulType       = TSC_MSG_SWITCH_CTRL;  
				sTscMsgSts.ucMsgOpt     = 0;
				sTscMsgSts.uiMsgDataLen = 1;
				sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsgSts.pDataBuf) = CTRL_UTCS;  
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
			}
			else if ( 13 == ucValue )
			{
				SThreadMsg sTscMsgSts;
				sTscMsgSts.ulType       = TSC_MSG_SWITCH_CTRL;  
				sTscMsgSts.ucMsgOpt     = 0;
				sTscMsgSts.uiMsgDataLen = 1;
				sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsgSts.pDataBuf) = CTRL_LINE;  
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
			}
			else if ( 10 == ucValue )  
			{
				SThreadMsg sTscMsgSts;
				sTscMsgSts.ulType       = TSC_MSG_SWITCH_CTRL;  
				sTscMsgSts.ucMsgOpt     = 0;
				sTscMsgSts.uiMsgDataLen = 1;
				sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsgSts.pDataBuf) = CTRL_MANUAL;  
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
			}
			else
			{
				return false;
			}
			break;

		case OBJECT_SWITCH_STAGE: /*�0�0�0�3�0�4���0�9�0�2*/
			if ( (uiTscCtrl != CTRL_MANUAL)
				|| (uiWorkStatus != STANDARD ) )
			{
				return false;
			}
			else if ( (ucValue>0) && (ucValue<17) )
			{
				SThreadMsg sTscMsgSts;
				sTscMsgSts.ulType       = TSC_MSG_LOCK_STAGE;  
				sTscMsgSts.ucMsgOpt     = 0;
				sTscMsgSts.uiMsgDataLen = 1;
				sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsgSts.pDataBuf) = ucValue; 
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
			}
			else
			{
				return false;
			}
			break;

		case OBJECT_GOSTEP: /***�0�5�0�4�0�4�0�3�0�0�0�6�0�9�0�6***/ 
			if ( (uiTscCtrl != CTRL_MANUAL)
				|| (uiWorkStatus != STANDARD ) )
			{
				return false;
			}
			else if ( 0 == ucValue )  //�0�5�0�4�0�4�0�3
			{
				SThreadMsg sTscMsgSts;
				sTscMsgSts.ulType       = TSC_MSG_LOCK_STEP;  
				sTscMsgSts.ucMsgOpt     = 0;
				sTscMsgSts.uiMsgDataLen = 1;
				sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsgSts.pDataBuf) = ucValue;
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
			}
			else if ( (ucValue > 0) && (ucValue < MAX_PHASE) )  //�0�9�0�3�0�9�0�8�0�3���0�2�0�3
			{
				SThreadMsg sTscMsgSts;
				sTscMsgSts.ulType       = TSC_MSG_LOCK_PHASE;  
				sTscMsgSts.ucMsgOpt     = 0;
				sTscMsgSts.uiMsgDataLen = 1;
				sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
				*((Byte*)sTscMsgSts.pDataBuf) = ucValue;
				CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
			}
			else
			{
				return false;
			}
			break;

		default:
			break;
		}	

	return true;
}


/**************************************************************
Function:       CGbtMsgQueue::ToObjectControlSwitch
Description:    ����ǰ�Ŀ���ģʽ��ת��ΪЭ��Ŀ���ģʽ����(OBJECT_CONTROL_SWITCH)
Input:         	uiWorkSts:����״̬  
				uiCtrl�� ���Ʒ�ʽ
Output:        	��
Return:         Э���������
***************************************************************/
Byte CGbtMsgQueue::ToObjectControlSwitch(unsigned int uiWorkSts,unsigned int uiCtrl)
{
	if ( uiWorkSts == SIGNALOFF )
	{
		return 1;
	}
	else if ( uiWorkSts == FLASH )
	{
		return 2;
	}
	else if ( uiWorkSts == ALLRED )
	{
		return 3;
	}
	else
	{
		switch ( uiCtrl )
		{
		case CTRL_VEHACTUATED:
		case CTRL_MAIN_PRIORITY:
		case CTRL_SECOND_PRIORITY:
			return 6;
		case CTRL_ACTIVATE:
			return 8;
		case CTRL_HOST:
			return 11;
		case CTRL_UTCS:
			return 12;
		case CTRL_LINE:
			return 13;
		default:
			return 14;
		}
	}
}



/**************************************************************
Function:       CGbtMsgQueue::ToObjectControlSwitch
Description:    ����ǰ�Ŀ���ģʽ��ת��ΪЭ��Ŀ���ģʽ����(OBJECT_CUTTSC_CTRL)
Input:         	uiCtrl�� ���Ʒ�ʽ
Output:        	��
Return:         Э���������
***************************************************************/
Byte CGbtMsgQueue::ToObjectCurTscCtrl(unsigned int uiCtrl)
{
	switch ( uiCtrl )
	{
	case CTRL_UTCS:
		return 2;
	case CTRL_HOST:
		return 3;
	case CTRL_PANEL:
		return 4;
	case CTRL_SCHEDULE:
		return 5;
	case CTRL_LINE:
		return 6;
	default:
		return 1;
	}
}


/**************************************************************
Function:       CGbtMsgQueue::SendToHost
Description:    ��ͻ��˷�����Ϣ
Input:         	ucDealDataIndex�� ��������Ϣ�±�
Output:        	��
Return:         ��
***************************************************************/
void CGbtMsgQueue::SendToHost(Byte ucDealDataIndex)
{
	int iSendToClient = 0;
	Byte ucOperateType = (m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[0]) & 0xf;  //�0�5�0�2���0�8���0�4�0�1

	if ( GBT_SET_REQ_NOACK == ucOperateType )  //������Ӧ��
	{
		//ACE_DEBUG((LM_DEBUG,"%s:%d ringht frame GBT_SET_REQ_NOACK\n",__FILE__,__LINE__));
	}
	else
	{
		if ( m_sGbtDealData[ucDealDataIndex].bReportSelf ) //�����ϱ�
		{
			/*
			if ( OBJECT_DETECTORSTS_TABLE == m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[1] )  //�0�3���0�5�0�9�0�4�¡��0�5�0�0�0�1
			{
				
				static Byte ucDetStatus[28] = {0};
				bool bSend = false;
				for (Byte ucIndex=0; ucIndex<28; ucIndex++ )
				{
					if ( ucDetStatus[ucIndex] != m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[ucIndex] )
					{
						bSend = true;
					}
				}
				
				if ( !bSend )  //�0�3���0�5�0�9�0�4�0�7�0�5�0�7�0�3�0�5�0�2�0�8�0�2���0�5�0�0�0�1�0�6�0�3�0�0�0�0 �0�5�0�3���0�1���0�4�0�9�0�1�0�0���0�8�0�4���0�5�0�0�0�1�0�6�0�2���0�1
				{
					CleanDealData(ucDealDataIndex);
					return;
				}
				else
				{
					ACE_OS::memcpy(ucDetStatus , m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf , 28);
				}
			}*/

			Byte ucTmp =  (m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[0]) & 0xf0;
			ucTmp      = ucTmp | GBT_SELF_REPORT; 
			m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[0] = ucTmp;
			m_sGbtDealData[ucDealDataIndex].bReportSelf = false;
		}
#ifdef GBT_TCP
		iSendToClient 
			= m_sGbtDealData[ucDealDataIndex].SockStreamClient.send(m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf,
			                            m_sGbtDealData[ucDealDataIndex].sSendFrame.iBufLen);
#else
		iSendToClient = m_sockLocal.send(m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf,
			                             m_sGbtDealData[ucDealDataIndex].sSendFrame.iBufLen,
								         m_sGbtDealData[ucDealDataIndex].AddrClient);
#endif
		#if 0
		if ( iSendToClient != m_sGbtDealData[ucDealDataIndex].sSendFrame.iBufLen )
		{
			ACE_DEBUG((LM_DEBUG,"CGbtMsgQueue.cpp:%d send to client error %d-%d\n"
				,__LINE__,iSendToClient,m_sGbtDealData[ucDealDataIndex].sSendFrame.iBufLen));
		}
		else
		{
			ACE_DEBUG((LM_DEBUG,"CGbtMsgQueue.cpp:%d send to client right %d\n",__LINE__,iSendToClient));
		}
		#endif
		
	}

	CleanDealData(ucDealDataIndex);
}


/**************************************************************
Function:       CGbtMsgQueue::CleanDealData
Description:    �������������
Input:         	ucDealDataIndex�� ��������Ϣ�±�
Output:        	��
Return:         ��
***************************************************************/
void CGbtMsgQueue::CleanDealData(Byte ucDealDataIndex)
{
	ACE_Guard<ACE_Thread_Mutex> guard(m_sMutex);
	
	m_sGbtDealData[ucDealDataIndex].sRecvFrame.iIndex  = 0;
	m_sGbtDealData[ucDealDataIndex].sRecvFrame.iBufLen = 0;
	m_sGbtDealData[ucDealDataIndex].sSendFrame.iIndex  = 0;
	m_sGbtDealData[ucDealDataIndex].sSendFrame.iBufLen = 0;
 
#ifdef GBT_TCP
 	//tcp ���������ݺ����ӻ�����
#else
	m_sGbtDealData[ucDealDataIndex].bIsDeal = false;   //udp ���ر������� �´λ�����ip��Ϣ����
#endif
}


/**************************************************************
Function:       CGbtMsgQueue::FirstRecv
Description:    ������Ϣ�����յ������ݿ��������SGbtDealData��sRecvFrame
				������pBuf���ڴ潫���ͷ�
Input:         	ucDealDataIndex�� ��������Ϣ�±�
				iBufLen           ���յ���udp���ݳ���
				pBuf              �������ݻ���ָ��
Output:        	��
Return:         ��
***************************************************************/
void CGbtMsgQueue::FirstRecv(Byte ucDealDataIndex,Uint iBufLen,Byte* pBuf)
{
	SThreadMsg sGbtMsg;
	m_sGbtDealData[ucDealDataIndex].sRecvFrame.iIndex = 0;
	m_sGbtDealData[ucDealDataIndex].sRecvFrame.iBufLen = iBufLen;
	ACE_OS::memcpy(m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf,pBuf,iBufLen);

	sGbtMsg.ulType       = GBT_MSG_DEAL_RECVBUF;  
	sGbtMsg.ucMsgOpt     = ucDealDataIndex;
	sGbtMsg.uiMsgDataLen = iBufLen;  //�0�8�0�2�0�8�0�3�0�8�0�9�0�8�0�4�0�8�0�5�0�0�0�6�0�8�0�2�0�5���0�4�0�3
	sGbtMsg.pDataBuf     = NULL;	
	SendGbtMsg(&sGbtMsg,sizeof(sGbtMsg));
}


/**************************************************************
Function:       CGbtMsgQueue::CheckMsg
Description:    �����յ������ݺϷ���
Input:         	ucDealDataIndex�� ��������Ϣ�±�
				iBufLen           ���յ���udp���ݳ���
				pBuf              �������ݻ���ָ��
Output:        	��
Return:         ��
***************************************************************/
int CGbtMsgQueue::CheckMsg(Byte ucDealDataIndex,Uint iBufLen,Byte* pBuf)
{
	if ( pBuf == NULL )
	{
		return -1;
	}

	if ( (int)iBufLen < MIN_GBT_MSG_LEN ) //�0�3�0�4�0�3�0�4�0�6��0�9�0�6�0�0�0�0�0�9�0�0
	{
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[0] = 0x86;
							//(m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[0] & 0xf0 ) | GBT_ERR_ACK; 
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[1] = GBT_ERROR_SHORT;
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[2] = 0;
	}
	else if ( (pBuf[0]&0xf)<GBT_SEEK_REQ || (pBuf[0]&0xf)>GBT_SET_REQ_NOACK || ((pBuf[0]>>7)&1)!=1 ) //�0�5�0�2���0�8���0�4�0�1�0�5���0�2��
	{
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[0] = 0x86;
							//(m_sGbtDealData[ucDealDataIndex].sRecvFrame.ucBuf[0] & 0xf0 ) | GBT_ERR_ACK;
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[1] = GBT_ERROR_OTHER;
		m_sGbtDealData[ucDealDataIndex].sSendFrame.ucBuf[2] = 0;
	}
	else
	{
		return 1; //�0�9�0�5�0�6���0�8�0�2�0�3�0�4�0�3�0�4
	}

	m_sGbtDealData[ucDealDataIndex].sSendFrame.iBufLen = 3;
	SThreadMsg sGbtMsg;
	sGbtMsg.ulType       = GBT_MSG_SEND_HOST;
	sGbtMsg.ucMsgOpt     = ucDealDataIndex;
	sGbtMsg.uiMsgDataLen = 0;
	sGbtMsg.pDataBuf     = NULL;	
	SendGbtMsg(&sGbtMsg,sizeof(sGbtMsg));

	return 0;
}

#ifdef GBT_TCP

/**************************************************************
Function:       CGbtMsgQueue::GetDealDataIndex
Description:    ��ȡ���������ݵ��±� TCP
Input:         	bReportSelf - �Ƿ�Ϊ�����ϱ�
     		    sockStreamRemote  - �ͻ��˵�ַ
Output:        	��
Return:         0-7 200��û�п��õĻ�������
***************************************************************/
Byte CGbtMsgQueue::GetDealDataIndex(bool bReportSelf , ACE_SOCK_Stream& sockStreamRemote)
{
	ACE_Guard<ACE_Thread_Mutex> guard(m_sMutex);

	int iDealIndex = -1;
	ACE_Addr addrNew;
	ACE_Addr addrOld;
	
	sockStreamRemote.get_remote_addr(addrNew);
	for (Byte i=0; i<MAX_CLIENT_NUM; i++)
	{
		if ( m_sGbtDealData[i].bIsDeal == false )
		{
			if ( -1 == iDealIndex )
			{
				iDealIndex = i;
			}	
		}
		else
		{
			m_sGbtDealData[i].SockStreamClient.get_remote_addr(addrOld);
			if ( addrNew == addrOld )
			{
				m_sGbtDealData[i].bReportSelf      = bReportSelf;
				m_sGbtDealData[i].SockStreamClient = sockStreamRemote;
				return i;
			}
		}
	}

	if ( iDealIndex >=0 && iDealIndex<MAX_CLIENT_NUM )
	{
		m_sGbtDealData[iDealIndex].bIsDeal          = true;
		m_sGbtDealData[iDealIndex].bReportSelf      = bReportSelf;
		m_sGbtDealData[iDealIndex].SockStreamClient = sockStreamRemote;
		return iDealIndex;
	}

	return 200;
}

SGbtDealData* CGbtMsgQueue::GetGbtDealDataPoint()
{
	return m_sGbtDealData;
}

#else

/**************************************************************
Function:       CGbtMsgQueue::GetDealDataIndex
Description:    ��ȡ���������ݵ��±� UDP
Input:         	bReportSelf - �Ƿ�Ϊ�����ϱ�
     		    addrRemote  - �ͻ��˵�ַ
Output:        	��
Return:         0-7 200��û�п��õĻ�������
***************************************************************/
Byte CGbtMsgQueue::GetDealDataIndex(bool bReportSelf , ACE_INET_Addr& addrRemote)
{
	
	ACE_Guard<ACE_Thread_Mutex> guard(m_sMutex);

	for (Byte i=0; i<MAX_CLIENT_NUM; i++)
	{
		if ( m_sGbtDealData[i].bIsDeal == false )
		{
			m_sGbtDealData[i].bIsDeal     = true;
			m_sGbtDealData[i].bReportSelf = bReportSelf;
			m_sGbtDealData[i].AddrClient.set(ACE_INET_Addr(addrRemote));
			//m_sGbtDealData[i].AddrClient.set_port_number(61394);  // ADD: 2013 07 30 in order to test udp send and recv.
			return i;
		}
	}

	return 200;
}
#endif


/**************************************************************
Function:       CGbtMsgQueue::RunGbtRecv
Description:    udp���ݽ����̺߳���,��������λ����UDP ���ݣ�Ȼ��ת�ʹ���
Input:         	arg  Ĭ��NULL���̺߳�������
Output:        	��
Return:         0
***************************************************************/
void* CGbtMsgQueue::RunGbtRecv(void* arg)
{
	ACE_DEBUG((LM_DEBUG,"%s:%d Begin Gbt date recv thread!\n",__FILE__,__LINE__));
	Byte ucDealDataIndex = 0;
	int iRecvCount = 0;
	Byte pBuf[MAX_BUF_LEN] = {0};
	SThreadMsg sMsg;
	CGbtMsgQueue* pGbtMsgQueue = CGbtMsgQueue::CreateInstance();
#ifdef GBT_TCP
	ACE_SOCK_Stream sockStreamRemote;
	SGbtDealData* pGbtDealData = pGbtMsgQueue->GetGbtDealDataPoint();
	ACE_Time_Value timeout(0,10*1000);
#else
	ACE_INET_Addr addrRemote;
#endif
	
	while ( true )
	{
		#ifdef GBT_TCP
		if ( pGbtMsgQueue->m_acceptor.accept(sockStreamRemote,NULL,&timeout) != -1 ) //�0�4�0�0�0�8�0�2�0�7�0�1�0�3��0�9�0�9�0�9�0�1�0�4�0�7
		{
			ucDealDataIndex = pGbtMsgQueue->GetDealDataIndex(false , sockStreamRemote);

			ACE_DEBUG((LM_DEBUG,"%s:%d **********%d \n",__FILE__,__LINE__,ucDealDataIndex));
		}

		for (Byte i=0; i<MAX_CLIENT_NUM; i++)
		{
			if ( pGbtDealData[i].bIsDeal == true )
			{
				if ( ( iRecvCount = pGbtDealData[i].SockStreamClient.recv(pBuf,MAX_BUF_LEN))  != -1 )		
				{
					//#if 1
					ACE_DEBUG((LM_DEBUG,"\n%s:%d ",__FILE__,__LINE__));
					for (int iIndex=0; iIndex<iRecvCount; iIndex++ )
					{
						ACE_DEBUG((LM_DEBUG," %x ",pBuf[iIndex]));
					}
					ACE_DEBUG((LM_DEBUG,"\n"));
					//#endif

					sMsg.ulType       = GBT_MSG_FIRST_RECV;
					sMsg.ucMsgOpt     = i;
					sMsg.uiMsgDataLen = iRecvCount;
					sMsg.pDataBuf     = ACE_OS::malloc(iRecvCount);
					ACE_OS::memcpy(sMsg.pDataBuf,pBuf,iRecvCount);

					pGbtMsgQueue->SendGbtMsg(&sMsg,sizeof(sMsg));

					ACE_OS::memset(&sMsg,0,sizeof(sMsg));
					ACE_OS::memset(pBuf,0,MAX_BUF_LEN);
				}
			}
		}
		
#else
		if ( ( iRecvCount = (pGbtMsgQueue->m_sockLocal).recv(pBuf, MAX_BUF_LEN,addrRemote))  != -1 )		
		{

			#if 1
			for (int iIndex=0; iIndex<iRecvCount; iIndex++ )
			{
				ACE_DEBUG((LM_DEBUG,"%x ",pBuf[iIndex]));
			}
			ACE_DEBUG((LM_DEBUG,"\n"));
			#endif

			ucDealDataIndex = pGbtMsgQueue->GetDealDataIndex(false , addrRemote);
			
			if ( ucDealDataIndex < MAX_CLIENT_NUM )
			{
				sMsg.ulType       = GBT_MSG_FIRST_RECV;
				sMsg.ucMsgOpt     = ucDealDataIndex;
				sMsg.uiMsgDataLen = iRecvCount;
				sMsg.pDataBuf     = ACE_OS::malloc(iRecvCount);
				ACE_OS::memcpy(sMsg.pDataBuf,pBuf,iRecvCount);

				pGbtMsgQueue->SendGbtMsg(&sMsg,sizeof(sMsg));
				//ACE_DEBUG((LM_DEBUG,"%s :%d recv from %s  %d !\n",__FILE__,__LINE__,addrRemote.get_host_addr(),addrRemote.get_port_number())); //ADD: 2013 0613 1008
			}
			else
			{	///����4���ͻ���С���޷��ٽ���������
				ACE_OS::memset(pBuf,0,MAX_GBT_MSG_LEN);
				pBuf[0] = 0x86;                 //����Ӧ��GBTЭ�����Ӧ��
				pBuf[1] = GBT_ERROR_OTHER + 1; //�������������͵���������
				pBuf[2] = 0;                    //������Ϊ�����ֶε�ֵ�����
				(pGbtMsgQueue->m_sockLocal).send(pBuf , 3 , addrRemote);
				//CGbtMsgQueue::CreateInstance()->PrintIpList();
			}
			ACE_OS::memset(&sMsg,0,sizeof(sMsg));
			ACE_OS::memset(pBuf,0,MAX_BUF_LEN);
		}
#endif
	}

	return 0;
}


/**************************************************************
Function:       CGbtMsgQueue::PrintIpList
Description:    ��ӡ�ͻ���ip�б�
Input:         	arg  Ĭ��NULL���̺߳�������
Output:        	��
Return:         0
***************************************************************/
void CGbtMsgQueue::PrintIpList()
{	
	for ( Byte ucIndex=0; ucIndex<MAX_CLIENT_NUM; ucIndex++ )
	{
		//ACE_DEBUG((LM_DEBUG,"%d %s %x\n"
			//, m_sGbtDealData[ucIndex].bIsDeal
			//, m_AddrArr[ucIndex].get_host_addr()
			//, m_AddrArr[ucIndex].get_ip_address()));

		//CleanDealData(ucIndex);
	}
	
}


/**************************************************************
Function:       CGbtMsgQueue::IsGetTscStatusObject
Description:    �ж��Ƿ�Ϊ��ȡ�źŻ�״̬����
Input:         	ucObjectFlag  �����־
Output:        	��
Return:         true:��  false:��
***************************************************************/
bool CGbtMsgQueue::IsGetTscStatusObject(Byte ucObjectFlag)
{
	switch ( ucObjectFlag )
	{
		case OBJECT_CHANNELSTATUS_TABLE:     //ͨ�����״̬��   �ƿض˿ڲ���
		case OBJECT_OVERLAPPHASE_STATUS:     //��˭��λ״̬��   ������λ����
		case OBJECT_PHASESTATUS_TABLE:		 //��λ���״̬��   ��λ����
		case OBJECT_CURTSC_FLASHCTRL:        //��ǰ�������ģʽ ��Ԫ����
		case OBJECT_ACTIVESCHEDULE_NO:       //�ʱ�α���   �����¼�����
		//case OBJECT_SWITCH_STAGE:
		case OBJECT_TSC_WARN2:				//�źŻ�����2       ��Ԫ����
		case OBJECT_TSC_WARN1:				//�źŻ�����1       ��Ԫ����
		case OBJECT_TSC_WARN_SUMMARY:       //�źŻ�����ժҪ    ��Ԫ����
		case OBJECT_ACTIVEDETECTOR_NUM:     //����������    ���������
		case OBJECT_DETECTORSTS_TABLE:      //�����״̬��      ���������
		case OBJECT_DETECTORDATA_TABLE:     //��ͨ������ݱ�    ���������
		case OBJECT_DETECTORWARN_TABLE:     //������������������ ���������
		case OBJECT_CURPATTERN_SCHTIMES:    //��ǰ�������׶��г�  ���Ʋ���
		case OBJECT_CURPATTERN_GREENTIMES:  //��ǰ�������ؼ���λ�̵��г� ���Ʋ���
#ifdef TSC_DEBUG
		ACE_DEBUG((LM_DEBUG,"\n IsGetTscStatusObject\n"));
#endif
		return true;
	default:
		return false;
	}
	return false;
}


/**************************************************************
Function:       CGbtMsgQueue::IsSendTscCommand
Description:    �ж��Ƿ�Ϊ�����źŻ���������
Input:         	ucObjectFlag  �����־
Output:        	��
Return:         true:��  false:��
***************************************************************/
bool CGbtMsgQueue::IsSendTscCommand(Byte ucObjectFlag)
{
	switch ( ucObjectFlag )
	{
		case OBJECT_CURTSC_CTRL:     //��ǰ�źŻ�����״̬   ��Ԫ����
		case OBJECT_SWITCH_MANUALCONTROL: //�ֶ����Ʒ���   ���Ʋ���
		case OBJECT_SWITCH_SYSTEMCONTROL: //ϵͳ���Ʒ���  ���Ʋ���
		case OBJECT_SWITCH_CONTROL:       //���Ʒ�ʽ     ���Ʋ���
		case OBJECT_SWITCH_STAGE:         //�׶�״̬     ���Ʋ���
		case OBJECT_GOSTEP:               //����ָ��     ���Ʋ��� 
#ifdef TSC_DEBUG
		ACE_DEBUG((LM_DEBUG,"\n IsSendTscCommand\n"));
#endif
		return true;
	default:
		return false;
	}

	return false;
}


/**************************************************************
Function:       CGbtMsgQueue::IsOtherObject
Description:    �ж��Ƿ������������󣬷ǻ�ȡ״̬���ǿ������
				�����ݿ��������
Input:         	ucObjectFlag  �����־
Output:        	��
Return:         true:��  false:��
***************************************************************/
bool CGbtMsgQueue::IsOtherObject(Byte ucObjectFlag)
{
	switch ( ucObjectFlag )
	{
	case OBJECT_UTC_TIME:
	case OBJECT_LOCAL_TIME:
#ifdef TSC_DEBUG
		ACE_DEBUG((LM_DEBUG,"\n IsOtherObject\n"));
#endif
		return true;
	default:
		return false;
	}

	return false;
}


/**************************************************************
Function:       CGbtMsgQueue::IsOtherObject
Description:    �ж��Ƿ�Ϊ��չ����
Input:         	ucObjectFlag  �����־
Output:        	��
Return:         true:��  false:��
***************************************************************/
bool CGbtMsgQueue::IsExtendObject(Byte ucObjectFlag)
{
	switch ( ucObjectFlag )
	{
		case OBJECT_IP:                 //�źŻ�IP ��չ����
		//case OBJECT_MULTICAST_INFO:	//
		case OBJECT_WATCH_PARA:         //������Ͳ��� ��չ����
		case OBJECT_MODULE_STATUS:      //ģ��״̬    ��չ����
		case OBJECT_EXT_TSC_STATUS:     //״̬���ͱ���� ��չ����
		case OBJECT_YWFLASH_CFG:        //��������չ����
		case OBJECT_DET_EXTCFG :  		//�������չ����
		case OBJECT_LAMPBOARD_CFG :     //���ݼ������
		case OBJECT_PSCBTN_NUM :         //ģ��8λ���˰�ť����
		case OBJECT_TMPPATTERN_CFG :     //��ʱ12���������ϣ�����60��Ĭ��
		case OBJECT_SYSFUNC_CFG :        //ϵͳ������������
			return true;
		default:
			return false;
	}
	return false;
}


/**************************************************************
Function:       CGbtMsgQueue::GbtCtrl
Description:    Э�����״̬תΪ�źŻ��Ŀ���״̬
Input:         	ucctrl  Э�����ֵ
Output:        	��
Return:         ����״ֵ̬
***************************************************************/
int CGbtMsgQueue::GbtCtrl2TscCtrl(Byte ucctrl)
{
	switch ( ucctrl )
	{
	case 2:
		return CTRL_UTCS;
	case 3:
		return CTRL_HOST;
	case 4:
		return CTRL_PANEL;
	case 5:
		return CTRL_SCHEDULE;
	case 6:
		return CTRL_LINE;
	default:
		return -1;
	}
}



/**************************************************************
Function:       CGbtMsgQueue::ReworkNetPara
Description:    �޸�����������Ϣ
Input:         	cIp      - ip��ַ
				cMask           - ����
				cGateWay        - ����
Output:        	��
Return:         ��
***************************************************************/
void CGbtMsgQueue::ReworkNetPara(Byte* pIp , Byte* pMask , Byte* pGateWay)
{
	Byte cHwEther[6] = {0};
	Byte cIp[4]      = {0};
	Byte cMask[4]    = {0};
	Byte cGateWay[4] = {0};
	char cIpAndMaskCmd[64] = {0};
	char cGatewayCmd[64]   = {0};

	GetNetPara(cHwEther , cIp , cMask , cGateWay);
	
	if ( pIp != NULL )
	{
		ACE_OS::memcpy(cIp,pIp,4);
	}
	if ( pMask != NULL )
	{
		ACE_OS::memcpy(cMask,pMask,4);
	}
	if ( pGateWay != NULL )
	{
		ACE_OS::memcpy(cGateWay,pGateWay,4);
	}


	ACE_OS::sprintf(cIpAndMaskCmd , "/sbin/ifconfig eth0 %d.%d.%d.%d netmask %d.%d.%d.%d up\n",
		cIp[0]   , cIp[1]   , cIp[2]   , cIp[3] , 
		cMask[0] , cMask[1] , cMask[2] , cMask[3] ) ;
	ACE_OS::sprintf(cGatewayCmd , "/sbin/route add default gw %d.%d.%d.%d\n",
		cGateWay[0] , cGateWay[1] , cGateWay[2] , cGateWay[3] ) ;

	system("/sbin/ifconfig eth0 down\n");	
	system(cIpAndMaskCmd);
	system(cGatewayCmd);	
	
	RecordNetPara( cIpAndMaskCmd , cGatewayCmd );	
}


/**************************************************************
Function:       CGbtMsgQueue::RecordNetPara
Description:    �����޸��������ò���
Input:        
				pIpAndMaskCmd - ip��ַ����������
				pGatewayCmd   - ��������
Output:        	��
Return:         ��
***************************************************************/
void CGbtMsgQueue::RecordNetPara(/*char* pHwEtherCmd ,*/ char* pIpAndMaskCmd , char* pGatewayCmd )
{
	FILE* fpRcs    = NULL;
	FILE* fpRcsTmp = NULL;
	char  sTmp[1024] = {0};

	//if ( NULL == ( fpRcs = ACE_OS::fopen("/etc/init.d/rcS", "r") ) )
	if ( NULL == ( fpRcs = ACE_OS::fopen("rcS", "r") ) )
	{
		return;
	}

	//if ( (fpRcsTmp = ACE_OS::fopen("/etc/init.d/rcS_tmp", "w")) != NULL ) 
	if ( (fpRcsTmp = ACE_OS::fopen("rcS_tmp", "w")) != NULL ) 
	{
		while ( fgets(sTmp , 1024 , fpRcs) != NULL ) 
		{
			/*
			if ( strstr(sTmp , "hw") != NULL && strstr(sTmp , "ether") != NULL )
			{
				ACE_OS::memset(sTmp , 0 , 1024);
				ACE_OS::memcpy(sTmp , pHwEtherCmd , strlen(pHwEtherCmd) );
			}
			else*/ if ( strstr(sTmp , "eth0") != NULL && strstr(sTmp , "netmask") != NULL ) 
			{
				ACE_OS::memset(sTmp , 0 , 1024);
				ACE_OS::memcpy(sTmp , pIpAndMaskCmd , strlen(pIpAndMaskCmd) );
			}
			else if ( strstr(sTmp , "default") != NULL && strstr(sTmp , "gw") != NULL ) 
			{
				ACE_OS::memset(sTmp , 0 , 1024);
				ACE_OS::memcpy(sTmp , pGatewayCmd , strlen(pGatewayCmd) );
			}
			fputs(sTmp, fpRcsTmp);
			ACE_OS::memset(sTmp , 0 , 1024);
		}
		fclose(fpRcs);
		fclose(fpRcsTmp);
#ifdef LINUX
		TscCopyFile((char*)"/etc/init.d/rcS_tmp" , (char*)"/etc/init.d/rcS");
		remove((char*)"/etc/init.d/rcS_tmp");
#endif
	} 
	else 
	{
		fclose(fpRcs);
	}
}


/**************************************************************
Function:       CGbtMsgQueue::GetNetPara
Description:    ��ȡ�����ַ ip��ַ ���� ����
Input:        	pIp      - IP��ַ
				pHwEther - �����ַ
				pMask   - ��������
				pGateway - ����
Output:        	��
Return:         ��
***************************************************************/
void CGbtMsgQueue::GetNetPara(Byte* pHwEther , Byte* pIp , Byte* pMask , Byte* pGateway)
{
	FILE* fpRcs      = NULL;
	char* pStart     = NULL;
	//char* pEnd       = NULL;
	char  sTmp[200] = {0};

	if ( NULL == ( fpRcs = ACE_OS::fopen("/etc/eth0-setting", "r") ) )
	{
		return;
	}

	while ( fgets(sTmp , 100 , fpRcs) != NULL ) 
	{
		if ( (pStart=strstr(sTmp , "MAC=")) != NULL )
		{
			pStart = pStart + ACE_OS::strlen("MAC=");
			InterceptStr(pStart,(char*)":",pHwEther,5);
		}
		else if ( (pStart=strstr(sTmp , "IP=")) != NULL ) 
		{
			pStart = pStart + ACE_OS::strlen("IP=") ;
			InterceptStr(pStart,(char*)".",pIp,3);
		}
		else if( (pStart=strstr(sTmp , "Mask=")) != NULL)
		{
			pStart = pStart + ACE_OS::strlen("Mask=") ;
			InterceptStr(pStart,(char*)".",pMask,3);
		}
		else if (( pStart=strstr(sTmp , "Gateway=") ) != NULL) 
		{
			pStart = pStart + ACE_OS::strlen((char*)"Gateway=");
			InterceptStr(pStart,(char*)".",pGateway,3);
		}

		pStart = NULL;
		ACE_OS::memset(sTmp , 0 , 200);
	}

	fclose(fpRcs);
}


/**************************************************************
Function:       CGbtMsgQueue::InterceptStr
Description:   ��ȡ��Ҫ��ֵ
Input:        	pBuf  - ��ȡ���ַ��� pstr  - ��ȡ��ʾ
				pData - ת�����ֵ   ucCnt - ����
Output:        	��
Return:         ��
***************************************************************/
void CGbtMsgQueue::InterceptStr(char* pBuf, char* pstr , Byte* pData , Byte ucCnt)
{
	Byte ucIndex  = 0;
	char* pStrart = pBuf;
	char* pEnd    = pBuf;
	char  sTmp[8] = {0};

	while ( ucIndex < ucCnt )
	{
		if ( (pEnd = strstr(pStrart,pstr)) != NULL )
		{
			ACE_OS::strncpy(sTmp , pStrart , pEnd - pStrart);
			pData[ucIndex] = atoi(sTmp);
			
			pStrart = ++pEnd;
			ACE_OS::memset(sTmp , 0 , 8);
		}
	
		ucIndex++;
	}

	if ( (pEnd = strstr(pStrart," ")) != NULL )
	{
		ACE_OS::strncpy(sTmp , pStrart , pEnd - pStrart);
		pData[ucIndex] = atoi(sTmp);
	}
	else
	{
		pData[ucIndex] = atoi(pStrart);
	}
}


/**************************************************************
Function:       CGbtMsgQueue::ReworkIp
Description:    �޸�ip��������Ч
Input:        	ucIp1 - 192
		        ucIp2 - 168     
                ucIp3 - 1
                ucIp4 - 20
Output:        	��
Return:         ��
***************************************************************/
void CGbtMsgQueue::ReworkIp(Byte ucIp1,Byte ucIp2,Byte ucIp3,Byte ucIp4)
{
	FILE* fpRcs    = NULL;
	FILE* fpRcsTmp = NULL;
	char  sTmp[1024] = {0};
	
	if ( NULL == ( fpRcs = ACE_OS::fopen("/etc/init.d/rcS", "r") ) )
	{
		return;
	}
	
	if ( (fpRcsTmp = ACE_OS::fopen("/etc/init.d/rcS_tmp", "w")) != NULL ) 
	{
		while ( fgets(sTmp , 1024 , fpRcs) != NULL ) 
		{
			if ( strstr(sTmp , "eth0") != NULL && strstr(sTmp , "netmask") != NULL )
			{
				ACE_OS::memset(sTmp , 0 , 1024);
				ACE_OS::sprintf(sTmp, "/sbin/ifconfig eth0 %d.%d.%d.%d\n"
					        , ucIp1 , ucIp2 , ucIp3 , ucIp4);
			}
			fputs(sTmp, fpRcsTmp);
			ACE_OS::memset(sTmp , 0 , 1024);
		}
		fclose(fpRcs);
		fclose(fpRcsTmp);
#ifdef LINUX
		TscCopyFile((char*)"/etc/init.d/rcS_tmp" , (char*)"/etc/init.d/rcS");
		remove("/etc/init.d/rcS_tmp");
#endif
	} 
	else 
	{
		fclose(fpRcs);
	}
}



/**************************************************************
Function:       CGbtMsgQueue::TscCopyFile
Description:    �ļ�����
Input:        	fpSrc - Դ�ļ�����
				fpDest- Ŀ���ļ�����
Output:        	��
Return:         ��
***************************************************************/
void CGbtMsgQueue::TscCopyFile(char* fpSrc, char* fpDest)
{
#ifdef LINUX
	int iSrcFd, iDestFd;
	char cBuf[100] = {0};
	int iReadCnt = 0;

	if ( strcmp(fpSrc, fpDest) == 0 )
	{
		return;
	}

	iSrcFd = open(fpSrc, O_RDONLY);
	iDestFd = open(fpDest, O_CREAT | O_WRONLY | O_TRUNC, 0755);
	if ( -1 == iSrcFd || -1 == iDestFd )
	{
		return;
	}

	while ( (iReadCnt = read(iSrcFd, cBuf, sizeof(cBuf))) > 0 ) 
	{
#ifdef TSC_DEBUG
		ACE_DEBUG((LM_DEBUG,"%s:%d iReadCnt:%d cBuf:%s" , __FILE__ , __LINE__ , iReadCnt , cBuf));
#endif
		write(iDestFd, cBuf, iReadCnt);
	}
	
	close(iSrcFd);
	close(iDestFd);
#endif
}
