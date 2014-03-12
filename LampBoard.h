#ifndef __LAMPBOARD__H__
#define __LAMPBOARD__H__
#include "ComDef.h"
#include "ComStruct.h"
#include <ace/Thread_Mutex.h>
#include <ace/OS.h>
#include "ManaKernel.h"
#define LAMPBOARD_DEBUG
/*
*灯控板信息输出以及灯状态获取等
*/
class CLampBoard 
{
public:
	static CLampBoard* CreateInstance();
	void ReviseLampInfo(Byte ucType,Byte pLampInfo[MAX_LAMP]);
	void SetLamp(Byte* pLampOn,Byte* pLampFlash);
	void SetSeriousFlash(bool isflash);
	bool IsFlash() ;	
	//µÆÉ«ÊýŸÝ 
	void SendLamp();
	void SendSingleLamp(Byte ucLampBoardId);
	//µÆ¿Ø°åÏÂ·¢ÅäÖÃÊýŸÝ£¬ÈçµÆÅÝŒì²â¿ª¹Ø£¬ºìÂÌ³åÍ»Œì²â¿ª¹Ø
	void SendCfg();
	void SendSingleCfg(Byte ucLampBoardId);
	//µÆÅÝŒì²âÊýŸÝ£¬ºìÂÌ³åÍ»Œì²âÊýŸÝ
	void CheckLight();
	void CheckSingleLight(Byte ucLampBoardId);
	//µÆÅÝµçÁ÷Œì²âœá¹ûÊýŸÝ
	void CheckLampElect(Byte ucLampBoardId,Byte ucType);
	void CheckSingleElect(Byte ucLampBoardId);
	void CheckElect();
	//µÆ¿Ø°å°åÔØÎÂ¶ÈŒì²âµÄÎÂ¶ÈÖµ
	void CheckTemp();
	void CheckSingleTemp(Byte ucLampBoardId);
	void RecvLampCan(Byte ucBoardAddr,SCanFrame sRecvCanTmp); //ADD: 2013 0712 CAN接收灯泡检测数据
private:
	CLampBoard();
	~CLampBoard();
public:
	CManaKernel * pManakernel ;
	bool m_bRecordSts[MAX_LAMP/12];  //各个灯控板的通信状态
	bool IsChkLight ;
	Byte m_ucCheckCfg[MAX_LAMP_BOARD];        //bit 0 1 存是否开启灯泡损坏检测,bit 2 3保存是否开启红绿冲突检测  //ADD: 2013 0712 1111
private:
	//input para
	Byte m_ucLampBoardError[MAX_LAMP_BOARD] ;
	Byte m_ucLampOn[MAX_LAMP];         //与物理端子对应 亮：1 灭：0
	Byte m_ucLampFlash[MAX_LAMP];
	bool m_bSeriousFlash;	
	Byte m_ucLampOnCfg[MAX_LAMP_BOARD][3] ;   // 配置每块板每个通道的灯泡亮灭与闪烁状况，下发到灯控板的配置必须与此匹配 3字节的每两位表示一个通道
	bool m_bLampErrFlag[MAX_LAMP];
	//output para
	//Byte m_ucLampSts[MAX_LAMP];      //各个灯状态
	Byte m_ucLampStas[MAX_LAMP] ;     // 单个灯控板灯泡损坏状态   //ADD: 2013 0712 1111
	Byte m_ucLampConflic[MAX_LAMP_BOARD][4] ; // 灯控板四组信号灯的红绿冲突情况 ADD:20130802 1350 
	Byte m_ucChannelSts[(MAX_LAMP+3)/4]; //各个通道状态
	Ushort m_usLampElect[4][8];  //各个灯的电流
	int iLampBoardTemp[4]; //各个灯控板卡的板载温度
	
	ACE_Thread_Mutex  m_mutexLamp;
};

#endif   //__LAMPBOARD__H__
