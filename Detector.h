#ifndef _DETECTOR_H_
#define _DETECTOR_H_

#include "ComStruct.h"
#include "ManaKernel.h"
#include "ComDef.h"

/*
	检测器交互控制状态
*/
enum
{
	DET_HEAD_VEHSTS     = 0x02 , //主控板请求检测器发送16个通道的车辆检测状态
	DET_HEAD_SPEED0104  = 0x03 , //主控板请求检测器发送1-4组测速线圈的平均速度
	DET_HEAD_SPEED0508  = 0x04 , //主控板请求检测器发送5-8组测速线圈的平均速度
	
	DET_HEAD_SEN0108_GET = 0x05 , //主控板请求检测器发送1-8通道检测器的检测灵敏度
	DET_HEAD_SEN0916_GET = 0x06 , //主控板请求检测器发送9-16通道检测器的检测灵敏度
	
	DET_HEAD_COIL0104_GET = 0x07 , //主控板请求检测器发送1-4组测速线圈的绑定情况
	DET_HEAD_COIL0508_GET = 0x08 , //主控板请求检测器发送5-8组测速线圈的绑定情况
	
	DET_HEAD_DISTAN0104_GET = 0x09 , //主控板请求检测器发送1-4组测速线圈的线圈距离
	DET_HEAD_DISTAN0508_GET = 0x0a , //主控板请求检测器发送5-8组测速线圈的线圈距离

	DET_HEAD_SEN0108_SET = 0x0b ,  //主控板发送1 - 8通道灵敏度设置给检测器
	DET_HEAD_SEN0916_SET = 0x0c ,  //主控板发送9 - 16通道灵敏度设置给检测器

	DET_HEAD_COIL0104_SET = 0x0d , //主控板发送1 – 4组测速线圈的绑定数据给检测器板
	DET_HEAD_COIL0508_SET = 0x0e , //主控板发送5 – 8组测速线圈的绑定数据给检测器板
	
	DET_HEAD_COILALLOW_SET= 0x1b,  //主控板设置是否允许线圈绑定。  ADD 2013 0816 1420

	DET_HEAD_DISTAN0104_SET = 0x0f , //主控板发送1 – 4组测速线圈的线圈距离给检测器板
	DET_HEAD_DISTAN0508_SET = 0x10 , //主控板发送5 – 8组测速线圈的线圈距离给检测器板

	DET_HEAD_STS = 0x11 , //主控板请求检测器发送16个通道的检测器工作状态   //包括正常，短路，开路等等信息	
	
	DET_HEAD_SENDATA0107_GET = 0x12 ,         //表示主控板请求检测器发送1-7级的灵敏度数值   //ADD: 2013 08 05 1600 
	DET_HEAD_SENDATA0814_GET = 0x13 ,         //表示主控板请求检测器发送8-14级的灵敏度数值
	DET_HEAD_SENDATA1516_GET = 0x14 ,         //表示主控板请求检测器发送1-7级的灵敏度数值

	DET_HEAD_SENDATA0107_SET = 0x15 ,         //表示主控板设置检测器发送1-7级的灵敏度数值
	DET_HEAD_SENDATA0814_SET = 0x16 ,         //表示主控板设置检测器发送8-14级的灵敏度数值
	DET_HEAD_SENDATA1516_SET = 0x17 ,         //表示主控板设置检测器发送1-7级的灵敏度数值

	DET_HEAD_FRENCY_GET = 0x18 , 		 	  //表示主控板请求检测器发送16个通道的震荡频率设置
	DET_HEAD_FRENCY_SET = 0x19 , 		      //表示主控板发送给检测器16个通道的震荡频率设置。

	DET_HEAD_WORK_SET = 0x1a  		 		 //表示主控板设置检测器工作方式。
	
};



class CDetector
{
public:
	static CDetector* CreateInstance();
	
	void SelectDetectorBoardCfg(int *pDetCfg);
	bool CheckDetector();	//Œì²â£¬Œì²âÆ÷°åµÄŽæÔÚÊÇ·ñ
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

	//主控板请求全部检测器发送16个通道的车辆检测状态
	void GetAllVehSts(Byte QueryType,Byte ucBdindex);  //ADD: 2013 0710 10 54

	//主控板请求单个检测器发送16个通道的车辆检测状态
	void GetVehSts(Byte ucBoardAddr,Byte QueryType);   //ADD: 2013 0710 10 54

	//主控板请求发送所有检测器的工作状态 //ADD: 2013 1114 0930
	void GetAllWorkSts(); 

	void RecvDetCan(Byte ucBoardAddr,SCanFrame sRecvCanTmp);// 处理从CAN总线接收回来的数据   //ADD: 2013 0710 10 54

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
	
	Byte m_ucActiveBoard1;   //1  - 16 »î¶¯µÄŒì²âÆ÷°å
	Byte m_ucActiveBoard2;   //17 - 32 »î¶¯µÄŒì²âÆ÷°å
	Byte m_ucActiveBoard3;	//33 - 48
	bool m_bErrFlag[MAX_DETECTOR];      //ŽíÎó±êÖŸ

	bool m_bRecordSts[MAX_DET_BOARD];      //ÉÏÒ»ŽÎŒÇÂŒŒì²â°åµÄÍšÐÅ×ŽÌ¬

