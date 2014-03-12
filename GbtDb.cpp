
/***************************************************************
Copyright(c) 2013  AITON. All rights reserved.
Author:     AITON
FileName:   GbtDb.cpp
Date:       2013-1-1
Description:信号机处理与数据库交互有关的操作
Version:    V1.0
History:
***************************************************************/
#include <ace/OS.h>
#include <ace/Guard_T.h>
#include <ace/Log_Msg.h>

#include "GbtDb.h"
#include "GbtExchg.h"
#include "ComStruct.h"
#include "ComFunc.h"

#ifndef SHORT_FILE
#define SHORT_FILE "GbtDb.cpp" //MOD: 201309231000整理顺序
#endif

namespace GBT_DB  //定义全局数据库命名空间
{
	CGbtTscDb*  m_gTscDb = NULL;
	
/***************定义信号机数据库包含的所有表名字符数组*******************/
	char* Table_Name[]   = {(char*)"Tbl_System",
                         (char*)"Tbl_Constant",
                         (char*)"Tbl_Module", 
                         (char*)"Tbl_Plan", 
                         (char*)"Tbl_Schedule", 
                         (char*)"Tbl_Phase", 
                         (char*)"Tbl_Collision",
                         (char*)"Tbl_Detector",
                         (char*)"Tbl_Channel", 
                         (char*)"Tbl_Pattern", 
                         (char*)"Tbl_StagePattern",
                         (char*)"Tbl_OverlapPhase",
                         (char*)"Tbl_EventType",
                         (char*)"Tbl_EventLog",
                         (char*)"Tbl_VehicleStat",
                         (char*)"Tbl_CntDownDev",
                         (char*)"Tbl_SpecFun",
						 (char*)"Tbl_DetExtend",
						 (char*)"Tbl_StageFactTime",
						 (char*)"Tbl_AdaptStageTime",
						 (char*)"Tbl_PhaseToDirec",
						 (char*)"Tbl_AdaptPara" ,
						 (char *)"Tbl_LampCheck"   //ADD:2013 0801 0940
						};
/***************定义信号机数据库包含的所有表名字符数组*******************/

/***************定义信号机数据库包含的所有表的字段名字符数组*******************/
char* Tbl_LampCheck[] ={
						(char *)"ucId",
						(char *)"ucFlag"	};

char* Tbl_Plan[]     = {(char*)"Id", 
                        (char*)"usMonthFlag", 
                        (char*)"ucWeekFlag", 
                        (char*)"ulDayFlag", 
                        (char*)"ucScheduleId"};

char* Tbl_Schedule[] = {(char*)"ucScheduleId", 
                        (char*)"ucEvtId", 
                        (char*)"ucBgnHour", 
                        (char*)"ucBgnMinute", 
                        (char*)"ucCtrlMode", 
                        (char*)"ucPatternNo", 
                        (char*)"ucAuxOut",
                        (char*)"ucSpecialOut"};

char* Tbl_Module[]   = {(char*)"ucModuleId", 
                        (char*)"sDevNode",
                        (char*)"sCompany",
                        (char*)"sModel",
                        (char*)"sVersion",
                        (char*)"ucType"};

char* Tbl_Phase[]    = {(char*)"ucPhaseId",
                        (char*)"ucPedestrianGreen",
                        (char*)"ucPedestrianClear",
                        (char*)"ucMinGreen",
                        (char*)"ucGreenDelayUnit",
                        (char*)"ucMaxGreen1",
                        (char*)"ucMaxGreen2",
                        (char*)"ucFixGreen",
                        (char*)"ucGreenFlash",
                        (char*)"ucPhaseTypeFlag",
                        (char*)"ucPhaseOption",
                        (char*)"ucExtend"};

char* Tbl_Collision[] = {(char*)"ucPhaseId",
                         (char*)"usCollisonFlag"};

char* Tbl_Detector[]   = {(char*)"ucDetectorId",
                        (char*)"ucPhaseId",
                        (char*)"ucDetFlag",
                        (char*)"ucDirect",
						(char*)"ucValidTime",
                        (char*)"ucOptionFlag",
                        (char*)"usSaturationFlow",
                        (char*)"ucSaturationOccupy"};

char* Tbl_Channel[]   = {(char*)"ucChannelId",
                         (char*)"ucCtrlSrc",
                         (char*)"ucAutoFlsCtrlFlag",
                         (char*)"ucCtrlType"};

char* Tbl_Pattern[]   = {(char*)"ucPatternId",
                        (char*)"ucCycleTime",
                        (char*)"ucOffset",
                        (char*)"ucCoorPhase",
                        (char*)"ucStagePatternId"};

char* Tbl_StagePattern[] = {(char*)"ucStagePatternId",
                          (char*)"ucStageNo",
                          (char*)"usAllowPhase",
                          (char*)"ucGreenTime",
                          (char*)"ucYellowTime",
                          (char*)"ucRedTime",
                          (char*)"ucOption"};

char* TBL_OverlapPhase[] = {(char*)"ucOverlapPhaseId",
                        (char*)"ucOperateType",
                        (char*)"ucIncldPhaseCount",
                        (char*)"sIncldPhase",
                        (char*)"ucAdjustPhaseCount",
                        (char*)"sAdjustPhase",
                        (char*)"ucTailGreen",
                        (char*)"ucTailYellow",
                        (char*)"ucTailRed"};

char* Tbl_EventType[] = {(char*)"ucEvtTypeId",
                        (char*)"ulClearTime",
                        (char*)"sEvtDesc"};

char* Tbl_EventLog[] = {(char*)"ucEventId",
                        (char*)"ucEvtType",
                        (char*)"ulHappenTime",
                        (char*)"ulEvtValue",
                        (char*)"ulEventTime",
                        };

char* Tbl_VehicleStat[] = {(char*)"ulId",
                           (char*)"ucDetId",
						   (char*)"ulCarTotal",
						   (char*)"ucOccupancy",
						   (char*)"ulAddTime"};

char* Tbl_CntDownDev[] = {(char*)"ucDevId",
                          (char*)"usPhase",
						  (char*)"ucOverlapPhase",
						  (char*)"ucMode"
						  };

char* Tbl_SpecFun[] = {(char*)"ucFunType",
					   (char*)"ucValue",};

char* Tbl_DetExtend[] = {(char*)"ucId",
						 (char*)"ucSensi",
						 (char*)"ucGrpNo",
					     (char*)"ucPro",
						 (char*)"ucOcuDefault",
						 (char*)"usCarFlow",
						  (char *)"ucFrency",
						 (char *)"ucGrpData",
						 (char *)"ucGrpDistns"
						 };

char* Tbl_StageFactTime[] = {(char*)"ulAddTime",
						     (char*)"ucStageCnt",
							 (char*)"sStageGreenTime"};

char* Tbl_AdaptStageTime[] = {(char*)"ucId",
							  (char*)"ucWeekType",
							  (char*)"ucHour",
							  (char*)"ucMin",
							  (char*)"ucStageCnt",
							  (char*)"sStageTime"};

char* Tbl_PhaseToDirec[] = {(char*)"ucId",
							(char*)"ucPhase",
							(char*)"ucOverlapPhase",
							(char*)"ucRoadCnt"
							};

char* Tbl_AdaptPara[] = {(char*)"ucType",
						 (char*)"ucFirstPro",
						 (char*)"ucSecPro",
						 (char*)"ucThirdPro",
						 (char*)"ucOcuPro",
						 (char*)"ucCarFlowPro",
						 (char*)"ucSmoothPara"
						};
/***************定义信号机数据库包含的所有表的字段名字符数组*******************/


/***************定义信号机数据库包含的所有表的创建语法字符*******************/
char* Table_Script[] = {
(char*)"create table Tbl_System(\n\
usDeviceId int not null,\n\
ucSynchTime tinyint not null default(0),\n\
usSynchFlag int not null default(0),\n\
lZone int not null default(0),\n\
ucDetDataSeqNo tinyint not null default(1),\n\
ucDetDataCycle tinyint not null default(180),\n\
ucDetPulseSeqNo tinyint not null default(1),\n\
ucDetPulseCycle tinyint not null default(30),\n\
ucFlsTimeWhenStart tinyint not null default(12),\n\
ucAllRedTimeWhenStart tinyint not null default(6),\n\
ucRemoteCtrlFlag tinyint not null default(0),\n\
ucFlashFreq tinyint not null default(1),\n\
ulBrtCtrlBgnTime bigint not null default(0),\n\
ulBrtCtrlEndTime bigint not null default(0),\n\
ucGlobalCycle tinyint not null default(0),\n\
ucCoorPhaseOffset tinyint not null default(0),\n\
ucDegradeMode tinyint not null default(0),\n\
sDegradePattern varchar(256) not null default(' '),\n\
ucCtrlMasterOptFlag tinyint not null default(0),\n\
ucDownloadFlag tinyint not null default(0),\n\
usBaseAddr int not null default(0),\n\
ucSigDevCount tinyint not null default(0),\n\
ucEypDevSerial  varchar(256) not null default(''))",
/*************************************************/
(char*)"create table Tbl_Constant(\n\
ucMaxModule tinyint not null default(16),\n\
ucMaxPlan tinyint not null default(40),\n\
ucMaxSchedule tinyint not null default(16),\n\
ucMaxScheduleEvt tinyint not null default(48),\n\
ucMaxEventType tinyint not null default(255),\n\
ucMaxEventLog tinyint not null default(255),\n\
ucMaxDetector tinyint not null default(48),\n\
ucMaxPhase tinyint not null default(16),\n\
ucMaxChannel tinyint not null default(16),\n\
ucMaxPattern tinyint not null default(32),\n\
ucMaxStagePattern tinyint not null default(16),\n\
ucMaxStage tinyint not null default(16),\n\
ucMaxOverlapPhase tinyint not null default(8))",
/******************************************************/
(char*)"create table Tbl_Module(\n\
ucModuleId tinyint primary key,\n\
sDevNode varchar(256) not null,\n\
sCompany varchar(256) not null,\n\
sModel   varchar(256) not null,\n\
sVersion varchar(256) not null,\n\
ucType   tinyint not null default(0))",
/*********************************/
(char*)"create table Tbl_Plan(\n\
Id tinyint  primary key,\n\
usMonthFlag int not null default(0),\n\
ucWeekFlag  tinyint not null default(0),\n\
ulDayFlag   bigint not null default(0),\n\
ucScheduleId tinyint not null default(0))",
/***************************************/
(char*)"create table Tbl_Schedule(\n\
ucScheduleId tinyint,\n\
ucEvtId tinyint,\n\
ucBgnHour tinyint not null default(0),\n\
ucBgnMinute tinyint not null default(0),\n\
ucCtrlMode tinyint not null default(0),\n\
ucPatternNo tinyint not null default(0),\n\
ucAuxOut tinyint not null default(0),\n\
ucSpecialOut tinyint not null default(0),\n\
CONSTRAINT Schedule_Key PRIMARY KEY (ucScheduleId, ucEvtId))",
/*****************************************************/
(char*)"create table Tbl_Phase(\n\
 ucPhaseId tinyint primary key,\n\
 ucPedestrianGreen tinyint not null default(10),\n\
 ucPedestrianClear tinyint not null default(3),\n\
 ucMinGreen tinyint not null default(15),\n\
 ucGreenDelayUnit tinyint not null default(3),\n\
 ucMaxGreen1 tinyint not null default(45),\n\
 ucMaxGreen2 tinyint not null default(60),\n\
 ucFixGreen tinyint not null default(20),\n\
 ucGreenFlash tinyint not null default(3),\n\
 ucPhaseTypeFlag tinyint not null default(0),\n\
 ucPhaseOption tinyint not null default(0),\n\
 ucExtend tinyint not null default(0))",
 /**********************************************/
(char*)"create table Tbl_Collision(\n\
ucPhaseId tinyint primary key,\n\
usCollisonFlag smallint not null default(0))",
/***********************************************/
(char*)"create table Tbl_Detector(\n\
ucDetectorId tinyint primary key,\n\
ucPhaseId tinyint not null default(0),\n\
ucDetFlag tinyint not null default(0),\n\
ucDirect tinyint not null default(0),\n\
ucValidTime tinyint not null default(0),\n\
ucOptionFlag tinyint not null default(0),\n\
usSaturationFlow int not null default(0),\n\
ucSaturationOccupy tinyint not null default(0))",
/***************************************************/
(char*)"create table Tbl_Channel(\n\
ucChannelId tinyint primary key\n,\
ucCtrlSrc tinyint not null default(0),\n\
ucAutoFlsCtrlFlag tinyint not null default(0),\n\
ucCtrlType tinyint not null default(0))",
/********************************************************/
(char*)"create table Tbl_Pattern(\n\
ucPatternId tinyint primary key,\n\
ucCycleTime tinyint not null default(0),\n\
ucOffset tinyint not null default(0),\n\
ucCoorPhase tinyint not null default(0),\n\
ucStagePatternId tinyint not null default(0))",
/*************************************************************/
(char*)"create table Tbl_StagePattern(\n\
ucStagePatternId tinyint,\n\
ucStageNo tinyint,\n\
usAllowPhase int not null default(0),\n\
ucGreenTime tinyint not null default(0),\n\
ucYellowTime tinyint not null default(0),\n\
ucRedTime tinyint not null default(0),\n\
ucOption tinyint not null default(0),\n\
CONSTRAINT StagePattern_Key PRIMARY KEY (ucStagePatternId, ucStageNo))",
/*********************************************************************/
(char*)"create table Tbl_OverlapPhase(\n\
ucOverlapPhaseId tinyint primary key,\n\
ucOperateType tinyint not null default(0),\n\
ucIncldPhaseCount tinyint not null default(0),\n\
sIncldPhase varchar(256) not null,\n\
ucAdjustPhaseCount tinyint not null default(0),\n\
sAdjustPhase varchar(256) not null,\n\
ucTailGreen tinyint not null default(0),\n\
ucTailYellow tinyint not null default(0),\n\
ucTailRed tinyint not null default(0))",
/*******************************************************/
(char*)"create table Tbl_EventType(\n\
ucEvtTypeId tinyint primary key,\n\
ulClearTime bigint not null default(0),\n\
sEvtDesc varchar(256) not null,ucLogCount tinyint not null  default(0))",
/*********************************************************/

(char*)"create table Tbl_EventLog(\n\
ucEventId bigint not null,\n\
ucEvtType tinyint not null default(0),\n\
ulHappenTime bigint not null default(0),\n\
ulEvtValue bigint not null default(0),\n\
ulEventTime varchar(20))",

/*******************************************************/
(char*)"create table Tbl_VehicleStat(\n\
						  ulId bigint primary key,\n\
						  ucDetId tinyint not null default(0),\n\
						  ulCarTotal bigint not null default(0),\n\
						  ucOccupancy tinyint not null default(0),\n\
						  ulAddTime bigint not null default(0))",

/*******************************************************/
(char*)"create table Tbl_CntDownDev(\n\
						  ucDevId tinyint not null,\n\
						  usPhase int not null default(0),\n\
						  ucOverlapPhase tinyint not null default(0),\n\
						  ucMode tinyint not null default(0))",

/******************************************************/
(char*)"create table Tbl_SpecFun(\n\
						ucFunType tinyint not null,\n\
						ucValue tinyint not null default(0))",

/******************************************************/
(char*)"create table Tbl_DetExtend(\n\
						ucId tinyint primary key,\n\
						ucSensi tinyint not null default(0),\n\
						ucGrpNo tinyint not null default(0),\n\
						ucPro tinyint not null default(0),\n\
						ucOcuDefault tinyint not null default(0),\n\
						usCarFlow smallint not null default(0),\n\
						ucFrency  tinyint not null default(0),\n\
						ucGrpData smallint not null default(0),\n\
						ucGrpDistns smallint not null default(0))",

/******************************************************/
(char*)"create table Tbl_StageFactTime(\n\
						ulAddTime int primary key,\n\
						ucStageCnt tinyint not null default(0),\n\
						sStageGreenTime varchar(32) not null)",

/******************************************************/
(char*)"create table Tbl_AdaptStageTime(\n\
						ucId INTEGER primary key,\n\
						ucWeekType tinyint not null default(0),\n\
						ucHour tinyint not null default(0),\n\
						ucMin tinyint not null default(0),\n\
						ucStageCnt tinyint not null default(0),\n\
						sStageTime varchar(32) not null)",

/******************************************************/
(char*)"create table Tbl_PhaseToDirec(\n\
						ucId tinyint primary key,\n\
						ucPhase tinyint not null default(0),\n\
						ucOverlapPhase tinyint not null default(0),\n\
						ucRoadCnt tinyint not null default(0))",

/******************************************************/
(char*)"create table Tbl_AdaptPara(\n\
						ucType tinyint not null,\n\
						ucFirstPro tinyint not null default(0),\n\
						ucSecPro tinyint not null default(0),\n\
						ucThirdPro tinyint not null default(0),\n\
						ucOcuPro tinyint not null default(0),\n\
						ucCarFlowPro tinyint not null default(0),\n\
						ucSmoothPara tinyint not null default(0))",
/******************************************************/ //ADD:20130801 09 20

(char *) "create table  Tbl_LampCheck( ucId tinyint primary key, ucFlag tinyint not null default(0))"
 }; 
 /***************定义信号机数据库包含的所有表的创建语法字符*******************/
 
 /***************定义信号机数据库包含的所有表结构描述数组*********************/
TableDesc Table_Desc[] = {{TBL_SYSTEM,0,Table_Name[TBL_SYSTEM],NULL},
                          {TBL_CONSTANT, 0, Table_Name[TBL_CONSTANT], NULL},
                          {TBL_MODULE,6,Table_Name[TBL_MODULE],Tbl_Module},
                          {TBL_PLAN,5,Table_Name[TBL_PLAN],Tbl_Plan}, 
                          {TBL_SCHEDULE,8,Table_Name[TBL_SCHEDULE],Tbl_Schedule},
                          {TBL_PHASE,12,Table_Name[TBL_PHASE],Tbl_Phase},
                          {TBL_COLLISION,2,Table_Name[TBL_COLLISION],Tbl_Collision},
                          {TBL_DETECTOR,8,Table_Name[TBL_DETECTOR],Tbl_Detector},
                          {TBL_CHANNEL,4,Table_Name[TBL_CHANNEL],Tbl_Channel},
                          {TBL_PATTERN,5,Table_Name[TBL_PATTERN],Tbl_Pattern},
                          {TBL_STAGEPATTERN,7,Table_Name[TBL_STAGEPATTERN],Tbl_StagePattern},
                          {TBL_OVERLAPPHASE,9,Table_Name[TBL_OVERLAPPHASE],TBL_OverlapPhase},
                          {TBL_EVENTTYPE,3,Table_Name[TBL_EVENTTYPE],Tbl_EventType},
                          {TBL_EVENTLOG,5,Table_Name[TBL_EVENTLOG],Tbl_EventLog},
						  {TBL_VEHICLESTAT,5,Table_Name[TBL_VEHICLESTAT],Tbl_VehicleStat},
						  {TBL_CNTDOWNDEV,4,Table_Name[TBL_CNTDOWNDEV],Tbl_CntDownDev},
						  {TBL_SPECFUN,2,Table_Name[TBL_SPECFUN],Tbl_SpecFun},
						  {TBL_DET_EXTEND,9,Table_Name[TBL_DET_EXTEND],Tbl_DetExtend},
						  {TBL_STAGE_FACTTIME,3,Table_Name[TBL_STAGE_FACTTIME],Tbl_StageFactTime},
						  {TBL_ADAPT_STAGETIME,6,Table_Name[TBL_ADAPT_STAGETIME],Tbl_AdaptStageTime},
						  {TBL_PHASE2DIREC,4,Table_Name[TBL_PHASE2DIREC],Tbl_PhaseToDirec},
						  {TBL_ADAPT_PARA,7,Table_Name[TBL_ADAPT_PARA],Tbl_AdaptPara},
						  {TBL_LAMP_CHECK,2,Table_Name[TBL_LAMP_CHECK],Tbl_LampCheck}
                         };

TableDesc* m_gTableDesc = Table_Desc;
/***************定义信号机数据库包含的所有表结构描述数组*********************/

/****************定义GBT表数据交换函数指针，数据表函数******************/
typedef int (* EXCHANGEFUN)(int iOprType, Byte uIdxFst, Byte uIdxSnd, Byte uSubId, Byte* pData, int uDataSize, Byte& uErrorSts, Byte& uErrorIdx);

typedef struct
{
    Byte        ucObjId;
    EXCHANGEFUN  pExchgFun;
}ExchangeFunGrp;
ExchangeFunGrp gGroupExchange[] = 
{
    {0x81, ExchangeDeviceId},
    {0x82, ExchangeMaxModule},
    {0x83, ExchangeSynchTime},
    {0x84, ExchangeSynchFlag},
    {0x85, ExchangeModule},
    {0x87, ExchangeZone},
    {0x89, ExchangeMaxPlan},
    {0x8A, ExchangeMaxSchedule},
    {0x8B, ExchangeMaxSegment},
    {0x8D, ExchangePlan},
    {0x8E, ExchangeSchedule},
    {0x8F, ExchangeMaxEvtType},
    {0x90, ExchangeMaxEvtLog},
    {0x91, ExchangeEventType},
    {0x92, ExchangeEventLog},
    {0x93, ExchangeMaxPhase},
    {0x94, ExchangeMaxPhaseGrp},
    {0x95, ExchangePhase},
    {0x97, ExchangeCollision},
    {0x98, ExchangeMaxDetector},
    {0x99, ExchangeMaxDetGrp},
    {0x9A, ExchangeDetDataSeqNo},
    {0x9B, ExchangeDetDataCycle},
    {0x9C, ExchangeActiveDetCount},
    {0x9D, ExchangePulseSeqNo},
    {0x9E, ExchangePulseCycle},
    {0x9F, ExchangeDetector},
    {0xA3, ExchangeFlsTimeWhenStart},
    {0xA4, ExchangeAllRedTimeWhenStart},
    {0xAA, ExchangeRemoteCtrlFlag},
    {0xAB, ExchangeFlashFreq},
    {0xAC, ExchangeBrtCtrlBgnTime},
    {0xAD, ExchangeBrtCtrlEndTime},
    {0xAE, ExchangeMaxChannel},
    {0xAF, ExchangeMaxChanlGrp},
    {0xB0, ExchangeChannel},
    {0xB2, ExchangeMaxPattern},
    {0xB3, ExchangeMaxStagePattern},
    {0xB4, ExchangeMaxStage},
    {0xB8, ExchangeGlobalCycle},
    {0xB9, ExchangeCoorPhaseOffset},
    {0xBC, ExchangeDegradeMode},
    {0xBD, ExchangeDegradePattern},
    {0xC0, ExchangePattern},
    {0xC1, ExchangeStagePattern},
    {0xC2, ExchangeDownloadFlag},
    {0xC3, ExchangeCtrlMasterOptFlag},
    {0xC4, ExchangeBaseAddr},
    {0xC5, ExchangeSigDevCount},
    {0xC6, ExchangeMaxOverlapPhase},
    {0xC7, ExchangeMaxOverlapPhaseGrp},
    {0xC8, ExchangeOverlapPhase},
	//以下为扩展对象
	{0xf1, ExchangeCntDownDev},    //倒计时设备表
	{0xfa, ExchangePhaseToDirec},  //相位与方向对应表
	{0xfb, ExchangeAdaptPara},     //自适应参数值
	{0xfc, ExchangeDetExtend},     //检测器扩展表
	{0xfd, ExchangeAdaptStage},     //自适应自学习生成的阶段时间
    {0xff, ExchangeChannelChk}      //通道灯泡检测配置
};
/****************定义GBT表数据交换函数指针，数据表函数******************/


#define GET_CONST_CONST(FIELD, VALUE)\
{\
    Ulong   ulMaxTemp;\
    GetFieldData("Tbl_Constant", FIELD, ulMaxTemp);\
    VALUE = (Byte)ulMaxTemp;\
}

#define CHECK_ID(KIND, ID, RETURN)\
{\
    Ulong  ulMax;\
    GetFieldData("Tbl_Constant", KIND, ulMax);\
    if (0 == ID || ID > ulMax)\
    {\
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tInvalid %s Id(%d)\n", SHORT_FILE, __LINE__,KIND, ID));\
        return RETURN;\
    }\
}


/**************************************************************
Function:       GetFieldsValue
Description:    从字符串中分离出需要到字符			
Input:          sConstant  -- 待分离到字符长常量  
				uSplit   --   分离字符    
				sValue   --   结果保存指针     
				uValSize  --  分离对比次数   
Output:         无
Return:         分离得到字节数
***************************************************************/
Byte GetFieldsValue(const char* sConstant, const char uSplit, Byte* sValue, Byte uValSize)
{
    Byte uCount    = 0;
    Byte uVal      = 0;
    size_t uPos     = 0;
    size_t uLength  = ACE_OS::strlen(sConstant);

    while(uPos < uLength && uCount < uValSize)
    {
        if (uSplit == sConstant[uPos])
        {
            uPos += 1;
            sValue[uCount++] = uVal;
            uVal = 0;
            continue;
        }
        else if (sConstant[uPos] >= '0' && sConstant[uPos] <= '9')
        {
            uVal = uVal*10 + (sConstant[uPos] - '0');
			uPos += 1;
            continue;
        }
        else
        {
            break;
        }
    }

    return uCount;
}

/**************************************************************
Function:       ExchangeData
Description:    数据库于上位机数据交换分离处理			
Input:          iOprType -- 数据处理类型，查询或者设置
				uObjId  -- 数据库gbt对象标志  
				uIdxFst   --  头索引值  
				uIdxSnd   --  次索引值     
				uSubId  --    子对象值  
				pData   --     交换值指针 
				uDataSize --  数据大小
Output:         uErrorSts -- 数据处理错误状态
				uErrorIdx -- 数据处理错误索引
Return:         -1 - 函数处理错误  
***************************************************************/
int ExchangeData(int iOprType,
                Byte uObjId, 
                Byte uIdxFst, 
                Byte uIdxSnd, 
                Byte uSubId, 
                Byte* pData, 
                int uDataSize,
                Byte& uErrorSts,
                Byte& uErrorIdx)
{
    int i, iGrpSize = sizeof(gGroupExchange) / sizeof(ExchangeFunGrp);
    for (i = 0; i < iGrpSize; i++)
    {
        if (gGroupExchange[i].ucObjId == uObjId)
        {
            return gGroupExchange[i].pExchgFun(iOprType, uIdxFst, uIdxSnd, uSubId, pData, uDataSize, uErrorSts, uErrorIdx);
        }
    }
  
    uErrorIdx = 0;
    uErrorSts = 3 ; //GBT_MSG_OBJ_ERROR;
    return -1;
}

/**************************************************************
Function:       CSqliteRst::CSqliteRst
Description:    CSqliteRst数据库结果集类构造函数				
Input:          无              
Output:         无
Return:         无
***************************************************************/
CSqliteRst::CSqliteRst() : m_ppResult(NULL) , m_iRow(0) , m_iColum(0)
{
	;
}

/**************************************************************
Function:       CSqliteRst::~CSqliteRst
Description:    CSqliteRst类	析构函数	
Input:          无              
Output:         无
Return:         无
***************************************************************/
CSqliteRst::~CSqliteRst()
{
    Clear();
}


/**************************************************************
Function:       CSqliteRst::Clear
Description:    释放数据库表数据集	
Input:          无              
Output:         无
Return:         无
***************************************************************/
void CSqliteRst::Clear()
{
    if (NULL != m_ppResult)
    {
        ::sqlite3_free_table(m_ppResult);
        m_ppResult = NULL;
    }
    m_iRow   = 0;
    m_iColum = 0;
}

/**************************************************************
Function:       CGbtTscDb::CGbtTscDb
Description:    CGbtTscDb信号机数据库处理类构造函数	
Input:          无              
Output:         无
Return:         无
***************************************************************/
CGbtTscDb::CGbtTscDb() : m_pSqlite(NULL)
{
	ACE_DEBUG((LM_DEBUG,"%s:%d Init GbtDb object ok !\n",__FILE__,__LINE__));
}

/**************************************************************
Function:       CGbtTscDb::~CGbtTscDb
Description:    CGbtTscDb类	析构函数	
Input:          无              
Output:         无
Return:         无
***************************************************************/
CGbtTscDb::~CGbtTscDb()
{
    CloseDb();
	ACE_DEBUG((LM_DEBUG,"%s:%d Destruct GbtDb object ok !\n",__FILE__,__LINE__));
}

/**************************************************************
Function:       CGbtTscDb::InitDb
Description:    信号机数据库初始化
Input:          sDbPath - 数据库文件存放路径              
Output:         无
Return:         true - 初始化成功  false -初始化失败
***************************************************************/
bool CGbtTscDb::InitDb(const char* sDbPath)
{   
    if (NULL != m_pSqlite)
    {
        ::sqlite3_close(m_pSqlite);
    }
    m_pSqlite = NULL;
	ACE_DEBUG ((LM_DEBUG, "%s:%04d\t Begin init DB!\n", SHORT_FILE, __LINE__));
    if (SQLITE_OK != ::sqlite3_open(sDbPath, &m_pSqlite))
    {      
		return false;
    }
	
    for (int i = 0; i < TABLE_COUNT; i++)        //初始化数据库表
    {
        if (!IsTableExist(Table_Desc[i].sTblName))
        {
            if (!ExecuteCmd(Table_Script[i]))
            {
                ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to create %s\n", SHORT_FILE, __LINE__, Table_Desc[i].sTblName));
            }
        }
    }
    AddDefault();     //添加部分数据库表默认值
    m_gTscDb = this;
	return true;
}


/**************************************************************
Function:       CGbtTscDb::CloseDb
Description:    关闭信号机数据库
Input:          无            
Output:         无
Return:         无
***************************************************************/
void CGbtTscDb::CloseDb()
{
    ACE_Guard<ACE_Thread_Mutex>  guard(m_mutexDb);

    if (NULL != m_pSqlite)
    {
        ::sqlite3_close(m_pSqlite);
    }
    m_pSqlite = NULL;
}

/**************************************************************
Function:       CGbtTscDb::IsTableExist
Description:    判断数据库表数据是否存在
Input:          sTable  -- 表名称            
Output:         无
Return:         true - 表存在 false -表不存在或者无数据记录
***************************************************************/
bool CGbtTscDb::IsTableExist(const char* sTable)
{
    char  sSql[SMALL_BUF_SIZE];
    CSqliteRst  qryRst;
	
    ACE_OS::sprintf(sSql, "select * from sqlite_master where type='table' and name='%s'", sTable);
    if (!Query(sSql, qryRst))
    {
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
        return false;
    }
    if (qryRst.m_iRow <= 0)
    {
        return false;
    }
    return true;
}


/**************************************************************
Function:       CGbtTscDb::Query
Description:    数据表查询
Input:          sSql  -- 数据查询语句            
Output:         sRst  -- 数据查询结果保存对象
Return:         true - 查询成功 false -查询失败
***************************************************************/
bool CGbtTscDb::Query(const char* sSql, CSqliteRst& sRst)
{
    char* sErrMsg = NULL;
    ACE_Guard<ACE_Thread_Mutex>  guard(m_mutexDb);

    if (NULL == m_pSqlite)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\t m_pSqlite== NULL\n", SHORT_FILE, __LINE__));
		return false;
    }
    sRst.Clear();

    if (SQLITE_OK != ::sqlite3_get_table(m_pSqlite, sSql, &(sRst.m_ppResult), &sRst.m_iRow, &sRst.m_iColum, &sErrMsg))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFiled to sqlite3_get_table\n sql = %s\n", SHORT_FILE, __LINE__, sSql));
        if (NULL != sErrMsg)
        {
			ACE_DEBUG ((LM_DEBUG, "ErrMsg = %s\n", sErrMsg));
			::sqlite3_free(sErrMsg);
        }
        return false;
    }

    return true;
}


/**************************************************************
Function:       CGbtTscDb::ExecuteCmd
Description:    执行数据库sql语句
Input:          sSql  -- 数据查询语句            
Output:         无
Return:         true - 语句执行成功 false -语句执行失败
***************************************************************/
bool CGbtTscDb::ExecuteCmd(const char *sSql)
{
    char* sErrMsg = NULL;
    ACE_Guard<ACE_Thread_Mutex>  guard(m_mutexDb);

    if (SQLITE_OK != ::sqlite3_exec(m_pSqlite, sSql, NULL, NULL, &sErrMsg))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to sqlite3_exec\n sql = %s\n", SHORT_FILE, __LINE__, sSql));
        if (NULL != sErrMsg)
        {
            ACE_DEBUG ((LM_DEBUG, "ErrMsg = %s\n", sErrMsg));
			::sqlite3_free(sErrMsg);
        }
        return false;
    }

