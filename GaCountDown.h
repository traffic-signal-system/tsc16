#ifndef _GACOUNTDOWN_H_
#define _GACOUNTDOWN_H_

#include "ComStruct.h"

#define GACOUNTDOWN_CPP "GaCountDown.cpp"

//const int GA_YELLOW_COUNT = 0;   //�ƵƲ�����ʱ >0�͵�
const int GA_MAX_DIRECT    = 4;   //����� �� �� �� �� 
const int GA_MAX_LANE      = 4;   //������� �� ֱ �� ����
const int GA_MAX_COLOR     = 3;   //�� �� ��
const int GA_MAX_SEND_BUF  = 5;  //����ʱЭ�鶨���ÿ֡�����ֽ���

enum
{
	GA_DIRECT_NORTH  = 0 ,  //��
	GA_DIRECT_EAST   = 1 ,  //��
	GA_DIRECT_SOUTH  = 2 ,  //��
	GA_DIRECT_WEST   = 3 ,  //��
	GA_DIRECT_OTHER  = 4 
};

enum
{
	GA_LANE_LEFT     = 0 ,  //��   
	GA_LANE_STRAIGHT = 1 ,  //ֱ
	GA_LANE_RIGHT    = 2 ,  //��
	GA_LANE_FOOT     = 3 ,  //����
	GA_LANE_OTHER    = 4
};

enum
{
	GA_COLOR_RED    = 0 , //��
	GA_COLOR_YELLOW = 1 , //��
	GA_COLOR_GREEN  = 2 , //��
	GA_COLOR_OTHER  = 3   //����
};

/*
*������������ʱ��(����GAT508-2004Э��)
*ÿ������ֻ��һ������ʱ��  Ϊ�����е���ʱ���ڿ��Ƿ�Χ
*/
class CGaCountDown
{
public:
	static CGaCountDown* CreateInstance();

	void GaGetCntDownInfo();
	void GaUpdateCntDownCfg();
	void GaSendStepPer();

	static Byte m_ucGaDirVector[GA_MAX_DIRECT][GA_MAX_LANE];
	 

	CGaCountDown();
	~CGaCountDown();

	Byte GaGetCntTime(Byte ucSignalId);
	void GaGetDirLane(Byte ucTableId , Byte& ucDir , Byte& ucLane);
	void GaSetSendBuffer();
	bool ComputeColorCount( Byte ucDirIndex , Byte& ucColor , Byte& ucCountTime );
	bool PackSendBuf( Byte ucDirIndex , Byte ucColor , Byte ucCountTime );
	void sendblack();    //���ͺ�����Ϣ
	void send8cntdown(); //����8�뵹��ʱ

	bool m_bGaNeedCntDown[GA_MAX_DIRECT][GA_MAX_LANE]; //��Ҫ����ʱ�ķ���
	Byte m_ucGaRuntime[GA_MAX_DIRECT][GA_MAX_LANE];    //�õ�ɫ����ά�ֵ�ʱ��
	Byte m_ucGaColor[GA_MAX_DIRECT][GA_MAX_LANE];      //��ɫ
	GBT_DB::PhaseToDirec m_sGaPhaseToDirec[GA_MAX_DIRECT][GA_MAX_LANE];

	Byte m_sGaSendBuf[GA_MAX_DIRECT][GA_MAX_SEND_BUF];   //���ͻ���
private:
};

#endif //_GACOUNTDOWN_H_
