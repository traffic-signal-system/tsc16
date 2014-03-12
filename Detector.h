#ifndef _DETECTOR_H_
#define _DETECTOR_H_

#include "ComStruct.h"
#include "ManaKernel.h"
#include "ComDef.h"

/*
	�������������״̬
*/
enum
{
	DET_HEAD_VEHSTS     = 0x02 , //���ذ�������������16��ͨ���ĳ������״̬
	DET_HEAD_SPEED0104  = 0x03 , //���ذ�������������1-4�������Ȧ��ƽ���ٶ�
	DET_HEAD_SPEED0508  = 0x04 , //���ذ�������������5-8�������Ȧ��ƽ���ٶ�
	
	DET_HEAD_SEN0108_GET = 0x05 , //���ذ�������������1-8ͨ��������ļ��������
	DET_HEAD_SEN0916_GET = 0x06 , //���ذ�������������9-16ͨ��������ļ��������
	
	DET_HEAD_COIL0104_GET = 0x07 , //���ذ�������������1-4�������Ȧ�İ����
	DET_HEAD_COIL0508_GET = 0x08 , //���ذ�������������5-8�������Ȧ�İ����
	
	DET_HEAD_DISTAN0104_GET = 0x09 , //���ذ�������������1-4�������Ȧ����Ȧ����
	DET_HEAD_DISTAN0508_GET = 0x0a , //���ذ�������������5-8�������Ȧ����Ȧ����

	DET_HEAD_SEN0108_SET = 0x0b ,  //���ذ巢��1 - 8ͨ�����������ø������
	DET_HEAD_SEN0916_SET = 0x0c ,  //���ذ巢��9 - 16ͨ�����������ø������

	DET_HEAD_COIL0104_SET = 0x0d , //���ذ巢��1 �C 4�������Ȧ�İ����ݸ��������
	DET_HEAD_COIL0508_SET = 0x0e , //���ذ巢��5 �C 8�������Ȧ�İ����ݸ��������
	
	DET_HEAD_COILALLOW_SET= 0x1b,  //���ذ������Ƿ�������Ȧ�󶨡�  ADD 2013 0816 1420

	DET_HEAD_DISTAN0104_SET = 0x0f , //���ذ巢��1 �C 4�������Ȧ����Ȧ������������
	DET_HEAD_DISTAN0508_SET = 0x10 , //���ذ巢��5 �C 8�������Ȧ����Ȧ������������

	DET_HEAD_STS = 0x11 , //���ذ�������������16��ͨ���ļ��������״̬   //������������·����·�ȵ���Ϣ	
	
	DET_HEAD_SENDATA0107_GET = 0x12 ,         //��ʾ���ذ�������������1-7������������ֵ   //ADD: 2013 08 05 1600 
	DET_HEAD_SENDATA0814_GET = 0x13 ,         //��ʾ���ذ�������������8-14������������ֵ
	DET_HEAD_SENDATA1516_GET = 0x14 ,         //��ʾ���ذ�������������1-7������������ֵ

	DET_HEAD_SENDATA0107_SET = 0x15 ,         //��ʾ���ذ����ü��������1-7������������ֵ
	DET_HEAD_SENDATA0814_SET = 0x16 ,         //��ʾ���ذ����ü��������8-14������������ֵ
	DET_HEAD_SENDATA1516_SET = 0x17 ,         //��ʾ���ذ����ü��������1-7������������ֵ

	DET_HEAD_FRENCY_GET = 0x18 , 		 	  //��ʾ���ذ�������������16��ͨ������Ƶ������
	DET_HEAD_FRENCY_SET = 0x19 , 		      //��ʾ���ذ巢�͸������16��ͨ������Ƶ�����á�

	DET_HEAD_WORK_SET = 0x1a  		 		 //��ʾ���ذ����ü����������ʽ��
	
};



class CDetector
{
public:
	static CDetector* CreateInstance();
	