	if (NULL != sErrMsg)
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFree sErrMsg\n", SHORT_FILE, __LINE__));
		::sqlite3_free(sErrMsg);
	}

    return true;
}

/**************************************************************
Function:       CGbtTscDb::QueryPlan
Description:    查询计划方案表，并保存到结构体中
Input:          无            
Output:         tblPlan  -- 计划方案类对象
Return:         true - 查询成功 false -查询失败
***************************************************************/
bool CGbtTscDb::QueryPlan(TblPlan& tblPlan)
{
    int    i;
	char*  pStop;
    Byte uCount;
    char   sSql[SMALL_BUF_SIZE];
    CSqliteRst  qryRst;
    Plan*   pData = NULL;

    ACE_OS::sprintf(sSql, "select * from %s order by %s", 
               				 Table_Desc[TBL_PLAN].sTblName, 
               				 Table_Desc[TBL_PLAN].sFieldName[0]);

    if (!Query(sSql, qryRst))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
        return false;
    }

    if (qryRst.m_iColum != Table_Desc[TBL_PLAN].iFieldCount)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= %d\n", SHORT_FILE, __LINE__, Table_Desc[TBL_PLAN].sTblName, qryRst.m_iColum,Table_Desc[TBL_PLAN].iFieldCount));
        return false;
    }

    if (qryRst.m_iRow <= 0)
    {
        return false;
    }
    
    uCount = (Byte)qryRst.m_iRow;
    if (!tblPlan.AllocData(uCount))
    {
        return false;
    }

    pData = tblPlan.GetData(uCount);
    for (i = 0; i < qryRst.m_iRow; i++)
    { 
        pData[i].ucId           = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum]);
        pData[i].usMonthFlag    = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 1]);
        pData[i].ucWeekFlag     = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 2]);
		pData[i].ulDayFlag      = ACE_OS::strtoul(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 3], &pStop, 10);
        pData[i].ucScheduleId   = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 4]);    
    }
    
    return true;
}

/**************************************************************
Function:       CGbtTscDb::QueryPlan
Description:    根据计划IDH号查询计划方案表，并保存到结构体中
Input:          uPlanId  -- 计划方案号            
Output:         sPlan  --   保存计划方案结构体
Return:         true - 查询成功 false -查询失败
***************************************************************/
bool CGbtTscDb::QueryPlan(Byte uPlanId, Plan& sPlan)
{
    char   sSql[SMALL_BUF_SIZE];
    CSqliteRst  qryRst;

    ACE_OS::sprintf(sSql, "select * from %s where %s=%u", 
                			Table_Desc[TBL_PLAN].sTblName, 
                			Table_Desc[TBL_PLAN].sFieldName[0], uPlanId);

    if (!Query(sSql, qryRst))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
        return false;
    }

    if (qryRst.m_iColum != Table_Desc[TBL_PLAN].iFieldCount)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= %d\n", SHORT_FILE, __LINE__, Table_Desc[TBL_PLAN].sTblName,  	 qryRst.m_iColum,Table_Desc[TBL_PLAN].iFieldCount));
        return false;
    }

    if (qryRst.m_iRow <= 0)
    {
        return false;
    }
 
    sPlan.ucId           = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum]);
    sPlan.usMonthFlag    = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 1]);
    sPlan.ucWeekFlag     = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 2]);
    sPlan.ulDayFlag      = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 3]);
    sPlan.ucScheduleId   = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 4]);    
    return true;
}


/**************************************************************
Function:       CGbtTscDb::AddPlan
Description:    添加调度计划表
Input:          uPlanId  -- 计划方案号  
				sPlan  --   待添加方案结构体          
Output:         无
Return:         true - 添加成功 false -添加失败
***************************************************************/
bool CGbtTscDb::AddPlan(Byte ucPlanId, Plan& sPlan)   
{
    char   sSql[LARGE_BUF_SIZE];
    
    if (!IsPlanValid(ucPlanId, sPlan))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tInvalid Plan\n", SHORT_FILE, __LINE__));
        return false;
    }

    ACE_OS::sprintf(sSql, "insert into %s(%s,%s,%s,%s,%s)values(%u,%u,%u,%lu,%u)",
                   			Table_Desc[TBL_PLAN].sTblName,
                  			Table_Desc[TBL_PLAN].sFieldName[0], 
                  			Table_Desc[TBL_PLAN].sFieldName[1], 
                   			Table_Desc[TBL_PLAN].sFieldName[2], 
                   			Table_Desc[TBL_PLAN].sFieldName[3], 
                  			Table_Desc[TBL_PLAN].sFieldName[4], 
                   			ucPlanId, 
                  			sPlan.usMonthFlag, 
                   			sPlan.ucWeekFlag, 
                   			sPlan.ulDayFlag, 
                   			sPlan.ucScheduleId);

    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }

    return true;
}


/**************************************************************
Function:       CGbtTscDb::DelPlan
Description:    删除时基调度表
Input:          uPlanId  -- 计划方案号      
Output:         无
Return:         true - 删除成功 false -删除失败
***************************************************************/
bool CGbtTscDb::DelPlan(Byte ucPlanId)
{
    char   sSql[SMALL_BUF_SIZE];

    ACE_OS::sprintf(sSql, "delete from %s where %s=%u",
                  		 Table_Desc[TBL_PLAN].sTblName,
                  		 Table_Desc[TBL_PLAN].sFieldName[0], ucPlanId);

    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }

    return true;
}


/**************************************************************
Function:       CGbtTscDb::DelPlan
Description:    删除全部时基调度表
Input:          无     
Output:         无
Return:         true - 删除成功 false -删除失败
***************************************************************/
bool CGbtTscDb::DelPlan()
{
	char   sSql[SMALL_BUF_SIZE];

	ACE_OS::sprintf(sSql, "delete from %s",	Table_Desc[TBL_PLAN].sTblName);

	if (!ExecuteCmd(sSql))
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
		return false;
	}

	return true;
}

/**************************************************************
Function:       CGbtTscDb::ModPlan
Description:    修改时基调度表记录，若不存在则添加
Input:          uPlanId  -- 计划方案号  
				sPlan  --   待修改方案结构体        
Output:         无
Return:         true - 修改成功 false -修改失败
***************************************************************/
bool CGbtTscDb::ModPlan(Byte ucPlanId, Plan& sPlan)
{
	Plan sTemp;
	if (!QueryPlan(ucPlanId, sTemp))
	{
		return AddPlan(ucPlanId, sPlan);
	}

	if (!IsPlanValid(ucPlanId, sPlan))
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tInvalid Plan\n", SHORT_FILE, __LINE__));
		return false;
	}

	char   sSql[LARGE_BUF_SIZE];
	ACE_OS::sprintf(sSql, "update %s set %s=%u, %s=%u, %s=%lu, %s=%u where %s=%u",
							Table_Desc[TBL_PLAN].sTblName,
							Table_Desc[TBL_PLAN].sFieldName[1], sPlan.usMonthFlag,
							Table_Desc[TBL_PLAN].sFieldName[2], sPlan.ucWeekFlag,
							Table_Desc[TBL_PLAN].sFieldName[3], sPlan.ulDayFlag,
							Table_Desc[TBL_PLAN].sFieldName[4], sPlan.ucScheduleId,
							Table_Desc[TBL_PLAN].sFieldName[0], ucPlanId);

	if (!ExecuteCmd(sSql))
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
		return false;
	}

	return true;
}


/**************************************************************
Function:       CGbtTscDb::IsPlanValid
Description:    判断方案号是否合法
Input:          uPlanId  -- 计划方案号  
				sPlan  --   方案结构体        
Output:         无
Return:         true - 合法 false -不合法
***************************************************************/
bool CGbtTscDb::IsPlanValid(Byte ucPlanId, Plan& sPlan)
{
    CHECK_ID(CONST_PLAN, ucPlanId, false);
    return true;
}


/**************************************************************
Function:       CGbtTscDb::QuerySchedule
Description:    查询时段表，并保存查询结果
Input:          tblSchedule -- 时段表对象   
Output:         无
Return:         true - 查询成功 false -查询失败
***************************************************************/
bool CGbtTscDb::QuerySchedule(TblSchedule& tblSchedule)
{
    int    i;
    Ushort uCount;
    char   sSql[SMALL_BUF_SIZE];
    CSqliteRst  qryRst;
    Schedule*   pData = NULL;

    ACE_OS::sprintf(sSql, "select * from %s order by %s,%s", 
                			Table_Desc[TBL_SCHEDULE].sTblName, 
                			Table_Desc[TBL_SCHEDULE].sFieldName[0], 
                			Table_Desc[TBL_SCHEDULE].sFieldName[1]);
    if (!Query(sSql, qryRst))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
        return false;
    }

    if (qryRst.m_iColum != Table_Desc[TBL_SCHEDULE].iFieldCount)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= %d\n", SHORT_FILE, __LINE__, Table_Desc[TBL_SCHEDULE].sTblName, qryRst.m_iColum,Table_Desc[TBL_SCHEDULE].iFieldCount));
        return false;
    }

    if (qryRst.m_iRow <= 0)
    {
        return false;
    }
    
    uCount = (Ushort)qryRst.m_iRow;
    if (!tblSchedule.AllocData(uCount))
    {
        return false;
    }

    pData = tblSchedule.GetData(uCount);
    for (i = 0; i < qryRst.m_iRow; i++)
    { 
        pData[i].ucScheduleId = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum]);
        pData[i].ucEvtId      = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 1]);
        pData[i].ucBgnHour    = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 2]);
        pData[i].ucBgnMinute  = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 3]);
        pData[i].ucCtrlMode   = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 4]);
        pData[i].ucPatternNo  = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 5]);
        pData[i].ucAuxOut     = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 6]);
        pData[i].ucSpecialOut = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 7]);
	}
    
    return true;
}


/**************************************************************
Function:       CGbtTscDb::QuerySchedule
Description:    根据时段号查询时段表，并保存查询结果
Input:          uScheduleId -- 时段表号
				tblSchedule -- 时段表对象   
Output:         无
Return:         true - 查询成功 false -查询失败
***************************************************************/
bool CGbtTscDb::QuerySchedule(Byte uScheduleId, TblSchedule& tblSchedule)
{
    int    i;
    Ushort uCount;
    char   sSql[SMALL_BUF_SIZE];
    CSqliteRst  qryRst;
    Schedule*   pData = NULL;

    ACE_OS::sprintf(sSql, "select * from %s where %s=%u order by %s , %s", 
               			 Table_Desc[TBL_SCHEDULE].sTblName, 
               			 Table_Desc[TBL_SCHEDULE].sFieldName[0],
               			 uScheduleId,
               			 Table_Desc[TBL_SCHEDULE].sFieldName[2],
						 Table_Desc[TBL_SCHEDULE].sFieldName[3]);

    if (!Query(sSql, qryRst))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
        return false;
    }

    if (qryRst.m_iColum != Table_Desc[TBL_SCHEDULE].iFieldCount)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= %d\n", SHORT_FILE, __LINE__, Table_Desc[TBL_SCHEDULE].sTblName, qryRst.m_iColum,Table_Desc[TBL_SCHEDULE].iFieldCount));
        return false;
    }

    if (qryRst.m_iRow <= 0)
    {
        return false;
    }
    
    uCount = (Ushort)qryRst.m_iRow;
    if (!tblSchedule.AllocData(uCount))
    {
        return false;
    }

    pData = tblSchedule.GetData(uCount);
    for (i = 0; i < qryRst.m_iRow; i++)
    { 
        pData[i].ucScheduleId = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum]);
        pData[i].ucEvtId      = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 1]);
        pData[i].ucBgnHour    = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 2]);
        pData[i].ucBgnMinute  = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 3]);
        pData[i].ucCtrlMode   = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 4]);
        pData[i].ucPatternNo  = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 5]);
        pData[i].ucAuxOut     = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 6]);
        pData[i].ucSpecialOut = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 7]);
    }
    
    return true;
}


/**************************************************************
Function:       CGbtTscDb::QuerySchedule
Description:    根据时段号和事件号查询时段表，并保存查询结果
Input:          uScheduleId -- 时段表号
				uEvtId      -- 事件号
				sSchedule -- 时段表记录结构体   
Output:         无
Return:         true - 查询成功 false -查询失败
***************************************************************/
bool CGbtTscDb::QuerySchedule(Byte uScheduleId, Byte uEvtId, Schedule& sSchedule)
{
    char   sSql[SMALL_BUF_SIZE];
    CSqliteRst  qryRst;

    ACE_OS::sprintf(sSql, "select * from %s where %s=%u and %s=%u", 
               				 Table_Desc[TBL_SCHEDULE].sTblName, 
               				 Table_Desc[TBL_SCHEDULE].sFieldName[0],
               				 uScheduleId,
               				 Table_Desc[TBL_SCHEDULE].sFieldName[1], uEvtId);

    if (!Query(sSql, qryRst))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
        return false;
    }

    if (qryRst.m_iColum != Table_Desc[TBL_SCHEDULE].iFieldCount)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= %d\n", SHORT_FILE, __LINE__, Table_Desc[TBL_SCHEDULE].sTblName, qryRst.m_iColum,Table_Desc[TBL_SCHEDULE].iFieldCount));
        return false;
    }

    if (qryRst.m_iRow <= 0)
    {
        return false;
    }
    
    sSchedule.ucScheduleId = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum]);
    sSchedule.ucEvtId      = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 1]);
    sSchedule.ucBgnHour    = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 2]);
    sSchedule.ucBgnMinute  = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 3]);
    sSchedule.ucCtrlMode   = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 4]);
    sSchedule.ucPatternNo  = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 5]);
    sSchedule.ucAuxOut     = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 6]);
    sSchedule.ucSpecialOut = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 7]);

    return true;
}


/**************************************************************
Function:       CGbtTscDb::AddSchedule
Description:    添加时段表
Input:          uScheduleId -- 时段表号
				uEvtId      -- 事件号
				sSchedule --   时段表记录结构体   
Output:         无
Return:         true - 添加成功 false -添加失败
***************************************************************/
bool CGbtTscDb::AddSchedule(Byte uScheduleId, Byte uEvtId, Schedule& sSchedule)
{
    char   sSql[SMALL_BUF_SIZE];

    if (!IsScheduleValid(uScheduleId, uEvtId, sSchedule))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tSchedule invalid\n", SHORT_FILE, __LINE__));
        return false;
    }
	if ( 0 == uEvtId ) //事件id为0不处理
	{
		return true;
	}

    ACE_OS::sprintf(sSql, "insert into %s(%s,%s,%s,%s,%s,%s,%s,%s)values(%u,%u,%u,%u,%u,%u,%u,%u)",
                  		   Table_Desc[TBL_SCHEDULE].sTblName,
                  		   Table_Desc[TBL_SCHEDULE].sFieldName[0],
                  	 	   Table_Desc[TBL_SCHEDULE].sFieldName[1],
                  		   Table_Desc[TBL_SCHEDULE].sFieldName[2],
                  		   Table_Desc[TBL_SCHEDULE].sFieldName[3], 
                   		   Table_Desc[TBL_SCHEDULE].sFieldName[4], 
                  		   Table_Desc[TBL_SCHEDULE].sFieldName[5], 
                   		   Table_Desc[TBL_SCHEDULE].sFieldName[6], 
                   		   Table_Desc[TBL_SCHEDULE].sFieldName[7],
                   		   uScheduleId, uEvtId, sSchedule.ucBgnHour, 
                   		   sSchedule.ucBgnMinute, sSchedule.ucCtrlMode, 
                  	       sSchedule.ucPatternNo, sSchedule.ucAuxOut, 
                  		   sSchedule.ucSpecialOut);

    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }

    return true;
}


/**************************************************************
Function:       CGbtTscDb::ModSchedule
Description:    修改时段表
Input:          uScheduleId -- 时段表号
				uEvtId      -- 事件号
				sSchedule --   时段表记录结构体   
Output:         无
Return:         true - 修改成功 false -修改失败
***************************************************************/
bool CGbtTscDb::ModSchedule(Byte uScheduleId, Byte uEvtId, Schedule& sSchedule)
{
    Schedule  sTemp;

	if ( 0 == uEvtId ) 
	{
		return true;
	}

    if (!QuerySchedule(uScheduleId, uEvtId, sTemp))
    {
        return AddSchedule(uScheduleId, uEvtId, sSchedule);
    }

    char   sSql[SMALL_BUF_SIZE];
    if (!IsScheduleValid(uScheduleId, uEvtId, sSchedule))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tSchedule invalid\n", SHORT_FILE, __LINE__));
        return false;
    }

    ACE_OS::sprintf(sSql, "update %s set %s=%u,%s=%u,%s=%u,%s=%u,%s=%u,%s=%u where %s=%u and %s=%u",
                   			Table_Desc[TBL_SCHEDULE].sTblName,
                  		    Table_Desc[TBL_SCHEDULE].sFieldName[2], sSchedule.ucBgnHour,
                   			Table_Desc[TBL_SCHEDULE].sFieldName[3], sSchedule.ucBgnMinute,
                  			Table_Desc[TBL_SCHEDULE].sFieldName[4], sSchedule.ucCtrlMode,
                  			Table_Desc[TBL_SCHEDULE].sFieldName[5], sSchedule.ucPatternNo,
                  			Table_Desc[TBL_SCHEDULE].sFieldName[6], sSchedule.ucAuxOut,
                  			Table_Desc[TBL_SCHEDULE].sFieldName[7], sSchedule.ucSpecialOut,
                   			Table_Desc[TBL_SCHEDULE].sFieldName[0], uScheduleId,
                   			Table_Desc[TBL_SCHEDULE].sFieldName[1], uEvtId);

    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }
    
    return true;
}


/**************************************************************
Function:       CGbtTscDb::DelSchedule
Description:    删除时段表
Input:          uScheduleId -- 时段表号
				uEvtId      -- 事件号
Output:         无
Return:         true - 删除成功 false -删除失败
***************************************************************/
bool CGbtTscDb::DelSchedule(Byte uScheduleId, Byte uEvtId)
{
    char   sSql[SMALL_BUF_SIZE];

    ACE_OS::sprintf(sSql, "delete from %s where %s=%u and %s=%u",
                  		   Table_Desc[TBL_SCHEDULE].sTblName,
                   		   Table_Desc[TBL_SCHEDULE].sFieldName[0], uScheduleId,
                   		   Table_Desc[TBL_SCHEDULE].sFieldName[1], uEvtId);

    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }

    return true;
}


/**************************************************************
Function:       CGbtTscDb::DelSchedule
Description:    根据时段号删除时段表
Input:          uScheduleId -- 时段表号
Output:         无
Return:         true - 删除成功 false -删除失败
***************************************************************/
bool CGbtTscDb::DelSchedule(Byte uScheduleId)
{
    char   sSql[SMALL_BUF_SIZE];

    ACE_OS::sprintf(sSql, "delete from %s where %s=%u",
                  		   Table_Desc[TBL_SCHEDULE].sTblName,
                  		   Table_Desc[TBL_SCHEDULE].sFieldName[0], uScheduleId);

    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }
    return true;
}


/**************************************************************
Function:       CGbtTscDb::DelSchedule
Description:    删除全部时段表
Input:          无
Output:         无
Return:         true - 删除成功 false -删除失败
***************************************************************/
bool CGbtTscDb::DelSchedule()
{
    char   sSql[SMALL_BUF_SIZE];

    ACE_OS::sprintf(sSql, "delete from %s",Table_Desc[TBL_SCHEDULE].sTblName);

    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }

    return true;
}

/**************************************************************
Function:       CGbtTscDb::IsScheduleValid
Description:    判断输时段表数据是否合法
Input:          uScheduleId -- 时段表号
				uEvtId      -- 事件号
				sSchedule --   时段表记录结构体
Output:         无
Return:         true - 输入合法 false -输入不合法
***************************************************************/
bool CGbtTscDb::IsScheduleValid(Byte uScheduleId, Byte uEvtId, Schedule& sSchedule)
{
    CHECK_ID(CONST_SCHEDULE,uScheduleId, false);
    
	if ( uEvtId != 0 )
	{
		CHECK_ID(CONST_SCHEDULEEVT,uEvtId, false);
	}

    if (sSchedule.ucBgnHour >= 24)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tucBgnHour = %d, invalid\n", SHORT_FILE, __LINE__,sSchedule.ucBgnHour));
        return false;
    }

    if (sSchedule.ucBgnMinute >= 60)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tucBgnMinute = %d, invalid\n", SHORT_FILE, __LINE__,sSchedule.ucBgnMinute));
        return false;
    }

    if (sSchedule.ucCtrlMode > 13)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tucCtrlMode = %d, invalid\n", SHORT_FILE, __LINE__,sSchedule.ucCtrlMode));
        return false;
    }
    if (0 != sSchedule.ucPatternNo && 255 != sSchedule.ucPatternNo && 254 != sSchedule.ucPatternNo)
    {
        CHECK_ID(CONST_PATTERN,sSchedule.ucPatternNo, false);
    }

    return true;
}
 
/**************************************************************
Function:       CGbtTscDb::QueryModule
Description:    信号机模块查询，并保存
Input:          tblModule -- 模块类对象
Output:         无
Return:         true - 查询成功 false -查询失败
***************************************************************/
bool CGbtTscDb::QueryModule(TblModule& tblModule)
{
    int    i;
    Byte  uCount;
    char   sSql[SMALL_BUF_SIZE];
    CSqliteRst  qryRst;
    Module* pData = NULL;

    ACE_OS::sprintf(sSql, "select * from %s order by %s", 
               			   Table_Desc[TBL_MODULE].sTblName,
               			   Table_Desc[TBL_MODULE].sFieldName[0]);

    if (!Query(sSql, qryRst))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
        return false;
    }

    if (qryRst.m_iColum != Table_Desc[TBL_MODULE].iFieldCount)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= %d\n", SHORT_FILE, __LINE__, Table_Desc[TBL_MODULE].sTblName, qryRst.m_iColum,Table_Desc[TBL_MODULE].iFieldCount));
        return false;
    }

    if (qryRst.m_iRow <= 0)
    {
        return false;
    }

    uCount = (Byte)qryRst.m_iRow;
    if (!tblModule.AllocData(uCount))
    {
        return false;
    }

    pData = tblModule.GetData(uCount);
    for (i = 0; i < qryRst.m_iRow; i++)
    {
        pData[i].ucModuleId = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum]);

        if (!pData[i].strDevNode.SetString(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 1]))
        {
            return false;
        }
        
        if (!pData[i].strCompany.SetString(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 2]))
        {
            return false;
        }

        if (!pData[i].strModel.SetString(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 3]))
        {
            return false;
        }

        if (!pData[i].strVersion.SetString(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 4]))
        {
            return false;
        }

        pData[i].ucType = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 5]);
    }
    return true;
}


/**************************************************************
Function:       CGbtTscDb::QueryModule
Description:    根据信号机模块表ID查询，并保存
Input:          uModuleId -- 模块ID
				sModule   -- 模块结构体
Output:         无
Return:         true - 查询成功 false -查询失败
***************************************************************/
bool CGbtTscDb::QueryModule(Byte uModuleId, Module& sModule)
{
    char   sSql[SMALL_BUF_SIZE];
    CSqliteRst  qryRst;

    ACE_OS::sprintf(sSql, "select * from %s where %s=%u", 
               			   Table_Desc[TBL_MODULE].sTblName,
               			   Table_Desc[TBL_MODULE].sFieldName[0], uModuleId);

    if (!Query(sSql, qryRst))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
        return false;
    }

    if (qryRst.m_iColum != Table_Desc[TBL_MODULE].iFieldCount)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= %d\n", SHORT_FILE, __LINE__, Table_Desc[TBL_MODULE].sTblName, qryRst.m_iColum,Table_Desc[TBL_MODULE].iFieldCount));
        return false;
    }

    if (qryRst.m_iRow <= 0)
    {
        return false;
    }

    sModule.ucModuleId = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum]);

    if (!sModule.strDevNode.SetString(qryRst.m_ppResult[qryRst.m_iColum + 1]))
    {
        return false;
    }
    
    if (!sModule.strCompany.SetString(qryRst.m_ppResult[qryRst.m_iColum + 2]))
    {
        return false;
    }

    if (!sModule.strModel.SetString(qryRst.m_ppResult[qryRst.m_iColum + 3]))
    {
        return false;
    }

    if (!sModule.strVersion.SetString(qryRst.m_ppResult[qryRst.m_iColum + 4]))
    {
        return false;
    }

    sModule.ucType = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum+5]);
   
    return true;
}


/**************************************************************
Function:       CGbtTscDb::QueryModule
Description:    根据生产商查询模块，并保存
Input:          sCompany -- 生产商名称字符串
				tblModule   -- 模块类对象
Output:         无
Return:         true - 查询成功 false -查询失败
***************************************************************/
bool CGbtTscDb::QueryModule(const char* sCompany, TblModule& tblModule)
{
    int    i;
    Byte  uCount;
    char   sSql[LARGE_BUF_SIZE];
    CSqliteRst  qryRst;
    Module* pData = NULL;

    ACE_OS::sprintf(sSql, "select * from %s where %s='%s' order by %s", 
              			   Table_Desc[TBL_MODULE].sTblName,
                		   Table_Desc[TBL_MODULE].sFieldName[2], 
                		   sCompany,
               			   Table_Desc[TBL_MODULE].sFieldName[0]);

    if (!Query(sSql, qryRst))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
        return false;
    }

    if (qryRst.m_iColum != Table_Desc[TBL_MODULE].iFieldCount)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= %d\n", SHORT_FILE, __LINE__, Table_Desc[TBL_MODULE].sTblName, qryRst.m_iColum,Table_Desc[TBL_MODULE].iFieldCount));
        return false;
    }

    if (qryRst.m_iRow <= 0)
    {
        return false;
    }

    uCount = (Byte)qryRst.m_iRow;
    if (!tblModule.AllocData(uCount))
    {
        return false;
    }

    pData = tblModule.GetData(uCount);
    for (i = 0; i < qryRst.m_iRow; i++)
    {
        pData[i].ucModuleId = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum]);

        if (!pData[i].strDevNode.SetString(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 1]))
        {
            return false;
        }
        
        if (!pData[i].strCompany.SetString(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 2]))
        {
            return false;
        }

        if (!pData[i].strModel.SetString(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 3]))
        {
            return false;
        }

        if (!pData[i].strVersion.SetString(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 4]))
        {
            return false;
        }

        pData[i].ucType = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 5]);
    }
    return true;
}


/**************************************************************
Function:       CGbtTscDb::AddModule
Description:    添加模块表，并保存
Input:          uModuleId -- 模块ID号
				sModule   -- 待保存模块结构体
Output:         无
Return:         true - 添加成功 false -添加失败
***************************************************************/
bool CGbtTscDb::AddModule(Byte uModuleId, Module& sModule)
{
    Ushort uCount;
    char   sSql[LARGE_BUF_SIZE];

    if (!IsModuleValid(uModuleId, sModule))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tsModule invalid\n", SHORT_FILE, __LINE__));
        return false;
    }

    ACE_OS::sprintf(sSql, "insert into %s(%s,%s,%s,%s,%s,%s)values(%u,'%s','%s','%s','%s',%u)",
                  		   Table_Desc[TBL_MODULE].sTblName,
                   		   Table_Desc[TBL_MODULE].sFieldName[0],
                   		   Table_Desc[TBL_MODULE].sFieldName[1], 
                   		   Table_Desc[TBL_MODULE].sFieldName[2], 
                   		   Table_Desc[TBL_MODULE].sFieldName[3], 
                   		   Table_Desc[TBL_MODULE].sFieldName[4], 
                   		   Table_Desc[TBL_MODULE].sFieldName[5],
                   		   uModuleId, 
                  		   sModule.strDevNode.GetData(uCount), 
                   		   sModule.strCompany.GetData(uCount), 
                  		   sModule.strModel.GetData(uCount), 
                  		   sModule.strVersion.GetData(uCount), 
                   		   sModule.ucType);

    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }

    return true;
}



/**************************************************************
Function:       CGbtTscDb::ModModule
Description:    修改模块表，并保存
Input:          uModuleId -- 模块ID号
				sModule   -- 待保存模块结构体
Output:         无
Return:         true - 修改成功 false -修改失败
***************************************************************/
bool CGbtTscDb::ModModule(Byte uModuleId, Module& sModule)
{
    Module sTemp;
    Ushort uCount;
    if (!QueryModule(uModuleId, sTemp))
    {
        return AddModule(uModuleId, sModule);
    }

    if (!IsModuleValid(uModuleId, sModule))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tsModule invalid\n", SHORT_FILE, __LINE__));
        return false;
    }

    char   sSql[LARGE_BUF_SIZE];
    CHECK_ID(CONST_MODULE, uModuleId, false);
    ACE_OS::sprintf(sSql, "update %s set %s='%s', %s='%s', %s='%s', %s='%s', %s=%u where %s=%u",
                  		   Table_Desc[TBL_MODULE].sTblName,
                   		   Table_Desc[TBL_MODULE].sFieldName[1], 
                   		   sModule.strDevNode.GetData(uCount),
                  		   Table_Desc[TBL_MODULE].sFieldName[2], 
                  		   sModule.strCompany.GetData(uCount),
                   		   Table_Desc[TBL_MODULE].sFieldName[3], 
                   		   sModule.strModel.GetData(uCount),
                   		   Table_Desc[TBL_MODULE].sFieldName[4], 
                   		   sModule.strVersion.GetData(uCount),
                  		   Table_Desc[TBL_MODULE].sFieldName[5], 
                  		   sModule.ucType,
                  		   Table_Desc[TBL_MODULE].sFieldName[0], 
                  		   uModuleId);

    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }

    return true;
}


