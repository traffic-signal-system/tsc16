#ifndef __LAMPBOARD__H__
#define __LAMPBOARD__H__
#include "ComDef.h"
#include "ComStruct.h"
#include <ace/Thread_Mutex.h>
#include <ace/OS.h>
#include "ManaKernel.h"
#define LAMPBOARD_DEBUG
/*
*�ƿذ���Ϣ����Լ���״̬��ȡ��
*/
class CLampBoard 
{
public:
	static CLampBoard* CreateInstance();
	void ReviseLampInfo(Byte ucType,Byte pLampInfo[MAX_LAMP]);
	void SetLamp(Byte* pLampOn,Byte* pLampFlash);
	void SetSeriousFlash(bool isflash);
	bool IsFlash() ;	
	//�0�8�0�4�0�7�0�0�0�8�0�5�0�0�0�6 
	void SendLamp();
	void SendSingleLamp(Byte ucLampBoardId);
	//�0�8�0�4�0�7�0�1��0�2�0�3�0�0���0�4�0�3�0�1�0�0�0�1�0�8�0�5�0�0�0�6�0�5�0�1�0�6�0�4�0�8�0�4�0�3�0�6�0�3���0�5�0�9�0�7�0�9�0�1�0�1�0�5�0�1�0�2���0�0�0�0�0�6�0�2�0�1�0�3�0�3���0�5�0�9�0�7�0�9�0�1�0�1
	void SendCfg();
	void SendSingleCfg(Byte ucLampBoardId);
	//�0�8�0�4�0�3�0�6�0�3���0�5�0�9�0�8�0�5�0�0�0�6�0�5�0�1�0�2���0�0�0�0�0�6�0�2�0�1�0�3�0�3���0�5�0�9�0�8�0�5�0�0�0�6
	void CheckLight();
	void CheckSingleLight(Byte ucLampBoardId);
	//�0�8�0�4�0�3�0�6�0�8�0�4�0�9�0�3���0�5�0�9�0�4���0�1�0�4�0�8�0�5�0�0�0�6
	void CheckLampElect(Byte ucLampBoardId,Byte ucType);
	void CheckSingleElect(Byte ucLampBoardId);
	void CheckElect();
	//�0�8�0�4�0�7�0�1��0�2��0�2�0�8�0�1�0�2�0�0�0�9�0�6�0�3���0�5�0�9�0�8�0�2�0�2�0�0�0�9�0�6�0�0�0�8
	void CheckTemp();
	void CheckSingleTemp(Byte ucLampBoardId);
	void RecvLampCan(Byte ucBoardAddr,SCanFrame sRecvCanTmp); //ADD: 2013 0712 CAN���յ��ݼ������
private:
	CLampBoard();
	~CLampBoard();
public:
	CManaKernel * pManakernel ;
	bool m_bRecordSts[MAX_LAMP/12];  //�����ƿذ��ͨ��״̬
	bool IsChkLight ;
	Byte m_ucCheckCfg[MAX_LAMP_BOARD];        //bit 0 1 ���Ƿ��������𻵼��,bit 2 3�����Ƿ������̳�ͻ���  //ADD: 2013 0712 1111
private:
	//input para
	Byte m_ucLampBoardError[MAX_LAMP_BOARD] ;
	Byte m_ucLampOn[MAX_LAMP];         //��������Ӷ�Ӧ ����1 ��0
	Byte m_ucLampFlash[MAX_LAMP];
	bool m_bSeriousFlash;	
	Byte m_ucLampOnCfg[MAX_LAMP_BOARD][3] ;   // ����ÿ���ÿ��ͨ���ĵ�����������˸״�����·����ƿذ�����ñ������ƥ�� 3�ֽڵ�ÿ��λ��ʾһ��ͨ��
	bool m_bLampErrFlag[MAX_LAMP];
	//output para
	//Byte m_ucLampSts[MAX_LAMP];      //������״̬
	Byte m_ucLampStas[MAX_LAMP] ;     // �����ƿذ������״̬   //ADD: 2013 0712 1111
	Byte m_ucLampConflic[MAX_LAMP_BOARD][4] ; // �ƿذ������źŵƵĺ��̳�ͻ��� ADD:20130802 1350 
	Byte m_ucChannelSts[(MAX_LAMP+3)/4]; //����ͨ��״̬
	Ushort m_usLampElect[4][8];  //�����Ƶĵ���
	int iLampBoardTemp[4]; //�����ƿذ忨�İ����¶�
	
	ACE_Thread_Mutex  m_mutexLamp;
};

#endif   //__LAMPBOARD__H__