	void SelectDetectorBoardCfg(int *pDetCfg);
	bool CheckDetector();	//�0�3���0�5�0�9�0�5�0�1�0�3���0�5�0�9�0�4�¡�0�2�0�8�0�2�0�5�0�3�0�8�0�3�0�8�0�5���0�9
	bool SelectBrekonCardStatus(Byte ucBoardIndex, Byte ucAddress);
	void SearchAllStatus();
	void SearchSpeed(Byte ucBoardIndex, Byte ucAddress, Byte ucRecAddress);
	void GetOccupy();
	int GetActiveDetSum();
	void GetDetStatus(SDetectorSts* pDetStatus);
	void GetDetData(SDetectorData* pDetData);
	void GetDetAlarm(SDetectorAlarm* pDetAlarm);
	bool IsDetError();
	int GetDetBoardType();
	bool HaveDetBoard();
	void IsVehileHaveCar();
	bool IsHaveCarPhaseGrp(Uint uiPhase,Byte& ucPhaseIndex , SPhaseDetector* pPhaseDet);

	//���ذ�����ȫ�����������16��ͨ���ĳ������״̬
	void GetAllVehSts(Byte QueryType,Byte ucBdindex);  //ADD: 2013 0710 10 54

	//���ذ����󵥸����������16��ͨ���ĳ������״̬
	void GetVehSts(Byte ucBoardAddr,Byte QueryType);   //ADD: 2013 0710 10 54

	//���ذ����������м�����Ĺ���״̬ //ADD: 2013 1114 0930
	void GetAllWorkSts(); 

	void RecvDetCan(Byte ucBoardAddr,SCanFrame sRecvCanTmp);// �����CAN���߽��ջ���������   //ADD: 2013 0710 10 54

/*
#ifndef WINDOWS
	void GetHaveCarTime(time_t* pTime);
#endif
*/
	//void SendDetLink(Byte ucBoardIndex);
	Byte GetDecAddr(Byte ucBoardIndex);
	void SendDecIsLink(Byte ucBoardIndex,Byte IsAllowLink);
	void GetDecVars(Byte ucBoardIndex,Byte GetType);
	void SendDecWorkType(Byte ucBoardIndex);
	void SendDecFrency(Byte ucBoardIndex);
	void SendDecSenData(Byte ucBoardIndex,Byte ucSetType) ; //ADD  20130816 1600
	void SendDetLink(Byte ucBoardIndex,Byte SetType);
	void SearchDetLink(Byte ucBoardIndex);

	//void SendRoadDistance(Byte ucBoardIndex);
	void SendRoadDistance(Byte ucBoardIndex,Byte SetType);
	void SearchRoadDistance(Byte ucBoardIndex);

	void SearchRoadSpeed(Byte ucBoardIndex);

	//void SendDelicacy(Byte ucBoardIndex);
	void SendDelicacy(Byte ucBoardIndex,Byte SetType);
	void SearchDelicacy(Byte ucBoardIndex);

	void GetAdaptInfo(int* pDetTimeLen , int* pTotalStat);
	void SetStatCycle(Byte ucCycle);

	void SendRecordBoardMsg(Byte ucDetIndex,Byte ucType);
	void PrintDetInfo(char* pFileName,int iFileLine,Byte ucBoardIndex,char* sErrSrc,int iPrintCnt,Byte* ucRecvBuf);

public:
	
	Byte m_ucActiveBoard1;   //1  - 16 �0�3�0�6�0�9�0�4�0�8�0�2�0�3���0�5�0�9�0�4�¡�0�2
	Byte m_ucActiveBoard2;   //17 - 32 �0�3�0�6�0�9�0�4�0�8�0�2�0�3���0�5�0�9�0�4�¡�0�2
	Byte m_ucActiveBoard3;	//33 - 48
	bool m_bErrFlag[MAX_DETECTOR];      //�0�5���0�2�������0�0�0�0

	bool m_bRecordSts[MAX_DET_BOARD];      //�0�7�0�3�0�6�0�3�0�5�0�2�0�3�0�5�0�0�0�3�0�3���0�5�0�9��0�2�0�8�0�2�0�1�0�8�0�4�0�3���0�5�0�0�0�1