/**************************************************************
Function:       CGbtTscDb::DelModule
Description:    根据模块ID号删除模块表记录
Input:          uModuleId -- 模块ID号
Output:         无
Return:         true - 删除成功 false -删除失败
***************************************************************/
bool CGbtTscDb::DelModule(Byte uModuleId)
{
    char   sSql[LARGE_BUF_SIZE];
    ACE_OS::sprintf(sSql, "delete from %s where %s=%u",
                   		   Table_Desc[TBL_MODULE].sTblName,
                   		   Table_Desc[TBL_MODULE].sFieldName[0], 
                   		   uModuleId);

    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }

    return true;
}

/**************************************************************
Function:       CGbtTscDb::IsModuleValid
Description:    判断模块数据是否合法
Input:          uModuleId -- 模块ID号
				sModule   -- 模块结构体数据
Output:         无
Return:         true - 数据校验合法 false -数据校验非法
***************************************************************/
bool CGbtTscDb::IsModuleValid(Byte uModuleId, Module& sModule)
{
    CHECK_ID(CONST_MODULE, uModuleId, false);

    if (sModule.ucType <= 0 || sModule.ucType > 3)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tucType invalid, ucType = %d\n", SHORT_FILE, __LINE__, sModule.ucType));
        return false;
    }

    return true;
}


/**************************************************************
Function:       CGbtTscDb::QueryStageFactTime
Description:    查询全部阶段绿灯表，并保存
Input:          tblStageFactTime -- 阶段绿灯类对象
Output:         无
Return:         true - 查询成功 false -查询失败
***************************************************************/
bool CGbtTscDb::QueryStageFactTime(TblStageFactTime& tblStageFactTime)
{
	int    i;
	Ulong  uCount;
	char   sSql[SMALL_BUF_SIZE] = {0};
	CSqliteRst  qryRst;
	StageFactTime* pData = NULL;

	ACE_OS::sprintf(sSql, "select * from %s order by %s", 
					       Table_Desc[TBL_STAGE_FACTTIME].sTblName,
						   Table_Desc[TBL_STAGE_FACTTIME].sFieldName[0]);

	if (!Query(sSql, qryRst))
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
		return false;
	}

	if (qryRst.m_iColum != Table_Desc[TBL_STAGE_FACTTIME].iFieldCount)
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= %d\n", 
							  SHORT_FILE, __LINE__, Table_Desc[TBL_STAGE_FACTTIME].sTblName
							 , qryRst.m_iColum,Table_Desc[TBL_STAGE_FACTTIME].iFieldCount));
		return false;
	}

	if (qryRst.m_iRow <= 0)
	{
		return false;
	}

	uCount = (Byte)qryRst.m_iRow;
	if (!tblStageFactTime.AllocData(uCount))
	{
		return false;
	}

	pData = tblStageFactTime.GetData(uCount);
	for (i = 0; i < qryRst.m_iRow; i++)
	{
		pData[i].uiAddTime = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum]);

		pData[i].ucStageCnt = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 1]);

		if (!pData[i].sStageGreenTime.SetString(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 2]))
		{
			return false;
		}
	}
	return true;
}


/**************************************************************
Function:       CGbtTscDb::QueryStageFactTime
Description:    查询特定时间段里的阶段绿灯表
Input:          tblStageFactTime -- 阶段绿灯类对象
				ulStartTime      -- 起始时间
				ulEndTime        -- 结束时间
Output:         无
Return:         true - 查询成功 false -查询失败
***************************************************************/
bool CGbtTscDb::QueryStageFactTime(TblStageFactTime& tblStageFactTime,Ulong ulStartTime,Ulong ulEndTime)
{
	int    i;
	Ulong  uCount;
	char   sSql[SMALL_BUF_SIZE] = {0};
	CSqliteRst  qryRst;
	StageFactTime* pData = NULL;

	ACE_OS::sprintf(sSql, "select * from %s where %s>%lu and %s<%lu", 
						  Table_Desc[TBL_STAGE_FACTTIME].sTblName,
						  Table_Desc[TBL_STAGE_FACTTIME].sFieldName[0],
						  ulStartTime,
						  Table_Desc[TBL_STAGE_FACTTIME].sFieldName[0],
						  ulEndTime);

	if (!Query(sSql, qryRst))
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
		return false;
	}

	if (qryRst.m_iColum != Table_Desc[TBL_STAGE_FACTTIME].iFieldCount)
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= %d\n", 
							  SHORT_FILE, __LINE__, Table_Desc[TBL_STAGE_FACTTIME].sTblName
					          , qryRst.m_iColum,Table_Desc[TBL_STAGE_FACTTIME].iFieldCount));
		return false;
	}

	if (qryRst.m_iRow <= 0)
	{
		return false;
	}

	uCount = (Byte)qryRst.m_iRow;
	if (!tblStageFactTime.AllocData(uCount))
	{
		return false;
	}

	pData = tblStageFactTime.GetData(uCount);
	for (i = 0; i < qryRst.m_iRow; i++)
	{
		pData[i].uiAddTime = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum]);

		pData[i].ucStageCnt = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 1]);

		if (!pData[i].sStageGreenTime.SetString(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 2]))
		{
			return false;
		}
	}
	return true;
}


/**************************************************************
Function:       CGbtTscDb::AddStageFactTime
Description:    添加阶段绿灯表
Input:          sStageFactTime  -- 阶段绿灯结构体数据
Output:         无
Return:         true - 添加数据成功 false -添加数据失败
***************************************************************/
bool CGbtTscDb::AddStageFactTime(StageFactTime& sStageFactTime)
{
	Ushort uCount;
	Ulong   uTime;
	CSqliteRst  qryRst;
	char   sSql[LARGE_BUF_SIZE] = {0};

	//判断最小的日期 存储的最长1个月
	ACE_OS::sprintf(sSql, "select MIN(%s) from %s", 
						  Table_Desc[TBL_STAGE_FACTTIME].sFieldName[0],
					      Table_Desc[TBL_STAGE_FACTTIME].sTblName);

	if (!Query(sSql, qryRst))
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
		return false;
	}

	if ( NULL == qryRst.m_ppResult[qryRst.m_iColum] )
	{
		uTime = sStageFactTime.uiAddTime;
	}
	else
	{
		uTime = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum]);
	}
	
	if ( sStageFactTime.uiAddTime - uTime > MAX_STAGEFACT_CNT * 24 * 3600 )
	{
		DeleteStageFactTime(uTime);
	}

	ACE_OS::memset(sSql,0,LARGE_BUF_SIZE);
	ACE_OS::sprintf(sSql, "insert into %s(%s,%s,%s)values(%lu,%u,'%s')",
						   Table_Desc[TBL_STAGE_FACTTIME].sTblName,
						   Table_Desc[TBL_STAGE_FACTTIME].sFieldName[0], 
						   Table_Desc[TBL_STAGE_FACTTIME].sFieldName[1], 
						   Table_Desc[TBL_STAGE_FACTTIME].sFieldName[2],
						   sStageFactTime.uiAddTime,
						   sStageFactTime.ucStageCnt,
						   sStageFactTime.sStageGreenTime.GetData(uCount)
							);

	if (!ExecuteCmd(sSql))
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
		return false;
	}

	return true;
}


/**************************************************************
Function:       CGbtTscDb::DeleteStageFactTime
Description:    删除阶段绿灯表
Input:          ulAddTime  -- 阶段绿灯时间
Output:         无
Return:         true - 删除数据成功 false -删除数据失败
***************************************************************/
bool CGbtTscDb::DeleteStageFactTime(Ulong ulAddTime)
{
	char   sSql[LARGE_BUF_SIZE];
	ACE_OS::sprintf(sSql, "delete from %s where %s=%lu",
						   Table_Desc[TBL_STAGE_FACTTIME].sTblName,
						   Table_Desc[TBL_STAGE_FACTTIME].sFieldName[0], 
						   ulAddTime);

	if (!ExecuteCmd(sSql))
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
		return false;
	}

	return true;
}



/**************************************************************
Function:       CGbtTscDb::QueryAdaptStageTime
Description:    查询全部自适应阶段绿灯表
Input:          tblAdaptStageTime  -- 自适应阶段绿灯表类对象
Output:         无
Return:         true - 查询数据成功 false -查询数据失败
***************************************************************/
bool CGbtTscDb::QueryAdaptStageTime(TblAdaptStageTime& tblAdaptStageTime)
{
	int    i;
	Byte  uCount;
	char   sSql[SMALL_BUF_SIZE];
	CSqliteRst  qryRst;
	AdaptStageTime* pData = NULL;

	ACE_OS::sprintf(sSql, "select * from %s order by %s", 
					       Table_Desc[TBL_ADAPT_STAGETIME].sTblName,
						   Table_Desc[TBL_ADAPT_STAGETIME].sFieldName[0]);

	if (!Query(sSql, qryRst))
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
		return false;
	}

	if (qryRst.m_iColum != Table_Desc[TBL_ADAPT_STAGETIME].iFieldCount)
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= %d\n", SHORT_FILE, __LINE__, Table_Desc[TBL_ADAPT_STAGETIME].sTblName, qryRst.m_iColum,Table_Desc[TBL_ADAPT_STAGETIME].iFieldCount));
		return false;
	}

	if (qryRst.m_iRow <= 0)
	{
		return false;
	}

	uCount = (Byte)qryRst.m_iRow;
	if ( !tblAdaptStageTime.AllocData(uCount) )
	{
		return false;
	}

	pData = tblAdaptStageTime.GetData(uCount);
	for ( i = 0; i < qryRst.m_iRow; i++ )
	{
		pData[i].ucId       = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum]);
		pData[i].ucWeekType = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 1]);
		pData[i].ucHour     = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 2]);
		pData[i].ucMin      = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 3]);
		pData[i].ucStageCnt = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 4]);

		if (!pData[i].sStageGreenTime.SetString(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 5]))
		{
			return false;
		}
	}
	return true;
}


/**************************************************************
Function:       CGbtTscDb::QueryAdaptStageTime
Description:    根据星期查询全部阶段绿灯表
Input:          tblAdaptStageTime  -- 自适应阶段绿灯表类对象
				ucWeekType  -- 星期
Output:         无
Return:         true - 查询数据成功 false -查询数据失败
***************************************************************/
bool CGbtTscDb::QueryAdaptStageTime(TblAdaptStageTime& tblAdaptStageTime,Byte ucWeekType)
{
	int    i;
	Byte  uCount;
	char   sSql[SMALL_BUF_SIZE];
	CSqliteRst  qryRst;
	AdaptStageTime* pData = NULL;

	ACE_OS::sprintf(sSql, "select * from %s where %s=%u order by %s , %s", 
						   Table_Desc[TBL_ADAPT_STAGETIME].sTblName,
						   Table_Desc[TBL_ADAPT_STAGETIME].sFieldName[1],
						   ucWeekType,
						   Table_Desc[TBL_ADAPT_STAGETIME].sFieldName[2],
						   Table_Desc[TBL_ADAPT_STAGETIME].sFieldName[3]);

	if (!Query(sSql, qryRst))
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
		return false;
	}

	if (qryRst.m_iColum != Table_Desc[TBL_ADAPT_STAGETIME].iFieldCount)
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= %d\n", SHORT_FILE, __LINE__, Table_Desc[TBL_ADAPT_STAGETIME].sTblName, qryRst.m_iColum,Table_Desc[TBL_ADAPT_STAGETIME].iFieldCount));
		return false;
	}

	if (qryRst.m_iRow <= 0)
	{
		return false;
	}

	uCount = (Byte)qryRst.m_iRow;
	if ( !tblAdaptStageTime.AllocData(uCount) )
	{
		return false;
	}

	pData = tblAdaptStageTime.GetData(uCount);
	for ( i = 0; i < qryRst.m_iRow; i++ )
	{
		pData[i].ucId       = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum]);
		pData[i].ucWeekType = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 1]);
		pData[i].ucHour     = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 2]);
		pData[i].ucMin      = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 3]);
		pData[i].ucStageCnt = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 4]);

		if (!pData[i].sStageGreenTime.SetString(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 5]))
		{
			return false;
		}
	}
	return true;
}


/**************************************************************
Function:       CGbtTscDb::AddAdaptStageTime
Description:    添加自适应阶段绿灯表数据记录
Input:          sAdaptStageTime  -- 自适应阶段绿灯结构体
Output:         无
Return:         true - 添加数据成功 false -添加数据失败
***************************************************************/
bool CGbtTscDb::AddAdaptStageTime(AdaptStageTime& sAdaptStageTime)
{
	char   sSql[LARGE_BUF_SIZE];
	ACE_OS::sprintf(sSql, "insert into %s(%s,%s,%s,%s,%s)values(%u,%u,%u,%u,'%s')",
						   Table_Desc[TBL_ADAPT_STAGETIME].sTblName,
						   Table_Desc[TBL_ADAPT_STAGETIME].sFieldName[1], 
						   Table_Desc[TBL_ADAPT_STAGETIME].sFieldName[2], 
						   Table_Desc[TBL_ADAPT_STAGETIME].sFieldName[3], 
						   Table_Desc[TBL_ADAPT_STAGETIME].sFieldName[4], 
						   Table_Desc[TBL_ADAPT_STAGETIME].sFieldName[5],
						   sAdaptStageTime.ucWeekType, 
						   sAdaptStageTime.ucHour,
						   sAdaptStageTime.ucMin,
						   sAdaptStageTime.ucStageCnt,
						   sAdaptStageTime.sStageGreenTime.GetData()
							);

	if (!ExecuteCmd(sSql))
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
		return false;
	}

	return true;
}


/**************************************************************
Function:       CGbtTscDb::QueryAdaptStageTime
Description:    根据ucid查询自适应阶段绿灯表数据记录
Input:          ucId  -- ID号
				sAdaptStageTime  -- 自适应阶段绿灯结构体
Output:         无
Return:         true - 查询数据成功 false -查询数据失败
***************************************************************/
bool CGbtTscDb::QueryAdaptStageTime(Byte ucId, AdaptStageTime& sAdaptStageTime)
{
	char   sSql[SMALL_BUF_SIZE];
	CSqliteRst  qryRst;

	ACE_OS::sprintf(sSql, "select * from %s where %s=%u ", 
						  Table_Desc[TBL_ADAPT_STAGETIME].sTblName,
						  Table_Desc[TBL_ADAPT_STAGETIME].sFieldName[0],
						  ucId);

	if (!Query(sSql, qryRst))
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
		return false;
	}

	if (qryRst.m_iColum != Table_Desc[TBL_ADAPT_STAGETIME].iFieldCount)
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= %d\n", SHORT_FILE, __LINE__, Table_Desc[TBL_ADAPT_STAGETIME].sTblName, qryRst.m_iColum,Table_Desc[TBL_ADAPT_STAGETIME].iFieldCount));
		return false;
	}

	if (qryRst.m_iRow <= 0)
	{
		return false;
	}

	sAdaptStageTime.ucId       = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum]);
	sAdaptStageTime.ucWeekType = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 1]);
	sAdaptStageTime.ucHour     = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 2]);
	sAdaptStageTime.ucMin      = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 3]);
	sAdaptStageTime.ucStageCnt = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 4]);

	if ( !sAdaptStageTime.sStageGreenTime.SetString(qryRst.m_ppResult[qryRst.m_iColum + 5]) )
	{
		return false;
	}
	
	return true;
}

/**************************************************************
Function:       CGbtTscDb::ModAdaptStageTime
Description:    修改自适应阶段绿灯表数据记录
Input:          sAdaptStageTime  -- 自适应阶段绿灯结构体
Output:         无
Return:         true - 修改数据成功 false -修改数据失败
***************************************************************/
bool CGbtTscDb::ModAdaptStageTime(AdaptStageTime& sAdaptStageTime)
{
	AdaptStageTime sTemp;
	if ( !QueryAdaptStageTime(sAdaptStageTime.ucId, sTemp) )
	{
		return AddAdaptStageTime(sAdaptStageTime);
	}

	char   sSql[LARGE_BUF_SIZE];
	
	ACE_OS::sprintf(sSql, "update %s set %s=%u, %s=%u, %s=%u, %s=%u, %s='%s' where %s=%u",
							Table_Desc[TBL_ADAPT_STAGETIME].sTblName,
							Table_Desc[TBL_ADAPT_STAGETIME].sFieldName[1], 
							sAdaptStageTime.ucWeekType,
							Table_Desc[TBL_ADAPT_STAGETIME].sFieldName[2], 
							sAdaptStageTime.ucHour,
							Table_Desc[TBL_ADAPT_STAGETIME].sFieldName[3], 
							sAdaptStageTime.ucMin,
							Table_Desc[TBL_ADAPT_STAGETIME].sFieldName[4], 
							sAdaptStageTime.ucStageCnt,
							Table_Desc[TBL_ADAPT_STAGETIME].sFieldName[5], 
							sAdaptStageTime.sStageGreenTime.GetData(),
							Table_Desc[TBL_ADAPT_STAGETIME].sFieldName[0], 
							sAdaptStageTime.ucId);

	if (!ExecuteCmd(sSql))
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
		return false;
	}

	return true;
}


/**************************************************************
Function:       CGbtTscDb::DeleteAdaptStageTime
Description:    删除全部自适应阶段绿灯表
Input:          无
Output:         无
Return:         true - 删除数据成功 false -删除数据失败
***************************************************************/
bool CGbtTscDb::DeleteAdaptStageTime()
{
	char   sSql[LARGE_BUF_SIZE];
	ACE_OS::sprintf(sSql, "delete from %s",  Table_Desc[TBL_ADAPT_STAGETIME].sTblName);

	if (!ExecuteCmd(sSql))
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
		return false;
	}
	return true;
}

/**************************************************************
Function:       CGbtTscDb::QueryPhase
Description:    相位查询
Input:          tblPhase  -- 相位表类对象
Output:         无
Return:         true - 查询数据成功 false -查询数据失败
***************************************************************/
bool CGbtTscDb::QueryPhase(TblPhase& tblPhase)
{
    int    i;
    Byte  uCount;
    char   sSql[SMALL_BUF_SIZE];
    CSqliteRst  qryRst;
    Phase* pData = NULL;

    ACE_OS::sprintf(sSql, "select * from %s order by %s", 
               			   Table_Desc[TBL_PHASE].sTblName,
               			   Table_Desc[TBL_PHASE].sFieldName[0]);

    if (!Query(sSql, qryRst))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
        return false;
    }

    if (qryRst.m_iColum != Table_Desc[TBL_PHASE].iFieldCount)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= %d\n", SHORT_FILE, __LINE__, Table_Desc[TBL_PHASE].sTblName, qryRst.m_iColum,Table_Desc[TBL_PHASE].iFieldCount));
        return false;
    }

    if (qryRst.m_iRow <= 0)
    {
        return false;
    }

    uCount = (Byte)qryRst.m_iRow;
    if (!tblPhase.AllocData(uCount))
    {
        return false;
    }

    pData = tblPhase.GetData(uCount);
    for (i = 0; i < qryRst.m_iRow; i++)
    {
        pData[i].ucPhaseId          = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum]);
        pData[i].ucPedestrianGreen  = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 1]);
        pData[i].ucPedestrianClear  = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 2]);
        pData[i].ucMinGreen         = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 3]);
        pData[i].ucGreenDelayUnit   = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 4]);
        pData[i].ucMaxGreen1        = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 5]);
        pData[i].ucMaxGreen2        = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 6]);
        pData[i].ucFixGreen         = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 7]);
        pData[i].ucGreenFlash       = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 8]);
        pData[i].ucPhaseTypeFlag    = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 9]);
        pData[i].ucPhaseOption      = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 10]);
        pData[i].ucExtend           = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 11]);

    }
    return true;
}

/**************************************************************
Function:       CGbtTscDb::QueryPhase
Description:    根据相位ID号相位查询
Input:          uPhaseId  -- 相位ID
				sPhase   -- 相位结构体
Output:         无
Return:         true - 查询数据成功 false -查询数据失败
***************************************************************/
bool CGbtTscDb::QueryPhase(Byte uPhaseId, Phase& sPhase)
{
    char   sSql[SMALL_BUF_SIZE];
    CSqliteRst  qryRst;

    ACE_OS::sprintf(sSql, "select * from %s where %s=%d", 
                		  Table_Desc[TBL_PHASE].sTblName,
                		  Table_Desc[TBL_PHASE].sFieldName[0], uPhaseId);

    if (!Query(sSql, qryRst))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
        return false;
    }

    if (qryRst.m_iColum != Table_Desc[TBL_PHASE].iFieldCount)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= %d\n", SHORT_FILE, __LINE__, Table_Desc[TBL_PHASE].sTblName, qryRst.m_iColum,Table_Desc[TBL_PHASE].iFieldCount));
        return false;
    }

    if (qryRst.m_iRow <= 0)
    {
        return false;
    }

    sPhase.ucPhaseId          = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum]);
    sPhase.ucPedestrianGreen  = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 1]);
    sPhase.ucPedestrianClear  = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 2]);
    sPhase.ucMinGreen         = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 3]);
    sPhase.ucGreenDelayUnit   = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 4]);
    sPhase.ucMaxGreen1        = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 5]);
    sPhase.ucMaxGreen2        = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 6]);
    sPhase.ucFixGreen         = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 7]);
    sPhase.ucGreenFlash       = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 8]);
    sPhase.ucPhaseTypeFlag    = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 9]);
    sPhase.ucPhaseOption      = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 10]);
    sPhase.ucExtend           = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 11]);
 
    return true;
}
  
/**************************************************************
Function:       CGbtTscDb::AddPhase
Description:    添加相位表记录
Input:          uPhaseId  -- 相位ID
				sPhase   -- 待添加相位结构体数据
Output:         无
Return:         true - 添加数据成功 false -添加数据失败
***************************************************************/
bool CGbtTscDb::AddPhase(Byte uPhaseId, Phase& sPhase)
{
    char   sSql[LARGE_BUF_SIZE];
    if (!IsPhaseValid(uPhaseId, sPhase))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tsPhase invalid\n", SHORT_FILE, __LINE__));
        return false;
    }
    ACE_OS::sprintf(sSql, "insert into %s(%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)values(%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u)",
				           Table_Desc[TBL_PHASE].sTblName,
				           Table_Desc[TBL_PHASE].sFieldName[0],
				           Table_Desc[TBL_PHASE].sFieldName[1], 
				           Table_Desc[TBL_PHASE].sFieldName[2], 
				           Table_Desc[TBL_PHASE].sFieldName[3],
				           Table_Desc[TBL_PHASE].sFieldName[4],
				           Table_Desc[TBL_PHASE].sFieldName[5], 
				           Table_Desc[TBL_PHASE].sFieldName[6], 
				           Table_Desc[TBL_PHASE].sFieldName[7], 
				           Table_Desc[TBL_PHASE].sFieldName[8], 
				           Table_Desc[TBL_PHASE].sFieldName[9], 
				           Table_Desc[TBL_PHASE].sFieldName[10], 
				           Table_Desc[TBL_PHASE].sFieldName[11], 
				           uPhaseId,
				           sPhase.ucPedestrianGreen,
				           sPhase.ucPedestrianClear,
				           sPhase.ucMinGreen, 
				           sPhase.ucGreenDelayUnit, 
				           sPhase.ucMaxGreen1,
				           sPhase.ucMaxGreen2,
				           sPhase.ucFixGreen,
				           sPhase.ucGreenFlash,
				           sPhase.ucPhaseTypeFlag,
				           sPhase.ucPhaseOption,
				           sPhase.ucExtend);

    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }
    return true;
}
   
/**************************************************************
Function:       CGbtTscDb::ModPhase
Description:    修改相位表记录数据
Input:          uPhaseId  -- 相位ID
				sPhase   -- 待修改相位结构体数据
Output:         无
Return:         true - 修改数据成功 false -修改数据失败
***************************************************************/
bool CGbtTscDb::ModPhase(Byte uPhaseId, Phase& sPhase)
{
    Phase sTemp;
    if (!QueryPhase(uPhaseId, sTemp))
    {
        return AddPhase(uPhaseId, sPhase);
    }

    char   sSql[LARGE_BUF_SIZE];
    if (!IsPhaseValid(uPhaseId, sPhase))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tsPhase invalid\n", SHORT_FILE, __LINE__));
        return false;
    }

    ACE_OS::sprintf(sSql, "update %s set %s=%u, %s=%u, %s=%u, %s=%u, %s=%u, %s=%u, %s=%u, %s=%u,%s=%u, %s=%u, %s=%u where %s=%u",
				           Table_Desc[TBL_PHASE].sTblName,
				           Table_Desc[TBL_PHASE].sFieldName[1], sPhase.ucPedestrianGreen,
				           Table_Desc[TBL_PHASE].sFieldName[2], sPhase.ucPedestrianClear,
				           Table_Desc[TBL_PHASE].sFieldName[3], sPhase.ucMinGreen,
				           Table_Desc[TBL_PHASE].sFieldName[4], sPhase.ucGreenDelayUnit,
				           Table_Desc[TBL_PHASE].sFieldName[5], sPhase.ucMaxGreen1,
				           Table_Desc[TBL_PHASE].sFieldName[6], sPhase.ucMaxGreen2,
				           Table_Desc[TBL_PHASE].sFieldName[7], sPhase.ucFixGreen,
				           Table_Desc[TBL_PHASE].sFieldName[8], sPhase.ucGreenFlash,
				           Table_Desc[TBL_PHASE].sFieldName[9], sPhase.ucPhaseTypeFlag,
				           Table_Desc[TBL_PHASE].sFieldName[10], sPhase.ucPhaseOption,
				           Table_Desc[TBL_PHASE].sFieldName[11], sPhase.ucExtend,
				           Table_Desc[TBL_PHASE].sFieldName[0], uPhaseId);

    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }

    return true;
}

/**************************************************************
Function:       CGbtTscDb::DelPhase
Description:    根据相位ID号删除相位表记录数据
Input:          uPhaseId  -- 相位ID
Output:         无
Return:         true - 删除数据成功 false -删除数据失败
***************************************************************/
bool CGbtTscDb::DelPhase(Byte uPhaseId)
{
    char   sSql[LARGE_BUF_SIZE];
    ACE_OS::sprintf(sSql, "delete from %s where %s=%u",
				           Table_Desc[TBL_PHASE].sTblName,
				           Table_Desc[TBL_PHASE].sFieldName[0], uPhaseId);

    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }
    return true;
}

/**************************************************************
Function:       CGbtTscDb::DelPhase
Description:    删除全部相位表记录数据
Input:          uPhaseId  -- 相位ID
Output:         无
Return:         true - 删除数据成功 false -删除数据失败
***************************************************************/
bool CGbtTscDb::DelPhase()
{
	char   sSql[LARGE_BUF_SIZE];
	ACE_OS::sprintf(sSql, "delete from %s",	Table_Desc[TBL_PHASE].sTblName);

	if (!ExecuteCmd(sSql))
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
		return false;
	}
	return true;
}

/**************************************************************
Function:       CGbtTscDb::IsPhaseValid
Description:    判断相位表数据合法性
Input:          uPhaseId  -- 相位ID
				sPhase    -- 相位数据结构体
Output:         无
Return:         true - 数据合法 false -数据非法
***************************************************************/
bool CGbtTscDb::IsPhaseValid(Byte uPhaseId, Phase& sPhase)
{
    CHECK_ID(CONST_PHASE, uPhaseId, false);

    return true;
}

/**************************************************************
Function:       CGbtTscDb::QueryCollision
Description:    查询相位冲突表数据
Input:          tblCollision  -- 相位冲突表对象
Output:         无
Return:         true - 查询 数据成功false -查询数据失败
***************************************************************/
bool CGbtTscDb::QueryCollision(TblCollision& tblCollision)
{
    int    i;
    Byte  uCount;
    char   sSql[SMALL_BUF_SIZE];
    CSqliteRst  qryRst;
    Collision*  pData = NULL;

    ACE_OS::sprintf(sSql, "select * from %s order by %s", 
						    Table_Desc[TBL_COLLISION].sTblName,
						    Table_Desc[TBL_COLLISION].sFieldName[0]);

    if (!Query(sSql, qryRst))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
        return false;
    }

    if (qryRst.m_iColum != Table_Desc[TBL_COLLISION].iFieldCount)
    {
#ifdef TSC_DEBUG
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= %d\n", SHORT_FILE, __LINE__, Table_Desc[TBL_COLLISION].sTblName, qryRst.m_iColum,Table_Desc[TBL_COLLISION].iFieldCount));
#endif
        return false;
    }

    if (qryRst.m_iRow <= 0)
    {
        return false;
    }

    uCount = (Byte)qryRst.m_iRow;
    if (!tblCollision.AllocData(uCount))
    {
        return false;
    }

    pData = tblCollision.GetData(uCount);
    for (i = 0; i < qryRst.m_iRow; i++)
    {
        pData[i].ucPhaseId          = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum]);
        pData[i].usCollisionFlag    = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 1]);
    }
    return true;
}

/*žùŸÝÏàÎ»ºÅ²éÑ¯*/
bool CGbtTscDb::QueryCollision(Byte uPhaseId, Collision& sCollision)
{
     char   sSql[SMALL_BUF_SIZE];
    CSqliteRst  qryRst;

    ACE_OS::sprintf(sSql, "select * from %s where %s=%d", 
                Table_Desc[TBL_COLLISION].sTblName,
                Table_Desc[TBL_COLLISION].sFieldName[0], uPhaseId);

    if (!Query(sSql, qryRst))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
        return false;
    }

    if (qryRst.m_iColum != Table_Desc[TBL_COLLISION].iFieldCount)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= %d\n", SHORT_FILE, __LINE__, Table_Desc[TBL_COLLISION].sTblName, qryRst.m_iColum,Table_Desc[TBL_COLLISION].iFieldCount));
        return false;
    }

    if (qryRst.m_iRow <= 0)
    {
        return false;
    }

    sCollision.ucPhaseId          = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum]);
    sCollision.usCollisionFlag    = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 1]);

    return true;
}
    
/**************************************************************
Function:       CGbtTscDb::AddCollision
Description:    根据相位ID号添加相位冲突表数据
Input:          uPhaseId  -- 冲突相位ID号
				sCollision --冲突相位结构体数据
Output:         无
Return:         true - 添加数据成功  false -添加数据失败
***************************************************************/
bool CGbtTscDb::AddCollision(Byte uPhaseId, Collision& sCollision)
{
    char   sSql[LARGE_BUF_SIZE];
    if (!IsCollisionValid(uPhaseId, sCollision))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tCollision invalid\n", SHORT_FILE, __LINE__));
        return false;
    }

    ACE_OS::sprintf(sSql, "insert into %s(%s, %s)values(%u,%d)",
				           Table_Desc[TBL_COLLISION].sTblName,
				           Table_Desc[TBL_COLLISION].sFieldName[0],
				           Table_Desc[TBL_COLLISION].sFieldName[1], 
				           uPhaseId,
				           sCollision.usCollisionFlag);

    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }
    return true;
}
    