	Byte m_ucDetError[MAX_DETECTOR];     //¹ÊÕÏ×ŽÌ¬      32 - 47:µÚ1¿éŒì²âÆ÷      48 - 63:µÚ2¿éŒì²âÆ÷
	Byte m_ucLastDetError[MAX_DETECTOR]; //ÉÏŽÎ¹ÊÕÏ×ŽÌ¬   32 - 47:µÚ1¿éŒì²âÆ÷      48 - 63:µÚ2¿éŒì²âÆ÷
	Byte m_ucDetErrTime[MAX_DETECTOR];   //¹ÊÕÏŽÎÊý      32 - 47:µÚ1¿éŒì²âÆ÷      48 - 63:µÚ2¿éŒì²âÆ÷

	int m_iAdapDetTimeLen[MAX_DETECTOR];   //ÓÐ³µÊ±ŒäµÄÍ³ŒÆ  100ms/µ¥Î» ÓÃÓÚ×ÔÊÊÓŠ¿ØÖÆ
	int m_iAdapTotalStat[MAX_DETECTOR];    //³µÁŸÍ³ŒÆ   /ŽÎ ÓÃÓÚ×ÔÊÊÓŠ¿ØÖÆ

	Byte m_ucRoadSpeed[MAX_DET_BOARD][8];     //³µµÀµÄÆœŸùËÙ¶È 

	Byte m_ucSetRoadDis[MAX_DET_BOARD][8];    //ÉèÖÃµÄ³µµÀŸàÀë
	Byte m_ucGetRoadDis[MAX_DET_BOARD][8];    //»ñÈ¡µœµÄ³µµÀŸàÀë

	Byte m_ucSetDetDelicacy[MAX_DET_BOARD][MAX_DETECTOR_PER_BOARD];   //ÉèÖÃŒì²âÆ÷°åµÄÁéÃô¶È
	Byte m_ucGetDetDelicacy[MAX_DET_BOARD][MAX_DETECTOR_PER_BOARD];   //»ñÈ¡µœµÄŒì²âÆ÷°åµÄÁéÃô¶È

	Byte m_sSetLookLink[MAX_DET_BOARD][8];  //ÉèÖÃµÄÏßÈŠ¶ÔÓŠ¹ØÏµ
	Byte m_sGetLookLink[MAX_DET_BOARD][8];  //»ñÈ¡µœµÄÏßÈŠ¶ÔÓŠ¹ØÏµ
	
	Byte m_ucSetFrency[MAX_DET_BOARD][MAX_DETECTOR_PER_BOARD] ;
	Byte m_ucGetFrency[MAX_DET_BOARD][MAX_DETECTOR_PER_BOARD] ;
	Byte m_iChkType ;
	Byte m_ucSetSensibility[MAX_DET_BOARD][MAX_DETECTOR_PER_BOARD] ; //ADD 2013 0816 1530
	Byte m_ucGetSensibility[MAX_DET_BOARD][MAX_DETECTOR_PER_BOARD] ; //ADD 2013 0816 1530
private:
	CDetector();
	~CDetector();

	Byte m_ucNoCnt[MAX_DET_BOARD];         //Á¬ÐøÃ»ÓÐœÓÊÜÊýŸÝµÄŽÎÊý
	Byte m_ucErrAddrCnt[MAX_DET_BOARD];    //Á¬ÐøœÓÊÜµœŽíÎóµØÖ·µÄŽÎÊý
	Byte m_ucErrCheckCnt[MAX_DET_BOARD];   //Á¬ÐøœÓÊÜµœÐ£ÑéŽíÎóµÄŽÎÊý
	Byte m_ucRightCnt[MAX_DET_BOARD];      //Á¬ÐøœÓÊÜµœÕýÈ·ÊýŸÝµÄŽÎÊý

	int m_iDevFd;
	int m_iTotalDistance;             //Í³ŒÆŒäžô 
	
	
	int m_iDetCfg[MAX_DET_BOARD];     // 0-²»ÆôÓÃ 1-µÚÒ»žöŒì²âÆ÷ 17-µÚ17žöŒì²âÆ÷
	int m_iBoardErr[MAX_DET_BOARD];   //true:ºÃ false:»µµô
	int m_iLastDetSts[MAX_DETECTOR];  //ÉÏŽÎ³µµÄ×ŽÌ¬
	int m_iDetStatus[MAX_DETECTOR];   //1:ÓÐ³µ 0:ÎÞ³µ  0 - 15:µÚ1¿éŒì²âÆ÷œÓ¿Ú°å 16 - 31:µÚ2¿éŒì²âÆ÷œÓ¿Ú°å
	int m_iDetTimeLen[MAX_DETECTOR];  //ÓÐ³µÊ±ŒäµÄÍ³ŒÆ  100ms/µ¥Î»
	int m_iDetOccupy[MAX_DETECTOR];   //ÕŒÓÐÂÊ
	Byte m_ucTotalStat[MAX_DETECTOR];   //³µÁŸÍ³ŒÆ   /ŽÎ
	
	Byte m_ucDetSts[MAX_DET_BOARD][MAX_DETECTOR_PER_BOARD]; //有车无车标志 ADD: 2013 0710 1050
	//int m_iLastDetTimeLen[MAX_DETECTOR];  //ÉÏžöÍ³ŒÆÖÜÆÚµÄÓÐ³µÊ±ŒäµÄÍ³ŒÆ  100ms/µ¥Î»	
	
	STscConfig* m_pTscCfg;

	ACE_Thread_Mutex  m_sMutex;
};

#endif //_DETECTOR_H_