	Byte m_ucDetError[MAX_DETECTOR];     //�0�1�0�8�0�9�0�3���0�5�0�0�0�1      32 - 47:�0�8�0�31�0�7���0�3���0�5�0�9�0�4��      48 - 63:�0�8�0�32�0�7���0�3���0�5�0�9�0�4��
	Byte m_ucLastDetError[MAX_DETECTOR]; //�0�7�0�3�0�5�0�2�0�1�0�8�0�9�0�3���0�5�0�0�0�1   32 - 47:�0�8�0�31�0�7���0�3���0�5�0�9�0�4��      48 - 63:�0�8�0�32�0�7���0�3���0�5�0�9�0�4��
	Byte m_ucDetErrTime[MAX_DETECTOR];   //�0�1�0�8�0�9�0�3�0�5�0�2�0�8�0�5      32 - 47:�0�8�0�31�0�7���0�3���0�5�0�9�0�4��      48 - 63:�0�8�0�32�0�7���0�3���0�5�0�9�0�4��

	int m_iAdapDetTimeLen[MAX_DETECTOR];   //�0�7�0�4�0�6�0�8�0�8���0�3�0�1�0�8�0�2�0�1�0�6�0�3�0�4  100ms/�0�8�0�6�0�2�0�3 �0�7�0�1�0�7�0�3���0�8�0�8�0�8�0�7�0�7�0�7�0�1�0�0�0�4
	int m_iAdapTotalStat[MAX_DETECTOR];    //�0�6�0�8�0�9�0�0�0�1�0�6�0�3�0�4   /�0�5�0�2 �0�7�0�1�0�7�0�3���0�8�0�8�0�8�0�7�0�7�0�7�0�1�0�0�0�4

	Byte m_ucRoadSpeed[MAX_DET_BOARD][8];     //�0�6�0�8�0�8�0�8�0�8�0�2�0�4�0�4�0�0���0�9�0�2�0�9�0�6 

	Byte m_ucSetRoadDis[MAX_DET_BOARD][8];    //�0�7���0�0�0�1�0�8�0�2�0�6�0�8�0�8�0�8�0�0���0�8�0�5
	Byte m_ucGetRoadDis[MAX_DET_BOARD][8];    //�0�3�0�9�0�6�0�3�0�8�0�4�0�8�0�2�0�6�0�8�0�8�0�8�0�0���0�8�0�5

	Byte m_ucSetDetDelicacy[MAX_DET_BOARD][MAX_DETECTOR_PER_BOARD];   //�0�7���0�0�0�1�0�3���0�5�0�9�0�4�¡�0�2�0�8�0�2�0�9���0�1�0�0�0�9�0�6
	Byte m_ucGetDetDelicacy[MAX_DET_BOARD][MAX_DETECTOR_PER_BOARD];   //�0�3�0�9�0�6�0�3�0�8�0�4�0�8�0�2�0�3���0�5�0�9�0�4�¡�0�2�0�8�0�2�0�9���0�1�0�0�0�9�0�6

	Byte m_sSetLookLink[MAX_DET_BOARD][8];  //�0�7���0�0�0�1�0�8�0�2�0�3�0�8�0�6�0�7�0�9�0�8�0�7�0�7�0�1�0�1�0�3�0�8
	Byte m_sGetLookLink[MAX_DET_BOARD][8];  //�0�3�0�9�0�6�0�3�0�8�0�4�0�8�0�2�0�3�0�8�0�6�0�7�0�9�0�8�0�7�0�7�0�1�0�1�0�3�0�8
	
	Byte m_ucSetFrency[MAX_DET_BOARD][MAX_DETECTOR_PER_BOARD] ;
	Byte m_ucGetFrency[MAX_DET_BOARD][MAX_DETECTOR_PER_BOARD] ;
	Byte m_iChkType ;
	Byte m_ucSetSensibility[MAX_DET_BOARD][MAX_DETECTOR_PER_BOARD] ; //ADD 2013 0816 1530
	Byte m_ucGetSensibility[MAX_DET_BOARD][MAX_DETECTOR_PER_BOARD] ; //ADD 2013 0816 1530
private:
	CDetector();
	~CDetector();