/**************************************************************
Function:       CGbtTscDb::ModCollision
Description:    修改相位冲突表数据
Input:          uPhaseId  -- 冲突相位ID号
				sCollision --冲突相位结构体数据
Output:         无
Return:         true - 修改数据成功  false -修改数据失败
***************************************************************/
bool CGbtTscDb::ModCollision(Byte uPhaseId, Collision& sCollision)
{
    Collision sTemp;
    if (!QueryCollision(uPhaseId, sTemp))
    {
        return AddCollision(uPhaseId, sCollision);
    }

    char   sSql[LARGE_BUF_SIZE];
    if (!IsCollisionValid(uPhaseId, sCollision))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tCollision invalid\n", SHORT_FILE, __LINE__));
        return false;
    }
    ACE_OS::sprintf(sSql, "update %s set %s=%d where %s=%u",
				           Table_Desc[TBL_COLLISION].sTblName,
				           Table_Desc[TBL_COLLISION].sFieldName[1], sCollision.usCollisionFlag,
				           Table_Desc[TBL_COLLISION].sFieldName[0], uPhaseId);

    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }

    return true;
}

/**************************************************************
Function:       CGbtTscDb::DelCollision
Description:    根据冲突相位IDH号删除相位冲突表数据记录
Input:          uPhaseId  -- 冲突相位ID号
Output:         无
Return:         true - 删除数据成功  false -删除数据失败
***************************************************************/
bool CGbtTscDb::DelCollision(Byte uPhaseId)
{
    char   sSql[LARGE_BUF_SIZE];
    ACE_OS::sprintf(sSql, "delete from %s where %s=%u",
				           Table_Desc[TBL_COLLISION].sTblName,
				           Table_Desc[TBL_COLLISION].sFieldName[0], 
				           uPhaseId);

    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }

    return true;
}

bool CGbtTscDb::IsCollisionValid(Byte uPhaseId, Collision& sCollision)
{
    CHECK_ID(CONST_PHASE, uPhaseId, false);

    return true;
}

/**************************************************************
Function:       CGbtTscDb::QueryDetPara
Description:    查询检测器参数
Input:          detPara  -- 检测器参数结构体
Output:         无
Return:         true - 查询参数成功  false -查询参数失败
***************************************************************/
bool CGbtTscDb::QueryDetPara(DetectorPara& detPara)
{
    Ulong   ulTemp;

    GetFieldData(Table_Desc[TBL_CONSTANT].sTblName, "ucMaxDetector", ulTemp);
    detPara.uDetCount = (Byte)ulTemp;

    GetFieldData(Table_Desc[TBL_CONSTANT].sTblName, "ucMaxDetector", ulTemp);
    detPara.uMaxDetGrpCount = (Byte)(ulTemp+7)/8;
    
    GetFieldData(Table_Desc[TBL_SYSTEM].sTblName, "ucDetDataSeqNo", ulTemp);
    detPara.ucDataSeqNo = (Byte)ulTemp;

    GetFieldData(Table_Desc[TBL_SYSTEM].sTblName, "ucDetDataCycle", ulTemp);
    detPara.ucDataCycle = (Byte)ulTemp;

    GetFieldData(Table_Desc[TBL_SYSTEM].sTblName, "ucDetPulseSeqNo", ulTemp);
    detPara.ucPulseSeqNo = (Byte)ulTemp;

    GetFieldData(Table_Desc[TBL_SYSTEM].sTblName, "ucDetPulseCycle", ulTemp);
    detPara.ucPulseCycle = (Byte)ulTemp;

    detPara.uDetCount = 0;
    return true;
}


/**************************************************************
Function:       CGbtTscDb::QueryDetector
Description:    查询检测器配置参数
Input:          tblDetector  -- 检测器配置参数表对象
Output:         无
Return:         true - 查询成功  false -查询失败
***************************************************************/
bool CGbtTscDb::QueryDetector(TblDetector& tblDetector)
{
    int    i;
    Byte  uCount;
    char   sSql[SMALL_BUF_SIZE];
    CSqliteRst  qryRst;
    Detector*  pData = NULL;

    ACE_OS::sprintf(sSql, "select * from %s order by %s", 
						    Table_Desc[TBL_DETECTOR].sTblName,
						    Table_Desc[TBL_DETECTOR].sFieldName[0]);

    if (!Query(sSql, qryRst))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
        return false;
    }

    if (qryRst.m_iColum != Table_Desc[TBL_DETECTOR].iFieldCount)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= %d\n", SHORT_FILE, __LINE__, Table_Desc[TBL_DETECTOR].sTblName, qryRst.m_iColum,Table_Desc[TBL_DETECTOR].iFieldCount));
        return false;
    }

    if (qryRst.m_iRow <= 0)
    {
        return false;
    }

    uCount = (Byte)qryRst.m_iRow;
    if (!tblDetector.AllocData(uCount))
    {
        return false;
    }

    pData = tblDetector.GetData(uCount);
    for (i = 0; i < qryRst.m_iRow; i++)
    {
        pData[i].ucDetectorId   = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum]);
        pData[i].ucPhaseId      = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 1]);
        pData[i].ucDetFlag      = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 2]);
        pData[i].ucDirect       = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 3]);
		pData[i].ucValidTime    = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 4]);
        pData[i].ucOptionFlag   = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 5]);
        pData[i].usSaturationFlow   = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 6]);
        pData[i].ucSaturationOccupy = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 7]);
    }
    return true;
}

/**************************************************************
Function:       CGbtTscDb::QueryDetector
Description:    根据检测器ID号查询检测器配置参数
Input:          uDetectorId  -- 检测器ID号
				sDetector    -- 检测器参数结构体
Output:         无
Return:         true - 查询成功  false -查询失败
***************************************************************/
bool CGbtTscDb::QueryDetector(Byte uDetectorId, Detector& sDetector)
{
    char   sSql[SMALL_BUF_SIZE];
    CSqliteRst  qryRst;

    ACE_OS::sprintf(sSql, "select * from %s where %s=%d", 
						    Table_Desc[TBL_DETECTOR].sTblName,
						    Table_Desc[TBL_DETECTOR].sFieldName[0], 
						    uDetectorId);

    if (!Query(sSql, qryRst))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
        return false;
    }

    if (qryRst.m_iColum != Table_Desc[TBL_DETECTOR].iFieldCount)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= %d\n", SHORT_FILE, __LINE__, Table_Desc[TBL_DETECTOR].sTblName, qryRst.m_iColum,Table_Desc[TBL_DETECTOR].iFieldCount));
        return false;
    }

    if (qryRst.m_iRow <= 0)
    {
        return false;
    }

    sDetector.ucDetectorId   = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum]);
    sDetector.ucPhaseId      = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 1]);
    sDetector.ucDetFlag      = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 2]);
    sDetector.ucDirect       = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 3]);
	sDetector.ucValidTime    = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 4]);
    sDetector.ucOptionFlag   = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 5]);
    sDetector.usSaturationFlow   = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 6]);
    sDetector.ucSaturationOccupy = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 7]);
    return true;
}
    
/**************************************************************
Function:       CGbtTscDb::AddDetector
Description:    根据检测器ID号添加检测器配置参数
Input:          uDetectorId  -- 检测器ID号
				sDetector    -- 检测器参数结构体
Output:         无
Return:         true - 添加数据成功  false -添加数据失败
***************************************************************/
bool CGbtTscDb::AddDetector(Byte uDetectorId, Detector& sDetector)
{
    char   sSql[LARGE_BUF_SIZE];
    if (!IsDetectorValid(uDetectorId, sDetector))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tDetector invalid\n", SHORT_FILE, __LINE__));
        return false;
    }
    ACE_OS::sprintf(sSql, "insert into %s(%s, %s, %s, %s, %s, %s, %s ,%s)values(%u,%u,%u,%u,%u,%d,%u,%u)",
				           Table_Desc[TBL_DETECTOR].sTblName,
				           Table_Desc[TBL_DETECTOR].sFieldName[0],
				           Table_Desc[TBL_DETECTOR].sFieldName[1], 
				           Table_Desc[TBL_DETECTOR].sFieldName[2],
				           Table_Desc[TBL_DETECTOR].sFieldName[3], 
				           Table_Desc[TBL_DETECTOR].sFieldName[4],
				           Table_Desc[TBL_DETECTOR].sFieldName[5], 
				           Table_Desc[TBL_DETECTOR].sFieldName[6], 
						   Table_Desc[TBL_DETECTOR].sFieldName[7], 
				           uDetectorId,
				           sDetector.ucPhaseId,
				           sDetector.ucDetFlag,
				           sDetector.ucDirect,
						   sDetector.ucValidTime,
				           sDetector.ucOptionFlag ,
				           sDetector.usSaturationFlow ,
				           sDetector.ucSaturationOccupy);

    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }
    return true;
}
    
/**************************************************************
Function:       CGbtTscDb::ModDetector
Description:    根据检测器ID号修改检测器配置参数
Input:          uDetectorId  -- 检测器ID号
				sDetector    -- 检测器参数结构体
Output:         无
Return:         true - 修改数据成功  false -修改数据失败
***************************************************************/
bool CGbtTscDb::ModDetector(Byte uDetectorId, Detector& sDetector)
{
    Detector sTemp;
    if (!QueryDetector(uDetectorId, sTemp))
    {
        return AddDetector(uDetectorId, sDetector);
    }

    char   sSql[LARGE_BUF_SIZE];
    if (!IsDetectorValid(uDetectorId, sDetector))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tDetector invalid\n", SHORT_FILE, __LINE__));
        return false;
    }

    ACE_OS::sprintf(sSql, "update %s set %s=%u, %s=%u, %s=%u, %s=%u, %s=%d, %s=%u, %s=%u where %s=%u",
				           Table_Desc[TBL_DETECTOR].sTblName,
				           Table_Desc[TBL_DETECTOR].sFieldName[1], sDetector.ucPhaseId,
				           Table_Desc[TBL_DETECTOR].sFieldName[2], sDetector.ucDetFlag,
				           Table_Desc[TBL_DETECTOR].sFieldName[3], sDetector.ucDirect,
						   Table_Desc[TBL_DETECTOR].sFieldName[4], sDetector.ucValidTime,
				           Table_Desc[TBL_DETECTOR].sFieldName[5], sDetector.ucOptionFlag,
				           Table_Desc[TBL_DETECTOR].sFieldName[6], sDetector.usSaturationFlow,
				           Table_Desc[TBL_DETECTOR].sFieldName[7], sDetector.ucSaturationOccupy,
				           Table_Desc[TBL_DETECTOR].sFieldName[0], uDetectorId);

    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }

    return true;
}

/**************************************************************
Function:       CGbtTscDb::DelDetector
Description:    根据检测器ID号删除检测器配置参数
Input:          uDetectorId  -- 检测器ID号
				sDetector    -- 检测器参数结构体
Output:         无
Return:         true - 删除数据成功  false -删除数据失败
***************************************************************/
bool CGbtTscDb::DelDetector(Byte uDetectorId)
{
    char   sSql[LARGE_BUF_SIZE];
    ACE_OS::sprintf(sSql, "delete from %s where %s=%u",
				           Table_Desc[TBL_DETECTOR].sTblName,
				           Table_Desc[TBL_DETECTOR].sFieldName[0], 
				           uDetectorId);

    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }

    return true;
}

/**************************************************************
Function:       CGbtTscDb::DelDetector
Description:    判断检测器表数据合法性
Input:          无
Output:         无
Return:         true - 删除数据成功  false -删除数据失败
***************************************************************/
bool CGbtTscDb::DelDetector()
{
	char   sSql[LARGE_BUF_SIZE];
	ACE_OS::sprintf(sSql, "delete from %s",	Table_Desc[TBL_DETECTOR].sTblName);

	if (!ExecuteCmd(sSql))
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
		return false;
	}

	return true;
}


/**************************************************************
Function:       CGbtTscDb::IsDetectorValid
Description:    删除全部检测器配置参数表记录
Input:          uDetectorId  -- 检测器ID号
				sDetector    -- 检测器参数结构体
Output:         无
Return:         true - 数据合法  false -数据非法
***************************************************************/
bool CGbtTscDb::IsDetectorValid(Byte uDetectorId, Detector& sDetector)
{
    CHECK_ID(CONST_DETECTOR, uDetectorId, false);

    if (sDetector.ucPhaseId != 0)
    {
        CHECK_ID(CONST_PHASE, sDetector.ucPhaseId, false);
    }

    if (sDetector.ucSaturationOccupy > 200)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tucSaturationOccupy = %d, invalid\n", SHORT_FILE, __LINE__, sDetector.ucSaturationOccupy));
        return false;
    }
    return true;
}

/**************************************************************
Function:       CGbtTscDb::QueryDetExtend
Description:    查询检测器扩展配置表数据
Input:          tblDetExtend -- 检测器扩展配置表对象
Output:         无
Return:         true - 查询成功  false -查询失败
***************************************************************/
bool CGbtTscDb::QueryDetExtend(TblDetExtend& tblDetExtend)
{
	int    i;
	Byte  uCount;
	char   sSql[SMALL_BUF_SIZE];
	CSqliteRst  qryRst;
	DetExtend*  pData = NULL;

	ACE_OS::sprintf(sSql, "select * from %s order by %s", 
							Table_Desc[TBL_DET_EXTEND].sTblName,
							Table_Desc[TBL_DET_EXTEND].sFieldName[0]);

	if (!Query(sSql, qryRst))
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
		return false;
	}

	if (qryRst.m_iColum != Table_Desc[TBL_DET_EXTEND].iFieldCount)
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= %d\n", SHORT_FILE, __LINE__, Table_Desc[TBL_DET_EXTEND].sTblName, qryRst.m_iColum,Table_Desc[TBL_DET_EXTEND].iFieldCount));
		return false;
	}

	if (qryRst.m_iRow <= 0)
	{
		return false;
	}

	uCount = (Byte)qryRst.m_iRow;
	if (!tblDetExtend.AllocData(uCount))
	{
		return false;
	}

	pData = tblDetExtend.GetData(uCount);
	for (i = 0; i < qryRst.m_iRow; i++)
	{
		pData[i].ucId         = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum]);
		pData[i].ucSensi      = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 1]);
		pData[i].ucGrpNo      = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 2]);
		pData[i].ucPro        = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 3]);
		pData[i].ucOcuDefault = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 4]);
		pData[i].usCarFlow    = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 5]);
	}
	return true;
}

/**************************************************************
Function:       CGbtTscDb::QueryDetExtend
Description:    根据扩展结构体查询检测器扩展配置表数据
Input:          sDetExtend -- 检测器扩展配置结构体
Output:         无
Return:         true - 查询成功  false -查询失败
***************************************************************/
bool CGbtTscDb::QueryDetExtend(DetExtend& sDetExtend)
{
	char   sSql[SMALL_BUF_SIZE];
	CSqliteRst  qryRst;

	ACE_OS::sprintf(sSql, "select * from %s where %s=%d", 
							Table_Desc[TBL_DET_EXTEND].sTblName,
							Table_Desc[TBL_DET_EXTEND].sFieldName[0], 
							sDetExtend.ucId);

	if (!Query(sSql, qryRst))
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
		return false;
	}

	if (qryRst.m_iColum != Table_Desc[TBL_DET_EXTEND].iFieldCount)
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= %d\n", SHORT_FILE, __LINE__, Table_Desc[TBL_DET_EXTEND].sTblName, qryRst.m_iColum,Table_Desc[TBL_DET_EXTEND].iFieldCount));
		return false;
	}

	if (qryRst.m_iRow <= 0)
	{
		return false;
	}

	sDetExtend.ucId         = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum]);
	sDetExtend.ucSensi      = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 1]);
	sDetExtend.ucGrpNo      = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 2]);
	sDetExtend.ucPro        = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 3]);
	sDetExtend.ucOcuDefault = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 4]);
	sDetExtend.usCarFlow    = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 5]);
	
	return true;
}

/**************************************************************
Function:       CGbtTscDb::QueryDetExtend
Description:    根据扩展ID号查询检测器扩展配置表数据
Input:          uDetId    -- 扩展ID号
				sDetExtend -- 检测器扩展配置结构体
Output:         无
Return:         true - 查询成功  false -查询失败
***************************************************************/
bool CGbtTscDb::QueryDetExtend(Byte uDetId, DetExtend& sDetExtend)
{
	char   sSql[SMALL_BUF_SIZE];
	CSqliteRst  qryRst;

	ACE_OS::sprintf(sSql, "select * from %s where %s=%u", 
							Table_Desc[TBL_DET_EXTEND].sTblName,
							Table_Desc[TBL_DET_EXTEND].sFieldName[0], 
							uDetId);

	if (!Query(sSql, qryRst))
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
		return false;
	}

	if (qryRst.m_iColum != Table_Desc[TBL_DET_EXTEND].iFieldCount)
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= %d\n", SHORT_FILE, __LINE__, Table_Desc[TBL_DET_EXTEND].sTblName, qryRst.m_iColum,Table_Desc[TBL_DET_EXTEND].iFieldCount));
		return false;
	}

	if ( qryRst.m_iRow <= 0 )
	{
		return false;
	}

	sDetExtend.ucId         = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum]);
	sDetExtend.ucSensi      = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 1]);
	sDetExtend.ucGrpNo      = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 2]);
	sDetExtend.ucPro        = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 3]);
	sDetExtend.ucOcuDefault = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 4]);
	sDetExtend.usCarFlow    = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 5]);

	return true;
}

/**************************************************************
Function:       CGbtTscDb::AddDetExtend
Description:    添加检测器扩展配置表数据
Input:         	sDetExtend -- 检测器扩展配置结构体
Output:         无
Return:         true - 添加扩展配置记录成功  false -添加失败
***************************************************************/
bool CGbtTscDb::AddDetExtend(DetExtend sDetExtend)
{
	char   sSql[LARGE_BUF_SIZE] = {0};
	
	ACE_OS::sprintf(sSql, "insert into %s(%s, %s, %s, %s, %s, %s,%s,%s,%s)values(%u,%u,%u,%u,%u,%u,%u,%u,%u)",
							Table_Desc[TBL_DET_EXTEND].sTblName,
							Table_Desc[TBL_DET_EXTEND].sFieldName[0],
							Table_Desc[TBL_DET_EXTEND].sFieldName[1], 
							Table_Desc[TBL_DET_EXTEND].sFieldName[2],
							Table_Desc[TBL_DET_EXTEND].sFieldName[3],
							Table_Desc[TBL_DET_EXTEND].sFieldName[4],
							Table_Desc[TBL_DET_EXTEND].sFieldName[5],
							Table_Desc[TBL_DET_EXTEND].sFieldName[6],
							Table_Desc[TBL_DET_EXTEND].sFieldName[7],
							Table_Desc[TBL_DET_EXTEND].sFieldName[8],
							sDetExtend.ucId,
							sDetExtend.ucSensi,
							sDetExtend.ucGrpNo,
							sDetExtend.ucPro,
							sDetExtend.ucOcuDefault,
							sDetExtend.usCarFlow,
		
							sDetExtend.ucFrency,
							sDetExtend.ucGrpData,
							sDetExtend.ucGrpDistns
							);

	if (!ExecuteCmd(sSql))
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
		return false;
	}
	return true;
}

/**************************************************************
Function:       CGbtTscDb::ModDetExtend
Description:    添加检测器扩展配置表数据
Input:         	sDetExtend -- 检测器扩展配置结构体
				uDetExtendId -- 扩展配置IDH号
Output:         无
Return:         true - 修改扩展配置记录成功  false -修改失败
***************************************************************/
bool CGbtTscDb::ModDetExtend(Byte uDetExtendId, DetExtend& sDetExtend)
{
	DetExtend sTemp;
	if (!QueryDetExtend(uDetExtendId, sTemp))
	{
		sDetExtend.ucId = uDetExtendId;
		return AddDetExtend(sDetExtend);
	}

	char   sSql[LARGE_BUF_SIZE];

	ACE_OS::sprintf(sSql, "update %s set %s=%u,%s=%u,%s=%u,%s=%u,%s=%u ,%s=%u,%s=%u,%s=%u where %s=%u",
							Table_Desc[TBL_DET_EXTEND].sTblName,
							Table_Desc[TBL_DET_EXTEND].sFieldName[3], sDetExtend.ucPro,
							Table_Desc[TBL_DET_EXTEND].sFieldName[1], sDetExtend.ucSensi,
							Table_Desc[TBL_DET_EXTEND].sFieldName[2], sDetExtend.ucGrpNo,
							Table_Desc[TBL_DET_EXTEND].sFieldName[4], sDetExtend.ucOcuDefault,
							Table_Desc[TBL_DET_EXTEND].sFieldName[5], sDetExtend.usCarFlow,
							Table_Desc[TBL_DET_EXTEND].sFieldName[6], sDetExtend.ucFrency,
							Table_Desc[TBL_DET_EXTEND].sFieldName[7], sDetExtend.ucGrpData,
							Table_Desc[TBL_DET_EXTEND].sFieldName[8], sDetExtend.ucGrpDistns,
							Table_Desc[TBL_DET_EXTEND].sFieldName[0], uDetExtendId);

	if (!ExecuteCmd(sSql))
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
		return false;
	}

	return true;
}

/**************************************************************
Function:       CGbtTscDb::QueryAdaptPara
Description:    查询自适应参数表数据
Input:         	tblAdaptPara -- 自适应参数表对象
Output:         无
Return:         true - 查询数据成功  false -查询数据失败
***************************************************************/
bool CGbtTscDb::QueryAdaptPara(TblAdaptPara& tblAdaptPara)
{
	int    i;
	Byte  uCount;
	char   sSql[SMALL_BUF_SIZE];
	CSqliteRst  qryRst;
	AdaptPara*  pData = NULL;

	ACE_OS::sprintf(sSql, "select * from %s order by %s", Table_Desc[TBL_ADAPT_PARA].sTblName,
						  Table_Desc[TBL_ADAPT_PARA].sFieldName[0]);

	if (!Query(sSql, qryRst))
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
		return false;
	}

	if (qryRst.m_iColum != Table_Desc[TBL_ADAPT_PARA].iFieldCount)
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= %d\n", SHORT_FILE, __LINE__, Table_Desc[TBL_ADAPT_PARA].sTblName, qryRst.m_iColum,Table_Desc[TBL_ADAPT_PARA].iFieldCount));
		return false;
	}

	if (qryRst.m_iRow <= 0)
	{
		return false;
	}

	uCount = (Byte)qryRst.m_iRow;
	if (!tblAdaptPara.AllocData(uCount))
	{
		return false;
	}

	pData = tblAdaptPara.GetData(uCount);
	for (i = 0; i < qryRst.m_iRow; i++)
	{
		pData[i].ucType       = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum]);
		pData[i].ucFirstPro   = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 1]);
		pData[i].ucSecPro     = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 2]);
		pData[i].ucThirdPro   = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 3]);
		pData[i].ucOcuPro     = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 4]);
		pData[i].ucCarFlowPro = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 5]);
		pData[i].ucSmoothPara = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 6]);
	}
	return true;
}

/**************************************************************
Function:       CGbtTscDb::QueryAdaptPara
Description:    根据自适应参数结构体数据查询自适应参数表数据
Input:         	sAdaptPara -- 自适应参数结构体
Output:         无
Return:         true - 查询数据成功  false -查询数据失败
***************************************************************/
bool CGbtTscDb::QueryAdaptPara(AdaptPara& sAdaptPara)
{
	char   sSql[SMALL_BUF_SIZE];
	CSqliteRst  qryRst;

	ACE_OS::sprintf(sSql, "select * from %s where %s=%d", 
							Table_Desc[TBL_ADAPT_PARA].sTblName,
							Table_Desc[TBL_ADAPT_PARA].sFieldName[0], 
							sAdaptPara.ucType);

	if (!Query(sSql, qryRst))
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
		return false;
	}

	if (qryRst.m_iColum != Table_Desc[TBL_ADAPT_PARA].iFieldCount)
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= %d\n", SHORT_FILE, __LINE__, Table_Desc[TBL_ADAPT_PARA].sTblName, qryRst.m_iColum,Table_Desc[TBL_ADAPT_PARA].iFieldCount));
		return false;
	}

	if (qryRst.m_iRow <= 0)
	{
		return false;
	}

	sAdaptPara.ucType       = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum]);
	sAdaptPara.ucFirstPro   = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 1]);
	sAdaptPara.ucSecPro     = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 2]);
	sAdaptPara.ucThirdPro   = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 3]);
	sAdaptPara.ucOcuPro     = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 4]);
	sAdaptPara.ucCarFlowPro = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 5]);
	sAdaptPara.ucSmoothPara = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 6]);

	return true;
}


/**************************************************************
Function:       CGbtTscDb::AddAdaptPara
Description:    添加自适应参数表数据
Input:         	sAdaptPara -- 自适应参数结构体
Output:         无
Return:         true - 添加数据成功  false -添加数据失败
***************************************************************/
bool CGbtTscDb::AddAdaptPara(AdaptPara sAdaptPara)
{
	char   sSql[LARGE_BUF_SIZE] = {0};

	ACE_OS::sprintf(sSql, "insert into %s(%s, %s, %s, %s, %s, %s, %s)values(%u,%u,%u,%u,%u,%u,%u)",
							Table_Desc[TBL_ADAPT_PARA].sTblName,
							Table_Desc[TBL_ADAPT_PARA].sFieldName[0],
							Table_Desc[TBL_ADAPT_PARA].sFieldName[1], 
							Table_Desc[TBL_ADAPT_PARA].sFieldName[2],
							Table_Desc[TBL_ADAPT_PARA].sFieldName[3],
							Table_Desc[TBL_ADAPT_PARA].sFieldName[4],
							Table_Desc[TBL_ADAPT_PARA].sFieldName[5],
							Table_Desc[TBL_ADAPT_PARA].sFieldName[6],
							sAdaptPara.ucType,
							sAdaptPara.ucFirstPro,
							sAdaptPara.ucSecPro,
							sAdaptPara.ucThirdPro,
							sAdaptPara.ucOcuPro,
							sAdaptPara.ucCarFlowPro,
							sAdaptPara.ucSmoothPara);

	if (!ExecuteCmd(sSql))
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
		return false;
	}
	return true;
}


/**************************************************************
Function:       CGbtTscDb::ModAdaptPara
Description:    修改自适应参数表数据
Input:         	sAdaptPara -- 待修改自适应参数结构体
Output:         无
Return:         true - 修改数据成功  false -修改数据失败
***************************************************************/
bool CGbtTscDb::ModAdaptPara(AdaptPara& sAdaptPara)
{
	AdaptPara sTemp;
	sTemp.ucType = sAdaptPara.ucType;
	if ( !QueryAdaptPara(sTemp) )
	{
		return AddAdaptPara(sAdaptPara);
	}

	char   sSql[LARGE_BUF_SIZE];

	ACE_OS::sprintf(sSql, "update %s set %s=%u,%s=%u,%s=%u,%s=%u,%s=%u,%s=%u where %s=%u",
							Table_Desc[TBL_ADAPT_PARA].sTblName,
							Table_Desc[TBL_ADAPT_PARA].sFieldName[1], sAdaptPara.ucFirstPro,
							Table_Desc[TBL_ADAPT_PARA].sFieldName[2], sAdaptPara.ucSecPro,
							Table_Desc[TBL_ADAPT_PARA].sFieldName[3], sAdaptPara.ucThirdPro,
							Table_Desc[TBL_ADAPT_PARA].sFieldName[4], sAdaptPara.ucOcuPro,
							Table_Desc[TBL_ADAPT_PARA].sFieldName[5], sAdaptPara.ucCarFlowPro,
							Table_Desc[TBL_ADAPT_PARA].sFieldName[6], sAdaptPara.ucSmoothPara,
							Table_Desc[TBL_ADAPT_PARA].sFieldName[0], sAdaptPara.ucType);

	if (!ExecuteCmd(sSql))
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
		return false;
	}

	return true;
}

/**************************************************************
Function:       CGbtTscDb::QueryCntDownDev
Description:    倒计时设备表数据查询
Input:         	tblCntDownDev -- 倒计时设备表对象
Output:         无
Return:         true - 查询数据成功  false -查询数据失败
***************************************************************/
bool CGbtTscDb::QueryCntDownDev(TblCntDownDev& tblCntDownDev)
{
	int    i;
	Byte  uCount;
	char   sSql[SMALL_BUF_SIZE];
	CSqliteRst  qryRst;
	CntDownDev*  pData = NULL;

	ACE_OS::sprintf(sSql, "select * from %s order by %s", 
							Table_Desc[TBL_CNTDOWNDEV].sTblName,
							Table_Desc[TBL_CNTDOWNDEV].sFieldName[0]);

	if (!Query(sSql, qryRst))
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
		return false;
	}

	if (qryRst.m_iColum != Table_Desc[TBL_CNTDOWNDEV].iFieldCount)
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= %d\n", SHORT_FILE, __LINE__, Table_Desc[TBL_CNTDOWNDEV].sTblName, qryRst.m_iColum,Table_Desc[TBL_CNTDOWNDEV].iFieldCount));
		return false;
	}

	if (qryRst.m_iRow <= 0)
	{
		return false;
	}

	uCount = (Byte)qryRst.m_iRow;
	if (!tblCntDownDev.AllocData(uCount))
	{
		return false;
	}

	pData = tblCntDownDev.GetData(uCount);
	for (i = 0; i < qryRst.m_iRow; i++)
	{
		pData[i].ucDevId        = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum]);
		pData[i].usPhase        = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 1]);
		pData[i].ucOverlapPhase = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 2]);
		pData[i].ucMode         = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 3]);
	}
	return true;
}


/**************************************************************
Function:       CGbtTscDb::QueryCntDownDev
Description:    根据设备号查询倒计时设备表数据
Input:         	ucDevId -- 设备ID号
				sCntDownDev -- 倒计时设备结构体
Output:         无
Return:         true - 查询数据成功  false -查询数据失败
***************************************************************/
bool CGbtTscDb::QueryCntDownDev(Byte ucDevId, CntDownDev& sCntDownDev)
{
	char   sSql[SMALL_BUF_SIZE];
	CSqliteRst  qryRst;

	ACE_OS::sprintf(sSql, "select * from %s where %s=%d", 
							Table_Desc[TBL_CNTDOWNDEV].sTblName,
							Table_Desc[TBL_CNTDOWNDEV].sFieldName[0], 
							ucDevId);

	if ( !Query(sSql, qryRst) )
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
		return false;
	}

	if (qryRst.m_iColum != Table_Desc[TBL_CNTDOWNDEV].iFieldCount)
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= %d\n", SHORT_FILE, __LINE__, Table_Desc[TBL_CNTDOWNDEV].sTblName, qryRst.m_iColum,Table_Desc[TBL_CNTDOWNDEV].iFieldCount));
		return false;
	}

	if (qryRst.m_iRow <= 0)
	{
		return false;
	}

	sCntDownDev.ucDevId        = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum]);
	sCntDownDev.usPhase        = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 1]);
	sCntDownDev.ucOverlapPhase = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 1]);
	sCntDownDev.ucMode         = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 2]);

	return true;
}

/**************************************************************
Function:       CGbtTscDb::AddCntDownDev
Description:    添加倒计时设备表数据
Input:         	sCntDownDev -- 倒计时设备结构体
Output:         无
Return:         true - 添加数据成功  false -添加数据失败
***************************************************************/
bool CGbtTscDb::AddCntDownDev(CntDownDev& sCntDownDev)
{
	char   sSql[LARGE_BUF_SIZE];
	
	ACE_OS::sprintf(sSql, "insert into %s(%s, %s, %s, %s)values(%u,%u,%u,%u)",
							Table_Desc[TBL_CNTDOWNDEV].sTblName,
							Table_Desc[TBL_CNTDOWNDEV].sFieldName[0],
							Table_Desc[TBL_CNTDOWNDEV].sFieldName[1], 
							Table_Desc[TBL_CNTDOWNDEV].sFieldName[2],
							Table_Desc[TBL_CNTDOWNDEV].sFieldName[3],
							sCntDownDev.ucDevId,
							sCntDownDev.usPhase,
							sCntDownDev.ucOverlapPhase,
							sCntDownDev.ucMode);

	if (!ExecuteCmd(sSql))
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
		return false;
	}
	return true;
}

/**************************************************************
Function:       CGbtTscDb::DelCntDownDev
Description:    根据设备号删除倒计时设备表数据记录
Input:         	ucDevId -- 倒计时设备号
Output:         无
Return:         true - 删除数据成功  false -删除数据失败
***************************************************************/
bool CGbtTscDb::DelCntDownDev(Byte ucDevId)
{
	char   sSql[LARGE_BUF_SIZE];
	ACE_OS::sprintf(sSql, "delete from %s where %s=%u",
							Table_Desc[TBL_CNTDOWNDEV].sTblName,
							Table_Desc[TBL_CNTDOWNDEV].sFieldName[0], 
							ucDevId);

	if (!ExecuteCmd(sSql))
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
		return false;
	}

	return true;
}

/**************************************************************
Function:       CGbtTscDb::ModCntDownDev
Description:    根据设备号修改倒计时设备表数据记录
Input:         	ucDevId -- 倒计时设备号
				sCntDownDev -- 倒计时设备结构体
Output:         无
Return:         true - 修改数据成功  false -修改数据失败
***************************************************************/
bool CGbtTscDb::ModCntDownDev(Byte ucDevId, CntDownDev& sCntDownDev)
{
	CntDownDev sTemp;
	if (!QueryCntDownDev(ucDevId, sTemp))
	{
		return AddCntDownDev(sCntDownDev);
	}

	char   sSql[LARGE_BUF_SIZE];

	ACE_OS::sprintf(sSql, "update %s set %s=%u, %s=%u, %s=%u where %s=%u",
							Table_Desc[TBL_CNTDOWNDEV].sTblName,
							Table_Desc[TBL_CNTDOWNDEV].sFieldName[1] , sCntDownDev.usPhase,
							Table_Desc[TBL_CNTDOWNDEV].sFieldName[2] , sCntDownDev.ucOverlapPhase,
							Table_Desc[TBL_CNTDOWNDEV].sFieldName[3] , sCntDownDev.ucMode,
							Table_Desc[TBL_CNTDOWNDEV].sFieldName[0] , sCntDownDev.ucDevId);

	if (!ExecuteCmd(sSql))
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
		return false;
	}

	return true;
}

/**************************************************************
Function:       CGbtTscDb::QuerySpecFun
Description:    根据特殊功能结构体查询特殊功能表
Input:         	sSpecFun -- 特殊功能结构体数据
Output:         无
Return:         true - 查询数据成功  false -查询数据失败
***************************************************************/
bool CGbtTscDb::QuerySpecFun(SpecFun& sSpecFun)
{
	char   sSql[SMALL_BUF_SIZE];
	CSqliteRst  qryRst;

	ACE_OS::sprintf(sSql, "select * from %s where %s=%d", 
							Table_Desc[TBL_SPECFUN].sTblName,
							Table_Desc[TBL_SPECFUN].sFieldName[0], 
							sSpecFun.ucFunType);

	if ( !Query(sSql, qryRst) )
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
		return false;
	}

	if ( qryRst.m_iColum != Table_Desc[TBL_SPECFUN].iFieldCount )
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= %d\n", SHORT_FILE, __LINE__, Table_Desc[TBL_SPECFUN].sTblName, qryRst.m_iColum,Table_Desc[TBL_SPECFUN].iFieldCount));
		return false;
	}

	if ( qryRst.m_iRow <= 0 )
	{
		return false;
	}

	sSpecFun.ucFunType = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum]);
	sSpecFun.ucValue   = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 1]);

	return true;
}

/**************************************************************
Function:       CGbtTscDb::QuerySpecFun
Description:    查询全部特殊功能表数据
Input:         	tblSpecFun -- 特殊功能表对象
Output:         无
Return:         true - 查询数据成功  false -查询数据失败
***************************************************************/
bool CGbtTscDb::QuerySpecFun(TblSpecFun& tblSpecFun)
{
	int    i;
	Byte  uCount;
	char   sSql[SMALL_BUF_SIZE];
	CSqliteRst  qryRst;
	SpecFun*  pData = NULL;

	ACE_OS::sprintf(sSql, "select * from %s order by %s", 
							Table_Desc[TBL_SPECFUN].sTblName,
							Table_Desc[TBL_SPECFUN].sFieldName[0]);

	if (!Query(sSql, qryRst))
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
		return false;
	}

	if (qryRst.m_iColum != Table_Desc[TBL_SPECFUN].iFieldCount)
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= %d\n", SHORT_FILE, __LINE__, Table_Desc[TBL_SPECFUN].sTblName, qryRst.m_iColum,Table_Desc[TBL_SPECFUN].iFieldCount));
		return false;
	}

	if (qryRst.m_iRow <= 0)
	{
		return false;
	}

	uCount = (Byte)qryRst.m_iRow;
	if (!tblSpecFun.AllocData(uCount))
	{
		return false;
	}

	pData = tblSpecFun.GetData(uCount);
	for (i = 0; i < qryRst.m_iRow; i++)
	{
		pData[i].ucFunType = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum]);
		pData[i].ucValue   = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 1]);
	}

	return true;
}

/**************************************************************
Function:       CGbtTscDb::AddSpecFun
Description:    添加特殊功能表数据
Input:         	tblSpecFun -- 特殊功能表对象
Output:         无
Return:         true - 添加数据成功  false -添加数据失败
***************************************************************/
bool CGbtTscDb::AddSpecFun(Byte ucFunType,Byte ucValue)
{
	char   sSql[LARGE_BUF_SIZE];

	ACE_OS::sprintf(sSql, "insert into %s(%s, %s)values(%u,%u)",
							Table_Desc[TBL_SPECFUN].sTblName,
							Table_Desc[TBL_SPECFUN].sFieldName[0],
							Table_Desc[TBL_SPECFUN].sFieldName[1],
							ucFunType,
							ucValue);

	if (!ExecuteCmd(sSql))
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
		return false;
	}

	return true;
}

/**************************************************************
Function:       CGbtTscDb::ModSpecFun
Description:    根据特殊功能类型修改特殊功能表数据
Input:         	ucFunType -- 特殊功能类型
				ucValue   -- 修改值
Output:         无
Return:         true - 修改数据成功  false -修改数据失败
***************************************************************/
bool CGbtTscDb::ModSpecFun(Byte ucFunType,Byte ucValue)
{
	SpecFun sTemp;
	sTemp.ucFunType = ucFunType;
	if ( !CGbtTscDb::QuerySpecFun(sTemp) )
	{
		return AddSpecFun(ucFunType,ucValue);
	}

	char   sSql[LARGE_BUF_SIZE] = {0};

	ACE_OS::sprintf(sSql, "update %s set %s=%u where %s=%u",
							Table_Desc[TBL_SPECFUN].sTblName,
							Table_Desc[TBL_SPECFUN].sFieldName[1] , 
							ucValue,
							Table_Desc[TBL_SPECFUN].sFieldName[0] , 
							ucFunType);

	if (!ExecuteCmd(sSql))
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
		return false;
	}

	return true;
}

/**************************************************************
Function:       CGbtTscDb::QueryChannel
Description:    信号机通道查询
Input:         	tblChannel -- 通道数据表对象
Output:         无
Return:         true - 查询数据成功  false -查询数据失败
***************************************************************/
bool CGbtTscDb::QueryChannel(TblChannel& tblChannel)
{
    int    i;
    Byte  uCount;
    char   sSql[SMALL_BUF_SIZE];
    CSqliteRst  qryRst;
    Channel*  pData = NULL;

    ACE_OS::sprintf(sSql, "select * from %s order by %s", 
						    Table_Desc[TBL_CHANNEL].sTblName,
						    Table_Desc[TBL_CHANNEL].sFieldName[0]);

    if (!Query(sSql, qryRst))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
        return false;
    }

    if (qryRst.m_iColum != Table_Desc[TBL_CHANNEL].iFieldCount)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= %d\n", SHORT_FILE, __LINE__, Table_Desc[TBL_CHANNEL].sTblName, qryRst.m_iColum,Table_Desc[TBL_CHANNEL].iFieldCount));
        return false;
    }

    if (qryRst.m_iRow <= 0)
    {
        return false;
    }

    uCount = (Byte)qryRst.m_iRow;
    if (!tblChannel.AllocData(uCount))
    {
        return false;
    }

    pData = tblChannel.GetData(uCount);
    for (i = 0; i < qryRst.m_iRow; i++)
    {
        pData[i].ucChannelId        = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum]);
        pData[i].ucCtrlSrc          = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 1]);
        pData[i].ucAutoFlsCtrlFlag  = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 2]);
        pData[i].ucCtrlType         = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 3]);
    }
    return true;
}

/**************************************************************
Function:       CGbtTscDb::QueryChannel
Description:    根据通道IDH号信号机通道查询
Input:         	uChannelId -- 通道ID号
				sChannel   -- 通道结构体数据
Output:         无
Return:         true - 查询数据成功  false -查询数据失败
***************************************************************/
bool CGbtTscDb::QueryChannel(Byte uChannelId, Channel& sChannel)
{
     char   sSql[SMALL_BUF_SIZE];
    CSqliteRst  qryRst;

    ACE_OS::sprintf(sSql, "select * from %s where %s=%d", 
						    Table_Desc[TBL_CHANNEL].sTblName,
						    Table_Desc[TBL_CHANNEL].sFieldName[0], 
						    uChannelId);

    if (!Query(sSql, qryRst))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
        return false;
    }

    if (qryRst.m_iColum != Table_Desc[TBL_CHANNEL].iFieldCount)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= %d\n", SHORT_FILE, __LINE__, Table_Desc[TBL_CHANNEL].sTblName, qryRst.m_iColum,Table_Desc[TBL_CHANNEL].iFieldCount));
        return false;
    }

    if (qryRst.m_iRow <= 0)
    {
        return false;
    }

    sChannel.ucChannelId    = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum]);
    sChannel.ucCtrlSrc      = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 1]);
    sChannel.ucAutoFlsCtrlFlag = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 2]);
    sChannel.ucCtrlType     = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 3]);
    return true;
}
    
/**************************************************************
Function:       CGbtTscDb::AddChannel
Description:    添加信号机通道表记录数据
Input:         	uChannelId -- 通道ID号
				sChannel   -- 通道结构体数据
Output:         无
Return:         true - 添加数据成功  false -添加数据失败
***************************************************************/
bool CGbtTscDb::AddChannel(Byte uChannelId, Channel& sChannel)
{
    char   sSql[LARGE_BUF_SIZE];
    if (!IsChannelValid(uChannelId, sChannel))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tChannel invalid\n", SHORT_FILE, __LINE__));
        return false;
    }
    ACE_OS::sprintf(sSql, "insert into %s(%s, %s, %s, %s)values(%u,%u,%u,%u)",
				           Table_Desc[TBL_CHANNEL].sTblName,
				           Table_Desc[TBL_CHANNEL].sFieldName[0],
				           Table_Desc[TBL_CHANNEL].sFieldName[1], 
				           Table_Desc[TBL_CHANNEL].sFieldName[2],
				           Table_Desc[TBL_CHANNEL].sFieldName[3],
				           uChannelId,
				           sChannel.ucCtrlSrc,
				           sChannel.ucAutoFlsCtrlFlag,
				           sChannel.ucCtrlType);

    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }
    return true;
}
    
/**************************************************************
Function:       CGbtTscDb::ModChannel
Description:    修改信号机通道表记录数据
Input:         	uChannelId -- 通道ID号
				sChannel   -- 待修改通道结构体数据
Output:         无
Return:         true - 修改数据成功  false -修改数据失败
***************************************************************/
bool CGbtTscDb::ModChannel(Byte uChannelId, Channel& sChannel)
{
    Channel sTemp;
    if (!QueryChannel(uChannelId, sTemp))
    {
        return AddChannel(uChannelId, sChannel);
    }

    char   sSql[LARGE_BUF_SIZE];
    if (!IsChannelValid(uChannelId, sChannel))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tChannel invalid\n", SHORT_FILE, __LINE__));
        return false;
    }
    ACE_OS::sprintf(sSql, "update %s set %s=%u, %s=%u, %s=%u where %s=%u",
				           Table_Desc[TBL_CHANNEL].sTblName,
				           Table_Desc[TBL_CHANNEL].sFieldName[1], sChannel.ucCtrlSrc,
				           Table_Desc[TBL_CHANNEL].sFieldName[2], sChannel.ucAutoFlsCtrlFlag,
				           Table_Desc[TBL_CHANNEL].sFieldName[3], sChannel.ucCtrlType,
				           Table_Desc[TBL_CHANNEL].sFieldName[0], uChannelId);

    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }

    return true;
}

/**************************************************************
Function:       CGbtTscDb::DelChannel
Description:    根据通道ID号删除信号机通道表记录数据
Input:         	uChannelId -- 通道ID号
Output:         无
Return:         true - 删除数据成功  false -删除数据失败
***************************************************************/
bool CGbtTscDb::DelChannel(Byte uChannelId)
{
    char   sSql[LARGE_BUF_SIZE];
    ACE_OS::sprintf(sSql, "delete from %s where %s=%u",
				           Table_Desc[TBL_CHANNEL].sTblName,
				           Table_Desc[TBL_CHANNEL].sFieldName[0], 
				           uChannelId);

    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }

    return true;
}

/**************************************************************
Function:       CGbtTscDb::DelChannel
Description:    删除全部信号机通道表记录数据
Input:         	无
Output:         无
Return:         true - 删除数据成功  false -删除数据失败
***************************************************************/
bool CGbtTscDb::DelChannel()
{
	char   sSql[LARGE_BUF_SIZE];
	ACE_OS::sprintf(sSql, "delete from %s",	Table_Desc[TBL_CHANNEL].sTblName);

	if (!ExecuteCmd(sSql))
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
		return false;
	}

	return true;
}

/**************************************************************
Function:       CGbtTscDb::IsChannelValid
Description:    判断信号及通道数据合法性
Input:         	uChannelId -- 通道ID号
				sChannel   -- 待修改通道结构体数据
Output:         无
Return:         true - 数据合法 false -数据非法
***************************************************************/
bool CGbtTscDb::IsChannelValid(Byte uChannelId, Channel& sChannel)
{
    CHECK_ID(CONST_CHANNEL, uChannelId, false);

    if (sChannel.ucCtrlSrc > 16)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tucCtrlSrc=%d,invalid\n", SHORT_FILE, __LINE__, sChannel.ucCtrlSrc));
        return false;
    }

   // if (sChannel.ucCtrlType > 4 || 0 == sChannel.ucCtrlType) MOD:201311131500
	if (sChannel.ucCtrlType > 4)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tucCtrlType=%d,invalid\n", SHORT_FILE, __LINE__, sChannel.ucCtrlType));
        return false;
    }

    return true;
}

/**************************************************************
Function:       CGbtTscDb::QueryPattern
Description:    查询配时方案
Input:          tblPattern  -- 配时方案表对象
Output:         无
Return:         true - 查询成功 false -查询失败
***************************************************************/
bool CGbtTscDb::QueryPattern(TblPattern& tblPattern)
{
    int    i;
    Byte  uCount;
    char   sSql[SMALL_BUF_SIZE];
    CSqliteRst  qryRst;
    Pattern*  pData = NULL;

    ACE_OS::sprintf(sSql, "select * from %s order by %s", 
						    Table_Desc[TBL_PATTERN].sTblName,
						    Table_Desc[TBL_PATTERN].sFieldName[0]);

    if (!Query(sSql, qryRst))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
        return false;
    }

    if (qryRst.m_iColum != Table_Desc[TBL_PATTERN].iFieldCount)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= %d\n", SHORT_FILE, __LINE__, Table_Desc[TBL_PATTERN].sTblName, qryRst.m_iColum,Table_Desc[TBL_PATTERN].iFieldCount));
        return false;
    }

    if (qryRst.m_iRow <= 0)
    {
        return false;
    }

    uCount = (Byte)qryRst.m_iRow;
    if (!tblPattern.AllocData(uCount))
    {
        return false;
    }

    pData = tblPattern.GetData(uCount);
    for (i = 0; i < qryRst.m_iRow; i++)
    {
        pData[i].ucPatternId        = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum]);
        pData[i].ucCycleTime        = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 1]);
        pData[i].ucOffset           = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 2]);
        pData[i].ucCoorPhase        = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 3]);
        pData[i].ucStagePatternId   = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 4]);
    }
    return true;
}

/**************************************************************
Function:       CGbtTscDb::QueryPattern
Description:    根据配时方案号查询配时方案表数据
Input:          uPatternId  -- 配时方案号
				sPattern    -- 配时方案结构体
Output:         无
Return:         true - 查询成功 false -查询失败
***************************************************************/
bool CGbtTscDb::QueryPattern(Byte uPatternId, Pattern& sPattern)
{
    char   sSql[SMALL_BUF_SIZE];
    CSqliteRst  qryRst;

    ACE_OS::sprintf(sSql, "select * from %s where %s=%d", 
						    Table_Desc[TBL_PATTERN].sTblName,
						    Table_Desc[TBL_PATTERN].sFieldName[0], uPatternId);

    if (!Query(sSql, qryRst))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
        return false;
    }

    if (qryRst.m_iColum != Table_Desc[TBL_PATTERN].iFieldCount)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= %d\n", SHORT_FILE, __LINE__, Table_Desc[TBL_PATTERN].sTblName, qryRst.m_iColum,Table_Desc[TBL_PATTERN].iFieldCount));
        return false;
    }

    if (qryRst.m_iRow <= 0)
    {
        return false;
    }

    sPattern.ucPatternId        = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum]);
    sPattern.ucCycleTime        = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 1]);
    sPattern.ucOffset           = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 2]);
    sPattern.ucCoorPhase        = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 3]);
    sPattern.ucStagePatternId   = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 4]);
    return true;
}
    
/**************************************************************
Function:       CGbtTscDb::AddPattern
Description:    根据配时方案号添加配时方案表数据
Input:          uPatternId  -- 配时方案号
				sPattern    -- 配时方案结构体
Output:         无
Return:         true - 添加数据成功 false -添加数据失败
***************************************************************/
bool CGbtTscDb::AddPattern(Byte uPatternId, Pattern& sPattern)
{
    char   sSql[LARGE_BUF_SIZE];
    if (!IsPatternValid(uPatternId, sPattern))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tPattern invalid\n", SHORT_FILE, __LINE__));
        return false;
    }
    ACE_OS::sprintf(sSql, "insert into %s(%s, %s, %s, %s, %s)values(%u,%u,%u,%u,%u)",
				           Table_Desc[TBL_PATTERN].sTblName,
				           Table_Desc[TBL_PATTERN].sFieldName[0],
				           Table_Desc[TBL_PATTERN].sFieldName[1], 
				           Table_Desc[TBL_PATTERN].sFieldName[2],
				           Table_Desc[TBL_PATTERN].sFieldName[3],
				           Table_Desc[TBL_PATTERN].sFieldName[5],
				           uPatternId,
				           sPattern.ucCycleTime,
				           sPattern.ucOffset,
				           sPattern.ucCoorPhase,
				           sPattern.ucStagePatternId);

    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }
    return true;
}
    
/**************************************************************
Function:       CGbtTscDb::ModPattern
Description:    根据配时方案号修改配时方案表数据
Input:          uPatternId  -- 配时方案号
				sPattern    -- 配时方案结构体
Output:         无
Return:         true - 修改数据成功 false -修改数据失败
***************************************************************/
bool CGbtTscDb::ModPattern(Byte uPatternId, Pattern& sPattern)
{
    Pattern sTemp;
    if (!QueryPattern(uPatternId, sTemp))
    {
        return AddPattern(uPatternId, sPattern);
    }

    char   sSql[LARGE_BUF_SIZE];
    if (!IsPatternValid(uPatternId, sPattern))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tPattern invalid\n", SHORT_FILE, __LINE__));
        return false;
    }
    ACE_OS::sprintf(sSql, "update %s set %s=%u, %s=%u, %s=%u, %s=%u where %s=%u",
				           Table_Desc[TBL_PATTERN].sTblName,
				           Table_Desc[TBL_PATTERN].sFieldName[1], sPattern.ucCycleTime,
				           Table_Desc[TBL_PATTERN].sFieldName[2], sPattern.ucOffset,
				           Table_Desc[TBL_PATTERN].sFieldName[3], sPattern.ucCoorPhase,
				           Table_Desc[TBL_PATTERN].sFieldName[4], sPattern.ucStagePatternId,
				           Table_Desc[TBL_PATTERN].sFieldName[0], uPatternId);

    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }

    return true;
}

/**************************************************************
Function:       CGbtTscDb::DelPattern
Description:    根据配时方案号删除配时方案表数据
Input:          uPatternId  -- 配时方案号
				sPattern    -- 配时方案结构体
Output:         无
Return:         true - 删除数据成功 false -删除数据失败
***************************************************************/
bool CGbtTscDb::DelPattern(Byte uPatternId)
{
    char   sSql[LARGE_BUF_SIZE];
    ACE_OS::sprintf(sSql, "delete from %s where %s=%u",
                   Table_Desc[TBL_PATTERN].sTblName,
                   Table_Desc[TBL_PATTERN].sFieldName[0], uPatternId);

    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }

    return true;
}

/**************************************************************
Function:       CGbtTscDb::DelPattern
Description:    删除全部配时方案表数据
Input:          无
Output:         无
Return:         true - 删除数据成功 false -删除数据失败
***************************************************************/
bool CGbtTscDb::DelPattern()
{
	char   sSql[LARGE_BUF_SIZE];
	ACE_OS::sprintf(sSql, "delete from %s",	Table_Desc[TBL_PATTERN].sTblName);

	if (!ExecuteCmd(sSql))
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
		return false;
	}

	return true;
}

/**************************************************************
Function:       CGbtTscDb::IsPatternValid
Description:    判断配时方案数据合法性
Input:          uPatternId  -- 配时方案号
				sPattern    -- 配时方案结构体
Output:         无
Return:         true - 数据合法 false -数据非法
***************************************************************/
bool CGbtTscDb::IsPatternValid(Byte uPatternId, Pattern& sPattern)
{
    CHECK_ID(CONST_PATTERN, uPatternId, false);

    if (sPattern.ucCoorPhase > 16)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tucCoorPhase=%d,invalid\n", SHORT_FILE, __LINE__, sPattern.ucCoorPhase));
        return false;
    }
    if (sPattern.ucStagePatternId > 16)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tucStagePatternId=%d,invalid\n", SHORT_FILE, __LINE__, sPattern.ucStagePatternId));
        return false;
    }

    return true;
}

/**************************************************************
Function:       CGbtTscDb::QueryStagePattern
Description:    查询阶段配时表数据
Input:          tblStagePattern  -- 阶段配时表对象
Output:         无
Return:         true - 查询数据成功 false -查询数据失败
***************************************************************/
bool CGbtTscDb::QueryStagePattern(TblStagePattern& tblStagePattern)
{
    int    i;
    Ushort  uCount;
    char   sSql[SMALL_BUF_SIZE];
    CSqliteRst  qryRst;
    StagePattern*  pData = NULL;

    ACE_OS::sprintf(sSql, "select * from %s order by %s, %s", 
						    Table_Desc[TBL_STAGEPATTERN].sTblName,
						    Table_Desc[TBL_STAGEPATTERN].sFieldName[0],
						    Table_Desc[TBL_STAGEPATTERN].sFieldName[1]);

    if (!Query(sSql, qryRst))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
        return false;
    }

    if (qryRst.m_iColum != Table_Desc[TBL_STAGEPATTERN].iFieldCount)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= %d\n", SHORT_FILE, __LINE__, Table_Desc[TBL_STAGEPATTERN].sTblName, qryRst.m_iColum,Table_Desc[TBL_STAGEPATTERN].iFieldCount));
        return false;
    }

    if (qryRst.m_iRow <= 0)
    {
        return false;
    }

    uCount = (Byte)qryRst.m_iRow;
    if (!tblStagePattern.AllocData(uCount))
    {
        return false;
    }

    pData = tblStagePattern.GetData(uCount);
    for (i = 0; i < qryRst.m_iRow; i++)
    {
        pData[i].ucStagePatternId   = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum]);
        pData[i].ucStageNo          = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 1]);
        pData[i].usAllowPhase       = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 2]);
        pData[i].ucGreenTime        = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 3]);
        pData[i].ucYellowTime       = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 4]);
        pData[i].ucRedTime          = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 5]);
        pData[i].ucOption           = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 6]);
    }
    return true;
}

/**************************************************************
Function:       CGbtTscDb::QueryStagePattern
Description:    根据阶段配时号查询阶段配时表数据
Input:          uStagePatternId  -- 阶段配时号
				tblStagePattern  -- 阶段配时表对象
Output:         无
Return:         true - 查询数据成功 false -查询数据失败
***************************************************************/
bool CGbtTscDb::QueryStagePattern(Byte uStagePatternId, TblStagePattern& tblStagePattern)
{
    int    i;
    Ushort  uCount;
    char   sSql[SMALL_BUF_SIZE];
    CSqliteRst  qryRst;
    StagePattern*  pData = NULL;

    ACE_OS::sprintf(sSql, "select * from %s where %s=%u order by %s", 
						    Table_Desc[TBL_STAGEPATTERN].sTblName,
						    Table_Desc[TBL_STAGEPATTERN].sFieldName[0], 
						    uStagePatternId,
						    Table_Desc[TBL_STAGEPATTERN].sFieldName[1]);

    if (!Query(sSql, qryRst))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
        return false;
    }

    if (qryRst.m_iRow <= 0)
    {
        return false;
    }

    if (qryRst.m_iColum != Table_Desc[TBL_STAGEPATTERN].iFieldCount)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= %d\n", SHORT_FILE, __LINE__, Table_Desc[TBL_STAGEPATTERN].sTblName, qryRst.m_iColum,Table_Desc[TBL_STAGEPATTERN].iFieldCount));
        return false;
    }

    uCount = (Byte)qryRst.m_iRow;
    if (!tblStagePattern.AllocData(uCount))
    {
        return false;
    }

    pData = tblStagePattern.GetData(uCount);
    for (i = 0; i < qryRst.m_iRow; i++)
    {
        pData[i].ucStagePatternId   = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum]);
        pData[i].ucStageNo          = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 1]);
        pData[i].usAllowPhase       = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 2]);
        pData[i].ucGreenTime        = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 3]);
        pData[i].ucYellowTime       = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 4]);
        pData[i].ucRedTime          = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 5]);
        pData[i].ucOption           = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 6]);
    }
    return true;
}

/**************************************************************
Function:       CGbtTscDb::QueryStagePattern
Description:    根据阶段配时表号，阶段号查询阶段配时表数据
Input:          uStagePatternId  -- 阶段配时号
				uStageNo         -- 阶段号
				sStagePattern    -- 阶段配时结构体
Output:         无
Return:         true - 查询数据成功 false -查询数据失败
***************************************************************/
bool CGbtTscDb::QueryStagePattern(Byte uStagePatternId, Byte uStageNo, StagePattern& sStagePattern)
{
    char   sSql[SMALL_BUF_SIZE];
    CSqliteRst  qryRst;

    ACE_OS::sprintf(sSql, "select * from %s where %s=%u and %s=%u", 
						    Table_Desc[TBL_STAGEPATTERN].sTblName,
						    Table_Desc[TBL_STAGEPATTERN].sFieldName[0], 
						    uStagePatternId,
						    Table_Desc[TBL_STAGEPATTERN].sFieldName[1], 
						    uStageNo);

    if (!Query(sSql, qryRst))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
        return false;
    }

    if (qryRst.m_iColum != Table_Desc[TBL_STAGEPATTERN].iFieldCount)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= %d\n", SHORT_FILE, __LINE__, Table_Desc[TBL_STAGEPATTERN].sTblName, qryRst.m_iColum,Table_Desc[TBL_STAGEPATTERN].iFieldCount));
        return false;
    }

    if (qryRst.m_iRow <= 0)
    {
        return false;
    }

    sStagePattern.ucStagePatternId   = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum]);
    sStagePattern.ucStageNo          = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 1]);
    sStagePattern.usAllowPhase       = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 2]);
    sStagePattern.ucGreenTime        = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 3]);
    sStagePattern.ucYellowTime       = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 4]);
    sStagePattern.ucRedTime          = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 5]);
    sStagePattern.ucOption           = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 6]);

    return true;
}

/**************************************************************
Function:       CGbtTscDb::AddStagePattern
Description:    添加阶段配时表数据记录
Input:          uStagePatternId  -- 阶段配时号
				uStageNo         -- 阶段号
				sStagePattern    -- 阶段配时结构体
Output:         无
Return:         true - 添加数据成功 false -添加数据失败
***************************************************************/
bool CGbtTscDb::AddStagePattern(Byte uStagePatternId, Byte uStageNo, StagePattern& sStagePattern)
{
    char   sSql[LARGE_BUF_SIZE];

	if ( 0 == uStageNo )  //阶段号不能为0
	{
		return true;
	}

    if (!IsStagePatternValid(uStagePatternId, uStageNo, sStagePattern))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tStagePattern invalid\n", SHORT_FILE, __LINE__));
        return false;
    }
    ACE_OS::sprintf(sSql, "insert into %s(%s, %s, %s, %s, %s, %s, %s)values(%u,%u,%u,%u,%u,%u,%u)",
				           Table_Desc[TBL_STAGEPATTERN].sTblName,
				           Table_Desc[TBL_STAGEPATTERN].sFieldName[0],
				           Table_Desc[TBL_STAGEPATTERN].sFieldName[1], 
				           Table_Desc[TBL_STAGEPATTERN].sFieldName[2],
				           Table_Desc[TBL_STAGEPATTERN].sFieldName[3],
				           Table_Desc[TBL_STAGEPATTERN].sFieldName[4],
				           Table_Desc[TBL_STAGEPATTERN].sFieldName[5],
				           Table_Desc[TBL_STAGEPATTERN].sFieldName[6],
				           uStagePatternId,
				           uStageNo,
				           sStagePattern.usAllowPhase,
				           sStagePattern.ucGreenTime,
				           sStagePattern.ucYellowTime,
				           sStagePattern.ucRedTime,
				           sStagePattern.ucOption);


    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }
    return true;
}