	Byte m_ucNoCnt[MAX_DET_BOARD];         //�0�9�0�1�0�4�0�3�0�1�0�3�0�7�0�4�0�4�0�7�0�8�0�5�0�8�0�5�0�0�0�6�0�8�0�2�0�5�0�2�0�8�0�5
	Byte m_ucErrAddrCnt[MAX_DET_BOARD];    //�0�9�0�1�0�4�0�3�0�4�0�7�0�8�0�5�0�8�0�4�0�5���0�2���0�8�0�1�0�0���0�8�0�2�0�5�0�2�0�8�0�5
	Byte m_ucErrCheckCnt[MAX_DET_BOARD];   //�0�9�0�1�0�4�0�3�0�4�0�7�0�8�0�5�0�8�0�4�0�4�0�5�0�5���0�5���0�2���0�8�0�2�0�5�0�2�0�8�0�5
	Byte m_ucRightCnt[MAX_DET_BOARD];      //�0�9�0�1�0�4�0�3�0�4�0�7�0�8�0�5�0�8�0�4�0�9�0�5�0�6���0�8�0�5�0�0�0�6�0�8�0�2�0�5�0�2�0�8�0�5

	int m_iDevFd;
	int m_iTotalDistance;             //�0�1�0�6�0�3�0�4�0�3�0�1�0�6�0�0 
	
	
	int m_iDetCfg[MAX_DET_BOARD];     // 0-�0�5�0�3�0�4�0�0�0�7�0�1 1-�0�8�0�3�0�6�0�3�0�6�0�2�0�3���0�5�0�9�0�4�� 17-�0�8�0�317�0�6�0�2�0�3���0�5�0�9�0�4��
	int m_iBoardErr[MAX_DET_BOARD];   //true:�0�2�0�1 false:�0�3�0�8�0�8�0�0
	int m_iLastDetSts[MAX_DETECTOR];  //�0�7�0�3�0�5�0�2�0�6�0�8�0�8�0�2���0�5�0�0�0�1
	int m_iDetStatus[MAX_DETECTOR];   //1:�0�7�0�4�0�6�0�8 0:�0�2�0�7�0�6�0�8  0 - 15:�0�8�0�31�0�7���0�3���0�5�0�9�0�4�0�4�0�7�0�7�0�3��0�2 16 - 31:�0�8�0�32�0�7���0�3���0�5�0�9�0�4�0�4�0�7�0�7�0�3��0�2
	int m_iDetTimeLen[MAX_DETECTOR];  //�0�7�0�4�0�6�0�8�0�8���0�3�0�1�0�8�0�2�0�1�0�6�0�3�0�4  100ms/�0�8�0�6�0�2�0�3
	int m_iDetOccupy[MAX_DETECTOR];   //�0�9�0�3�0�7�0�4�0�0�0�8
	Byte m_ucTotalStat[MAX_DETECTOR];   //�0�6�0�8�0�9�0�0�0�1�0�6�0�3�0�4   /�0�5�0�2
	
	Byte m_ucDetSts[MAX_DET_BOARD][MAX_DETECTOR_PER_BOARD]; //�г��޳���־ ADD: 2013 0710 1050
	//int m_iLastDetTimeLen[MAX_DETECTOR];  //�0�7�0�3�0�6�0�2�0�1�0�6�0�3�0�4�0�0�0�5�0�4�0�3�0�8�0�2�0�7�0�4�0�6�0�8�0�8���0�3�0�1�0�8�0�2�0�1�0�6�0�3�0�4  100ms/�0�8�0�6�0�2�0�3	
	
	STscConfig* m_pTscCfg;

	ACE_Thread_Mutex  m_sMutex;
};

#endif //_DETECTOR_H_