/**************************************************************
Function:       CGbtTscDb::ModStagePattern
Description:    根据阶段配时号，阶段号修改配时表数据记录
Input:          uStagePatternId  -- 阶段配时号
				uStageNo         -- 阶段号
				sStagePattern    -- 阶段配时结构体
Output:         无
Return:         true - 修改数据成功 false -修改数据失败
***************************************************************/
bool CGbtTscDb::ModStagePattern(Byte uStagePatternId, Byte uStageNo, StagePattern& sStage)
{
    StagePattern sTemp;
    if (!QueryStagePattern(uStagePatternId, uStageNo, sTemp))
    {
        return AddStagePattern(uStagePatternId, uStageNo,sStage);
    }

    char   sSql[LARGE_BUF_SIZE];
    if (!IsStagePatternValid(uStagePatternId, uStageNo, sStage))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tStagePattern invalid\n", SHORT_FILE, __LINE__));
        return false;
    }
    ACE_OS::sprintf(sSql, "update %s set %s=%u, %s=%u, %s=%u, %s=%u, %s=%u where %s=%u and %s=%u",
				           Table_Desc[TBL_STAGEPATTERN].sTblName,
				           Table_Desc[TBL_STAGEPATTERN].sFieldName[2], sStage.usAllowPhase,
				           Table_Desc[TBL_STAGEPATTERN].sFieldName[3], sStage.ucGreenTime,
				           Table_Desc[TBL_STAGEPATTERN].sFieldName[4], sStage.ucYellowTime,
				           Table_Desc[TBL_STAGEPATTERN].sFieldName[5], sStage.ucRedTime,
				           Table_Desc[TBL_STAGEPATTERN].sFieldName[6], sStage.ucOption,
				           Table_Desc[TBL_STAGEPATTERN].sFieldName[0], uStagePatternId,
				           Table_Desc[TBL_STAGEPATTERN].sFieldName[1], uStageNo);

    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }

    return true;
}

/**************************************************************
Function:       CGbtTscDb::DelStagePattern
Description:    根据阶段配时号删除配时表数据记录
Input:          uStagePatternId  -- 阶段配时号
Output:         无
Return:         true - 删除数据成功 false -删除数据失败
***************************************************************/
bool CGbtTscDb::DelStagePattern(Byte uStagePatternId)
{
    char   sSql[LARGE_BUF_SIZE];
    ACE_OS::sprintf(sSql, "delete from %s where %s=%u",
				           Table_Desc[TBL_STAGEPATTERN].sTblName,
				           Table_Desc[TBL_STAGEPATTERN].sFieldName[0], 
				           uStagePatternId);

    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }

    return true;
}

/**************************************************************
Function:       CGbtTscDb::DelStagePattern
Description:    根据阶段配时号，阶段号删除配时表数据记录
Input:          uStagePatternId  -- 阶段配时号
				uStageNo         -- 阶段号
Output:         无
Return:         true - 删除数据成功 false -删除数据失败
***************************************************************/
bool CGbtTscDb::DelStagePattern(Byte uStagePatternId, Byte uStageNo)
{
    char   sSql[LARGE_BUF_SIZE];
    ACE_OS::sprintf(sSql, "delete from %s where %s=%u and %s=%u",
				           Table_Desc[TBL_STAGEPATTERN].sTblName,
				           Table_Desc[TBL_STAGEPATTERN].sFieldName[0], 
				           uStagePatternId,
				           Table_Desc[TBL_STAGEPATTERN].sFieldName[1], 
				           uStageNo);

    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }

    return true;
}

/**************************************************************
Function:       CGbtTscDb::DelStagePattern
Description:    删除全部配时表数据记录
Input:          无
Output:         无
Return:         true - 删除数据成功 false -删除数据失败
***************************************************************/
bool CGbtTscDb::DelStagePattern()
{
	char   sSql[LARGE_BUF_SIZE];
	ACE_OS::sprintf(sSql, "delete from %s",	Table_Desc[TBL_STAGEPATTERN].sTblName);

	if (!ExecuteCmd(sSql))
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
		return false;
	}

	return true;
}

/**************************************************************
Function:       CGbtTscDb::IsStagePatternValid
Description:    判断阶段配时数据合法性
Input:          uStagePatternId  -- 阶段配时号
				uStageNo         -- 阶段号
				sStagePattern    -- 阶段配时结构体
Output:         无
Return:         true - 数据合法 false -数据非法
***************************************************************/
bool CGbtTscDb::IsStagePatternValid(Byte uStagePatternId, Byte uStageNo, StagePattern& sStage)
{
    if (uStagePatternId == 17)
		return true ;
    CHECK_ID(CONST_STAGEPATTERN, uStagePatternId, false);
    CHECK_ID(CONST_STAGE, uStageNo, false);

    return true;
}


/**************************************************************
Function:       CGbtTscDb::QueryOverlapPhase
Description:    查询跟随相位
Input:         tblOverlapPhase -- 跟随相位表对象
Output:         无
Return:         true - 查询成功 false -查询失败
***************************************************************/
bool CGbtTscDb::QueryOverlapPhase(TblOverlapPhase& tblOverlapPhase)
{
    int    i;
    Byte  uCount;
    char   sSql[SMALL_BUF_SIZE];
    CSqliteRst  qryRst;
    OverlapPhase*  pData = NULL;

    ACE_OS::sprintf(sSql, "select * from %s order by %s", 
						    Table_Desc[TBL_OVERLAPPHASE].sTblName,
						    Table_Desc[TBL_OVERLAPPHASE].sFieldName[0]);

    if (!Query(sSql, qryRst))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
        return false;
    }

    if (qryRst.m_iColum != Table_Desc[TBL_OVERLAPPHASE].iFieldCount)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= %d\n", SHORT_FILE, __LINE__, Table_Desc[TBL_OVERLAPPHASE].sTblName, qryRst.m_iColum,Table_Desc[TBL_OVERLAPPHASE].iFieldCount));
        return false;
    }

    if (qryRst.m_iRow <= 0)
    {
        return false;
    }

    uCount = (Byte)qryRst.m_iRow;
    if (!tblOverlapPhase.AllocData(uCount))
    {
        return false;
    }

    pData = tblOverlapPhase.GetData(uCount);
    for (i = 0; i < qryRst.m_iRow; i++)
    {
        pData[i].ucOverlapPhaseId    = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum]);
        pData[i].ucOperateType      = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 1]);

        pData[i].ucIncldPhaseCnt    = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 2]);
        GetFieldsValue(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 3], ',', pData[i].ucIncldPhase, MAX_PHASE_COUNT);

        pData[i].ucAdjustPhaseCnt   = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 4]);
        GetFieldsValue(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 5], ',', pData[i].ucAdjustPhase, MAX_PHASE_COUNT);

        pData[i].ucTailGreen        = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 6]);
        pData[i].ucTailYellow       = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 7]);
        pData[i].ucTailRed          = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 8]); 
    }
    return true;
}

/**************************************************************
Function:       CGbtTscDb::QueryOverlapPhase
Description:    根据相位ID号查询跟随相位表数据
Input:         uOverlapPhaseId -- 跟随相位ID
				sOverlapPhase -- 跟随相位结构体
Output:         无
Return:         true - 查询成功 false -查询失败
***************************************************************/
bool CGbtTscDb::QueryOverlapPhase(Byte uOverlapPhaseId, OverlapPhase& sOverlapPhase)
{
    char   sSql[SMALL_BUF_SIZE];
    CSqliteRst  qryRst;

    ACE_OS::sprintf(sSql, "select * from %s where %s=%u", 
						    Table_Desc[TBL_OVERLAPPHASE].sTblName,
						    Table_Desc[TBL_OVERLAPPHASE].sFieldName[0], 
						    uOverlapPhaseId);

    if (!Query(sSql, qryRst))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
        return false;
    }

    if (qryRst.m_iColum != Table_Desc[TBL_OVERLAPPHASE].iFieldCount)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= %d\n", SHORT_FILE, __LINE__, Table_Desc[TBL_OVERLAPPHASE].sTblName, qryRst.m_iColum,Table_Desc[TBL_OVERLAPPHASE].iFieldCount));
        return false;
    }

    if (qryRst.m_iRow <= 0)
    {
        return false;
    }

    sOverlapPhase.ucOverlapPhaseId    = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum]);
    sOverlapPhase.ucOperateType      = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 1]);

    sOverlapPhase.ucIncldPhaseCnt    = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 2]);
    GetFieldsValue(qryRst.m_ppResult[qryRst.m_iColum + 3], ',', sOverlapPhase.ucIncldPhase, MAX_PHASE_COUNT);

    sOverlapPhase.ucAdjustPhaseCnt   = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 4]);
    GetFieldsValue(qryRst.m_ppResult[qryRst.m_iColum + 5], ',', sOverlapPhase.ucAdjustPhase, MAX_PHASE_COUNT);

    sOverlapPhase.ucTailGreen        = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 6]);
    sOverlapPhase.ucTailYellow       = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 7]);
    sOverlapPhase.ucTailRed          = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 8]); 
   
    return true;
}
    
/**************************************************************
Function:       CGbtTscDb::AddOverlapPhase
Description:    根据相位ID号添加跟随相位表数据
Input:          uOverlapPhaseId -- 跟随相位ID
				sOverlapPhase -- 跟随相位结构体
Output:         无
Return:         true - 添加数据成功 false -添加数据失败
***************************************************************/
bool CGbtTscDb::AddOverlapPhase(Byte uOverlapPhaseId, OverlapPhase& sOverlapPhase)
{
    Byte  i;
    char   sIncldPhase[SMALL_BUF_SIZE];
    char   sAdjustPhase[SMALL_BUF_SIZE];
    char   sSql[LARGE_BUF_SIZE];

    if(!IsOverlapPhaseValid(uOverlapPhaseId, sOverlapPhase))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tOverlapPhase invalid\n", SHORT_FILE, __LINE__));
        return false;
    }

    if (sOverlapPhase.ucIncldPhaseCnt > MAX_PHASE_COUNT || sOverlapPhase.ucAdjustPhaseCnt > MAX_PHASE_COUNT)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s sOverlapPhase.ucIncldPhaseCnt = %d, sOverlapPhase.ucAdjustPhaseCnt = %d\n", SHORT_FILE, __LINE__, sOverlapPhase.ucIncldPhaseCnt, sOverlapPhase.ucAdjustPhaseCnt));
        return false;
    }

    ACE_OS::memset(sIncldPhase, 0, SMALL_BUF_SIZE);
    for (i = 0; i < MAX_PHASE_COUNT; i++)
    {
        ACE_OS::sprintf(sSql, "%d,", sOverlapPhase.ucIncldPhase[i]);
        ACE_OS::strcat(sIncldPhase, sSql);
    }

    ACE_OS::memset(sAdjustPhase, 0, SMALL_BUF_SIZE);
    for (i = 0; i < MAX_PHASE_COUNT; i++)
    {
        ACE_OS::sprintf(sSql, "%d,", sOverlapPhase.ucAdjustPhase[i]);
        ACE_OS::strcat(sAdjustPhase, sSql);
    }
    
    ACE_OS::sprintf(sSql, "insert into %s(%s, %s, %s, %s, %s, %s, %s, %s, %s)values(%u,%u,%u,'%s',%u,'%s',%u,%u,%u)",
				           Table_Desc[TBL_OVERLAPPHASE].sTblName,
				           Table_Desc[TBL_OVERLAPPHASE].sFieldName[0],
				           Table_Desc[TBL_OVERLAPPHASE].sFieldName[1], 
				           Table_Desc[TBL_OVERLAPPHASE].sFieldName[2],
				           Table_Desc[TBL_OVERLAPPHASE].sFieldName[3],
				           Table_Desc[TBL_OVERLAPPHASE].sFieldName[4],
				           Table_Desc[TBL_OVERLAPPHASE].sFieldName[5],
				           Table_Desc[TBL_OVERLAPPHASE].sFieldName[6],
				           Table_Desc[TBL_OVERLAPPHASE].sFieldName[7],
				           Table_Desc[TBL_OVERLAPPHASE].sFieldName[8],
				           uOverlapPhaseId,
				           sOverlapPhase.ucOperateType,
				           sOverlapPhase.ucIncldPhaseCnt,
				           sIncldPhase,
				           sOverlapPhase.ucAdjustPhaseCnt,
				           sAdjustPhase,
				           sOverlapPhase.ucTailGreen,
				           sOverlapPhase.ucTailYellow,
				           sOverlapPhase.ucTailRed);


    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }
    return true;
}
    
/**************************************************************
Function:       CGbtTscDb::ModOverlapPhase
Description:    根据相位ID号修改跟随相位表数据
Input:          uOverlapPhaseId -- 跟随相位ID
				sOverlapPhase -- 跟随相位结构体
Output:         无
Return:         true - 修改数据成功 false -修改数据失败
***************************************************************/
bool CGbtTscDb::ModOverlapPhase(Byte uOverlapPhaseId, OverlapPhase& sOverlapPhase)
{
    OverlapPhase sTemp;
    if (!QueryOverlapPhase(uOverlapPhaseId, sTemp))
    {
        return AddOverlapPhase(uOverlapPhaseId, sOverlapPhase);
    }

    Byte  i;
    char   sIncldPhase[SMALL_BUF_SIZE];
    char   sAdjustPhase[SMALL_BUF_SIZE];
    char   sSql[LARGE_BUF_SIZE];

    if(!IsOverlapPhaseValid(uOverlapPhaseId, sOverlapPhase))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tOverlapPhase invalid\n", SHORT_FILE, __LINE__));
        return false;
    }
    if (sOverlapPhase.ucIncldPhaseCnt > MAX_PHASE_COUNT  || sOverlapPhase.ucAdjustPhaseCnt > MAX_PHASE_COUNT)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s sOverlapPhase.ucIncldPhaseCnt = %d, sOverlapPhase.ucAdjustPhaseCnt = %d\n", SHORT_FILE, __LINE__, sOverlapPhase.ucIncldPhaseCnt, sOverlapPhase.ucAdjustPhaseCnt));
        return false;
    }
    ACE_OS::memset(sIncldPhase, 0, SMALL_BUF_SIZE);
    for (i = 0; i < MAX_PHASE_COUNT; i++)
    {
        ACE_OS::sprintf(sSql, "%d,", sOverlapPhase.ucIncldPhase[i]);
        ACE_OS::strcat(sIncldPhase, sSql);
    }
    ACE_OS::memset(sAdjustPhase, 0, SMALL_BUF_SIZE);
    for (i = 0; i < MAX_PHASE_COUNT; i++)
    {
        ACE_OS::sprintf(sSql, "%d,", sOverlapPhase.ucAdjustPhase[i]);
        ACE_OS::strcat(sAdjustPhase, sSql);
    }
    ACE_OS::sprintf(sSql, "update %s set %s=%u, %s=%u, %s='%s', %s=%u, %s='%s', %s=%u, %s=%u, %s=%u where %s=%u",
				           Table_Desc[TBL_OVERLAPPHASE].sTblName,
				           Table_Desc[TBL_OVERLAPPHASE].sFieldName[1], sOverlapPhase.ucOperateType,
				           Table_Desc[TBL_OVERLAPPHASE].sFieldName[2], sOverlapPhase.ucIncldPhaseCnt,
				           Table_Desc[TBL_OVERLAPPHASE].sFieldName[3], sIncldPhase,
				           Table_Desc[TBL_OVERLAPPHASE].sFieldName[4], sOverlapPhase.ucAdjustPhaseCnt,
				           Table_Desc[TBL_OVERLAPPHASE].sFieldName[5], sAdjustPhase,
				           Table_Desc[TBL_OVERLAPPHASE].sFieldName[6], sOverlapPhase.ucTailGreen,
				           Table_Desc[TBL_OVERLAPPHASE].sFieldName[7], sOverlapPhase.ucTailYellow,
				           Table_Desc[TBL_OVERLAPPHASE].sFieldName[8], sOverlapPhase.ucTailRed,
				           Table_Desc[TBL_OVERLAPPHASE].sFieldName[0], uOverlapPhaseId);

    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }

    return true;
}

/**************************************************************
Function:       CGbtTscDb::DelOverlapPhase
Description:    根据相位ID号删除跟随相位表数据
Input:          uOverlapPhaseId -- 跟随相位ID
Output:         无
Return:         true - 删除数据成功 false -删除数据失败
***************************************************************/
bool CGbtTscDb::DelOverlapPhase(Byte uOverlapPhaseId)
{
    char   sSql[LARGE_BUF_SIZE];
    ACE_OS::sprintf(sSql, "delete from %s where %s=%u",
				           Table_Desc[TBL_OVERLAPPHASE].sTblName,
				           Table_Desc[TBL_OVERLAPPHASE].sFieldName[0], uOverlapPhaseId);

    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }

    return true;
}

/**************************************************************
Function:       CGbtTscDb::DelOverlapPhase
Description:    删除全部跟随相位表数据
Input:          无
Output:         无
Return:         true - 删除数据成功 false -删除数据失败
***************************************************************/
bool CGbtTscDb::DelOverlapPhase()
{
	char   sSql[LARGE_BUF_SIZE];
	ACE_OS::sprintf(sSql, "delete from %s",	Table_Desc[TBL_OVERLAPPHASE].sTblName);

	if (!ExecuteCmd(sSql))
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
		return false;
	}

	return true;
}


/**************************************************************
Function:       CGbtTscDb::IsOverlapPhaseValid
Description:   判断跟随相位表数据合法性
Input:          uOverlapPhaseId -- 跟随相位ID
				sOverlapPhase -- 跟随相位结构体
Output:         无
Return:         true - 数据合法 false -数据非法
***************************************************************/
bool CGbtTscDb::IsOverlapPhaseValid(Byte uOverlapPhaseId, OverlapPhase& sOverlapPhase)
{
    CHECK_ID(CONST_OVAERLAPPHASE, uOverlapPhaseId, false);
    return true;
}

/**************************************************************
Function:       CGbtTscDb::QueryEventType
Description:   查询日志类型表记录
Input:          tblEvtType -- 数据类型表对象
Output:         无
Return:         true - 查询成功 false -查询失败
***************************************************************/
bool CGbtTscDb::QueryEventType(TblEventType& tblEvtType)
{
    int    i;
    Byte  uCount;
    char   sSql[SMALL_BUF_SIZE];
    CSqliteRst  qryRst;
    TblEventLog tblLog;
    EventType*  pData = NULL;

    ACE_OS::sprintf(sSql, "select * from %s order by %s", 
						    Table_Desc[TBL_EVENTTYPE].sTblName,
						    Table_Desc[TBL_EVENTTYPE].sFieldName[0]);

    if (!Query(sSql, qryRst))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
        return false;
    }


    if (qryRst.m_iColum != Table_Desc[TBL_EVENTTYPE].iFieldCount)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= %d\n", SHORT_FILE, __LINE__, Table_Desc[TBL_EVENTTYPE].sTblName, qryRst.m_iColum,Table_Desc[TBL_EVENTTYPE].iFieldCount));
        return false;
    }

    if (qryRst.m_iRow <= 0)
    {
        return false;
    }

    uCount = (Byte)qryRst.m_iRow;
    if (!tblEvtType.AllocData(uCount))
    {
        return false;
    }

    pData = tblEvtType.GetData(uCount);
    for (i = 0; i < qryRst.m_iRow; i++)
    {
        pData[i].ucEvtTypeId        = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum]);
        pData[i].ulClearTime        = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 1]);
        pData[i].strEvtDesc.SetString(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 2]);

        pData[i].ucLogCount = 0;
        if (QueryEventLog(pData[i].ucEvtTypeId, tblLog))
        {
            pData[i].ucLogCount = tblLog.GetCount();
        }
    }
    return true;
}

/**************************************************************
Function:       CGbtTscDb::QueryEventType
Description:   根据日志类型ID查询日志类型表记录
Input:          uEvtTypeId -- 日志类型ID
				sEvtType   -- 日志类型结构体
Output:         无
Return:         true - 查询成功 false -查询失败
***************************************************************/
bool CGbtTscDb::QueryEventType(Byte uEvtTypeId, EventType& sEvtType)
{
    char   sSql[SMALL_BUF_SIZE];
    CSqliteRst  qryRst;
    TblEventLog tblLog;

    ACE_OS::sprintf(sSql, "select * from %s where %s=%u", 
						    Table_Desc[TBL_EVENTTYPE].sTblName,
						    Table_Desc[TBL_EVENTTYPE].sFieldName[0], 
						    uEvtTypeId);

    if (!Query(sSql, qryRst))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
        return false;
    }

    if (qryRst.m_iColum != Table_Desc[TBL_EVENTTYPE].iFieldCount)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= %d\n", SHORT_FILE, __LINE__, Table_Desc[TBL_EVENTTYPE].sTblName, qryRst.m_iColum,Table_Desc[TBL_EVENTTYPE].iFieldCount));
        return false;
    }

    if (qryRst.m_iRow <= 0)
    {
        return false;
    }

    sEvtType.ucEvtTypeId   = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum]);
    sEvtType.ulClearTime   = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 1]);
    sEvtType.strEvtDesc.SetString(qryRst.m_ppResult[qryRst.m_iColum + 2]);

    sEvtType.ucLogCount = 0;
    if (QueryEventLog(sEvtType.ucEvtTypeId, tblLog))
    {
        sEvtType.ucLogCount = tblLog.GetCount();
    }
    return true;
}
    
/**************************************************************
Function:       CGbtTscDb::AddEventType
Description:   添加日志类型表记录
Input:          uEvtTypeId -- 日志类型ID
				sEvtType   -- 日志类型结构体
Output:         无
Return:         true - 添加成功 false -添加失败
***************************************************************/
bool CGbtTscDb::AddEventType(Byte uEvtTypeId, EventType& sEvtType)
{
    Ushort uCount;
    char   sSql[LARGE_BUF_SIZE];
    CHECK_ID(CONST_EVENTTYPE, uEvtTypeId, false);
    ACE_OS::sprintf(sSql, "insert into %s(%s, %s, %s)values(%u, %lu,'%s')",
				           Table_Desc[TBL_EVENTTYPE].sTblName,
				           Table_Desc[TBL_EVENTTYPE].sFieldName[0],
				           Table_Desc[TBL_EVENTTYPE].sFieldName[1], 
				           Table_Desc[TBL_EVENTTYPE].sFieldName[2],
				           uEvtTypeId,
				           sEvtType.ulClearTime,
				           sEvtType.strEvtDesc.GetData(uCount));

    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }

    return true;
}

/**************************************************************
Function:       CGbtTscDb::ModEventType
Description:    根据日志类型ID号修改日志类型表记录
Input:          uEvtTypeId -- 日志类型ID
				sEvtType   -- 日志类型结构体
Output:         无
Return:         true - 添加成功 false -添加失败
***************************************************************/
bool CGbtTscDb::ModEventType(Byte uEvtTypeId, EventType& sEvtType)
{
    EventType sTemp;
    if (!QueryEventType(uEvtTypeId, sTemp))
    {
        return AddEventType(uEvtTypeId, sEvtType);
    }

    char   sSql[LARGE_BUF_SIZE];
    CHECK_ID(CONST_EVENTTYPE, uEvtTypeId, false);
    ACE_OS::sprintf(sSql, "update %s set %s=%lu, %s='%s' where %s=%u",
				           Table_Desc[TBL_EVENTTYPE].sTblName,
				           Table_Desc[TBL_EVENTTYPE].sFieldName[1], sEvtType.ulClearTime,
				           Table_Desc[TBL_EVENTTYPE].sFieldName[2], sEvtType.strEvtDesc.GetData(),
				           Table_Desc[TBL_EVENTTYPE].sFieldName[0], uEvtTypeId);

    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }

    if (0 == sEvtType.ucLogCount)
    {
        DelEventLog(sEvtType.ucEvtTypeId);
    }
    return true;
}

/**************************************************************
Function:       CGbtTscDb::DelEventType
Description:    根据日志类型ID号删除日志类型表记录
Input:          uEvtTypeId -- 日志类型ID
Output:         无
Return:         true - 删除记录成功 false -删除记录失败
***************************************************************/
bool CGbtTscDb::DelEventType(Byte uEvtTypeId)
{
    char   sSql[LARGE_BUF_SIZE];
    ACE_OS::sprintf(sSql, "delete from %s where %s=%u",
				           Table_Desc[TBL_EVENTTYPE].sTblName,
				           Table_Desc[TBL_EVENTTYPE].sFieldName[0], 
				           uEvtTypeId);

    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }

    return true;
}

/**************************************************************
Function:       CGbtTscDb::DelEventType
Description:    删除全部日志类型表记录
Input:          无
Output:         无
Return:         true - 删除记录成功 false -删除记录失败
***************************************************************/
bool CGbtTscDb::DelEventType()
{
    char   sSql[LARGE_BUF_SIZE];
    ACE_OS::sprintf(sSql, "delete from %s", Table_Desc[TBL_EVENTTYPE].sTblName);

    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }

    return true;
}

/**************************************************************
Function:       CGbtTscDb::DelEventType
Description:    删除全部日志类型表记录
Input:          uEvtTypeId  - 事件类型
				uStartTime  - 开始时间
				uEndTime    - 结束时间
Output:         无
Return:         true - 删除记录成功 false -删除记录失败
***************************************************************/
bool CGbtTscDb::DelEventLog(Byte uEvtTypeId ,Uint uStartTime ,Uint uEndTime)
{
	char   sSql[LARGE_BUF_SIZE] = {0};
	if(uStartTime >uEndTime)		
		return false ;
	else if(uStartTime == 0xFFFFFFFF && uEndTime == 0xFFFFFFFF && uEvtTypeId == 0xFF)
		ACE_OS::sprintf(sSql, "delete from %s  ", Table_Desc[TBL_EVENTLOG].sTblName);
	else if(uStartTime == 0xFFFFFFFF && uEndTime == 0xFFFFFFFF && uEvtTypeId != 0xFF)
		ACE_OS::sprintf(sSql, "delete from %s where %s = %u ", 
								Table_Desc[TBL_EVENTLOG].sTblName,
								Table_Desc[TBL_EVENTLOG].sFieldName[1],
								uEvtTypeId);
	else 
    	ACE_OS::sprintf(sSql, "delete from %s where %s = %u  and (%s >= %u and %s <= %u)",
    							Table_Desc[TBL_EVENTLOG].sTblName,
    							Table_Desc[TBL_EVENTLOG].sFieldName[1],
    							uEvtTypeId ,
    							Table_Desc[TBL_EVENTLOG].sFieldName[2],
    							uStartTime,
    							Table_Desc[TBL_EVENTLOG].sFieldName[2],
    							uEndTime);

    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }

}


/**************************************************************
Function:       CGbtTscDb::QueryEventLog
Description:    事件日志表记录查询
Input:          tblLog -- 日志记录表对象
Output:         无
Return:         true - 查询记录成功 false -查询记录失败
***************************************************************/
bool CGbtTscDb::QueryEventLog(TblEventLog& tblLog)
{
    unsigned int i;
	Uint   uiCount;
    char*  pStop;
    char   sSql[SMALL_BUF_SIZE];
    CSqliteRst  qryRst;
    EventLog*  pData = NULL;

    ACE_OS::sprintf(sSql, "select * from %s order by %s, %s", 
						    Table_Desc[TBL_EVENTLOG].sTblName,
							Table_Desc[TBL_EVENTLOG].sFieldName[2],
						    Table_Desc[TBL_EVENTLOG].sFieldName[1]
						    );

    if (!Query(sSql, qryRst))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
        return false;
    }

    if (qryRst.m_iColum != Table_Desc[TBL_EVENTLOG].iFieldCount)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= %d\n", SHORT_FILE, __LINE__, Table_Desc[TBL_EVENTLOG].sTblName, qryRst.m_iColum,Table_Desc[TBL_EVENTLOG].iFieldCount));
        return false;
    }

    if (qryRst.m_iRow <= 0)
    {
        return false;
    }

    uiCount = (unsigned int)qryRst.m_iRow;
    if (!tblLog.AllocData(uiCount))
    {
        return false;
    }

    pData = tblLog.GetData(uiCount);
    for (i = 0; i < uiCount; i++)
    {
        pData[i].ucEventId       = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum]);
        pData[i].ucEvtType       = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 1]);
        pData[i].ulHappenTime    = ACE_OS::strtoul(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 2], &pStop, 10);
        pData[i].ulEvtValue      = ACE_OS::strtoul(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 3], &pStop, 10);
    }
    return true;
}

/**************************************************************
Function:       CGbtTscDb::QueryEventLog
Description:    根据事件类型查询事件日志表记录
Input:          uEvtTypeId -- 日志类型ID号
				tblLog     -- 日志表对象
Output:         无
Return:         true - 查询记录成功 false -查询记录失败
***************************************************************/
bool CGbtTscDb::QueryEventLog(Byte uEvtTypeId, TblEventLog& tblLog)
{
    unsigned int i;
    unsigned int uiCount;
    char*  pStop;
    char   sSql[SMALL_BUF_SIZE];
    CSqliteRst  qryRst;
    EventLog*  pData = NULL;

    ACE_OS::sprintf(sSql, "select * from %s where %s=%u order by %s", 
						    Table_Desc[TBL_EVENTLOG].sTblName,
						    Table_Desc[TBL_EVENTLOG].sFieldName[1], 
						    uEvtTypeId,
						    Table_Desc[TBL_EVENTLOG].sFieldName[2]);

    if (!Query(sSql, qryRst))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
        return false;
    }

    if (qryRst.m_iColum != Table_Desc[TBL_EVENTLOG].iFieldCount)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= %d\n", SHORT_FILE, __LINE__, Table_Desc[TBL_EVENTLOG].sTblName, qryRst.m_iColum,Table_Desc[TBL_EVENTLOG].iFieldCount));
        return false;
    }

    if (qryRst.m_iRow <= 0)
    {
        return false;
    }

    uiCount = (unsigned int)qryRst.m_iRow;
    if (!tblLog.AllocData(uiCount))
    {
        return false;
    }

    pData = tblLog.GetData(uiCount);
    for (i = 0; i < uiCount; i++)
    {
        pData[i].ucEventId       = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum]);
        pData[i].ucEvtType       = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 1]);
        pData[i].ulHappenTime    = ACE_OS::strtoul(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 2], &pStop, 10);
        pData[i].ulEvtValue      = ACE_OS::strtoul(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 3], &pStop, 10);
    }
    return true;
}

/**************************************************************
Function:       CGbtTscDb::QueryEventLog
Description:    根据事件IDH号查询事件日志表记录
Input:          uEvtLogId -- 日志ID号
				sEvtLog   -- 日志结构体
Output:         无
Return:         true - 查询记录成功 false -查询记录失败
***************************************************************/
bool CGbtTscDb::QueryEventLog(Byte uEvtLogId, EventLog& sEvtLog)
{
    char*  pStop;
    char   sSql[SMALL_BUF_SIZE];
    CSqliteRst  qryRst;

    ACE_OS::sprintf(sSql, "select * from %s where %s=%u", 
						    Table_Desc[TBL_EVENTLOG].sTblName,
						    Table_Desc[TBL_EVENTLOG].sFieldName[0], 
						    uEvtLogId);

    if (!Query(sSql, qryRst))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
        return false;
    }

    if (qryRst.m_iColum != Table_Desc[TBL_EVENTLOG].iFieldCount)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= %d\n", SHORT_FILE, __LINE__, Table_Desc[TBL_EVENTLOG].sTblName, qryRst.m_iColum,Table_Desc[TBL_EVENTLOG].iFieldCount));
        return false;
    }

    if (qryRst.m_iRow <= 0)
    {
        return false;
    }
  
    sEvtLog.ucEventId       = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum]);
    sEvtLog.ucEvtType       = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 1]);
    sEvtLog.ulHappenTime    = ACE_OS::strtoul(qryRst.m_ppResult[qryRst.m_iColum + 2], &pStop, 10);
    sEvtLog.ulEvtValue      = ACE_OS::strtoul(qryRst.m_ppResult[qryRst.m_iColum + 3], &pStop, 10);
    return true;
}

/**************************************************************
Function:       CGbtTscDb::GetLogMaxMin
Description:    获取日志表最大和最小ID
Input:          无
Output:         pMaxId --最大日志号数据指针
				pMinId --最小日志号数据指针
Return:         true - 查询成功 false -查询失败
***************************************************************/
bool CGbtTscDb::GetLogMaxMin(Uint32* pMaxId, Uint32* pMinId)
{
	char   sSql[LARGE_BUF_SIZE] = {0};
	CSqliteRst  qryRst;

	ACE_OS::memset(sSql, 0, LARGE_BUF_SIZE);

	ACE_OS::sprintf(sSql, "select max(%s) from %s"
							,Table_Desc[TBL_EVENTLOG].sFieldName[0]
							,Table_Desc[TBL_EVENTLOG].sTblName);
	
	if (!Query(sSql, qryRst))
	{
		
		return false;
	}
	if ( NULL == qryRst.m_ppResult[qryRst.m_iColum] )
	{
		*pMaxId = 0;
	}
	else
	{
		*pMaxId = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum]);
	}
	
	if ( 0 == *pMaxId )
	{
		*pMinId = 1;
		
		return true;
	}

	ACE_OS::memset(sSql, 0, LARGE_BUF_SIZE);
	ACE_OS::sprintf(sSql, "select %s from %s where %s=(select min(%s) from %s)"
							,Table_Desc[TBL_EVENTLOG].sFieldName[0]
							,Table_Desc[TBL_EVENTLOG].sTblName
							,Table_Desc[TBL_EVENTLOG].sFieldName[2]
							,Table_Desc[TBL_EVENTLOG].sFieldName[2]
							,Table_Desc[TBL_EVENTLOG].sTblName);
	if (!Query(sSql, qryRst))
	{
		
		return false;
	}
	if ( NULL == qryRst.m_ppResult[qryRst.m_iColum] )
	{
		*pMinId = 1;
	}
	else
	{
		*pMinId = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum]);
	}
	ACE_DEBUG ((LM_DEBUG, "%s:%04d pmaxid =%d pminid= %d\n", SHORT_FILE, __LINE__,*pMaxId,*pMinId));
	return true;
}

/**************************************************************
Function:       CGbtTscDb::AddEventLog
Description:    添加日志表记录
Input:          pMaxId --最大日志号数据指针
				pMinId --最小日志号数据指针
				uEvtType -- 日志类型
				uEvtValue-- 日志值
Output:         无
Return:         true - 添加日志成功 false -添加日志失败
***************************************************************/
bool CGbtTscDb::AddEventLog(Uint32* pMaxId, Uint32* pMinId, Byte uEvtType, Ulong uEvtValue)
{	
	Uint32 uiCurEventId = 0;
    time_t tCurrent = ACE_OS::time(NULL);
	tCurrent = tCurrent + 8*3600;
	char   sSql[LARGE_BUF_SIZE] = {0};
	CSqliteRst  qryRst;

	ACE_OS::memset(sSql, 0 , LARGE_BUF_SIZE);
	if ( *pMaxId < 800 )
	{
		uiCurEventId = *pMaxId + 1;
		ACE_OS::sprintf(sSql, "insert into %s values( %u, %u, %lu, %lu,datetime('now','localtime'))",
								Table_Desc[TBL_EVENTLOG].sTblName,
								uiCurEventId,
								uEvtType,
								tCurrent,
								uEvtValue);
	   *pMaxId += 1;
	}
	else  
	{
		uiCurEventId = *pMinId;
		ACE_OS::sprintf(sSql, "update %s set %s=%u,%s=%u,%s=%lu where %s=%u",
			Table_Desc[TBL_EVENTLOG].sTblName,
			Table_Desc[TBL_EVENTLOG].sFieldName[1],
			uEvtType,
			Table_Desc[TBL_EVENTLOG].sFieldName[2],
			(int)tCurrent,
			Table_Desc[TBL_EVENTLOG].sFieldName[3],
			uEvtValue,
			Table_Desc[TBL_EVENTLOG].sFieldName[0],
			uiCurEventId);

		uiCurEventId++;

		*pMinId = uiCurEventId > 800 ? 1 : uiCurEventId; 
	}

    if ( !ExecuteCmd(sSql) )
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }
    //ACE_DEBUG ((LM_DEBUG, "%s:%d uEvtValue = %d\n",__FILE__,__LINE__,uEvtValue));
    return true;
}

/**************************************************************
Function:       CGbtTscDb::DelEventLog
Description:    根据日志类型ID删除对应类型日志表记录
Input:          uEvtTypeId  -- 日志类型ID
Output:         无
Return:         true - 删除日志成功 false -删除日志失败
***************************************************************/
bool CGbtTscDb::DelEventLog(Byte uEvtTypeId)
{
    char   sSql[LARGE_BUF_SIZE];
    ACE_OS::sprintf(sSql, "delete from %s where %s=%u",
				           Table_Desc[TBL_EVENTLOG].sTblName,
				           Table_Desc[TBL_EVENTLOG].sFieldName[1], 
				           uEvtTypeId);

    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }

    return true;
}

/**************************************************************
Function:       CGbtTscDb::DelEventLog
Description:    删除全部对应类型日志表记录
Input:          无
Output:         无
Return:         true - 删除日志成功 false -删除日志失败
***************************************************************/
bool CGbtTscDb::DelEventLog()
{
    char   sSql[LARGE_BUF_SIZE];
    ACE_OS::sprintf(sSql, "delete from %s", Table_Desc[TBL_EVENTLOG].sTblName);

    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }

    return true;
}




/**************************************************************
Function:       CGbtTscDb::QueryChannelChk
Description:    查询通道灯泡检测配置表 ADD:201308011101
Input:          tblChanChk  -- 通道灯泡检测配置表对象
Output:         无
Return:         true - 查询数据成功 false -查询数据失败
***************************************************************/
bool CGbtTscDb::QueryChannelChk(TblChannelChk & tblChanChk)
{
    int    i;
    Byte  uCount;
    char   sSql[SMALL_BUF_SIZE];
    CSqliteRst  qryRst;   
	ChannelChk* pData =NULL ;

    ACE_OS::sprintf(sSql, "select * from %s order by %s",  
    						Table_Desc[TBL_LAMP_CHECK].sTblName, 
    						Table_Desc[TBL_LAMP_CHECK].sFieldName[0]);

    if (!Query(sSql, qryRst))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
        return false;
    }


    if (qryRst.m_iColum != Table_Desc[TBL_LAMP_CHECK].iFieldCount)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= %d\n", SHORT_FILE, __LINE__, Table_Desc[TBL_LAMP_CHECK].sTblName, qryRst.m_iColum,Table_Desc[TBL_LAMP_CHECK].iFieldCount));
        return false;
    }

    if (qryRst.m_iRow <= 0)
    {
        return false;
    }

    uCount = (Byte)qryRst.m_iRow;
    if (!tblChanChk.AllocData(uCount))
    {
        return false;
    }

    pData = tblChanChk.GetData(uCount);
    for (i = 0; i < qryRst.m_iRow; i++)
    {
        pData[i].ucSubChannelId        = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum]);
        pData[i].ucIsCheck             = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 1]);       
    
    }
    return true;
}


/**************************************************************
Function:       CGbtTscDb::QueryChannelChk
Description:    删除通道灯泡检测配置表 ADD:201308011401
Input:          无
Output:         无
Return:         true - 删除数据成功 false -删除数据失败
***************************************************************/
bool CGbtTscDb::DelChannelChk()
{
    char   sSql[LARGE_BUF_SIZE];
    ACE_OS::sprintf(sSql, "delete from %s",Table_Desc[TBL_LAMP_CHECK].sTblName);

    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }
    return true;
}
    

/**************************************************************
Function:       CGbtTscDb::AddChannelChk
Description:    添加通道灯泡检测配置表 ADD:201308011501
Input:          uChanChkId  -- 信号灯通道ID号
				sChannelChk -- 通道灯泡检测结构体
Output:         无
Return:         true - 添加数据成功 false -添加数据失败
***************************************************************/
bool CGbtTscDb::AddChannelChk(Byte uChanChkId, ChannelChk& sChannelChk)
{
   
    char   sSql[LARGE_BUF_SIZE];
   
    ACE_OS::sprintf(sSql, "insert into %s(%s, %s)values(%u, %u)",
				           Table_Desc[TBL_LAMP_CHECK].sTblName,
				           Table_Desc[TBL_LAMP_CHECK].sFieldName[0],
				           Table_Desc[TBL_LAMP_CHECK].sFieldName[1],                   
				           uChanChkId,
				           sChannelChk.ucIsCheck );
                 

    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }

    return true;
}


/**************************************************************
Function:       CGbtTscDb::ModChannelChk
Description:    修改通道灯泡检测配置表 ADD:201308011548
Input:          uChanChkId  -- 信号灯通道ID号
				sChannelChk -- 通道灯泡检测结构体
Output:         无
Return:         true - 修改数据成功 false -修改数据失败
***************************************************************/
bool CGbtTscDb::ModChannelChk(Byte uChanChkId, ChannelChk& sChannelChk)
{  
    char   sSql[LARGE_BUF_SIZE];
   if(uChanChkId <1 || uChanChkId>MAX_LAMP)
   		return false;
    ACE_OS::sprintf(sSql, "update %s set %s=%u where %s=%u",
				           Table_Desc[TBL_LAMP_CHECK].sTblName,
				           Table_Desc[TBL_LAMP_CHECK].sFieldName[1], 
				           sChannelChk.ucIsCheck,                   
				           Table_Desc[TBL_LAMP_CHECK].sFieldName[0], 
				           uChanChkId);

    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }
  
    return true;
}


/**************************************************************
Function:       CGbtTscDb::AddVehicleStat
Description:    添加车辆统计记录
Input:          ucDetId     - 检测器id
*       		ulCarTotal  - 车流量
*      		    ucOccupancy - 占有率
Output:         无
Return:         true - 添加数据成功 false -添加数据失败
***************************************************************/
bool CGbtTscDb::AddVehicleStat(Byte ucDetId,Ulong ulCarTotal , Byte ucOccupancy)
{
	int iMaxId = 0;
	int iDelId = 0;
	int iCurId = 0;
	time_t tCurrent = ACE_OS::time(NULL);
	char   sSql[LARGE_BUF_SIZE] = {0};	

	CSqliteRst  qryRst;	

	ACE_OS::memset(sSql, 0, LARGE_BUF_SIZE);
	ACE_OS::sprintf(sSql, "select max(%s) from %s"
							,Table_Desc[TBL_VEHICLESTAT].sFieldName[0]
							,Table_Desc[TBL_VEHICLESTAT].sTblName);
	if (!Query(sSql, qryRst))
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
		return false;
	}
	if ( NULL == qryRst.m_ppResult[qryRst.m_iColum] )
	{
		iMaxId = 0;
	}
	else
	{
		iMaxId = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum]);
	}
	//ACE_DEBUG ((LM_DEBUG, "%s:%04d iMaxEventId:%d\n", SHORT_FILE, __LINE__,iMaxEventId));

	if ( iMaxId < 300 )
	{
		iCurId = iMaxId + 1;
	}
	else  //需要删除数据
	{
		ACE_OS::memset(sSql, 0, LARGE_BUF_SIZE);
		ACE_OS::sprintf(sSql, "select %s from %s where %s=(select min(%s) from %s)"
								,Table_Desc[TBL_VEHICLESTAT].sFieldName[0]
								,Table_Desc[TBL_VEHICLESTAT].sTblName
								,Table_Desc[TBL_VEHICLESTAT].sFieldName[4]
								,Table_Desc[TBL_VEHICLESTAT].sFieldName[4]
								,Table_Desc[TBL_VEHICLESTAT].sTblName);
		if (!Query(sSql, qryRst))
		{
			//ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
			return false;
		}
		if ( NULL == qryRst.m_ppResult[qryRst.m_iColum] )
		{
			iDelId = 1;
		}
		else
		{
			iDelId = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum]);
		}
		//ACE_DEBUG ((LM_DEBUG, "%s:%04d iDelEventId:%d\n", SHORT_FILE, __LINE__,iDelEventId));
 
		DelVehicleStat(iDelId);
		iCurId = iDelId;
	}

	ACE_OS::memset(sSql, 0, LARGE_BUF_SIZE);

	ACE_OS::sprintf(sSql, "insert into %s values( %u, %u, %lu, %u, %lu)",
							Table_Desc[TBL_VEHICLESTAT].sTblName,		
							iCurId,
							ucDetId,
							ulCarTotal,
							ucOccupancy,
							tCurrent);


	if (!ExecuteCmd(sSql))
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
		return false;
	}
	return true;
}

/**************************************************************
Function:       CGbtTscDb::DelVehicleStat
Description:    根据ulId删除车辆记录
Input:          ulId     - 车辆ulId
Output:         无
Return:         true - 删除数据成功 false -删除数据失败
***************************************************************/
bool CGbtTscDb::DelVehicleStat(Ulong ulId)
{
	char   sSql[LARGE_BUF_SIZE];
	ACE_OS::sprintf(sSql, "delete from %s where %s=%lu",
							Table_Desc[TBL_VEHICLESTAT].sTblName,
							Table_Desc[TBL_VEHICLESTAT].sFieldName[0], 
							ulId);

	if (!ExecuteCmd(sSql))
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
		return false;
	}

	return true;
}

/**************************************************************
Function:       CGbtTscDb::QueryVehileStat
Description:    查询最近的车辆记录
Input:          无
Output:         sVehicleStat     - 车辆记录结构体
Return:         true - 查询记录成功 false -查询记录失败
***************************************************************/
bool CGbtTscDb::QueryVehileStat(VehicleStat& sVehicleStat)
{
	char*  pStop;
	char   sSql[SMALL_BUF_SIZE];
	CSqliteRst  qryRst;

	ACE_OS::sprintf(sSql, "select * from %s where %s=(select max(%s) from %s)", 
							Table_Desc[TBL_VEHICLESTAT].sTblName,
							Table_Desc[TBL_VEHICLESTAT].sFieldName[4],
							Table_Desc[TBL_VEHICLESTAT].sFieldName[4],
							Table_Desc[TBL_VEHICLESTAT].sTblName);

	if (!Query(sSql, qryRst))
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
		return false;
	}

	if (qryRst.m_iColum != Table_Desc[TBL_VEHICLESTAT].iFieldCount)
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= %d\n", SHORT_FILE, __LINE__, Table_Desc[TBL_EVENTLOG].sTblName, qryRst.m_iColum,Table_Desc[TBL_VEHICLESTAT].iFieldCount));
		return false;
	}

	if (qryRst.m_iRow <= 0)
	{
		return false;
	}

	sVehicleStat.ulId        = ACE_OS::strtoul(qryRst.m_ppResult[qryRst.m_iColum], &pStop, 10);
	sVehicleStat.ucDetId     = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum+1]);
	sVehicleStat.ulCarTotal  = ACE_OS::strtoul(qryRst.m_ppResult[qryRst.m_iColum + 2], &pStop, 10);
	sVehicleStat.ucOccupancy = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 3]);
	sVehicleStat.ulAddtime   = ACE_OS::strtoul(qryRst.m_ppResult[qryRst.m_iColum + 4], &pStop, 10);

	return true;
}

/**************************************************************
Function:       CGbtTscDb::QueryVehileStat
Description:    查询全部的车辆记录
Input:          tblVehicleStat   -- 车辆记录表对象
Output:         无
Return:         true - 查询记录成功 false -查询记录失败
***************************************************************/
bool CGbtTscDb::QueryVehicleStat(TblVehicleStat& tblVehicleStat)
{
	int    i;
	Ulong  uCount;
	char*  pStop;
	char   sSql[SMALL_BUF_SIZE];
	CSqliteRst  qryRst;
	VehicleStat*  pData = NULL;

	ACE_OS::sprintf(sSql, "select * from %s order by %s, %s", 
							Table_Desc[TBL_VEHICLESTAT].sTblName,
							Table_Desc[TBL_VEHICLESTAT].sFieldName[4],
							Table_Desc[TBL_VEHICLESTAT].sFieldName[0]);

	if (!Query(sSql, qryRst))
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
		return false;
	}

	if (qryRst.m_iColum != Table_Desc[TBL_VEHICLESTAT].iFieldCount)
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= %d\n", SHORT_FILE, __LINE__, Table_Desc[TBL_VEHICLESTAT].sTblName, qryRst.m_iColum,Table_Desc[TBL_VEHICLESTAT].iFieldCount));
		return false;
	}

	if (qryRst.m_iRow <= 0)
	{
		return false;
	}

	uCount = (Ulong)qryRst.m_iRow;
	if (!tblVehicleStat.AllocData(uCount))
	{
		return false;
	}

	pData = tblVehicleStat.GetData(uCount);
	for (i = 0; i < qryRst.m_iRow; i++)
	{
		pData[i].ulId        = ACE_OS::strtoul(qryRst.m_ppResult[(i+1)*qryRst.m_iColum], &pStop, 10);
		pData[i].ucDetId     = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum+1]);
		pData[i].ulCarTotal  = ACE_OS::strtoul(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 2], &pStop, 10);
		pData[i].ucOccupancy = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 3]);
		pData[i].ulAddtime   = ACE_OS::strtoul(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 4], &pStop, 10);
	}
	return true;
}

/**************************************************************
Function:       CGbtTscDb::GetCommonPara
Description:    获取公共配置相关参数
Input:          无
Output:         usDeviceId   -- 设备ID
				ucMaxModule  -- 最大模块数
				ucSynchTime  -- 同步时间
				usSynchFlag  -- 同步标志
Return:         true - 查询参数成功 
***************************************************************/
bool CGbtTscDb::GetCommonPara(Ushort& usDeviceId, Byte& ucMaxModule, Byte& ucSynchTime, Ushort& usSynchFlag)
{
    Ulong ulTemp;
    GetFieldData(Table_Desc[TBL_SYSTEM].sTblName, "usDeviceId", ulTemp);
    usDeviceId  = (Ushort)ulTemp;

    GetFieldData(Table_Desc[TBL_CONSTANT].sTblName, "ucMaxModule", ulTemp);
    ucMaxModule  = (Byte)ulTemp;

    GetFieldData(Table_Desc[TBL_SYSTEM].sTblName, "ucSynchTime", ulTemp);
    ucSynchTime  = (Byte)ulTemp;

    GetFieldData(Table_Desc[TBL_SYSTEM].sTblName, "ucSynchFlag", ulTemp);
    usSynchFlag  = (Ushort)ulTemp;

    return true;
}
   
/**************************************************************
Function:       CGbtTscDb::SetCommonPara
Description:    设置公共配置相关参数
Input:          usDeviceId   -- 设备ID数据指针
				ucMaxModule  -- 最大模块数指针
				ucSynchTime  -- 同步时间指针
				usSynchFlag  -- 同步标志指针
Output:         无
Return:         true - 查询参数成功 
***************************************************************/
bool CGbtTscDb::SetCommonPara(Ushort* usDeviceId, Byte* ucMaxModule, Byte* ucSynchTime, Ushort* usSynchFlag)
{
    if (NULL == usDeviceId && NULL == ucMaxModule && NULL == ucSynchTime && NULL == usSynchFlag)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tAll input is null\n", SHORT_FILE, __LINE__));
        return false;
    }

    if (NULL != ucMaxModule)
    {
        if (!SetFieldData(Table_Desc[TBL_CONSTANT].sTblName, "ucMaxModule", *ucMaxModule))
        {
            return false;
        }
    }

    if (NULL != usDeviceId)
    {
        if (!SetFieldData(Table_Desc[TBL_SYSTEM].sTblName, "usDeviceId", *usDeviceId))
        {
            return false;
        }
    }

    if (NULL != ucSynchTime)
    {
        if (!SetFieldData(Table_Desc[TBL_SYSTEM].sTblName, "ucSynchTime", *ucSynchTime))
        {
            return false;
        }
    }

    if (NULL != usSynchFlag)
    {
        if (!SetFieldData(Table_Desc[TBL_SYSTEM].sTblName, "usSynchFlag", *usSynchFlag))
        {
            return false;
        }
    }
 
    return true;
}

/**************************************************************
Function:       CGbtTscDb::GetCommTimePara
Description:    获取公共时间参数
Input:          无
Output:         lZone  		 		-- 时区
				ucMaxPlanCnt    	-- 最大时基数
				ucMaxScheduleCnt    -- 最大时段表数
				ucMaxEvtCount  		-- 最大事件数
Return:         true - 查询参数成功 
***************************************************************/
bool CGbtTscDb::GetCommTimePara(Long& lZone, Byte& ucMaxPlanCnt, Byte& ucMaxScheduleCnt, Byte& ucMaxEvtCount)
{
    Ulong ulTemp;

    GetFieldSignData(Table_Desc[TBL_SYSTEM].sTblName, "lZone", lZone);

    GetFieldData(Table_Desc[TBL_CONSTANT].sTblName, "ucMaxPlan", ulTemp);
    ucMaxPlanCnt  = (Byte)ulTemp;

    GetFieldData(Table_Desc[TBL_CONSTANT].sTblName, "ucMaxSchedule", ulTemp);
    ucMaxScheduleCnt  = (Byte)ulTemp;

    GetFieldData(Table_Desc[TBL_CONSTANT].sTblName, "ucMaxScheduleEvt", ulTemp);
    ucMaxEvtCount  = (Byte)ulTemp;

    return true;
}
    
/**************************************************************
Function:       CGbtTscDb::SetCommTimePara
Description:    设置公共时间参数
Input:          lZone  		 		-- 时区数据指针
				ucMaxPlanCnt    	-- 最大时基数指针
				ucMaxScheduleCnt    -- 最大时段表数指针
				ucMaxEvtCount  		-- 最大事件数指针
Output:         无
Return:         true - 设置公共时间参数成功 
***************************************************************/
bool CGbtTscDb::SetCommTimePara(Long* lZone, Byte* ucMaxPlan, Byte* ucMaxSchedule, Byte* ucMaxScheduleEvt)
{
    if (NULL == lZone && NULL == ucMaxPlan && NULL == ucMaxSchedule && NULL == ucMaxScheduleEvt)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tAll input is null\n", SHORT_FILE, __LINE__));
        return false;
    }

    if (NULL != ucMaxPlan)
    {
        if (!SetFieldData(Table_Desc[TBL_CONSTANT].sTblName, "ucMaxPlan", *ucMaxPlan))
        {
            return false;
        }
    }

    if (NULL != ucMaxSchedule)
    {
        if (!SetFieldData(Table_Desc[TBL_CONSTANT].sTblName, "ucMaxSchedule", *ucMaxSchedule))
        {
            return false;
        }
    }

    if (NULL != ucMaxScheduleEvt)
    {
        if (!SetFieldData(Table_Desc[TBL_CONSTANT].sTblName, "ucMaxScheduleEvt", *ucMaxScheduleEvt))
        {
            return false;
        }
    }
    if (NULL != lZone)
    {
        if (!SetFieldSignData(Table_Desc[TBL_SYSTEM].sTblName, "lZone", *lZone))
        {
            return false;
        }
    }
    return true;
}

/**************************************************************
Function:       CGbtTscDb::GetEventPara
Description:    获取事件参数
Input:          无	
Output:         ucMaxEventType  -- 最大事件类型
				ucMaxEventLog   -- 最大日志数
Return:         true - 获取日志参数成功 
***************************************************************/
bool CGbtTscDb::GetEventPara(Byte& ucMaxEventType, Byte& ucMaxEventLog)
{
    Ulong ulTemp;

    GetFieldData(Table_Desc[TBL_CONSTANT].sTblName, "ucMaxEventType", ulTemp);
    ucMaxEventType  = (Byte)ulTemp;

    GetFieldData(Table_Desc[TBL_CONSTANT].sTblName, "ucMaxEventLog", ulTemp);
    ucMaxEventLog  = (Byte)ulTemp;

    return true;
}

/**************************************************************
Function:       CGbtTscDb::SetEventPara
Description:    设置事件参数
Input:          ucMaxEventType  -- 最大事件类型数据指针
				ucMaxEventLog   -- 最大日志数数据指针
Output:         无
Return:         true - 设置日志参数成功  false -设置失败
***************************************************************/
bool CGbtTscDb::SetEventPara(Byte* ucMaxEventType, Byte* ucMaxEventLog)
{
    if (NULL == ucMaxEventType && NULL == ucMaxEventLog)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tAll input is null\n", SHORT_FILE, __LINE__));
        return false;
    }

    if (NULL != ucMaxEventType)
    {
        if (!SetFieldData(Table_Desc[TBL_CONSTANT].sTblName, "ucMaxEventType", *ucMaxEventType))
        {
            return false;
        }
    }

    if (NULL != ucMaxEventLog)
    {
        if (!SetFieldData(Table_Desc[TBL_CONSTANT].sTblName, "ucMaxEventLog", *ucMaxEventLog))
        {
            return false;
        }
    }

    return true;
}

/**************************************************************
Function:       CGbtTscDb::GetPhasePara
Description:    获取相位参数
Input:          无
Output:         ucMaxPhaseCount --  最大相位数
Return:         true - 获取相位参数成功 
***************************************************************/
bool CGbtTscDb::GetPhasePara(Byte& ucMaxPhaseCount)
{
    Ulong ulTemp;

    if (!GetFieldData(Table_Desc[TBL_CONSTANT].sTblName, "ucMaxPhase", ulTemp))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to GetFieldData(%s, ucMaxPhase)\n", SHORT_FILE, __LINE__, Table_Desc[TBL_CONSTANT].sTblName));
        return false;
    }
    
    ucMaxPhaseCount = (Byte)ulTemp;
    return true;
}

/**************************************************************
Function:       CGbtTscDb::SetPhasePara
Description:    设置相位参数
Input:          ucMaxPhaseCount --  最大相位数
Output:         无
Return:         true - 设置相位参数成功  false - 设置失败
***************************************************************/
bool CGbtTscDb::SetPhasePara(Byte& ucMaxPhaseCount)
{
   return SetFieldData(Table_Desc[TBL_CONSTANT].sTblName, "ucMaxPhase", ucMaxPhaseCount);
}

/**************************************************************
Function:       CGbtTscDb::GetChannelPara
Description:    获取通道参数
Input:          无
Output:         ucMaxChannel --  最大通道数
Return:         true - 获取通道参数成功  false - 获取失败
***************************************************************/
bool CGbtTscDb::GetChannelPara(Byte& ucMaxChannel)
{
    Ulong ulTemp;

    if (!GetFieldData(Table_Desc[TBL_CONSTANT].sTblName, "ucMaxChannel", ulTemp))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to GetFieldData(%s, ucMaxChannel)\n", SHORT_FILE, __LINE__, Table_Desc[TBL_CONSTANT].sTblName));
        return false;
    }
    
    ucMaxChannel = (Byte)ulTemp;
    return true;
}

/**************************************************************
Function:       CGbtTscDb::SetChannelPara
Description:    设置通道参数
Input:          ucMaxChannel --  最大通道数
Output:         无
Return:         true - 设置通道参数成功  false - 设置失败
***************************************************************/
bool CGbtTscDb::SetChannelPara(Byte& ucMaxChannel)
{
    return SetFieldData(Table_Desc[TBL_CONSTANT].sTblName, "ucMaxChannel", ucMaxChannel);
}

/**************************************************************
Function:       CGbtTscDb::GetUnitCtrlPara
Description:    获取单元配置参数
Input:          ucFlsTimeWhenStart  -- 开机黄闪时间
				ucAllRedTimeWhenStart -- 开机全红时间
				ucRemoteCtrlFlag      -- 远程控制标记
				ucFlashFreq           -- 黄闪频率
Output:         无
Return:         true - 获取单元参数成功  false - 获取失败
***************************************************************/
bool CGbtTscDb::GetUnitCtrlPara(Byte& ucFlsTimeWhenStart, Byte& ucAllRedTimeWhenStart, Byte& ucRemoteCtrlFlag, Byte& ucFlashFreq)
{
    Ulong ulTemp;

    GetFieldData(Table_Desc[TBL_SYSTEM].sTblName, "ucFlsTimeWhenStart", ulTemp);
    ucFlsTimeWhenStart  = (Byte)ulTemp;

    GetFieldData(Table_Desc[TBL_SYSTEM].sTblName, "ucAllRedTimeWhenStart", ulTemp);
    ucAllRedTimeWhenStart  = (Byte)ulTemp;

    GetFieldData(Table_Desc[TBL_SYSTEM].sTblName, "ucRemoteCtrlFlag", ulTemp);
    ucRemoteCtrlFlag  = (Byte)ulTemp;

    GetFieldData(Table_Desc[TBL_SYSTEM].sTblName, "ucFlashFreq", ulTemp);
    ucFlashFreq  = (Byte)ulTemp;
    
    return true;
}
    
/**************************************************************
Function:       CGbtTscDb::SetUnitCtrlPara
Description:    设置单元配置参数
Input:          无
Output:         ucFlsTimeWhenStart  -- 开机黄闪时间指针
				ucAllRedTimeWhenStart -- 开机全红时间指针
				ucRemoteCtrlFlag      -- 远程控制标记指针
				ucFlashFreq           -- 黄闪频率指针
Return:         true - 设置单元参数成功  false - 设置失败
***************************************************************/
bool CGbtTscDb::SetUnitCtrlPara(Byte* ucFlsTimeWhenStart, Byte* ucAllRedTimeWhenStart, Byte* ucRemoteCtrlFlag, Byte* ucFlashFreq)
{
    if (NULL == ucFlsTimeWhenStart && NULL == ucAllRedTimeWhenStart && NULL == ucRemoteCtrlFlag && NULL == ucFlashFreq)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tAll input is null\n", SHORT_FILE, __LINE__));
        return false;
    }

    if (NULL != ucFlsTimeWhenStart)
    {
        if (!SetFieldData(Table_Desc[TBL_SYSTEM].sTblName, "ucFlsTimeWhenStart", *ucFlsTimeWhenStart))
        {
            return false;
        }
    }

    if (NULL != ucAllRedTimeWhenStart)
    {
        if (!SetFieldData(Table_Desc[TBL_SYSTEM].sTblName, "ucAllRedTimeWhenStart", *ucAllRedTimeWhenStart))
        {
            return false;
        }
    }

    if (NULL != ucRemoteCtrlFlag)
    {
        if (!SetFieldData(Table_Desc[TBL_SYSTEM].sTblName, "ucRemoteCtrlFlag", *ucRemoteCtrlFlag))
        {
            return false;
        }
    }
    if (NULL != ucFlashFreq)
    {
        if (!SetFieldData(Table_Desc[TBL_SYSTEM].sTblName, "ucFlashFreq", *ucFlashFreq))
        {
            return false;
        }
    }

    return true;
}

/**************************************************************
Function:       CGbtTscDb::GetPatternPara
Description:    获取配时方案参数
Input:          无
Output:         ucMaxPattern  -- 最大方案数
				ucMaxStagePattern -- 最大阶段配时数
				ucMaxStage      -- 最大阶段数
Return:         true - 获取配时方案参数成功  false - 获取失败
***************************************************************/
bool CGbtTscDb::GetPatternPara(Byte& ucMaxPattern, Byte& ucMaxStagePattern, Byte& ucMaxStage)
{
    Ulong ulTemp;

    GetFieldData(Table_Desc[TBL_SYSTEM].sTblName, "ucMaxPattern", ulTemp);
    ucMaxPattern  = (Byte)ulTemp;

    GetFieldData(Table_Desc[TBL_SYSTEM].sTblName, "ucMaxStagePattern", ulTemp);
    ucMaxStagePattern  = (Byte)ulTemp;

    GetFieldData(Table_Desc[TBL_SYSTEM].sTblName, "ucMaxStage", ulTemp);
    ucMaxStage  = (Byte)ulTemp;

    return true;
}
    
/**************************************************************
Function:       CGbtTscDb::SetPatternPara
Description:    设置配时方案参数
Input:          ucMaxPattern  -- 最大方案数指针
				ucMaxStagePattern -- 最大阶段配时数指针
				ucMaxStage      -- 最大阶段数指针
Output:         无
Return:         true - 设置配时方案参数成功  false - 设置失败
***************************************************************/
bool CGbtTscDb::SetPatternPara(Byte* ucMaxPattern, Byte* ucMaxStagePattern, Byte* ucMaxStage)
{
    if (NULL == ucMaxPattern && NULL == ucMaxStagePattern && NULL == ucMaxStage)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tAll input is null\n", SHORT_FILE, __LINE__));
        return false;
    }

    if (NULL != ucMaxPattern)
    {
        if (!SetFieldData(Table_Desc[TBL_SYSTEM].sTblName, "ucMaxPattern", *ucMaxPattern))
        {
            return false;
        }
    }

    if (NULL != ucMaxStagePattern)
    {
        if (!SetFieldData(Table_Desc[TBL_SYSTEM].sTblName, "ucMaxStagePattern", *ucMaxStagePattern))
        {
            return false;
        }
    }

    if (NULL != ucMaxStage)
    {
        if (!SetFieldData(Table_Desc[TBL_SYSTEM].sTblName, "ucMaxStage", *ucMaxStage))
        {
            return false;
        }
    }

    return true;
}

/**************************************************************
Function:       CGbtTscDb::GetGlobalCycle
Description:    获取公共周期时长
Input:          无
Output:         ucGlobalCycle  --  公共周期时长
Return:         true - 获取公共周期时长成功  false - 获取失败
***************************************************************/
bool CGbtTscDb::GetGlobalCycle(Byte& ucGlobalCycle)
{
    Ulong ulTemp;

    if (!GetFieldData(Table_Desc[TBL_SYSTEM].sTblName, "ucGlobalCycle", ulTemp))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to GetFieldData(%s, ucGlobalCycle)\n", SHORT_FILE, __LINE__, Table_Desc[TBL_SYSTEM].sTblName));
        return false;
    }
    
    ucGlobalCycle = (Byte)ulTemp;
    return true;
}
   
/**************************************************************
Function:       CGbtTscDb::SetGlobalCycle
Description:    设置公共周期时长
Input:          ucGlobalCycle  --  公共周期时长
Output:         无
Return:         true - 设置公共周期时长成功  false - 设置失败
***************************************************************/
bool CGbtTscDb::SetGlobalCycle(Byte ucGlobalCycle)
{
    return SetFieldData(Table_Desc[TBL_SYSTEM].sTblName, "ucGlobalCycle", ucGlobalCycle);
}

/**************************************************************
Function:       CGbtTscDb::GetCoorPhaseOffset
Description:    获取协同相位差
Input:          无
Output:         ucPhaseOffset  --  协同相位差
Return:         true - 获取协同相位差成功  false - 获取失败
***************************************************************/
bool CGbtTscDb::GetCoorPhaseOffset(Byte& ucPhaseOffset)
{
    Ulong ulTemp;

    if (!GetFieldData(Table_Desc[TBL_SYSTEM].sTblName, "ucCoorPhaseOffset", ulTemp))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to GetFieldData(%s, ucCoorPhaseOffset)\n", SHORT_FILE, __LINE__, Table_Desc[TBL_SYSTEM].sTblName));
        return false;
    }
    
    ucPhaseOffset = (Byte)ulTemp;
    return true;
}

/**************************************************************
Function:       CGbtTscDb::SetCoorPhaseOffset
Description:    设置协同相位差
Input:          ucPhaseOffset  --  协同相位差
Output:         无
Return:         true - 设置协同相位差成功  false - 设置失败
***************************************************************/
bool CGbtTscDb::SetCoorPhaseOffset(Byte& ucPhaseOffset)
{
    return SetFieldData(Table_Desc[TBL_SYSTEM].sTblName, "ucCoorPhaseOffset", ucPhaseOffset);
}

/**************************************************************
Function:       CGbtTscDb::GetDegradeCfg
Description:    获取降级配置
Input:          无
Output:         ucDegradeMode  --  降级配置模式
				ucDegradePattern  - 降级配置方案
Return:         true - 获取降级配置成功  false - 获取失败
***************************************************************/
bool CGbtTscDb::GetDegradeCfg(Byte& ucDegradeMode, Byte& ucDegradePattern)
{
    char   sSql[SMALL_BUF_SIZE];
    Byte  ucPatternArray[14] = {0};

    CSqliteRst  qryRst;
    ACE_OS::sprintf(sSql, "select ucDegradeMode, sDegradePattern from %s",Table_Desc[TBL_SYSTEM].sTblName);

    if (!Query(sSql, qryRst))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
        return false;
    }

    if (qryRst.m_iColum != 2)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= 3\n", SHORT_FILE, __LINE__, Table_Desc[TBL_SYSTEM].sTblName));
        return false;
    }

    if (qryRst.m_iRow <= 0)
    {
        return false;
    }
  
    ucDegradeMode      = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum]);

    GetFieldsValue(qryRst.m_ppResult[qryRst.m_iColum+1], ',', ucPatternArray, 14);
    if (ucDegradeMode < 14)
    {
        ucDegradePattern = ucPatternArray[ucDegradeMode];
    }
    else
    {
        ucDegradePattern = 0;
    }
    return true;
}

/**************************************************************
Function:       CGbtTscDb::GetDegradePattern
Description:    获取降级配置方案表数据
Input:          无
Output:         tblDegrade  -- 降级方案表对象
Return:         true - 获取降级配置方案表成功  false - 获取失败
***************************************************************/
bool CGbtTscDb::GetDegradePattern(DbByteArray& tblDegrade)
{
    char   sSql[SMALL_BUF_SIZE];
    Byte  ucPatternArray[14] = {0};

    CSqliteRst  qryRst;
    ACE_OS::sprintf(sSql, "select sDegradePattern from %s",Table_Desc[TBL_SYSTEM].sTblName);

    if (!Query(sSql, qryRst))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
        return false;
    }

    if (qryRst.m_iColum != 1)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= 3\n", SHORT_FILE, __LINE__, Table_Desc[TBL_SYSTEM].sTblName));
        return false;
    }

    if (qryRst.m_iRow <= 0)
    {
        return false;
    }
  

    GetFieldsValue(qryRst.m_ppResult[qryRst.m_iColum], ',', ucPatternArray, 14);
    tblDegrade.SetData(ucPatternArray, 14);
    return true;
}

/**************************************************************
Function:       CGbtTscDb::GetDegradePattern
Description:    根据降级模式获取降级配置方案
Input:          ucDegradeMode  -- 降级模式
Output:         ucDegradePattern  -- 降级配置方案
Return:         true - 获取降级配置方案  false - 获取失败
***************************************************************/
bool CGbtTscDb::GetDegradePattern(Byte ucDegradeMode, Byte& ucDegradePattern)
{
    DbByteArray tblByte;
    Byte* pData;
    Byte  uCount;
    if (!GetDegradePattern(tblByte))
    {
        return false;
    }
    pData = tblByte.GetData(uCount);

    if (ucDegradeMode >= uCount)
    {
        return false;
    }

    ucDegradePattern = pData[ucDegradeMode];
    return true;
}

/**************************************************************
Function:       CGbtTscDb::SetDegradeMode
Description:    设置降级配置模式
Input:          ucDegradeMode  -- 降级模式
Output:         无
Return:         true - 设置降级配置模式成功  false - 设置失败
***************************************************************/
bool CGbtTscDb::SetDegradeMode(Byte ucDegradeMode)
{
    char   sSql[LARGE_BUF_SIZE];
    ACE_OS::sprintf(sSql, "update %s set ucDegradeMode = %u", Table_Desc[TBL_SYSTEM].sTblName, ucDegradeMode);

    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }

    return true;
}

/**************************************************************
Function:       CGbtTscDb::SetDegradePattern
Description:    设置降级配置方案
Input:          tblDegrade  -- 降级方案表对象
Output:         无
Return:         true - 设置降级配置方案成功  false - 设置失败
***************************************************************/
bool CGbtTscDb::SetDegradePattern(DbByteArray& tblDegrade)
{
    char   sValue[SMALL_BUF_SIZE] = {0};
    char   sSql[SMALL_BUF_SIZE];
    size_t uLength;
    Byte* pData;
    Byte  uCount, i;
    pData = tblDegrade.GetData(uCount);

    if (uCount > 14)
    {
        uCount = 14;
    }

    if (uCount <= 0)
    {
        return false;
    }


    for (i=0; i<uCount;i++)
    {
        uLength = ACE_OS::strlen(sValue);
        ACE_OS::sprintf(sValue+uLength, "%u,", pData[i]);
    }

    ACE_OS::sprintf(sSql, "update %s set sDegradePattern='%s'", Table_Desc[TBL_SYSTEM].sTblName, sValue);

    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }

    return true;
}

/**************************************************************
Function:       CGbtTscDb::SetDegradePattern
Description:    根据降级模式设置降级配置方案
Input:          ucDegradeMode  -- 降级模式
				ucDegradePattern -- 降级方案
Output:         无
Return:         true - 设置降级配置方案成功  false - 设置失败
***************************************************************/
bool CGbtTscDb::SetDegradePattern(Byte ucDegradeMode, Byte ucDegradePattern)
{
    DbByteArray tblDegrade;
    Byte* pData;
    Byte  uCount;
   

    if (!GetDegradePattern(tblDegrade))
    {
        return false;
    }

    pData = tblDegrade.GetData(uCount);
    if (ucDegradeMode >= uCount)
    {
        return false;
    }

    pData[ucDegradeMode] = ucDegradePattern;

    return SetDegradePattern(tblDegrade);
}

/**************************************************************
Function:       CGbtTscDb::GetCtrlMasterOptFlag
Description:    获取控制主机参数
Input:          无
Output:         ucCtrlOptFlag   -- 控制主机参数
Return:         true - 获取参数成功  false - 获取失败
***************************************************************/
bool CGbtTscDb::GetCtrlMasterOptFlag(Byte& ucCtrlOptFlag)
{
    Ulong ulTemp;

    if (!GetFieldData(Table_Desc[TBL_SYSTEM].sTblName, "ucCtrlMasterOptFlag", ulTemp))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to GetFieldData(%s, ucCtrlMasterOptFlag)\n", SHORT_FILE, __LINE__, Table_Desc[TBL_SYSTEM].sTblName));
        return false;
    }
    
    ucCtrlOptFlag = (Byte)ulTemp;
    return true;
}

/**************************************************************
Function:       CGbtTscDb::SetCtrlMasterOptFlag
Description:    设置控制主机参数
Input:          无
Output:         ucCtrlOptFlag   -- 控制主机参数
Return:         true - 获取参数成功  false - 获取失败
***************************************************************/
bool CGbtTscDb::SetCtrlMasterOptFlag(Byte ucCtrlOptFlag)
{
    return SetFieldData(Table_Desc[TBL_SYSTEM].sTblName, "ucCtrlMasterOptFlag", ucCtrlOptFlag);
}

/**************************************************************
Function:       CGbtTscDb::GetMultiCtrlPara
Description:    获取多路口信号机参数
Input:          无
Output:         usBaseAddr   -- 基准地址
				ucDevCount   -- 设备信号机数
Return:         true - 获取参数成功  false - 获取失败
***************************************************************/
bool CGbtTscDb::GetMultiCtrlPara(Ushort& usBaseAddr, Byte& ucDevCount)
{
    char   sSql[SMALL_BUF_SIZE];

    CSqliteRst  qryRst;
    ACE_OS::sprintf(sSql, "select usBaseAddr,ucSigDevCount from %s", Table_Desc[TBL_SYSTEM].sTblName);

    if (!Query(sSql, qryRst))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
        return false;
    }

    if (qryRst.m_iColum != 2)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= 3\n", SHORT_FILE, __LINE__, Table_Desc[TBL_SYSTEM].sTblName));
        return false;
    }

    if (qryRst.m_iRow <= 0)
    {
        return false;
    }
  
    usBaseAddr      = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum]);
    ucDevCount      = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum+1]);
    return true;
}

/**************************************************************
Function:       CGbtTscDb::SetMultiCtrlPara
Description:    设置多路口信号机参数
Input:          usBaseAddr   -- 基准地址参数指针
				ucDevCount   -- 设备信号机数参数指针
Output:         无
Return:         true - 设置参数成功  false - 设置失败
***************************************************************/
bool CGbtTscDb::SetMultiCtrlPara(Ushort* usBaseAddr, Byte* ucDevCount)
{
    if (NULL == usBaseAddr && NULL == ucDevCount)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tAll input is null\n", SHORT_FILE, __LINE__));
        return false;
    }

    bool   bFirst = true;
    char   sSql[LARGE_BUF_SIZE];
    size_t uLength;
    ACE_OS::sprintf(sSql, "update %s set", Table_Desc[TBL_SYSTEM].sTblName);

    if (NULL != usBaseAddr)
    {
        uLength = ACE_OS::strlen(sSql);
        if (bFirst)
        {
            bFirst = false;
            ACE_OS::sprintf(sSql+uLength, " usBaseAddr=%u", *usBaseAddr);
        }
        else
        {
            ACE_OS::sprintf(sSql+uLength, ",usBaseAddr=%u", *usBaseAddr);
        }
    }

    if (NULL != ucDevCount)
    {
        uLength = ACE_OS::strlen(sSql);
        if (bFirst)
        {
            bFirst = false;
            ACE_OS::sprintf(sSql+uLength, " ucSigDevCount=%u", *ucDevCount);
        }
        else
        {
            ACE_OS::sprintf(sSql+uLength, ",ucSigDevCount=%u", *ucDevCount);
        }
    }
    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }

    return true;
}


/**************************************************************
Function:       CGbtTscDb::SetEypSerial
Description:    设置保存加密验证字符串
Input:          无
Output:         无
Return:         true - 设置加密验证序号成功  false - 设置失败
***************************************************************/
bool CGbtTscDb::SetEypSerial()
{
	char   sSql[LARGE_BUF_SIZE];  
    char SysEypDevId[32]={0};
   	if(GetSysEnyDevId(SysEypDevId)==-1)
   		return false ;	
   	ACE_DEBUG ((LM_DEBUG, "%s:%d SysEypDevId =%s\n",__FILE__, __LINE__,SysEypDevId));  
    ACE_OS::sprintf(sSql, "update %s set ucEypDevSerial='%s'",Table_Desc[TBL_SYSTEM].sTblName,  SysEypDevId);

    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }
    return true;
    
}

/**************************************************************
Function:       CGbtTscDb::GetEypSerial
Description:    获取加密验证字符串
Input:          SysEypSerial- 加密啊序列号字符指针
Output:         无
Return:         true - 获取加密验证序号成功  false - 获取失败
***************************************************************/
bool CGbtTscDb::GetEypSerial(char * SysEypSerial)
{
    char   sSql[SMALL_BUF_SIZE];
    CSqliteRst  qryRst;
    ACE_OS::sprintf(sSql, "select ucEypDevSerial from %s", Table_Desc[TBL_SYSTEM].sTblName);

    if (!Query(sSql, qryRst))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
        return false;
    }

    if (qryRst.m_iRow <= 0)
    {
        return false;
    }

    if (qryRst.m_iColum != 1)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tcolum number != 1\n", SHORT_FILE, __LINE__, qryRst.m_iColum));
        return false;
    } 
    
    ACE_OS::strcpy(SysEypSerial,qryRst.m_ppResult[qryRst.m_iColum]);
    return true;
}


/**************************************************************
Function:       CGbtTscDb::AddDefault
Description:    数据库添加全局模式参数
Input:          无
Output:         无
Return:         无
***************************************************************/
void CGbtTscDb::AddDefault()
{
    char   sSql[LARGE_BUF_SIZE];

    CSqliteRst  qryRst;
    ACE_OS::sprintf(sSql, "select * from %s", Table_Desc[TBL_SYSTEM].sTblName);

    if (!Query(sSql, qryRst))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
    }

    if (qryRst.m_iRow > 0)
    {
        return;
    }

    ACE_OS::sprintf(sSql, "insert into %s(usDeviceId)values(0)",Table_Desc[TBL_SYSTEM].sTblName);
    
    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
    }     
    ACE_OS::sprintf(sSql, "select * from %s", Table_Desc[TBL_CONSTANT].sTblName);

    if (!Query(sSql, qryRst))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
    }

    if (qryRst.m_iRow <= 0)
    {
        ACE_OS::sprintf(sSql, "insert into %s(ucMaxModule)values(16)",Table_Desc[TBL_CONSTANT].sTblName);     
        if (!ExecuteCmd(sSql))
        {
            ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        }
    }

    return ;
}

/**************************************************************
Function:       CGbtTscDb::GetSystemData
Description:    查询系统表
Input:          sField  -- 查询字段指针
Output:         ulData  --  查询值
Return:         true -- 查询成功  false -查询失败
***************************************************************/
bool CGbtTscDb::GetSystemData(const char* sField, Ulong& ulData)
{
    char   sSql[SMALL_BUF_SIZE];
    CSqliteRst  qryRst;
    ACE_OS::sprintf(sSql, "select %s from %s", sField, Table_Desc[TBL_SYSTEM].sTblName);

    if (!Query(sSql, qryRst))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
        return false;
    }

    if (qryRst.m_iRow <= 0)
    {
        return false;
    }

    if (qryRst.m_iColum != 1)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= 3\n", SHORT_FILE, __LINE__, Table_Desc[TBL_SYSTEM].sTblName));
        return false;
    }
  
    ulData  = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum]);
    return true;
}

/**************************************************************
Function:       CGbtTscDb::SetSystemData
Description:    设置系统表
Input:          sField  -- 设置字段指针
				ulData  -- 设置值
Output:         
Return:         true -- 设置系统表成功  false -查询失败
***************************************************************/
bool CGbtTscDb::SetSystemData(const char* sField, Ulong ulData)
{
    char   sSql[LARGE_BUF_SIZE];
    ACE_OS::sprintf(sSql, "update %s set %s=%lu", Table_Desc[TBL_SYSTEM].sTblName, sField, ulData);

    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }

    return true;
}

/**************************************************************
Function:       CGbtTscDb::GetFieldData
Description:    查询数据表字段值
Input:          sTable  -- 数据表名字符串
				sField  -- 字段名
Output:         ulData  -- 字段值
Return:         true -- 查询表字段成功  false -查询失败
***************************************************************/
bool CGbtTscDb::GetFieldData(const char* sTable, const char* sField, Ulong& ulData)
{
    char   sSql[SMALL_BUF_SIZE];

    CSqliteRst  qryRst;
    ACE_OS::sprintf(sSql, "select %s from %s", sField, sTable);

    if (!Query(sSql, qryRst))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
        return false;
    }

    if (qryRst.m_iRow <= 0)
    {
        return false;
    }

    if (qryRst.m_iColum != 1)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tcolum number != 1\n", SHORT_FILE, __LINE__, qryRst.m_iColum));
        return false;
    }
  
    ulData  = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum]);
    return true;
}

/**************************************************************
Function:       CGbtTscDb::SetFieldData
Description:    设置数据表字段值
Input:          sTable  -- 数据表名字符串
				sField  -- 字段名
				ulData  -- 字段值
Output:         无
Return:         true -- 查设置表字段成功  false -设置失败
***************************************************************/
bool CGbtTscDb::SetFieldData(const char* sTable, const char* sField, Ulong ulData)
{
    char   sSql[LARGE_BUF_SIZE];

    if (0 == ACE_OS::strcmp(sTable, "Tbl_Constant"))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tTable(%s) Field(%s) can not be mod\n", SHORT_FILE, __LINE__, sTable, sField));
        return false;
    }
    else
    {
        if (!IsSysParaValid(sField, ulData))
        {
            ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s = %u, invalid\n", SHORT_FILE, __LINE__, sField, ulData));
            return false;
        }
    }
    ACE_OS::sprintf(sSql, "update %s set %s=%lu", sTable, sField, ulData);

    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }

    return true;
}

/**************************************************************
Function:       CGbtTscDb::GetFieldSignData
Description:    查询数据表字段值
Input:          sTable  -- 数据表名字符串
				sField  -- 字段名
Output:         lData  -- 字段值
Return:         true -- 查询表字段成功  false -查询失败
***************************************************************/
bool CGbtTscDb::GetFieldSignData(const char* sTable, const char* sField, Long& lData)
{
    char   sSql[SMALL_BUF_SIZE];

    CSqliteRst  qryRst;
    ACE_OS::sprintf(sSql, "select %s from %s", sField, sTable);

    if (!Query(sSql, qryRst))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
        return false;
    }

    if (qryRst.m_iRow <= 0)
    {
        return false;
    }

    if (qryRst.m_iColum != 1)
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tcolum number != 1\n", SHORT_FILE, __LINE__, qryRst.m_iColum));
        return false;
    }
  
    lData  = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum]);
    return true;
}

/**************************************************************
Function:       CGbtTscDb::SetFieldSignData
Description:    设置数据表字段值
Input:          sTable  -- 数据表名字符串
				sField  -- 字段名
				lData  -- 字段值
Output:         无
Return:         true -- 查设置表字段成功  false -设置失败
***************************************************************/
bool CGbtTscDb::SetFieldSignData(const char* sTable, const char* sField, Long lData)
{
    char   sSql[LARGE_BUF_SIZE];

    if (0 == ACE_OS::strcmp(sTable, "Tbl_Constant"))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tTable(%s) Field(%s) can not be mod\n", SHORT_FILE, __LINE__, sTable, sField));
        return false;
    }
    else
    {
        if (!IsSysParaValid(sField, lData))
        {
            ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s = %d, invalid\n", SHORT_FILE, __LINE__, sField, lData));
            return false;
        }
    }
    ACE_OS::sprintf(sSql, "update %s set %s=%ld", sTable, sField, lData);

    if (!ExecuteCmd(sSql))
    {
        ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
        return false;
    }

    return true;
}


/**************************************************************
Function:       CGbtTscDb::IsSysParaValid
Description:    判断设置到系统参数是否合法
Input:          sField  -- 参数类型
				ulValue  -- 参数值				
Output:         无
Return:         true -- 参数合法  false -参数非法
***************************************************************/
bool CGbtTscDb::IsSysParaValid(const char* sField, Ulong& ulValue)
{
    if (0 == ACE_OS::strcmp(sField, SYS_DEGRADE_MODE))
    {
        if (ulValue > 13)
        {
            return false;
        }
        return true;
    }
    else if (0 == ACE_OS::strcmp(sField, SYS_BASE_ADDR))
    {
        if (ulValue > 8192)
        {
            return false;
        }
        return true;
    }
    else if (0 == ACE_OS::strcmp(sField, SYS_SIGDEV_COUNT))
    {
        if (ulValue > 8 || 0 == ulValue)
        {
            return false;
        }
        return true;
    }

    return true;
}

/**************************************************************
Function:       CGbtTscDb::IsSysParaValid
Description:    判断设置到系统参数是否合法
Input:          sField  -- 参数类型
				lValue  -- 参数值				
Output:         无
Return:         true -- 参数合法  false -参数非法
***************************************************************/
bool CGbtTscDb::IsSysParaValid(const char* sField, Long& lValue)
{
    if (0 == ACE_OS::strcmp(sField, SYS_ZONE))
    {
        if (lValue > 43200 || lValue < -43200)
        {
            return false;
        }
        return true;
    }

    return true;
}

/**************************************************************
Function:       CGbtTscDb::QueryPhaseToDirec
Description:    查询相位方向表数据
Input:          tblPhaseToDirec - 相位方向表对象			
Output:         无
Return:         true -- 查询数据成功  false -查询数据失败
***************************************************************/
bool CGbtTscDb::QueryPhaseToDirec(TblPhaseToDirec& tblPhaseToDirec)
{
	int    i;
	Byte  uCount;
	char   sSql[SMALL_BUF_SIZE];
	CSqliteRst  qryRst;
	PhaseToDirec*  pData = NULL;

	ACE_OS::sprintf(sSql, "select * from %s order by %s", 
							Table_Desc[TBL_PHASE2DIREC].sTblName,
							Table_Desc[TBL_PHASE2DIREC].sFieldName[0]);

	if (!Query(sSql, qryRst))
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
		return false;
	}

	if (qryRst.m_iColum != Table_Desc[TBL_PHASE2DIREC].iFieldCount)
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= %d\n", SHORT_FILE, __LINE__, Table_Desc[TBL_PHASE2DIREC].sTblName, qryRst.m_iColum,Table_Desc[TBL_PHASE2DIREC].iFieldCount));
		return false;
	}

	if (qryRst.m_iRow <= 0)
	{
		return false;
	}

	uCount = (Byte)qryRst.m_iRow;
	if (!tblPhaseToDirec.AllocData(uCount))
	{
		return false;
	}

	pData = tblPhaseToDirec.GetData(uCount);
	for (i = 0; i < qryRst.m_iRow; i++)
	{
		pData[i].ucId           = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum]);
		pData[i].ucPhase        = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 1]);
		pData[i].ucOverlapPhase = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 2]);
		pData[i].ucRoadCnt      = ACE_OS::atoi(qryRst.m_ppResult[(i+1)*qryRst.m_iColum + 3]);
	}

	return true;
}

/**************************************************************
Function:       CGbtTscDb::QueryPhaseToDirec
Description:    根据ucId查询相位方向表数据
Input:          ucId - ucId号			
Output:         sPhaseToDirec  -- 相位方向数据结构体
Return:         true -- 查询数据成功  false -查询数据失败
***************************************************************/
bool CGbtTscDb::QueryPhaseToDirec(Byte ucId, PhaseToDirec& sPhaseToDirec)
{
	char   sSql[SMALL_BUF_SIZE];
	CSqliteRst  qryRst;

	ACE_OS::sprintf(sSql, "select * from %s where %s=%u", 
							Table_Desc[TBL_PHASE2DIREC].sTblName,
							Table_Desc[TBL_PHASE2DIREC].sFieldName[0], 
							ucId);

	if (!Query(sSql, qryRst))
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to Query\n", SHORT_FILE, __LINE__));
		return false;
	}

	if (qryRst.m_iColum != Table_Desc[TBL_PHASE2DIREC].iFieldCount)
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\t%s colum number(%d)!= %d\n", SHORT_FILE, __LINE__, Table_Desc[TBL_PHASE2DIREC].sTblName, qryRst.m_iColum,Table_Desc[TBL_PHASE2DIREC].iFieldCount));
		return false;
	}

	if ( qryRst.m_iRow <= 0 )
	{
		return false;
	}

	sPhaseToDirec.ucId           = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum]);
	sPhaseToDirec.ucPhase        = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 1]);
	sPhaseToDirec.ucOverlapPhase = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 2]);
	sPhaseToDirec.ucRoadCnt      = ACE_OS::atoi(qryRst.m_ppResult[qryRst.m_iColum + 3]);

	return true;
}

/**************************************************************
Function:       CGbtTscDb::AddPhaseToDirec
Description:    添加相位方向表数据记录
Input:          ucId - ucId号			
Output:         sPhaseToDirec  -- 相位方向数据结构体
Return:         true -- 添加数据成功  false -添加数据失败
***************************************************************/
bool CGbtTscDb::AddPhaseToDirec(Byte ucId, PhaseToDirec& sPhaseToDirec)
{
	char   sSql[LARGE_BUF_SIZE];

	ACE_OS::sprintf(sSql, "insert into %s(%s, %s, %s , %s)values(%u,%u,%u,%u)",
							Table_Desc[TBL_PHASE2DIREC].sTblName,
							Table_Desc[TBL_PHASE2DIREC].sFieldName[0],
							Table_Desc[TBL_PHASE2DIREC].sFieldName[1], 
							Table_Desc[TBL_PHASE2DIREC].sFieldName[2],
							Table_Desc[TBL_PHASE2DIREC].sFieldName[3],
							ucId,
							sPhaseToDirec.ucPhase,
							sPhaseToDirec.ucOverlapPhase,
							sPhaseToDirec.ucRoadCnt);

	if (!ExecuteCmd(sSql))
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
		return false;
	}
	return true;
}

/**************************************************************
Function:       CGbtTscDb::ModPhaseToDirec
Description:    修改相位方向表数据记录
Input:          ucId - ucId号			
Output:         sPhaseToDirec  -- 相位方向数据结构体
Return:         true -- 修改数据成功  false -修改数据失败
***************************************************************/
bool CGbtTscDb::ModPhaseToDirec(Byte ucId, PhaseToDirec& sPhaseToDirec)
{
	PhaseToDirec sTemp;
	if ( !QueryPhaseToDirec(ucId, sTemp) )
	{
		return AddPhaseToDirec(ucId, sPhaseToDirec);
	}

	char   sSql[LARGE_BUF_SIZE];

	ACE_OS::sprintf(sSql, "update %s set %s=%u, %s=%u , %s=%u where %s=%u",
							Table_Desc[TBL_PHASE2DIREC].sTblName,
							Table_Desc[TBL_PHASE2DIREC].sFieldName[1], sPhaseToDirec.ucPhase,
							Table_Desc[TBL_PHASE2DIREC].sFieldName[2], sPhaseToDirec.ucOverlapPhase,
							Table_Desc[TBL_PHASE2DIREC].sFieldName[3], sPhaseToDirec.ucRoadCnt,
							Table_Desc[TBL_PHASE2DIREC].sFieldName[0], ucId);

	if (!ExecuteCmd(sSql))
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
		return false;
	}

	return true;
}

/**************************************************************
Function:       CGbtTscDb::DelPhaseToDirec
Description:    删除全部相位于方向表数据记录
Input:          无		
Output:         无
Return:         true -- 删除数据成功  false -删除数据失败
***************************************************************/
bool CGbtTscDb::DelPhaseToDirec()
{
	char   sSql[LARGE_BUF_SIZE];
	ACE_OS::sprintf(sSql, "delete from %s",Table_Desc[TBL_PHASE2DIREC].sTblName);

	if (!ExecuteCmd(sSql))
	{
		ACE_DEBUG ((LM_DEBUG, "%s:%04d\tFailed to ExecuteCmd\n", SHORT_FILE, __LINE__));
		return false;
	}

	return true;
}

}
