/***********************************************************************************
Copyright(c) 2013  AITON. All rights reserved.
Author:     AITON
FileName:   ManaKernel.cpp
Date:       2013-1-1
Description:信号机核心业务处理类文件
Version:    V1.0
History:    201308010930  添加48个灯泡检测的配置数据库表和表处理
			201309251030 修改相位冲突检测位置，从下一周期移到下一步处进行检测
			201309251120 添加日志消息发送函数，将不同类日志消息统一到一个函数处理
			201309251030 修改相位冲突检测位置，从下一周期移到下一步处进行检测
			201309281025 Add "ComFunc.h" headfile
		    201401101130 修改阶段步伐获取的时候感应控制下绿灯最小绿，改为去放行相位最小绿大者
***********************************************************************************/
#include <stdlib.h>
#include "ace/Date_Time.h"
#include "SignalDefaultData.h"
#include "ManaKernel.h"
#include "TscMsgQueue.h"
#include "LampBoard.h"
#include "GbtMsgQueue.h"
#include "TimerManager.h"
#include "DbInstance.h"
#include "GbtDb.h"
#include "MainBoardLed.h"
#include "Detector.h"
#include "GaCountDown.h"
#include "Coordinate.h"
#include "FlashMac.h"
#include "Rs485.h"
#include "ComFunc.h" 



#define CNTDOWN_TIME 8
/*
信号灯颜色类型枚举
*/
enum
{
	LAMP_COLOR_RED = 0,  //红色灯
	LAMP_COLOR_YELLOW ,  //黄色灯
	LAMP_COLOR_GREEN     //绿色灯
};

/**************************************************************
Function:       CManaKernel::CManaKernel
Description:    信号机核心CManaKernel类构造函数，初始化核心变量
Input:          ucMaxTick                 
Output:         无
Return:         无
***************************************************************/
CManaKernel::CManaKernel()
{
	ACE_DEBUG((LM_DEBUG,"%s : %d create CManaKernel\n",__FILE__,__LINE__));

	m_bFinishBoot           = false;
	m_bRestart              = false;
	m_bSpePhase             = false;
	m_bWaitStandard         = false;
	//m_bPhaseDetCfg          = false;
	m_bSpeStatus            = false;
	m_bSpeStatusTblSchedule = false;
	m_bVirtualStandard      = false;
	m_bVehile 				= false ;   //ADD 20130815 1500
	m_bNextPhase			= false ;
	m_ucUtcsComCycle        = 0;   
	m_ucUtscOffset          = 0; 

	m_pTscConfig = new STscConfig;      //信号机配置信息
	m_pRunData   = new STscRunData;     //信号机动态参数信息
	bNextDirec = false ;
	bTmpPattern = false ;
	bValidSoftWare = true ;
	bUTS  = false ;
	bDegrade = false ; //ADD:201311121140
	iCntFlashTime = 0 ;
	ACE_OS::memset(m_pTscConfig->sOverlapPhase,0,MAX_OVERLAP_PHASE*sizeof(SOverlapPhase));
	ACE_OS::memset(m_pTscConfig , 0 , sizeof(STscConfig) );
	ACE_OS::memset(m_pRunData , 0 , sizeof(STscRunData) );
	ACE_DEBUG((LM_DEBUG,"%s:%d Init ManaKernel objetc ok!\n",__FILE__,__LINE__));
}

/**************************************************************
Function:       CManaKernel::~CManaKernel
Description:    CManaKernel类析构函数，释放类变量
Input:          无               
Output:         无
Return:         无
***************************************************************/
CManaKernel::~CManaKernel()
{
	if ( m_pTscConfig != NULL )
	{
		delete m_pTscConfig;
		m_pTscConfig = NULL;
	}
	if ( m_pRunData != NULL )
	{
		delete m_pRunData;
		m_pRunData = NULL;
	}
	ACE_DEBUG((LM_DEBUG,"%s:%d Destruct ManaKernel objetc ok!\n",__FILE__,__LINE__));
}


/**************************************************************
Function:       CManaKernel::CreateInstance
Description:    创建CManaKernel类静态对象		
Input:          无              
Output:         无
Return:         CManaKernel静态对象指针
***************************************************************/
CManaKernel* CManaKernel::CreateInstance()
{
	static CManaKernel cWorkParaManager; 

	return &cWorkParaManager;
}


/**************************************************************
Function:       CManaKernel::InitWorkPara
Description:    初始化CManaKernel核心类部分工作参数和数据库	
Input:          无              
Output:         无
Return:         无
***************************************************************/
void CManaKernel::InitWorkPara()
{	
	ACE_DEBUG((LM_DEBUG,"%s:%d 系统启动,初始化信号机参数! \n",__FILE__,__LINE__)); //ADD: 20130523 1053	
	// 插入默认的数据
	SignalDefaultData sdd;
	sdd.InsertGbDefaultData();//插入默认值到数据库 如果数据库表为空
	UpdateConfig();

	m_pRunData->bNeedUpdate  = false;
	m_pRunData->ucLockPhase  = 0;
	m_pRunData->bOldLock     = false;
	m_pRunData->uiCtrl       = CTRL_SCHEDULE;
	m_pRunData->uiOldCtrl    = CTRL_SCHEDULE;
	m_pRunData->uiWorkStatus = FLASH;
	m_pRunData->bStartFlash  = true;
	m_pRunData->bIsChkLght   = false ;
	m_pRunData->b8cndtown    = false ;
	ResetRunData(0);
	
	CLampBoard::CreateInstance()->SetLamp(m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucLampOn
							    	,m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucLampFlash);

	CMainBoardLed::CreateInstance()->DoModeLed(false,true);
	CMainBoardLed::CreateInstance()->DoAutoLed(true);
	CMainBoardLed::CreateInstance()->DoTscPscLed();
	//ValidSoftWare();  //ADD: 201310221530 验证软件的合法性
}




/**************************************************************
Function:       CManaKernel::SelectDataFromDb
Description:    从Sqlite数据库获取信号及全局配置数据	
Input:          无              
Output:         无
Return:         无
***************************************************************/
void CManaKernel::SelectDataFromDb() 
{
	Byte ucCount           = 0;
	unsigned short usCount = 0;
	int iIndex             = 0;
	int iBoardIndex        = 0;
	int iDetId             = 0;
	//bool bLampBoardExit[MAX_LAMP_BOARD] = {0};
	GBT_DB::TblPlan         tblPlan;
	GBT_DB::Plan*           pPlan = NULL;
	GBT_DB::TblSchedule     tblSchedule;
	GBT_DB::Schedule*       pSchedule = NULL;
	GBT_DB::TblPattern      tblPattern;
	GBT_DB::Pattern*        pPattern = NULL;
	GBT_DB::TblStagePattern tblStage;
	GBT_DB::StagePattern*   pStage = NULL;
	GBT_DB::TblPhase        tblPhase;
	GBT_DB::Phase*          pPhase = NULL;
	GBT_DB::TblOverlapPhase tblOverlapPhase;
	GBT_DB::OverlapPhase*   pOverlapPhase = NULL;
	GBT_DB::TblChannel      tblChannel;
	GBT_DB::Channel*        pChannel = NULL;
	GBT_DB::TblDetector     tblDetector;
	GBT_DB::Detector*       pDetector = NULL;
	GBT_DB::TblCollision    tblCollision;
	GBT_DB::Collision*      pCollision = NULL;
	GBT_DB::TblSpecFun      tblSpecFun;
	GBT_DB::SpecFun*        pSpecFun = NULL;
	GBT_DB::TblModule       tblModule;
	GBT_DB::Module*         pModule  = NULL;
	GBT_DB::TblDetExtend    tblDetExtend;
	GBT_DB::DetExtend*      pDetExtend = NULL;
	GBT_DB::DetectorPara    sDetPara;

	GBT_DB::TblChannelChk   tblChannelChk;     //ADD:20130801 09 30 
	GBT_DB::ChannelChk*     pChannelChk = NULL;

	GBT_DB::TblPhaseToDirec   tblPhaseToDirec;     //ADD:201310181652
	GBT_DB::PhaseToDirec*     pPhaseToDirec = NULL;
	
	(CDbInstance::m_cGbtTscDb).QueryDetPara(sDetPara);               //检测器结构体参数,从tbl_system tbl_constant 进行查询
	CDetector::CreateInstance()->SetStatCycle(sDetPara.ucDataCycle);  //数据采集周期设置

	ACE_OS::memset(m_pTscConfig , 0 , sizeof(STscConfig) ); //信号机动态运行参数全部初始化为0
	
	
	/*************相位与方向配置表****************/  //ADD:201310181706
	(CDbInstance::m_cGbtTscDb).QueryPhaseToDirec(tblPhaseToDirec);
	pPhaseToDirec = tblPhaseToDirec.GetData(ucCount);
	iIndex = 0;
	while(iIndex<ucCount)
	{
		ACE_OS::memcpy(m_pTscConfig->sPhaseToDirec+iIndex,pPhaseToDirec,sizeof(GBT_DB::PhaseToDirec));
		pPhaseToDirec++;
		iIndex++;
	}

	/*************通道灯泡检测配置表表****************/  //ADD:20130801 15 35
	(CDbInstance::m_cGbtTscDb).QueryChannelChk(tblChannelChk);
	pChannelChk = tblChannelChk.GetData(ucCount);
	iIndex = 0;
	while(iIndex<ucCount)
	{
		ACE_OS::memcpy(m_pTscConfig->sChannelChk+iIndex,pChannelChk,sizeof(GBT_DB::ChannelChk));
		pChannelChk++;
		iIndex++;

	}

    /*************调度计划表****************/
	(CDbInstance::m_cGbtTscDb).QueryPlan(tblPlan);
	pPlan = tblPlan.GetData(ucCount);
	iIndex = 0;
	while ( iIndex < ucCount )
	{
		ACE_OS::memcpy(m_pTscConfig->sTimeGroup+iIndex,pPlan,sizeof(GBT_DB::Plan));
		pPlan++;
		iIndex++;
	}
	ACE_OS::memset(m_pTscConfig->sTimeGroup+iIndex,0,sizeof(GBT_DB::Plan));


	/*************一天的时段表获取****************/
	ACE_Date_Time tvTime(GetCurTime());
	Byte ucCurScheduleId = GetScheduleId((Byte)tvTime.month(),(Byte)tvTime.day(),(Byte)tvTime.weekday()); 
	ACE_DEBUG((LM_DEBUG,"%s:%d Month =%d,day =%d weekday =%d !\n ",__FILE__,__LINE__,(Byte)tvTime.month(),(Byte)tvTime.day(),(Byte)tvTime.weekday()));
	if(ucCurScheduleId == 0)
	{
		ucCurScheduleId = 1 ;
		ACE_DEBUG((LM_DEBUG,"%s:%d Get CurScheduledId = %d ,Set default 1 !\n",__FILE__,__LINE__,ucCurScheduleId));
	}
	else
	{
		ACE_DEBUG((LM_DEBUG,"%s:%d Get CurScheduledId = %d from Tbl_plan !\n",__FILE__,__LINE__,ucCurScheduleId)); 
	}
	
	m_pRunData->ucScheduleId = ucCurScheduleId ; //ADD:20130917 1600
	(CDbInstance::m_cGbtTscDb).QuerySchedule(ucCurScheduleId,tblSchedule);
	pSchedule = tblSchedule.GetData(usCount);
	iIndex = 0;
	while ( iIndex < usCount )
	{
		ACE_OS::memcpy(m_pTscConfig->sSchedule+iIndex,pSchedule,sizeof(GBT_DB::Schedule));
		pSchedule++;
		iIndex++;
	}
	ACE_OS::memset(m_pTscConfig->sSchedule+iIndex,0,sizeof(SSchedule));	

	/*************配时方案表操作****************/
	(CDbInstance::m_cGbtTscDb).QueryPattern(tblPattern);
	pPattern = tblPattern.GetData(ucCount);
	iIndex = 0;
	while ( iIndex < ucCount )
	{
		ACE_OS::memcpy(m_pTscConfig->sTimePattern+iIndex,pPattern,sizeof(GBT_DB::Pattern));
		pPattern++;
		iIndex++;
	}
	ACE_OS::memset(&(m_pTscConfig->sTimePattern[iIndex]),0,sizeof(STimePattern));

	

	/************阶段配时表*************/
	Byte ucScheType = 0;
	Byte ucSubSche  = 0;
	(CDbInstance::m_cGbtTscDb).QueryStagePattern(tblStage);
	pStage = tblStage.GetData(usCount);
	iIndex = 0;
	while ( iIndex < usCount )
	{
		ucScheType = pStage->ucStagePatternId;  
		ucSubSche  = pStage->ucStageNo;
		
		if ( (ucScheType > 0) && (ucSubSche > 0) )
		{
			ACE_OS::memcpy(&(m_pTscConfig->sScheduleTime[ucScheType-1][ucSubSche-1])
				,pStage,sizeof(GBT_DB::StagePattern));
		}

		pStage++;
		iIndex++;
	}

	/********************相位表*****************/
	(CDbInstance::m_cGbtTscDb).QueryPhase(tblPhase);
	pPhase = tblPhase.GetData(ucCount);
	iIndex = 0;
	while ( iIndex < ucCount )
	{
		ACE_OS::memcpy(m_pTscConfig->sPhase+iIndex,pPhase,sizeof(GBT_DB::Phase));
		pPhase++;
		iIndex++;
	}
	ACE_OS::memset(m_pTscConfig->sPhase+iIndex,0,sizeof(SPhase));

	/********************跟随相位表****************/
	(CDbInstance::m_cGbtTscDb).QueryOverlapPhase(tblOverlapPhase);
	pOverlapPhase = tblOverlapPhase.GetData(ucCount);
	iIndex = 0;
	while ( iIndex < ucCount )
	{
		ACE_OS::memcpy(m_pTscConfig->sOverlapPhase+iIndex,pOverlapPhase,sizeof(GBT_DB::OverlapPhase));
		pOverlapPhase++;
		iIndex++;
	}
	ACE_OS::memset(m_pTscConfig->sOverlapPhase+iIndex,0,sizeof(SOverlapPhase));

	/********************冲突相位表****************/
	(CDbInstance::m_cGbtTscDb).QueryCollision(tblCollision);
	pCollision = tblCollision.GetData(ucCount);
	iIndex = 0;
	while ( iIndex < ucCount )
	{
		if ( pCollision->ucPhaseId > 0 )
		{
			ACE_OS::memcpy(m_pTscConfig->sPhaseConflict+pCollision->ucPhaseId-1,
				pCollision,sizeof(GBT_DB::Collision));
		}
		pCollision++;
		iIndex++;
	}
	//ACE_OS::memset(m_pTscConfig->sOverlapPhase+iIndex,0,sizeof(GBT_DB::Collision));
	
	/********************通道表*******************/
	(CDbInstance::m_cGbtTscDb).QueryChannel(tblChannel);
	pChannel = tblChannel.GetData(ucCount);
	iIndex = 0;
	while ( iIndex < ucCount )
	{
		ACE_OS::memcpy(m_pTscConfig->sChannel+iIndex,pChannel,sizeof(GBT_DB::Channel));
		pChannel++;
		iIndex++;
	}
	ACE_OS::memset(m_pTscConfig->sChannel+iIndex,0,sizeof(SChannel));
	
	Byte ucStartFlash     = 0;
	Byte ucStartAllRed    = 0;
	Byte ucRemoteCtrlFlag = 0; 
	Byte ucFlashFreq      = 0;
	

	/**********************单元参数 启动黄闪 全红时间*******************/
	(CDbInstance::m_cGbtTscDb).GetUnitCtrlPara(ucStartFlash, ucStartAllRed, ucRemoteCtrlFlag, ucFlashFreq);
	m_pTscConfig->sUnit.ucStartFlashTime  = ucStartFlash;
	m_pTscConfig->sUnit.ucStartAllRedTime = ucStartAllRed;
	
	/**********************降级模式 降级基准方案*******************/
	(CDbInstance::m_cGbtTscDb).GetDegradeCfg(m_pTscConfig->DegradeMode, m_pTscConfig->DegradePattern);

	/**********************公共周期时长 协调相位差********************/
	if ( false == (CDbInstance::m_cGbtTscDb).GetGlobalCycle(m_ucUtcsComCycle) )
	{
		m_ucUtcsComCycle = 0; 
	}
	if ( false == (CDbInstance::m_cGbtTscDb).GetCoorPhaseOffset(m_ucUtscOffset) )
	{
		m_ucUtscOffset   = 0;
	}

	
	/*********************检测器参数***************************/
	(CDbInstance::m_cGbtTscDb).QueryDetector(tblDetector);
	pDetector = tblDetector.GetData(ucCount);
	iIndex = 0;
	ACE_OS::memset(m_pTscConfig->sDetector , 0 , MAX_DETECTOR*sizeof(GBT_DB::Detector) );
	ACE_DEBUG((LM_DEBUG,"%s : %d detector count = %d \n",__FILE__,__LINE__,ucCount));
	while ( iIndex < ucCount )
	{
		if ( pDetector->ucDetectorId > 0 && pDetector->ucPhaseId > 0 )
		{
			ACE_DEBUG((LM_DEBUG,"%s : %d pDetector->ucDetectorId = %d \n",__FILE__,__LINE__,pDetector->ucDetectorId));
			ACE_OS::memcpy(m_pTscConfig->sDetector+(pDetector->ucDetectorId-1) , pDetector , sizeof(GBT_DB::Detector));
			//CDetector::CreateInstance()->m_ucDetError[pDetector->ucDetectorId-1] = DET_NORMAL ; //当存在检测器时，默认工作状态正常
		}
		
		pDetector++;
		iIndex++;
	}
	//ACE_OS::memset(m_pTscConfig->sDetector+iIndex , 0 , sizeof(GBT_DB::Detector));
	SetDetectorPhase();  //检测器与相位关系
	
	/*********************特殊功能开启表***********************/ //设置信号机特殊功能初始化参数
	(CDbInstance::m_cGbtTscDb).QuerySpecFun(tblSpecFun);
	pSpecFun = tblSpecFun.GetData(ucCount);
	iIndex   = 0;
	ACE_OS::memset(m_pTscConfig->sSpecFun , 0 , FUN_COUNT*sizeof(GBT_DB::SpecFun) );
	while ( iIndex < ucCount )
	{
		if ( pSpecFun->ucFunType > 0 )
		{
			ACE_OS::memcpy(m_pTscConfig->sSpecFun+(pSpecFun->ucFunType-1) ,	pSpecFun , sizeof(GBT_DB::SpecFun));
		}
		pSpecFun++;
		iIndex++;
	}
	
	/********************获取配置的灯控板个数********************/
	//GetUseLampBoard(bLampBoardExit);
	//CLamp::CreateInstance()->GetLampBoardExit(bLampBoardExit);

	/********************获取模块表 检测器板的配置情况***********/
	ACE_OS::memset(m_pTscConfig->iDetCfg , 0 , MAX_DET_BOARD*sizeof(int));
	(CDbInstance::m_cGbtTscDb).QueryModule(tblModule);
	pModule = tblModule.GetData(ucCount);
	iIndex = 0;
	
	
	while ( iIndex < ucCount)
	{	
		
		if ( strstr(pModule->strDevNode.GetData(),"DET") != NULL )
		{
			sscanf(pModule->strDevNode.GetData(),"DET-%d-%d",&iBoardIndex,&iDetId);			
			m_pTscConfig->iDetCfg[iBoardIndex] = iDetId;
			ACE_DEBUG((LM_DEBUG,"%s : %d m_pTscConfig->iDetCfg[%d] = %d\n",__FILE__,__LINE__,iBoardIndex,iDetId));
		}
		pModule++;	
		iIndex++;
	}

	CDetector::CreateInstance()->SelectDetectorBoardCfg(m_pTscConfig->iDetCfg);
	
	 
	/********************获取检测器扩展表**********************/
	(CDbInstance::m_cGbtTscDb).QueryDetExtend(tblDetExtend);
	pDetExtend = tblDetExtend.GetData(ucCount);
	iIndex     = 0;

	while ( iIndex < ucCount )
	{
		if ( pDetExtend->ucId > 0 )
		{
			ACE_OS::memcpy(m_pTscConfig->sDetExtend+pDetExtend->ucId-1,pDetExtend,sizeof(GBT_DB::DetExtend));
		}
		pDetExtend++;
		iIndex++;
	}

	//CAdaptive::CreateInstance()->SynPara();  //自适应控制参数

	if ( m_pTscConfig->sSpecFun[FUN_COUNT_DOWN].ucValue != 0 )  //倒计时设备参数
	{
		#ifdef GA_COUNT_DOWN
		CGaCountDown::CreateInstance()->GaUpdateCntDownCfg();
		#endif
	}


	
}


/**************************************************************
Function:       CManaKernel::SetDetectorPhase
Description:    设置检测器与相位的对应关系	
Input:          无              
Output:         无
Return:         无
***************************************************************/
void CManaKernel::SetDetectorPhase()
{
	//bool bIsOverlap = false;
	int iIndex      = 0;
	int iRoadwayCnt = 0;
	Byte ucDetId    = 0;
	Byte ucPhaseId  = 0;

	ACE_OS::memset(m_sPhaseDet , 0 , /*(*/MAX_PHASE/*+MAX_OVERLAP_PHASE)*/*sizeof(SPhaseDetector));

	//m_bPhaseDetCfg = false;

	while ( iIndex < MAX_DETECTOR )
	{
		if ( 0 == m_pTscConfig->sDetector[iIndex].ucDetectorId )
		{
			iIndex++;
			continue;
		}

		//bIsOverlap = ( m_pTscConfig->sDetector[iIndex].ucOptionFlag >> 6 ) & 1;
		ucDetId    = m_pTscConfig->sDetector[iIndex].ucDetectorId;
		ucPhaseId  = m_pTscConfig->sDetector[iIndex].ucPhaseId;

		/*
		if ( ( ucPhaseId > 0 ) 
			&& ( m_pTscConfig->sDetector[iIndex].ucDetFlag>>6 ) )
			*/
		if ( ucPhaseId > 0 )
		{
			/*
			if ( !m_bPhaseDetCfg )
			{
				m_bPhaseDetCfg = true;
			}
			*/

			iRoadwayCnt = m_sPhaseDet[ucPhaseId-1].iRoadwayCnt;
			m_sPhaseDet[ucPhaseId-1].iDetectorId[iRoadwayCnt] = ucDetId;
			m_sPhaseDet[ucPhaseId-1].iRoadwayCnt++;
		}

		iIndex++;
	}
}


/**************************************************************
Function:       CManaKernel::MemeryConfigDataRun
Description:    在没有数据库的情况下，模拟信号机的静态配置参数	
Input:          无              
Output:         无
Return:         无
***************************************************************/
void CManaKernel::MemeryConfigDataRun()
{
	m_pTscConfig->sTimeGroup[0].ucId           = 1;
	m_pTscConfig->sTimeGroup[0].usMonth        = 0xffff;
	m_pTscConfig->sTimeGroup[0].ucDayWithWeek  = 0xff;
	m_pTscConfig->sTimeGroup[0].uiDayWithMonth = 0xffffffff;
	m_pTscConfig->sTimeGroup[0].ucScheduleId   = 1;
	ACE_OS::memset(&(m_pTscConfig->sTimeGroup[1]),0,sizeof(SSchedule));

	m_pTscConfig->sSchedule[0].ucId   = 1;
	m_pTscConfig->sSchedule[0].ucHour = 6;
	m_pTscConfig->sSchedule[0].ucMin  = 0;
	m_pTscConfig->sSchedule[0].ucCtrl = CTRL_SCHEDULE;
	m_pTscConfig->sSchedule[0].ucTimePatternId = 1;
	m_pTscConfig->sSchedule[1].ucId   = 1;
	m_pTscConfig->sSchedule[1].ucHour = 14;
	m_pTscConfig->sSchedule[1].ucMin  = 0;
	m_pTscConfig->sSchedule[1].ucCtrl = CTRL_SCHEDULE;
	m_pTscConfig->sSchedule[1].ucTimePatternId = 1;
	m_pTscConfig->sSchedule[2].ucId   = 1;
	m_pTscConfig->sSchedule[2].ucHour = 17;
	m_pTscConfig->sSchedule[2].ucMin  = 0;
	m_pTscConfig->sSchedule[2].ucCtrl = CTRL_SCHEDULE;
	m_pTscConfig->sSchedule[2].ucTimePatternId = 1;
	ACE_OS::memset(&(m_pTscConfig->sSchedule[3]),0,sizeof(SSchedule));

	m_pTscConfig->sTimePattern[0].ucId             = 1;
	m_pTscConfig->sTimePattern[0].ucScheduleTimeId = 1;
	m_pTscConfig->sTimePattern[0].ucPhaseOffset    = 99;
	m_pTscConfig->sTimePattern[1].ucId             = 2;
	m_pTscConfig->sTimePattern[1].ucScheduleTimeId = 2;	
	m_pTscConfig->sTimePattern[1].ucPhaseOffset    = 99;
	m_pTscConfig->sTimePattern[2].ucId             = 3;
	m_pTscConfig->sTimePattern[2].ucScheduleTimeId = 3;
	m_pTscConfig->sTimePattern[2].ucPhaseOffset    = 99;
	ACE_OS::memset(&(m_pTscConfig->sTimePattern[3]),0,sizeof(STimePattern));

	m_pTscConfig->sScheduleTime[0][0].ucId = 1;
	m_pTscConfig->sScheduleTime[0][0].ucScheduleId = 1;
	m_pTscConfig->sScheduleTime[0][0].usAllowPhase = 0x50;  //南北左转
	m_pTscConfig->sScheduleTime[0][0].ucGreenTime  =  13;
	m_pTscConfig->sScheduleTime[0][0].ucYellowTime = 3;
	m_pTscConfig->sScheduleTime[0][0].ucRedTime    = 3;
	m_pTscConfig->sScheduleTime[0][1].ucId = 1;
	m_pTscConfig->sScheduleTime[0][1].ucScheduleId = 2;
	m_pTscConfig->sScheduleTime[0][1].usAllowPhase = 0x5; //南北直行(伴随南北右转，人行)
	m_pTscConfig->sScheduleTime[0][1].ucGreenTime  =  17;
	m_pTscConfig->sScheduleTime[0][1].ucYellowTime = 3;
	m_pTscConfig->sScheduleTime[0][1].ucRedTime    = 3;
	
	m_pTscConfig->sScheduleTime[0][2].ucId = 1;
	m_pTscConfig->sScheduleTime[0][2].ucScheduleId = 3;
	m_pTscConfig->sScheduleTime[0][2].usAllowPhase = 0xA0;  //东西左转
	m_pTscConfig->sScheduleTime[0][2].ucGreenTime  =  13;
	m_pTscConfig->sScheduleTime[0][2].ucYellowTime = 3;
	m_pTscConfig->sScheduleTime[0][2].ucRedTime    = 3;
	m_pTscConfig->sScheduleTime[0][3].ucId = 1;
	m_pTscConfig->sScheduleTime[0][3].ucScheduleId = 4;
	m_pTscConfig->sScheduleTime[0][3].usAllowPhase = 0xA; //东西直行(伴随东西右转，人行)
	m_pTscConfig->sScheduleTime[0][3].ucGreenTime  = 17;
	m_pTscConfig->sScheduleTime[0][3].ucYellowTime = 3;
	m_pTscConfig->sScheduleTime[0][3].ucRedTime = 3;
	ACE_OS::memset(&(m_pTscConfig->sScheduleTime[0][4]),0,sizeof(SScheduleTime));

	m_pTscConfig->sScheduleTime[1][0].ucId = 2;
	m_pTscConfig->sScheduleTime[1][0].ucScheduleId = 1;
	m_pTscConfig->sScheduleTime[1][0].usAllowPhase = 0x30;
	m_pTscConfig->sScheduleTime[1][0].ucGreenTime =  25;
	m_pTscConfig->sScheduleTime[1][0].ucYellowTime = 3;
	m_pTscConfig->sScheduleTime[1][0].ucRedTime = 3;
	m_pTscConfig->sScheduleTime[1][1].ucId = 2;
	m_pTscConfig->sScheduleTime[1][1].ucScheduleId = 1;
	m_pTscConfig->sScheduleTime[1][1].usAllowPhase = 0x0a;
	m_pTscConfig->sScheduleTime[1][1].ucGreenTime =  25;
	m_pTscConfig->sScheduleTime[1][1].ucYellowTime = 3;
	m_pTscConfig->sScheduleTime[1][1].ucRedTime = 3;
	ACE_OS::memset(&(m_pTscConfig->sScheduleTime[1][2]),0,sizeof(SScheduleTime));

	m_pTscConfig->sScheduleTime[2][0].ucId = 3;
	m_pTscConfig->sScheduleTime[2][0].ucScheduleId = 1;
	m_pTscConfig->sScheduleTime[2][0].usAllowPhase = 0x0a;
	m_pTscConfig->sScheduleTime[2][0].ucGreenTime =  25;
	m_pTscConfig->sScheduleTime[2][0].ucYellowTime = 3;
	m_pTscConfig->sScheduleTime[2][0].ucRedTime = 3;
	m_pTscConfig->sScheduleTime[2][1].ucId = 3;
	m_pTscConfig->sScheduleTime[2][1].ucScheduleId = 1;
	m_pTscConfig->sScheduleTime[2][1].usAllowPhase = 0xc5;
	m_pTscConfig->sScheduleTime[2][1].ucGreenTime =  25;
	m_pTscConfig->sScheduleTime[2][1].ucYellowTime = 3;
	m_pTscConfig->sScheduleTime[2][1].ucRedTime = 3;
	ACE_OS::memset(&(m_pTscConfig->sScheduleTime[2][2]),0,sizeof(SScheduleTime));
	ACE_OS::memset(&(m_pTscConfig->sScheduleTime[3][0]),0,sizeof(SScheduleTime));

	m_pTscConfig->sPhase[0].ucId         = 1;  //北 直行
	m_pTscConfig->sPhase[0].ucPedestrianGreen  = 0;
	m_pTscConfig->sPhase[0].ucFixedGreen = 12;
	m_pTscConfig->sPhase[0].ucType       = 0x80;
	m_pTscConfig->sPhase[0].ucGreenFlash = 3;
	
	m_pTscConfig->sPhase[1].ucId = 2;  //东 直行
	m_pTscConfig->sPhase[1].ucPedestrianGreen  = 0;
	m_pTscConfig->sPhase[1].ucFixedGreen = 12;
	m_pTscConfig->sPhase[1].ucType       = 0x80;
	m_pTscConfig->sPhase[1].ucGreenFlash = 3;
	
	m_pTscConfig->sPhase[2].ucId = 3; //南 直行
	m_pTscConfig->sPhase[2].ucPedestrianGreen  = 0;
	m_pTscConfig->sPhase[2].ucFixedGreen = 10;
	m_pTscConfig->sPhase[2].ucType       = 0x80;
	m_pTscConfig->sPhase[2].ucGreenFlash = 3;

	m_pTscConfig->sPhase[3].ucId = 4;  //西 直行
	m_pTscConfig->sPhase[3].ucPedestrianGreen  = 0;
	m_pTscConfig->sPhase[3].ucFixedGreen = 12;
	m_pTscConfig->sPhase[3].ucType       = 0x80;
	m_pTscConfig->sPhase[3].ucGreenFlash = 3;

	m_pTscConfig->sPhase[4].ucId = 5;  //北 左转
	m_pTscConfig->sPhase[4].ucPedestrianGreen  = 0;
	m_pTscConfig->sPhase[4].ucFixedGreen = 12;
	m_pTscConfig->sPhase[4].ucType       = 0x80;
	m_pTscConfig->sPhase[4].ucGreenFlash = 3;

	m_pTscConfig->sPhase[5].ucId = 6; //东 左转
	m_pTscConfig->sPhase[5].ucPedestrianGreen  = 0;
	m_pTscConfig->sPhase[5].ucFixedGreen = 12;
	m_pTscConfig->sPhase[5].ucType       = 0x80;
	m_pTscConfig->sPhase[5].ucGreenFlash = 3;

	m_pTscConfig->sPhase[6].ucId = 7;  //南 左转
	m_pTscConfig->sPhase[6].ucPedestrianGreen  = 0;
	m_pTscConfig->sPhase[6].ucFixedGreen = 12;
	m_pTscConfig->sPhase[6].ucType       = 0x80;
	m_pTscConfig->sPhase[6].ucGreenFlash = 3;

	m_pTscConfig->sPhase[7].ucId = 8;  //西 左转
	m_pTscConfig->sPhase[7].ucPedestrianGreen  = 0;
	m_pTscConfig->sPhase[7].ucFixedGreen = 12;
	m_pTscConfig->sPhase[7].ucType       = 0x80;
	m_pTscConfig->sPhase[7].ucGreenFlash = 3;

	for (int i=0; i<4; i++)  //右转
	{ 
		m_pTscConfig->sPhase[8+i].ucId         = 9 + i;  
		m_pTscConfig->sPhase[8+i].ucPedestrianGreen  = 0;
		m_pTscConfig->sPhase[8+i].ucFixedGreen = 12;
		m_pTscConfig->sPhase[8+i].ucType       = 0x80;
		m_pTscConfig->sPhase[8+i].ucGreenFlash = 3;
	}

	for (int i=0; i<4; i++)  //人行
	{ 
		m_pTscConfig->sPhase[12+i].ucId         = 13 + i;  
		m_pTscConfig->sPhase[12+i].ucPedestrianGreen  = 8;
		m_pTscConfig->sPhase[12+i].ucFixedGreen = 12;
		m_pTscConfig->sPhase[12+i].ucType       = 0x80;
		m_pTscConfig->sPhase[12+i].ucGreenFlash = 3;
	}
	ACE_OS::memset(&(m_pTscConfig->sPhase[16]),0,sizeof(SPhase));

	m_pTscConfig->sOverlapPhase[0].ucId = 1;
	m_pTscConfig->sOverlapPhase[0].ucIncludePhaseLen = 4;
	m_pTscConfig->sOverlapPhase[0].ucIncludePhase[0] = 9;
	m_pTscConfig->sOverlapPhase[0].ucIncludePhase[1] = 10;
	m_pTscConfig->sOverlapPhase[0].ucIncludePhase[2] = 13;
	m_pTscConfig->sOverlapPhase[0].ucIncludePhase[3] = 14;

	m_pTscConfig->sOverlapPhase[1].ucId = 2;
	m_pTscConfig->sOverlapPhase[1].ucIncludePhaseLen = 4;
	m_pTscConfig->sOverlapPhase[1].ucIncludePhase[0] = 11;
	m_pTscConfig->sOverlapPhase[1].ucIncludePhase[1] = 12;
	m_pTscConfig->sOverlapPhase[1].ucIncludePhase[2] = 15;
	m_pTscConfig->sOverlapPhase[1].ucIncludePhase[3] = 16;

	ACE_OS::memset(&(m_pTscConfig->sOverlapPhase[3]),0,sizeof(SOverlapPhase));

	//冲突相位暂不定
	//m_pTscConfig->sPhaseConfct[0].ucId = 0;
	
	m_pTscConfig->sChannel[0].ucId = 1;
	m_pTscConfig->sChannel[0].ucSourcePhase = 1;
	m_pTscConfig->sChannel[1].ucId = 2;
	m_pTscConfig->sChannel[1].ucSourcePhase = 2;
	m_pTscConfig->sChannel[2].ucId = 3;
	m_pTscConfig->sChannel[2].ucSourcePhase = 3;
	m_pTscConfig->sChannel[3].ucId = 4;
	m_pTscConfig->sChannel[3].ucSourcePhase = 4;
	m_pTscConfig->sChannel[4].ucId = 5;
	m_pTscConfig->sChannel[4].ucSourcePhase = 5;
	m_pTscConfig->sChannel[5].ucId = 6;
	m_pTscConfig->sChannel[5].ucSourcePhase = 6;
	m_pTscConfig->sChannel[6].ucId = 7;
	m_pTscConfig->sChannel[6].ucSourcePhase = 7;
	m_pTscConfig->sChannel[7].ucId = 8;
	m_pTscConfig->sChannel[7].ucSourcePhase = 8;
	m_pTscConfig->sChannel[8].ucId = 9;
	m_pTscConfig->sChannel[8].ucSourcePhase = 11;
	m_pTscConfig->sChannel[9].ucId = 10;
	m_pTscConfig->sChannel[9].ucSourcePhase = 9;
	m_pTscConfig->sChannel[10].ucId = 11;
	m_pTscConfig->sChannel[10].ucSourcePhase = 12;
	m_pTscConfig->sChannel[11].ucId = 12;
	m_pTscConfig->sChannel[11].ucSourcePhase = 10;
	m_pTscConfig->sChannel[12].ucId = 13;
	m_pTscConfig->sChannel[12].ucSourcePhase = 13;
	m_pTscConfig->sChannel[13].ucId = 14;
	m_pTscConfig->sChannel[13].ucSourcePhase = 15;
	m_pTscConfig->sChannel[14].ucId = 15;
	m_pTscConfig->sChannel[14].ucSourcePhase = 14;
	m_pTscConfig->sChannel[15].ucId = 16;
	m_pTscConfig->sChannel[15].ucSourcePhase = 16;

	m_pTscConfig->sUnit.ucStartFlashTime = 12;
	m_pTscConfig->sUnit.ucStartAllRedTime = 6;
}


/**************************************************************
Function:       CManaKernel::UpdateConfig
Description:    更新信号机静态配置参数,从数据库加载各个表的信息到
				内存中,当数据库得到更新时，在一个周期结束后调用该函数
Input:          无              
Output:         无
Return:         无
***************************************************************/
void CManaKernel::UpdateConfig()
{	
#ifdef NO_DATABASE_OPERATE
	MemeryConfigDataRun();
#else
	SelectDataFromDb();
#endif
}


/**************************************************************
Function:       CManaKernel::DecTime
Description:    1s执行一次，运行时间++ 主要功能是判断是否到了下一
				步时间，是的话就发送TSC消息步进，不是的话则将当前步
				剩余时间减少一秒
Input:          无              
Output:         无
Return:         无
***************************************************************/
void CManaKernel::DecTime()
{
	m_bAddTimeCount = true;
	/*
	if(m_bFinishBoot == false && m_pRunData->uiWorkStatus == ALLRED)
	{
		static Byte iChkTime = 0 ;
		iChkTime ++ ;
		if(CLampBoard::CreateInstance()->IsChkLight == false && m_pRunData->bIsChkLght == true && iChkTime == 3)
		{		
			iChkTime = 0 ;
			ACE_DEBUG((LM_DEBUG,"%s:%d  Begin LampCheck!\n",__FILE__,__LINE__));		
			CLampBoard::CreateInstance()->IsChkLight = true ;   
		}

	}
	*/
	
	if(m_pRunData->uiWorkStatus == STANDARD && m_pRunData->uiCtrl == CTRL_VEHACTUATED )
	//ACE_DEBUG((LM_DEBUG,"%s:%d ctrl:%d  ucElapseTime:%02d,ucStepTime:%02d m_iMinStepTime:%02d\n",
		//	__FILE__,__LINE__,m_pRunData->uiCtrl , m_pRunData->ucElapseTime , m_pRunData->ucStepTime,m_iMinStepTime	));	
	#ifdef  SPECIAL_CNTDOWN 
	if ( m_pTscConfig->sSpecFun[FUN_COUNT_DOWN].ucValue != 0  )
	{	
	if (m_iMinStepTime -7 ==  m_pRunData->ucElapseTime &&  m_pRunData->b8cndtown == false )
	{
		if(m_pRunData->uiCtrl == CTRL_VEHACTUATED && m_pRunData->uiWorkStatus == STANDARD)
		m_pRunData->b8cndtown = true ;		
		//ACE_DEBUG((LM_DEBUG,"%s:%d m_pRunData->b8cndtown = true\n",__FILE__,__LINE__));	
	}
	if( (m_pRunData->ucStepTime-m_pRunData->ucElapseTime) <=8 && m_pRunData->b8cndtown == true)
	{
		if(m_pRunData->uiWorkStatus == STANDARD && m_pRunData->uiCtrl == CTRL_VEHACTUATED)
		{
			if(iCntFlashTime>0)            //绿闪时间覆盖
				iCntFlashTime-- ;
			else		
				m_pRunData->b8cndtown = false ;	
			//ACE_DEBUG((LM_DEBUG,"%s:%d lefttime<8ge \n",__FILE__,__LINE__));
		}		
	}
	if(m_pRunData->b8cndtown == true && m_pRunData->uiCtrl == CTRL_VEHACTUATED)
	{		
		//ACE_DEBUG((LM_DEBUG,"%s:%d show 8 cntdown\n",__FILE__,__LINE__));	
		//CGaCountDown::CreateInstance()->cntdownsend();
			
			CGaCountDown::CreateInstance()->send8cntdown();
	}
	}		
	#endif
	m_pRunData->ucElapseTime++;

	if ( CTRL_UTCS == m_pRunData->uiCtrl )
	{
		m_pRunData->uiUtcsHeartBeat++;
	//	ACE_DEBUG((LM_DEBUG,"%s:%d uiUtcsHeartBeat = %d\n",__FILE__,__LINE__,++(m_pRunData->uiUtcsHeartBeat)));
	}

	if ( m_bWaitStandard )  //信号机正处于黄闪-->全红  手动又进入
	{
		if ( CTRL_PANEL == m_pRunData->uiCtrl )  //面板手动控制
		{
			return;
		}
	}
	else if ( !m_bWaitStandard )
	{
		if ( CTRL_MANUAL == m_pRunData->uiCtrl )  //手动控制
		{
			return;
		}

		if ( ( CTRL_PANEL == m_pRunData->uiCtrl ) && ( (SIGNALOFF == m_pRunData->uiWorkStatus ) || (ALLRED == m_pRunData->uiWorkStatus) 
			|| (FLASH == m_pRunData->uiWorkStatus ) ) )  //面板关灯、全红、黄闪
		{
			return;
		}

		if ( (CTRL_PANEL == m_pRunData->uiCtrl) && (0 == m_iTimePatternId || (250 == m_iTimePatternId)))  //面板控制且非指定配时方案
		{
			//ACE_DEBUG((LM_DEBUG,"\n%s:%d CTRL_PANEL and m_iTimePatternId=0 or 250\n",__FILE__,__LINE__));
			return;
		}
	}
	if ( m_pRunData->ucElapseTime >= m_pRunData->ucStepTime )
	{
		/*
		if ( CTRL_ACTIVATE == m_pRunData->uiCtrl )
		{
			CAdaptive::CreateInstance()->RecordStageData();
		}
		*/
		
		
		SThreadMsg sMsg;
		sMsg.ulType = TSC_MSG_NEXT_STEP;
		sMsg.pDataBuf = NULL;
		CTscMsgQueue::CreateInstance()->SendMessage(&sMsg,sizeof(sMsg));
	}

	if ( m_pTscConfig->sSpecFun[FUN_COUNT_DOWN].ucValue != 0 )
	{
		if (   ( SIGNALOFF   == m_pRunData->uiWorkStatus )
			|| ( ALLRED      == m_pRunData->uiWorkStatus ) 
			|| ( FLASH       == m_pRunData->uiWorkStatus ) 
			|| ( CTRL_MANUAL == m_pRunData->uiWorkStatus ) )//这里判断的状态取值有误?
		{
			;   //
		}
		else if ( m_pRunData->ucElapseTime != m_pRunData->ucStepTime - 1 )  //避免步伐的最后1S以及下一步开始的第1S重复
		{
			;   //
		}
	}

}


/**************************************************************
Function:       CManaKernel::GoNextStep
Description:    一个步伐走完，切换到下一个步伐
Input:          无              
Output:         无
Return:         无
***************************************************************/
void CManaKernel::GoNextStep()
{
	bool bStageFirstStep      = false;
	Byte ucStageNo            = 0;
	static Byte ucLastStageNo = 255;

	ACE_Guard<ACE_Thread_Mutex>  guard(m_mutexRunData);

	m_pRunData->ucStepNo++;
	//if(m_pRunData->uiWorkStatus == STANDARD)
	//	CGaCountDown::CreateInstance()->send8cntdown();
	//MOD:201309251030 修改相位冲突检测位置，从下一周期移到下一步处进行检测
	if ( InConflictPhase()  )  //相位冲突表 出现同时亮的绿灯
	{			
		SndMsgLog(LOG_TYPE_GREEN_CONFIG,3,0,0,0); // 3表示相位冲突 ADD：201309251130
		//DealGreenConflict(1);
		ACE_DEBUG((LM_DEBUG,"%s:%d InConflictPhase !\n",__FILE__,__LINE__));
		CFlashMac::CreateInstance()->FlashForceStart(2) ; //绿冲突，强制黄闪烁
		bDegrade = true ;
		return ;
	}
	if ( m_pRunData->ucStepNo >= m_pRunData->ucStepNum )
	{
		/*
		if ( CTRL_ACTIVATE == m_pRunData->uiCtrl )
		{
			CAdaptive::CreateInstance()->RecordCycleData();
		}
		*/

		SThreadMsg sMsg;
		sMsg.ulType = TSC_MSG_OVER_CYCLE;
		sMsg.pDataBuf = NULL;
		CTscMsgQueue::CreateInstance()->SendMessage(&sMsg,sizeof(sMsg));

		if ( ( (m_pTscConfig->sSpecFun[FUN_PRINT_FLAG].ucValue>>7) & 1 )  == 0 )//MOD:2013 0722 09 30
		{
			//ACE_DEBUG((LM_DEBUG,"TSC_MSG_OVER_CYCLE\n\n"));
		}

		return;
	}
	else if ( m_pRunData->ucStepNo < m_pRunData->ucStepNum )
	{
		if ( ( (m_pTscConfig->sSpecFun[FUN_PRINT_FLAG].ucValue>>7) & 1 )  == 0 ) //MOD:2013 0722 09 30
		{
			//ACE_DEBUG((LM_DEBUG,"NextStep\n"));
		}

		//从已有的步伐表快速加载下一步
		switch ( m_pRunData->uiCtrl )
		{
			case CTRL_WIRELESS:
				m_pRunData->ucStepTime = CWirelessCoord::CreateInstance()->GetStepLength(m_pRunData->ucStepNo);
				//m_pRunData->ucStepTime = m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucStepLen;
				break;

			case CTRL_UTCS:
				if ( (m_ucUtcsComCycle != 0) && (m_ucUtscOffset != 0) )
				{   ACE_DEBUG((LM_DEBUG,"%s:%d Change NextStep m_ucUtcsComCycle=%d ,m_ucUtscOffset= %d !\n",__FILE__,__LINE__,m_ucUtcsComCycle,m_ucUtscOffset ));
					m_pRunData->ucStepTime = CWirelessCoord::CreateInstance()->GetStepLength(m_pRunData->ucStepNo);
					if ( 0 == m_pRunData->ucStepTime )
					{   ACE_DEBUG((LM_DEBUG,"%s:%d Change NextStep after cooridate 0 == m_pRunData->ucStepTime%d !\n",__FILE__,__LINE__ ));
						m_pRunData->ucStepTime = m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucStepLen;
					}
				}
				else
				{	ACE_DEBUG((LM_DEBUG,"%s:%d Change NextStep m_ucUtcsComCycle=0 ,m_ucUtscOffset = 0 StepTime no changed!\n" ,__FILE__,__LINE__));

					    m_pRunData->ucStepTime = m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucStepLen;
				}
				break;

			case CTRL_ACTIVATE:
				ucStageNo = StepToStage(m_pRunData->ucStepNo,&bStageFirstStep);
				if ( bStageFirstStep && IsLongStep(m_pRunData->ucStepNo) )   //长步且为阶段的第一步
				{
					//m_pRunData->ucStepTime = 
						   //CAdaptive::CreateInstance()->GetStageGreenLen(m_pRunData->ucStepNo,ucStageNo);
				}
				else
				{
					m_pRunData->ucStepTime = m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucStepLen;
				}
				break;

			case CTRL_VEHACTUATED:   //感应控制状态下步伐改变
			case CTRL_MAIN_PRIORITY: 
			case CTRL_SECOND_PRIORITY: 
				StepToStage(m_pRunData->ucStepNo,&bStageFirstStep);
				if ( bStageFirstStep && IsLongStep(m_pRunData->ucStepNo) )   //长步且为阶段的第一步 长步就是当前有绿灯亮
				{
					m_iMinStepTime = GetCurStepMinGreen(m_pRunData->ucStepNo , &m_iMaxStepTime , &m_iAdjustTime  );
					m_bVehile = true;
				}
				else
				{
					m_bVehile = false;
				}
				m_pRunData->ucStepTime = m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucStepLen;
				break;

			default:
				m_pRunData->ucStepTime = m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucStepLen;
				break;
		}
		m_pRunData->ucElapseTime = 0;
		//m_pRunData->ucStepTime = m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucStepLen;
		m_pRunData->ucStageNo = StepToStage(m_pRunData->ucStepNo,NULL); //根据步伐号获取阶段号

		CLampBoard::CreateInstance()->SetLamp(m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucLampOn
			,m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucLampFlash);
	}
	
	
	if ( ucLastStageNo != m_pRunData->ucStageNo )
	{
		m_ucAddTimeCnt = 0;
		ucLastStageNo  = m_pRunData->ucStageNo;
	}
	
	CwpmGetCntDownSecStep();
}


/**************************************************************
Function:       CManaKernel::OverCycle
Description:    一个周期走完
Input:          无              
Output:         无
Return:         无
***************************************************************/
void CManaKernel::OverCycle()
{
	bool bStageFirstStep      = false;
	Byte ucStageNo            = 0;
	int iCurTimePatternId     = 1;
	int iStepLength[MAX_STEP] = {0};
	
	if( m_iTimePatternId == 251) //如果时临时相位组合方案则返回原来控制状态
	{
			m_iTimePatternId = 0 ;
			GetRunDataStandard(); //正常地构造动态数据
			CLampBoard::CreateInstance()->SetLamp(m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucLampOn
		,m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucLampFlash);
			ACE_DEBUG((LM_DEBUG,"%s:%d 60 seconds over\n",__FILE__,__LINE__));
			bTmpPattern = false ;
			return ;
	}
	if ( m_pRunData->bNeedUpdate )
	{
		UpdateConfig();
	}
	
	CDetector::CreateInstance()->GetAllWorkSts();     //一个周期检测一次检测器工作状态用于感应降级后再升级

	if ( FLASH == m_pRunData->uiWorkStatus )  //黄闪完 进入全红
	{	
		if ( m_bSpeStatusTblSchedule )  //时段表定义的黄闪状态
		{
			m_pRunData->uiOldWorkStatus = FLASH;
			m_pRunData->uiWorkStatus    = STANDARD;
			m_bVirtualStandard          = true;
		}
		else
		{
			if(m_bFinishBoot == false)
			{
				CDetector::CreateInstance()->SearchAllStatus();				
				
			}
			m_pRunData->uiWorkStatus = ALLRED;
		}
		ResetRunData(0);
		
	}
	else
	{
		if ( m_bSpeStatusTblSchedule )  //时段表定义的关灯或者全红
		{
			m_pRunData->uiOldWorkStatus = m_pRunData->uiWorkStatus;
			m_pRunData->uiWorkStatus    = STANDARD;
			m_bVirtualStandard         = true;
		}


		if ( ALLRED == m_pRunData->uiWorkStatus )  //全红完 进入标准
		{
			
			m_bWaitStandard             = false;
			m_pRunData->uiWorkStatus    = STANDARD;
			m_bFinishBoot               = true;
			CDetector::CreateInstance()->SearchAllStatus();
			Ulong mRestart = 0 ;
		(CDbInstance::m_cGbtTscDb).GetSystemData("ucDownloadFlag",mRestart);
		if(mRestart >0)
		{
			#ifdef LINUX
			usleep(100000);
			#endif
			CManaKernel::CreateInstance()->SndMsgLog(LOG_TYPE_OTHER,2,mRestart,0,0);	
			(CDbInstance::m_cGbtTscDb).SetSystemData("ucDownloadFlag",0);  // 故障清零
		}
			
	}
		if ( (m_pRunData->uiWorkStatus!=FLASH) && (m_pRunData->uiWorkStatus!=ALLRED) && (m_pRunData->uiWorkStatus!=SIGNALOFF) )
		{
			/*
			ValidSoftWare();
			if(bValidSoftWare == false)
			{
				CGbtMsgQueue::CreateInstance()->SendTscCommand(OBJECT_SWITCH_SYSTEMCONTROL,254);
				return ;
			}		
			*/
			switch ( m_pRunData->uiCtrl )
			{
			case CTRL_MANUAL:
			break;
			case CTRL_UTCS:
				ACE_DEBUG((LM_DEBUG,"%s:%d UtcsHeartBeat= %d ucCycle =%d\n",__FILE__,__LINE__,m_pRunData->uiUtcsHeartBeat,m_pRunData->ucCycle));
				if ( (Byte)m_pRunData->uiUtcsHeartBeat >= m_pRunData->ucCycle )  //1个周期没有收到心跳联网指令
				{					
						bUTS = false ;
						bDegrade = true ;
						m_pRunData->uiOldCtrl = CTRL_UTCS;
						
						if(m_pTscConfig->DegradeMode > 0)
						{
							if(m_pTscConfig->DegradeMode >= 5)
							{
								m_pRunData->uiCtrl    = CTRL_SCHEDULE;	
								SndMsgLog(LOG_TYPE_OTHER,1,m_pRunData->uiOldCtrl,m_pRunData->uiCtrl,1); //日志记录控制方式切换 ADD?201311041530													
								ACE_DEBUG((LM_DEBUG,"%s:%d oldctrl = %d newctrl =%d\n"	,__FILE__,__LINE__,CTRL_UTCS,CTRL_SCHEDULE));

								
								if(m_pTscConfig->DegradePattern>0)
								{
									m_iTimePatternId = m_pTscConfig->DegradePattern;
									bTmpPattern = true ;
								}
							}
							else
								CGbtMsgQueue::CreateInstance()->SendTscCommand(OBJECT_SWITCH_CONTROL,m_pTscConfig->DegradeMode);
							m_pRunData->bNeedUpdate = true;
							
						}
						else
						{
							m_pRunData->uiOldCtrl =CTRL_UTCS;
							m_pRunData->uiCtrl    = CTRL_SCHEDULE;
							m_pRunData->bNeedUpdate = true;
							SndMsgLog(LOG_TYPE_OTHER,1,CTRL_UTCS,CTRL_SCHEDULE,1); //日志记录控制方式切换 ADD?201311041530
							ACE_DEBUG((LM_DEBUG,"%s:%d oldctrl = %d newctrl =%d \n"	,__FILE__,__LINE__,CTRL_UTCS,CTRL_SCHEDULE));
						}
						CWirelessCoord::CreateInstance()->SetDegrade() ;   //降级清零设置  ADD：201311121430
						CMainBoardLed::CreateInstance()->DoModeLed(true,true);
						ACE_DEBUG((LM_DEBUG,"%s:%d too long no utcs command！\n"	,__FILE__,__LINE__));
				}
				break;
			case CTRL_VEHACTUATED:
			case CTRL_MAIN_PRIORITY:   
			case CTRL_SECOND_PRIORITY:
				if ( !CDetector::CreateInstance()->HaveDetBoard() || CTRL_VEHACTUATED != m_pRunData->uiScheduleCtrl )/*|| !m_bPhaseDetCfg*/  /*不存在检测器板*/
				{
					ACE_DEBUG((LM_DEBUG,"%s:%d When change cycle,no DetBoard or uiCtrl != VEHACTUATED\n",__FILE__,__LINE__));
					m_pRunData->uiOldCtrl = m_pRunData->uiCtrl;					
					m_pRunData->uiCtrl    = CTRL_SCHEDULE;
					
					m_pRunData->bNeedUpdate = true;	
					bDegrade = true ;	
					CMainBoardLed::CreateInstance()->DoModeLed(true,true);				
					SndMsgLog(LOG_TYPE_OTHER,1,m_pRunData->uiOldCtrl,m_pRunData->uiCtrl,1); //日志记录控制方式切换 ADD?201311041530
					ACE_DEBUG((LM_DEBUG,"%s:%d oldctrl = %d newctrl = %d\n"	,__FILE__,__LINE__,CTRL_VEHACTUATED,CTRL_SCHEDULE));
					break;
				}
				break;
			case CTRL_ACTIVATE:
				break;

			case CTRL_WIRELESS:
				
				iCurTimePatternId = m_pRunData->ucTimePatternId > 0 ? m_pRunData->ucTimePatternId : 1;
				/*
				if ( CTRL_WIRELESS != m_pRunData->uiScheduleCtrl
					|| m_pTscConfig->sTimePattern[iCurTimePatternId-1].ucPhaseOffset > 99 
					|| CManaKernel::CreateInstance()->m_pTscConfig->sSpecFun[FUN_GPS].ucValue == 0 )
				{
					m_pRunData->uiOldCtrl = m_pRunData->uiCtrl;
					m_pRunData->uiCtrl    = CTRL_SCHEDULE;
					ACE_DEBUG((LM_DEBUG,"%s:%d oldctrl = %d newctrl = %d\n"	,__FILE__,__LINE__,CTRL_WIRELESS,CTRL_SCHEDULE));
					SndMsgLog(LOG_TYPE_OTHER,1,m_pRunData->uiOldCtrl,m_pRunData->uiCtrl,1); //日志记录控制方式切换 ADD?201311041530
				}
				*/
				break;
			default:
				iCurTimePatternId = m_pRunData->ucTimePatternId > 0 ? m_pRunData->ucTimePatternId : 1;
				if ( CDetector::CreateInstance()->HaveDetBoard() /*&& m_bPhaseDetCfg*/ 
					&& CTRL_VEHACTUATED == m_pRunData->uiScheduleCtrl )  //存在检测器板且有配置检测器参数
				{
					ACE_DEBUG((LM_DEBUG,"%s:%d  switch break ",__FILE__,__LINE__));
					m_pRunData->uiOldCtrl = m_pRunData->uiCtrl;
					m_pRunData->uiCtrl    = CTRL_VEHACTUATED;
					
					m_pRunData->bNeedUpdate = true;
					ACE_DEBUG((LM_DEBUG,"%s:%d oldctrl = %d newctrl = %d\n"	,__FILE__,__LINE__,m_pRunData->uiCtrl, CTRL_VEHACTUATED));
					SndMsgLog(LOG_TYPE_OTHER,1,m_pRunData->uiOldCtrl,m_pRunData->uiCtrl,1); //日志记录控制方式切换 ADD?201311041530
					break;
				}
				else if ( CTRL_WIRELESS == m_pRunData->uiScheduleCtrl && m_pTscConfig->sTimePattern[iCurTimePatternId-1].ucPhaseOffset < 99 
					&& CManaKernel::CreateInstance()->m_pTscConfig->sSpecFun[FUN_GPS].ucValue != 0 )  //设置相位差并且开启gps
				{
					m_pRunData->uiOldCtrl = m_pRunData->uiCtrl;
					m_pRunData->uiCtrl    = CTRL_WIRELESS;
					SndMsgLog(LOG_TYPE_OTHER,1,m_pRunData->uiOldCtrl,m_pRunData->uiCtrl,1); //日志记录控制方式切换 ADD?201311041530
				}
				
				break;
			}
		}
		
		
		ResetRunData(0);  //正常的重新获取动态参数
		
		m_pRunData->bNeedUpdate = false;

		if ( m_pRunData->bOldLock )
		{
			m_pRunData->bOldLock = false;
			ACE_DEBUG((LM_DEBUG,"%s:%d  begin setcyclestepinfo\n",__FILE__,__LINE__));
			SetCycleStepInfo(0); //单单构造整个周期的步伐信息即可
		}

		m_pRunData->ucStepNo     = 0;
		m_pRunData->ucStageNo    = 0;
		m_pRunData->ucElapseTime = 0;
		m_pRunData->ucStepTime   = m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucStepLen;
		m_pRunData->ucRunTime    = m_pRunData->ucStepTime;
	}

	if ( CTRL_WIRELESS == m_pRunData->uiCtrl )
	{
		for ( int i=0; i<MAX_STEP && i<m_pRunData->ucStepNum; i++ )
		{
			iStepLength[i] = m_pRunData->sStageStepInfo[i].ucStepLen;
		}
		ACE_DEBUG((LM_DEBUG,"%s:%d  iCurTimePatternId=%d ucPhaseOffset=%d\n",__FILE__,__LINE__,m_pRunData->ucTimePatternId, m_pTscConfig->sTimePattern[m_pRunData->ucTimePatternId-1].ucPhaseOffset));
		CWirelessCoord::CreateInstance()->SetStepInfo( false
							    , m_pRunData->ucStepNum
								, m_pRunData->ucCycle
			                    , m_pTscConfig->sTimePattern[m_pRunData->ucTimePatternId-1].ucPhaseOffset
								, iStepLength);
		CWirelessCoord::CreateInstance()->OverCycle();
	}
	else if ( (CTRL_UTCS == m_pRunData->uiCtrl) && (m_ucUtcsComCycle != 0) 
				/*&& (m_ucUtscOffset != 0)*/ )   //联控标准时 即使相位差为0也进行周期时长的调整
	{
		if ( m_pRunData->ucCycle != m_ucUtcsComCycle )
		{  ACE_DEBUG((LM_DEBUG,"%s:%d  m_pRunData->ucCycle != m_ucUtcsComCycle,UtcsAdjustCycle\n",__FILE__,__LINE__));
			UtcsAdjustCycle();
		}
		for ( int i=0; i<MAX_STEP && i<m_pRunData->ucStepNum; i++ )
		{
			iStepLength[i] = m_pRunData->sStageStepInfo[i].ucStepLen;
		}
				ACE_DEBUG((LM_DEBUG,"%s:%d  begin UTCS setcyclestepinfo,m_pRunData->ucStepNum=%d\n",__FILE__,__LINE__,m_pRunData->ucStepNum));

		CWirelessCoord::CreateInstance()->SetStepInfo( true
			, m_pRunData->ucStepNum
			, m_ucUtcsComCycle
			, m_ucUtscOffset
			, iStepLength);
		CWirelessCoord::CreateInstance()->OverCycle();
	}
	else if ( ( CTRL_VEHACTUATED     == m_pRunData->uiCtrl ) 
		   || ( CTRL_MAIN_PRIORITY   == m_pRunData->uiCtrl ) 
		   || ( CTRL_SECOND_PRIORITY == m_pRunData->uiCtrl ) )
	{
		//ACE_DEBUG((LM_DEBUG,"%s:%d  when uiCtrl == 7 , GetAllStageDetector!\n",__FILE__,__LINE__));
		GetAllStageDetector();
	}
	else if ( (CTRL_ACTIVATE == m_pRunData->uiCtrl)  && ( STANDARD  == m_pRunData->uiWorkStatus) )
	{
		GetAllStageDetector();
		//CAdaptive::CreateInstance()->OverCycle();
	}

	switch ( m_pRunData->uiCtrl )
	{
		case CTRL_WIRELESS:
			m_pRunData->ucStepTime = CWirelessCoord::CreateInstance()->GetStepLength(m_pRunData->ucStepNo);
			//m_pRunData->ucStepTime = m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucStepLen;
			m_pRunData->ucRunTime  = m_pRunData->ucStepTime;
			break;

		case CTRL_UTCS:
			if ( (m_ucUtcsComCycle != 0) && (m_ucUtscOffset != 0) )
			{
				m_pRunData->ucStepTime = CWirelessCoord::CreateInstance()->GetStepLength(m_pRunData->ucStepNo);
				if ( 0 == m_pRunData->ucStepTime )
				{
					m_pRunData->ucStepTime = m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucStepLen;
				}
			}
			else
			{
				m_pRunData->ucStepTime = m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucStepLen;
			}
			break;

		case CTRL_VEHACTUATED:
		case CTRL_MAIN_PRIORITY: 
		case CTRL_SECOND_PRIORITY: 
			StepToStage(m_pRunData->ucStepNo,&bStageFirstStep);
			if ( IsLongStep(m_pRunData->ucStepNo) && bStageFirstStep )   //长步且为阶段的第一步
			{
				//ACE_DEBUG((LM_DEBUG,"%s:%d  uiCtrl == 7 and islongsetp and bFirstStep and m_bVehile == true!\n",__FILE__,__LINE__));
				m_iMinStepTime = GetCurStepMinGreen(m_pRunData->ucStepNo , &m_iMaxStepTime , &m_iAdjustTime  );
				m_bVehile = true;
			}
			else
			{
				m_bVehile = false;
			}
			break;
		case CTRL_ACTIVATE:
			ucStageNo = StepToStage(m_pRunData->ucStepNo,&bStageFirstStep);
			if ( bStageFirstStep && IsLongStep(m_pRunData->ucStepNo) )   //长步且为阶段的第一步
			{
				//m_pRunData->ucStepTime = 
					//CAdaptive::CreateInstance()->GetStageGreenLen(m_pRunData->ucStepNo,ucStageNo);
			}
			else
			{
				m_pRunData->ucStepTime = m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucStepLen;
			}
			break;
		default:
			break;
	}

	CLampBoard::CreateInstance()->SetLamp(m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucLampOn	,m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucLampFlash);

	if ( m_pRunData->bStartFlash ) 
	{
		m_pRunData->bStartFlash = false;
	}
	CwpmGetCntDownSecStep();
	//CLamp::CreateInstance()->SetOverCycle();
	SetCycleBit(true);
}


/**************************************************************
Function:       CManaKernel::SetCycleBit
Description:    设置周期位
Input:          无              
Output:         无
Return:         无
***************************************************************/
void CManaKernel::SetCycleBit(bool bSetCycle)
{
	//ACE_Guard<ACE_Thread_Mutex>  guard(m_mutexSetCycle);
	m_bCycleBit = bSetCycle;
}


/**************************************************************
Function:       CManaKernel::CwpmGetCntDownSecStep
Description:    发送倒计时信息
Input:          无              
Output:         无
Return:         无
***************************************************************/
void CManaKernel::CwpmGetCntDownSecStep()
{
	if ( (SIGNALOFF     == m_pRunData->uiWorkStatus)

		|| (ALLRED      == m_pRunData->uiWorkStatus) 

		|| (FLASH       == m_pRunData->uiWorkStatus) 

		|| (CTRL_MANUAL == m_pRunData->uiCtrl) 
		|| (CTRL_PANEL == m_pRunData->uiCtrl )) 		

	{

		return ;

	}
	if ( m_pTscConfig->sSpecFun[FUN_COUNT_DOWN].ucValue != 0 && MODE_TSC == m_pTscConfig->sSpecFun[FUN_CROSS_TYPE].ucValue )

	{
		#ifdef GA_COUNT_DOWN
		//ACE_DEBUG((LM_DEBUG,"%s:%d 输出倒计时，StepNum=%d！\n" ,__FILE__,__LINE__,m_pRunData->ucStepNo));
		CGaCountDown::CreateInstance()->GaSendStepPer();
		#endif

	}

	
}


/**************************************************************
Function:       CManaKernel::ResetRunData
Description:    重新设置信号机动态参数,正常一个周期一次,改变信号机
				状态的时候也会调用
Input:          ucTime :  黄闪或全红的时间           
Output:         无
Return:         无
***************************************************************/
void CManaKernel::ResetRunData(Byte ucTime)
{
#ifdef CHECK_MEMERY
	TestMem(__FILE__,__LINE__);
#endif

	if ( FLASH == m_pRunData->uiWorkStatus )// 初始化值为 CTRL_SCHEDULE;
	{
		if ( m_bSpeStatusTblSchedule )
		{
			m_pRunData->ucStepTime = MAX_SPESTATUS_CYCLE;
		}
		else if ( 0 == ucTime )
		{
			m_pRunData->ucStepTime = m_pTscConfig->sUnit.ucStartFlashTime;
			//ACE_DEBUG((LM_DEBUG,"%s:%d 启动进入黄闪 10秒开始\n" ,__FILE__,__LINE__));
		}
		else
		{
			m_pRunData->ucStepTime = ucTime;
		}
		
		m_pRunData->ucElapseTime = 0;
		m_pRunData->ucStepNo     = 0;
		m_pRunData->ucStepNum    = 1;
		m_pRunData->ucRunTime    = m_pRunData->ucStepTime;
		m_pRunData->ucStageNo    = 0;
		m_pRunData->ucStageCount = 1;
		
		if ( m_bSpePhase )  //特定相位绿闪
		{
			m_bSpePhase = false;
			for ( int i=0; i<MAX_LAMP; i++ )
			{
				if ( ( LAMP_COLOR_GREEN == i % 3 ) && ( 1 == m_ucLampOn[i] )  )
				{
					m_ucLampFlash[i] = 1;
				}
			}
			ACE_OS::memcpy(m_pRunData->sStageStepInfo[0].ucLampOn    , m_ucLampOn    , MAX_LAMP);
			ACE_OS::memcpy(m_pRunData->sStageStepInfo[0].ucLampFlash , m_ucLampFlash , MAX_LAMP);
		}
		else
		{
			ACE_OS::memset( &m_pRunData->sStageStepInfo[0] , 0 , sizeof(SStepInfo) );
			/******************* 黄闪之前，发送熄灯信号，避免黄闪不整齐**************************************/
			CLampBoard::CreateInstance()->SetLamp(m_pRunData->sStageStepInfo[0].ucLampOn, m_pRunData->sStageStepInfo[0].ucLampFlash);
			CLampBoard::CreateInstance()->SendLamp();
			/******************* 黄闪之前，发送熄灯信号，避免黄闪不整齐**************************************/
			m_pRunData->sStageStepInfo[0].ucStepLen = m_pRunData->ucStepTime;

			for ( int i=0; i<MAX_LAMP_BOARD; i++ )
			{
				for ( int j=0; j<MAX_LAMP_NUM_PER_BOARD; j++ )
				{
					if ( (j%3) == LAMP_COLOR_YELLOW )
					{
						m_pRunData->sStageStepInfo[0].ucLampOn[i*MAX_LAMP_NUM_PER_BOARD+j]    = 1;
						m_pRunData->sStageStepInfo[0].ucLampFlash[i*MAX_LAMP_NUM_PER_BOARD+j] = 1;
					}
					else
					{
						m_pRunData->sStageStepInfo[0].ucLampOn[i*MAX_LAMP_NUM_PER_BOARD+j]    = 0;
						m_pRunData->sStageStepInfo[0].ucLampFlash[i*MAX_LAMP_NUM_PER_BOARD+j] = 0;
					}
				}
			}
		}
		
		m_bSpeStatus = true;
	}
	else if ( m_pRunData->uiWorkStatus == ALLRED )  //全红
	{
		if ( m_bSpeStatusTblSchedule )
		{
			m_pRunData->ucStepTime = MAX_SPESTATUS_CYCLE;
		}
		else if ( 0 == ucTime )
		{
			m_pRunData->ucStepTime = m_pTscConfig->sUnit.ucStartAllRedTime;
		//	ACE_DEBUG((LM_DEBUG,"%s:%d 启动黄闪后进入全红 5秒开始\n" ,__FILE__,__LINE__));
		}
		else
		{
			m_pRunData->ucStepTime = ucTime;
		}
		
		m_pRunData->ucElapseTime = 0;
		m_pRunData->ucStepNo = 0;
		m_pRunData->ucStepNum = 1;
		m_pRunData->ucRunTime = m_pRunData->ucStepTime;
		m_pRunData->ucStageNo = 0;
		m_pRunData->ucStageCount = 1;
		
		ACE_OS::memset( &m_pRunData->sStageStepInfo[0] , 0 , sizeof(SStepInfo) );		
		
		m_pRunData->sStageStepInfo[0].ucStepLen = m_pRunData->ucStepTime;

		for ( int i=0; i<MAX_LAMP_BOARD; i++ )
		{
			for ( int j=0; j<MAX_LAMP_NUM_PER_BOARD; j++ )
			{
				if ( (j%3) == LAMP_COLOR_RED )
				{
					m_pRunData->sStageStepInfo[0].ucLampOn[i*MAX_LAMP_NUM_PER_BOARD+j] = 1;
				}
				else
				{
					m_pRunData->sStageStepInfo[0].ucLampOn[i*MAX_LAMP_NUM_PER_BOARD+j] = 0;
				}
				m_pRunData->sStageStepInfo[0].ucLampFlash[i*MAX_LAMP_NUM_PER_BOARD+j] = 0;
			}
		}
		
		m_bSpeStatus = true;
	}
	else if ( m_pRunData->uiWorkStatus == SIGNALOFF )  //关灯
	{
		if ( m_bSpeStatusTblSchedule )
		{
			m_pRunData->ucStepTime = MAX_SPESTATUS_CYCLE;
		}
		else
		{
			m_pRunData->ucStepTime   = ucTime;
		}
		m_pRunData->ucElapseTime = 0;
		m_pRunData->ucStepNo     = 0;
		m_pRunData->ucStepNum    = 1;
		
		ACE_OS::memset( &m_pRunData->sStageStepInfo[0] , 0 , sizeof(SStepInfo) );

		m_pRunData->sStageStepInfo[0].ucStepLen = m_pRunData->ucStepTime;

		//for ( int i=0; i<MAX_LAMP_BOARD; i++ )
		//{
			//for ( int j=0; j<MAX_LAMP_NUM_PER_BOARD; j++ )
			//{
				//m_pRunData->sStageStepInfo[0].ucLampOn[i*MAX_LAMP_NUM_PER_BOARD+j]    = 0;
				//m_pRunData->sStageStepInfo[0].ucLampFlash[i*MAX_LAMP_NUM_PER_BOARD+j] = 0;
			//}
		//}
		
		m_bSpeStatus = true;
	}
	else if ( m_pRunData->uiWorkStatus == STANDARD )  //标准
	{
		

		ACE_DEBUG((LM_DEBUG,"%s:%d ResetRunDate 开始进入标准状态 构造动态数据,call GetRunDataStandard()\n" ,__FILE__,__LINE__));
		GetRunDataStandard(); //正常地构造动态数据
		m_bSpeStatus = false;
		
	}

#ifdef CHECK_MEMERY
	TestMem(__FILE__,__LINE__);
#endif
}


/**************************************************************
Function:       CManaKernel::GetRunDataStandard
Description:    标准情况下，正常逻辑的获取动态信息 根据需要重新加
				载时段表  获取配时方案号 获取阶段配时表号,获取阶
				段信息
Input:          无          
Output:         无
Return:         无
***************************************************************/
void CManaKernel::GetRunDataStandard()
{
	bool bNewDay         = false;
	Byte ucCurScheduleId = 0;
	static Byte ucDay = 0 ;
	ACE_Date_Time tvTime(GetCurTime());
	if(ucDay == 0)
		ucDay = tvTime.day();
	//	printf("ucDay = %d\n",ucDay);

	if ( m_pRunData->bNeedUpdate || (0==tvTime.hour() && tvTime.day() !=ucDay ) )  //下一天,可能需要重新加载时段表信息
	{
		bNewDay = true;
		if( m_pRunData->bNeedUpdate)
			 ACE_DEBUG((LM_DEBUG,"%s:%d m_pRunData->bNeedUpdate== true!\n" ,__FILE__,__LINE__));
		else 
			{
				ucDay = tvTime.day();
				ACE_DEBUG((LM_DEBUG,"%s:%d 0==tvTime.hour()!\n" ,__FILE__,__LINE__));
			}
	}


	if ( bNewDay )
	{
		 // ACE_DEBUG((LM_DEBUG,"%s:%d it's be new day!\n" ,__FILE__,__LINE__));
		ucCurScheduleId = GetScheduleId((Byte)tvTime.month(),(Byte)tvTime.day(),(Byte)tvTime.weekday());  //获取当前的时段表号  
		
		if ( m_pRunData->bNeedUpdate || (ucCurScheduleId != m_pRunData->ucScheduleId) )
		{
			//重新从数据库加载1天时段表信息 
			unsigned short usCount = 0;
			int iIndex   = 0;
			GBT_DB::TblSchedule tblSchedule;
			GBT_DB::Schedule* pSchedule = NULL;
			ACE_OS::memset(&tblSchedule,0,sizeof(GBT_DB::TblSchedule));

			
			/*
			ACE_Date_Time tvTime(GetCurTime());
			Byte ucCurScheduleId = GetScheduleId((Byte)tvTime.month(),(Byte)tvTime.day(),(Byte)tvTime.weekday());  
			*/
			
			(CDbInstance::m_cGbtTscDb).QuerySchedule(ucCurScheduleId,tblSchedule);
			pSchedule = tblSchedule.GetData(usCount); 
			iIndex = 0;
			while ( iIndex < usCount )
			{
				ACE_OS::memcpy(m_pTscConfig->sSchedule+iIndex,pSchedule,sizeof(SSchedule));
				pSchedule++;
				iIndex++;
			}
			ACE_OS::memset(m_pTscConfig->sSchedule+iIndex,0,sizeof(SSchedule));
		}
	}

	ucCurScheduleId         = GetScheduleId((Byte)tvTime.month(),(Byte)tvTime.day(),(Byte)tvTime.weekday()); //从时基调度表获得当天的时段表号
	
	//根据时段信息重新获取配时方案号
	Byte ucCurCtrl          = m_pRunData->uiCtrl;
	Byte ucCurStatus        = m_pRunData->uiWorkStatus;
	Byte ucCurTimePatternId = 0 ;
	if(bTmpPattern == true  && m_iTimePatternId != 0)
	{
		//ACE_DEBUG((LM_DEBUG,"%s:%d ((CTRL_PANEL == m_pRunData->uiCtrl ) || ( CTRL_UTCS == m_pRunData->uiCtrl ) )  && 			( m_iTimePatternId != 0 )\n",__FILE__,__LINE__));
		ACE_DEBUG((LM_DEBUG,"%s:%d when m_iTimePatternId= %d >0 return !\n" ,__FILE__,__LINE__,m_iTimePatternId));
		if(m_iTimePatternId == 250)
		{
			 CManaKernel::CreateInstance()->bNextDirec = true;
		}
		ucCurTimePatternId = m_iTimePatternId;
	}
	else
		ucCurTimePatternId = GetTimePatternId( ucCurScheduleId , &ucCurCtrl , &ucCurStatus );// 函数根据当前时间分钟数获得配时方案号

	m_pRunData->ucScheduleId    = ucCurScheduleId; //时段表达控制方式和 参数表的控制变量不等
	m_pRunData->uiScheduleCtrl  = ucCurCtrl;   //记录时段表的控制方式  感应控制方式需要设置感应控制
	//ACE_DEBUG((LM_DEBUG,"%s:%d ucCurCtrl = %d m_pRunData->uiCtrl = %d\n" ,__FILE__,__LINE__,ucCurCtrl,m_pRunData->uiCtrl));	
	if ( ucCurCtrl != m_pRunData->uiCtrl )  //控制方式改变
	{
		//if ( (CTRL_SCHEDULE == ucCurCtrl) 
			//&& ( CTRL_WIRELESS == m_pRunData->uiCtrl || CTRL_VEHACTUATED == m_pRunData->uiCtrl 
			//  || CTRL_MAIN_PRIORITY == m_pRunData->uiCtrl || CTRL_SECOND_PRIORITY == m_pRunData->uiCtrl 
			//  || CTRL_UTCS == m_pRunData->uiCtrl ) )
		//{
			//ACE_DEBUG((LM_DEBUG,"%s:%d 从感应控制向多时段控制转变\n" ,__FILE__,__LINE__));
		//}
		//else
		//{
			if(!(ucCurCtrl ==CTRL_UTCS && bUTS == false) && m_pRunData->uiCtrl != CTRL_UTCS) //如果时段表中为CTRL_UTC

			{
				ACE_DEBUG((LM_DEBUG,"%s:%d 时段表里的控制方式发生了改变\n" ,__FILE__,__LINE__));
				SwitchCtrl(ucCurCtrl); //时段表里的控制方式发生了改变
			}
			
		//}
	}
	ACE_DEBUG((LM_DEBUG,"%s:%d m_iTimePatternId1 = %d \n" ,__FILE__,__LINE__,m_iTimePatternId));
	//获取阶段配时表
	//if ( m_pRunData->bNeedUpdate || (ucCurTimePatternId != m_pRunData->ucTimePatternId) || m_iTimePatternId == 251)//配时方案号不同或者为251特殊方案号
	 if(m_pRunData->bNeedUpdate || ucCurTimePatternId != m_pRunData->ucTimePatternId || m_iTimePatternId == 251 || m_iTimePatternId == 250)
	{
		Byte ucCurScheduleTimeId = GetScheduleTimeId(ucCurTimePatternId,m_ucUtcsComCycle,m_ucUtscOffset); //获取配时阶段表号
ACE_DEBUG((LM_DEBUG,"%s:%d m_iTimePatternId2 = %d \n" ,__FILE__,__LINE__,m_iTimePatternId));
		m_pRunData->ucTimePatternId = ucCurTimePatternId;
				
		if ( m_pRunData->bNeedUpdate|| (ucCurScheduleTimeId != m_pRunData->ucScheduleTimeId) ||m_iTimePatternId == 251 || m_iTimePatternId == 250)
		{
			m_pRunData->ucScheduleTimeId = ucCurScheduleTimeId;

			//重新加载动态数据的阶段配时信息sScheduleTime
			if(!GetSonScheduleTime(ucCurScheduleTimeId))
			{
				bDegrade = true ;
				CGbtMsgQueue::CreateInstance()->SendTscCommand(OBJECT_SWITCH_SYSTEMCONTROL,254);
				ACE_DEBUG((LM_DEBUG,"%s:%d 阶段配时为空，降级到黄闪\n" ,__FILE__,__LINE__));	
				CMainBoardLed::CreateInstance()->DoModeLed(true,true);
				return ;	
			}
			ACE_DEBUG((LM_DEBUG,"%s:%d 构造整个周期各阶段步伐信息，条件是当前配时方案号阶段配时号不同或者数据库有更新\n" ,__FILE__,__LINE__));		
			//重新构造整个周期的步伐信息
			SetCycleStepInfo(0);
			
		}
	}
	else if ( m_bSpeStatus && STANDARD == ucCurStatus)  //由特殊状态-->STANDARD 需要重新获取参数
	{
		Byte ucCurScheduleTimeId = GetScheduleTimeId(ucCurTimePatternId,m_ucUtcsComCycle,m_ucUtscOffset);

		m_pRunData->ucTimePatternId = ucCurTimePatternId;
		m_pRunData->ucScheduleTimeId = ucCurScheduleTimeId;

		//重新加载动态数据的阶段配时信息sScheduleTime
		GetSonScheduleTime(ucCurScheduleTimeId); // READ: 2013 0704 0915 获取当前子阶段总数 子阶段数据
		
		ACE_DEBUG((LM_DEBUG,"%s:%d 构造整个周期各阶段步伐信息，条件是特殊状态进入标准状态并且当前信号机工作状态是标准状态\n" ,__FILE__,__LINE__));
		//重新构造整个周期的步伐信息
		SetCycleStepInfo(0);
		
	}

	if ( ucCurStatus != m_pRunData->uiWorkStatus && m_pRunData->uiCtrl != CTRL_UTCS)  //工作状态改变 联网控制时候不闪光
	{
		SwitchStatus(ucCurStatus);
	}
	else if ( m_bVirtualStandard && (STANDARD == ucCurStatus) )  //STANDARD(FLASH、ALLRED、SINGLEALL) --> STANDARD
	{
		m_pRunData->uiWorkStatus = m_pRunData->uiOldWorkStatus;
		m_bVirtualStandard      = false;
		SwitchStatus(ucCurStatus);
		ACE_DEBUG((LM_DEBUG,"%s:%d m_bVirtualStandard && (STANDARD == ucCurStatus\n" ,__FILE__,__LINE__));
	}

}


/**************************************************************
Function:       CManaKernel::GetScheduleId
Description:    根据月、日、星期获取当前的时段表号
Input:          ucWeek 0:周日 1：周1......6：周6 代表星期
				ucMonth ：1-12 代表月份  
				ucDay   ：1-31 代表日期        
Output:         无
Return:         当日到时段表号，若没有查询到则返回0
***************************************************************/
Byte CManaKernel::GetScheduleId(Byte ucMonth,Byte ucDay , Byte ucWeek)
{
	//按日 节假日 特定月 + 特定日 + 周全选
	ACE_DEBUG((LM_DEBUG,"%s:%d ucId= %d, ucScheduledId = %d,month = %d ,day = %d ,week = %d \n" ,__FILE__,__LINE__,m_pTscConfig->sTimeGroup[0].ucId,m_pTscConfig->sTimeGroup[0].ucScheduleId,m_pTscConfig->sTimeGroup[0].usMonth,m_pTscConfig->sTimeGroup[0].uiDayWithMonth,m_pTscConfig->sTimeGroup[0].ucDayWithWeek));
	
	for (Byte i=0; i<MAX_TIMEGROUP; i++ )
	{
		if ( m_pTscConfig->sTimeGroup[i].ucId != 0 )
		{			
			if ( (m_pTscConfig->sTimeGroup[i].usMonth>>ucMonth) & 0x1 
				&& ( (m_pTscConfig->sTimeGroup[i].usMonth & 0x1ffe) != 0x1ffe ) )
			{
				if ( ( (m_pTscConfig->sTimeGroup[i].uiDayWithMonth)>>ucDay) & 0x1 
						&& ( ( (m_pTscConfig->sTimeGroup[i].uiDayWithMonth) & 0xfffffffe)!= 0xfffffffe) )
				{
					if ( ( (m_pTscConfig->sTimeGroup[i].ucDayWithWeek) & 0xfe) == 0xfe )
					{
						//ACE_DEBUG((LM_DEBUG,"%s:%d Priority 1: ucId =%d Spe month,Spe day ,All weeks Get ScheduleId = %d from Tbl_Plan !\n" ,__FILE__,__LINE__, m_pTscConfig->sTimeGroup[i].ucId,m_pTscConfig->sTimeGroup[i].ucScheduleId));
						if (m_pTscConfig->sTimeGroup[i].ucScheduleId >0)
							return m_pTscConfig->sTimeGroup[i].ucScheduleId;
					}
				}
			}
		}
		else
		{
			break;  //没有该天的时间方案
		}
	}
	//按周：全月   + 全日   + 特定周
	for (Byte i=0; i<MAX_TIMEGROUP; i++ )
	{	
		if ( m_pTscConfig->sTimeGroup[i].ucId != 0 )
		{			
			if ( ( (m_pTscConfig->sTimeGroup[i].usMonth) & 0x1ffe) == 0x1ffe )
			{
				if ( (m_pTscConfig->sTimeGroup[i].uiDayWithMonth & 0xfffffffe) == 0xfffffffe )
				{
					if ( ((m_pTscConfig->sTimeGroup[i].ucDayWithWeek)>>(ucWeek+1)) & 0x1       //星期移位比较的时候 ucWeek+1
						&& ( ( (m_pTscConfig->sTimeGroup[i].ucDayWithWeek) & 0xfe) != 0xfe ) )
					{
						//ACE_DEBUG((LM_DEBUG,"%s:%d ucId =%d Priority 2: All month,All day ,Spe week Get ScheduleId = %d from Tbl_Plan !\n" ,__FILE__,__LINE__, m_pTscConfig->sTimeGroup[i].ucId,m_pTscConfig->sTimeGroup[i].ucScheduleId));
						if (m_pTscConfig->sTimeGroup[i].ucScheduleId >0)
							return m_pTscConfig->sTimeGroup[i].ucScheduleId;
						
					}
				}
			}
		}
		else
		{
			break;  //没有该天的时间方案
		}
	}
	//按月：特定月 + 全日   + 全周
	for (Byte i=0; i<MAX_TIMEGROUP; i++ )
	{	
		if ( m_pTscConfig->sTimeGroup[i].ucId != 0 )
		{			
			if ( ((m_pTscConfig->sTimeGroup[i].usMonth)>>ucMonth) &0x1 
				 && ((m_pTscConfig->sTimeGroup[i].usMonth & 0x1ffe) != 0x1ffe ) )
			{
				if ( (m_pTscConfig->sTimeGroup[i].uiDayWithMonth & 0xfffffffe) == 0xfffffffe )
				{
					if ( (m_pTscConfig->sTimeGroup[i].ucDayWithWeek & 0xfe) == 0xfe )
					{
						//ACE_DEBUG((LM_DEBUG,"%s:%d ucId =%d Priority 3: Spe month,All day ,All weeks Get ScheduleId = %d from Tbl_Plan !\n" ,__FILE__,__LINE__, m_pTscConfig->sTimeGroup[i].ucId,m_pTscConfig->sTimeGroup[i].ucScheduleId));
						if (m_pTscConfig->sTimeGroup[i].ucScheduleId >0)
							return m_pTscConfig->sTimeGroup[i].ucScheduleId;
					}
				}
			}
		}
		else
		{
			break;  //没有该天的时间方案
		}
	}

	//其他：特定月 + 特定日 + 特定周
	for (Byte i=0; i<MAX_TIMEGROUP; i++ )
	{	
		if ( m_pTscConfig->sTimeGroup[i].ucId != 0 )
		{			
			if ( (m_pTscConfig->sTimeGroup[i].usMonth>>ucMonth) &0x1 )	//月份正确 b0 = 0 保留 b1:1月 b12:12
			{
				if ( ( ((m_pTscConfig->sTimeGroup[i].ucDayWithWeek)>>(ucWeek+1)) & 0x1 ) //b0=0 保留b1：周日 b2：周1 
					&& ( ((m_pTscConfig->sTimeGroup[i].uiDayWithMonth)>>ucDay) & 0x1 ) )  //b0 =1 保留 b1:1号 b2
				{
					//ACE_DEBUG((LM_DEBUG,"%s:%d ucId =%d Priority 4: Spe month,Spe day ,Spe weeks Get ScheduleId = %d from Tbl_Plan !\n" ,__FILE__,__LINE__, m_pTscConfig->sTimeGroup[i].ucId,m_pTscConfig->sTimeGroup[i].ucScheduleId));
					if (m_pTscConfig->sTimeGroup[i].ucScheduleId >0)
							return m_pTscConfig->sTimeGroup[i].ucScheduleId;
				}
			}
		}
		else
		{
			break;  //没有该天的时间方案
		}
	}	
	return 0;
}



/**************************************************************
Function:       CManaKernel::GetScheduleId
Description:    根据月、日、星期获取当前的时段表号
Input:          ucScheduleId ：时段表号       
Output:         ucCtrl	 ： 当前控制模式
				ucStatus ： 当前工作状态
				ucCycle      - 周期    暂时未用
		        ucOffSet     - 相位差  暂时未用
Return:         配时方案号 
***************************************************************/
Byte CManaKernel::GetTimePatternId(Byte ucScheduleId , Byte* ucCtrl , Byte* ucStatus )
{	
	ACE_DEBUG((LM_DEBUG,"%s:%d m_pRunData->uiCtrl= %d m_iTimePatternId= %d\n" ,__FILE__,__LINE__,m_pRunData->uiCtrl ,m_iTimePatternId));
	//如果当前的运行状态控制方式为面板控制或者系统优化控制，并且当前配饰方案号不为0
	//if ( ( ( ( CTRL_PANEL == m_pRunData->uiCtrl ) || ( CTRL_UTCS == m_pRunData->uiCtrl ) )  && ( m_iTimePatternId != 0 ) )||  m_iTimePatternId == 250)
	
	bool bLastTem           = false;
	int  iIndex             = 0;
	Byte ucCurTimePatternId = 0;
	Byte ucCtrlTmp          = 0;
	ACE_Date_Time tvTime(GetCurTime());
	for ( ; iIndex<MAX_SCHEDULE_PER_DAY; iIndex++)
	{		
		if ( m_pTscConfig->sSchedule[iIndex].ucId != 0 && (m_pTscConfig->sSchedule[iIndex].ucEventId != 0 ) )
		{
			if ( m_pTscConfig->sSchedule[iIndex+1].ucId == ucScheduleId )//判断是否到了最后一条时段
			{				
				if ((CompareTime(m_pTscConfig->sSchedule[iIndex].ucHour,m_pTscConfig->sSchedule[iIndex].ucMin ,
								(Byte)tvTime.hour(),(Byte)tvTime.minute()) )
								 && 
					(!CompareTime( m_pTscConfig->sSchedule[iIndex+1].ucHour,m_pTscConfig->sSchedule[iIndex+1].ucMin,
								 (Byte)tvTime.hour(),(Byte)tvTime.minute()))
					 )
				{
					ucCurTimePatternId = m_pTscConfig->sSchedule[iIndex].ucTimePatternId;
					ucCtrlTmp          = m_pTscConfig->sSchedule[iIndex].ucCtrl;
					
					break;
				}
			}
		}
		else //应该提前去
		{
			bLastTem = true; //
			break;           //
		}
	}

	if ( bLastTem && (iIndex != 0 )) //最后一段的时间阶段
	{
		
		ucCurTimePatternId =  m_pTscConfig->sSchedule[iIndex-1].ucTimePatternId;
		ucCtrlTmp = m_pTscConfig->sSchedule[iIndex-1].ucCtrl;
	}
	
	m_bSpeStatusTblSchedule = false;	 
    
	switch ( ucCtrlTmp )
	{
		case 0:     //0:多时段
			*ucCtrl   = CTRL_SCHEDULE;
			*ucStatus = STANDARD;
			printf("%s:%d now:STRL_SCHEDULE!\n",__FILE__,__LINE__);
			break;
		case 1:    //1:关灯
			*ucStatus = SIGNALOFF;
			m_bSpeStatusTblSchedule = true;
			break;
		case 2:    //2:闪光
			*ucStatus = FLASH;
			m_bSpeStatusTblSchedule = true;
			break;
		case 3:    //3:全红
			*ucStatus = ALLRED;
			m_bSpeStatusTblSchedule = true;
			break;
		case 4:    //4：主线半感应
			*ucCtrl   = CTRL_MAIN_PRIORITY;
			*ucStatus = STANDARD;
			break;
		case 5:    //5:次线半感应
			*ucCtrl   = CTRL_SECOND_PRIORITY;
			*ucStatus = STANDARD;
			break;
		case 6:    //6:全感应
			*ucCtrl   = CTRL_VEHACTUATED;
			*ucStatus = STANDARD;
			printf("%s:%d now:STRL_VEHACTUATED!\n",__FILE__,__LINE__);
			break;
		case 7:    //7:无电线缆
			*ucCtrl   = CTRL_WIRELESS;
			*ucStatus = STANDARD;
			break;
		case 8:    //8:自适应控制
			*ucCtrl   = CTRL_ACTIVATE;
			*ucStatus = STANDARD;
			break;
		case 12:   //12:联网
			*ucCtrl   = CTRL_UTCS;
			*ucStatus = STANDARD;
			break;
		
		default:   //默认多时段
			*ucCtrl   = CTRL_SCHEDULE;
			*ucStatus = STANDARD;
			break;
	}	
	return ucCurTimePatternId;
}


	

/**************************************************************
Function:       CManaKernel::GetScheduleTimeId
Description:    根据月、日、星期获取当前的时段表号
Input:          ucTimePatternId - 配时方案表号       
Output:         ucCycle         - 周期
*       	    ucOffSet        - 相位差
Return:         阶段配时表号 ，如果没有查询到合法阶段配时表号则返回1
***************************************************************/
Byte CManaKernel::GetScheduleTimeId(Byte ucTimePatternId,Byte& ucCycle,Byte& ucOffSet)
{	
	for ( int i=0; i<MAX_TIMEPATTERN; i++ )
	{
		if ( m_pTscConfig->sTimePattern[i].ucId != 0 )
		{
			if ( ucTimePatternId == m_pTscConfig->sTimePattern[i].ucId )
			{
				ucCycle  = m_pTscConfig->sTimePattern[i].ucCycleLen;
				ucOffSet = m_pTscConfig->sTimePattern[i].ucPhaseOffset;
				return m_pTscConfig->sTimePattern[i].ucScheduleTimeId;
			}	
		}
		else 
		{
			break;
		}
	}
	return 1;  //不合法的参数,黄闪？
}


/**************************************************************
Function:       CManaKernel::GetSonScheduleTime
Description:    根据动态数据的阶段配时表号(ucScheduleTimeId)获取
				各个子阶段的信息
Input:          ucScheduleTimeId - 阶段配时表号       
Output:         无
Return:         无
***************************************************************/
bool CManaKernel::GetSonScheduleTime(Byte ucScheduleTimeId)
{
	Byte ucIndex = 0;
	m_pRunData->ucStageCount = 0;

	ACE_OS::memset(m_pRunData->sScheduleTime,0,MAX_SON_SCHEDULE*sizeof(SScheduleTime));

	for ( int i=0; i<MAX_SCHEDULETIME_TYPE; i++ )
	{
		for ( int j=0; j<MAX_SON_SCHEDULE; j++ )
		{
			if ( m_pTscConfig->sScheduleTime[i][j].ucId == ucScheduleTimeId )
			{
				ACE_OS::memcpy(&m_pRunData->sScheduleTime[ucIndex++], &m_pTscConfig->sScheduleTime[i][j] , sizeof(SScheduleTime));
				m_pRunData->ucStageCount++;
			}
			else
			{
				break;
			}
		}
	}
	if(m_pRunData->ucStageCount > 0)
		return true;
	else
		return false ;
}


/**************************************************************
Function:       CManaKernel::StageToStep
Description:    根据阶段号获取步伐号 该阶段号对应结束步伐数
Input:          iStageNo:阶段号      
Output:         无
Return:         步伐号
***************************************************************/
Byte CManaKernel::StageToStep(int iStageNo)
{
	Byte ucCurrStepNo  = 0;
	Byte ucCurrStageNo = iStageNo;	
	for ( int i = 0; i<ucCurrStageNo; i++ )
	{
		ucCurrStepNo += m_pRunData->ucStageIncludeSteps[i];
	}

	return ucCurrStepNo;
}


/**************************************************************
Function:       CManaKernel::StepToStage
Description:    根据步伐号获取阶段号
Input:          iStepNo:步伐号      
Output:         bStageFirstStep:是否为阶段的第一步
Return:         阶段号
***************************************************************/
Byte CManaKernel::StepToStage(int iStepNo,bool* bStageFirstStep)
{
	int ucCurrStepNo = iStepNo;

	for ( int i=0; i<MAX_SON_SCHEDULE; i++ )
	{
		ucCurrStepNo = ucCurrStepNo - m_pRunData->ucStageIncludeSteps[i];
		if ( (0 == ucCurrStepNo) || (0==iStepNo) )
		{
			if ( bStageFirstStep != NULL )
			{	
				*bStageFirstStep = true;
			}

			if (0==iStepNo)
			{
				return 0;
			}
			return i+1;
		}
		if ( ucCurrStepNo < 0 )
		{
			return i;
		}
	}

	return 0;
}


/**************************************************************
Function:       CManaKernel::GetStageTime
Description:    获取阶段的预计总时间和阶段已经运行的时间
Input:          无     
Output:         pTotalTime-阶段的预计总时间  pElapseTime-阶段已
				经运行的时间
Return:         无
***************************************************************/
void CManaKernel::GetStageTime(Byte* pTotalTime,Byte* pElapseTime)
{
	Byte ucStartStepNo  = 0;
	Byte ucEndStepNo    = 0;
	Byte ucCurStepNo    = m_pRunData->ucStepNo;
	Byte ucCurStageNo   = m_pRunData->ucStageNo;
	Byte ucTotalTime    = 0;
	Byte ucElapseTime   = 0;

	ucStartStepNo = StageToStep(ucCurStageNo);
	if ( (ucCurStageNo+1) < m_pRunData->ucStageCount )
	{
		ucEndStepNo = StageToStep(ucCurStageNo+1) - 1;
	}
	else
	{
		ucEndStepNo = m_pRunData->ucStepNum - 1;
	}

	for ( Byte ucStepNo=ucStartStepNo; ucStepNo<=ucEndStepNo && ucStepNo<m_pRunData->ucStepNum; ucStepNo++ )
	{
		ucTotalTime += m_pRunData->sStageStepInfo[ucStepNo].ucStepLen;
	}

	for ( Byte ucStepNo=ucStartStepNo; ucStepNo<ucCurStepNo && ucStepNo<m_pRunData->ucStepNum; ucStepNo++ )
	{
		ucElapseTime += m_pRunData->sStageStepInfo[ucStepNo].ucStepLen;
	}
	ucElapseTime += m_pRunData->ucElapseTime + 1;

	if (CTRL_VEHACTUATED== m_pRunData->uiCtrl || CTRL_MAIN_PRIORITY == m_pRunData->uiCtrl || CTRL_SECOND_PRIORITY == m_pRunData->uiCtrl)
	{
		if ( !IsLongStep(ucCurStepNo) )
		{
			ucElapseTime += m_ucAddTimeCnt;
		}		
	}

	if ( pTotalTime != NULL )
	{
		*pTotalTime = ucTotalTime;
	}

	if ( pElapseTime != NULL )
	{
		*pElapseTime = ucElapseTime;		
	}
}


/**************************************************************
Function:       CManaKernel::SetCycleStepInfo
Description:    构造整个周期的步伐信息，步伐信息存储在
				m_pRunData->sStageStepInfo中
Input:          ucLockPhase - 锁定相位号     
Output:         无
Return:         无
***************************************************************/
void CManaKernel::SetCycleStepInfo(Byte ucLockPhase)
{
	ACE_Guard<ACE_Thread_Mutex>  guard(m_mutexRunData);

	Byte ucStepIndex  = 0;
	Byte ucStageIndex = 0;

	ACE_OS::memset(m_pRunData->sStageStepInfo,0,sizeof(m_pRunData->sStageStepInfo));

	if ( ucLockPhase != 0 )  //锁定相位 
	{
		//根据锁定相位设置整个周期的步伐信息..............
		m_pRunData->ucStepNo = 0;
		m_pRunData->ucStepNum = 1;
		m_pRunData->ucStageNo = 0;
		m_pRunData->ucStageCount = 1;
		m_pRunData->bOldLock = true;
		return;
	}

	//正常设置步伐信息
	for (ucStageIndex=0; ucStageIndex<MAX_SON_SCHEDULE; ucStageIndex++ )//MAX_SON_SCHEDULE
	{
 		if ( m_pRunData->ucScheduleTimeId == m_pRunData->sScheduleTime[ucStageIndex].ucId )  //ucScheduleTimeId  ucId阶段配时表号
 			
		{
			//根据一个子阶段构造步伐号
			SetStepInfoWithStage(ucStageIndex,&ucStepIndex,&(m_pRunData->sScheduleTime[ucStageIndex]));// ucStageIndex 子阶段号
			// ACE_DEBUG((LM_DEBUG,"%s:%d ucStageIncludeSteps= %d \n" ,__FILE__,__LINE__,m_pRunData->ucStageIncludeSteps[ucStageIndex]));
		}
		else
		{
			break;  //全部的子阶段遍历完毕
		}
	}

	m_pRunData->ucStepNo     = 0;
	m_pRunData->ucStepNum    = ucStepIndex;
	m_pRunData->ucStageNo    = 0;
	m_pRunData->ucStageCount = ucStageIndex;
	m_pRunData->ucStepTime   = m_pRunData->sStageStepInfo[0].ucStepLen;
	m_pRunData->ucRunTime    = m_pRunData->ucStepTime;
	m_pRunData->ucElapseTime = 0;

	m_pRunData->ucCycle = 0;
	for ( int i=0; i<MAX_STEP && i<m_pRunData->ucStepNum; i++ )
	{
		m_pRunData->ucCycle += m_pRunData->sStageStepInfo[i].ucStepLen;
	}

	//ACE_DEBUG((LM_DEBUG,"%s:%d ucCycle= %d ucStepNum=%d ucStageCount=%d uc0StepTime= %d\n" ,__FILE__,__LINE__,m_pRunData->ucCycle ,m_pRunData->ucStepNum,m_pRunData->ucStageCount,m_pRunData->ucStepTime));
}


/**************************************************************
Function:       CManaKernel::GetOverlapPhaseIndex
Description:    根据包含相位id获取全部的跟随相位
				m_pRunData->sStageStepInfo中
Input:          iPhaseId      - 包含相位id     
Output:         ucCnt         - 跟随相位个数
*       	    pOverlapPhase - 存储各个跟随相位
Return:         无
***************************************************************/
void CManaKernel::GetOverlapPhaseIndex(Byte iPhaseId , Byte* ucCnt , Byte* pOverlapPhase)
{
	int iIndex           = 0;
	int jIndex           = 0;
	int iIncludePhaseCnt = 0;
	*ucCnt = 0;

	while ( iIndex < MAX_OVERLAP_PHASE )
	{
		if ( m_pTscConfig->sOverlapPhase[iIndex].ucId != 0 )
		{
			iIncludePhaseCnt = m_pTscConfig->sOverlapPhase[iIndex].ucIncludePhaseLen;
			jIndex = 0;
			while ( jIndex < iIncludePhaseCnt )
			{
				if ( iPhaseId == m_pTscConfig->sOverlapPhase[iIndex].ucIncludePhase[jIndex] )
				{
					pOverlapPhase[*ucCnt] = iIndex;
					*ucCnt += 1;
					break;
				}
				jIndex++;
			}
		}

		iIndex++;
	}
}


/**************************************************************
Function:       CManaKernel::OverlapPhaseToPhase
Description:   根据跟随相位获取放行相位index				
Input:          iCurAllowPhase      - 阶段放行相位
*    		    ucOverlapPhaseIndex - 跟随相位下标 0 1 ....7    
Output:       
Return:         return: 对应放行相位的index 0 1 .....15
***************************************************************/
Byte CManaKernel::OverlapPhaseToPhase(Uint uiCurAllowPhase,Byte ucOverlapPhaseIndex)
{
	Byte ucIndex           = 0;
	Byte ucPhaseId         = 0;
	Byte ucIncludePhaseCnt = m_pTscConfig->sOverlapPhase[ucOverlapPhaseIndex].ucIncludePhaseLen;

	while ( ucIndex < ucIncludePhaseCnt )
	{
		ucPhaseId = m_pTscConfig->sOverlapPhase[ucOverlapPhaseIndex].ucIncludePhase[ucIndex] - 1;

		if ( (uiCurAllowPhase >> ucPhaseId) & 1 )
		{
			return ucPhaseId;
		}
		ucIndex++;
	}

	return MAX_PHASE+1;
}


/*********************************************************************************
Function:       CManaKernel::SetStepInfoWithStage
Description:    根据一个子阶段设置步伐信息			
Input:          ucStageIndex  : 当前的子阶段下标
*      			ucCurStepIndex：当前累计设置的步伐数,即sStageStepInfo的下标
*       		pScheduleTime : 子阶段信息的指针，为当前子阶段号对应阶段配时分配情况
Output:         ucCurStepIndex: 当前累计设置的步伐数,即sStageStepInfo的下标
Return:         无
**********************************************************************************/
void CManaKernel::SetStepInfoWithStage(Byte ucCurStageIndex, Byte* ucCurStepIndex,SScheduleTime* pScheduleTime)
{
	Byte ucPhaseIndex      = 0;
	Byte ucMinStepLen      = 200;
	Byte ucTmpStepLen      = 0;
	Byte ucSignalGrpNum    = 0;
	Byte ucSignalGrpIndex  = 0;
	Byte ucLampIndex       = 0;
	Byte ucCnt             = 0;
	Byte ucAllowPhaseIndex = 0;
	Byte *pPhaseColor      = NULL;
	Byte ucSignalGrp[MAX_CHANNEL]          = {0}; 
	Byte ucOverlapPhase[MAX_OVERLAP_PHASE] = {0};
	SPhaseStep sPhaseStep[MAX_PHASE+MAX_OVERLAP_PHASE];
	
	ACE_OS::memset(sPhaseStep,0,(MAX_PHASE+MAX_OVERLAP_PHASE)*sizeof(SPhaseStep));

	for (Byte i=0; i<MAX_PHASE; i++ )
	{
		if ( ( pScheduleTime->usAllowPhase>>i ) & 0x1 )  //放行相位情况,获取该子阶段放行相位的绿黄红时间
		{
			sPhaseStep[ucPhaseIndex].ucPhaseId     = i + 1;
			sPhaseStep[ucPhaseIndex].bIsAllowPhase = true;
			GetPhaseStepTime(i,pScheduleTime,sPhaseStep[ucPhaseIndex].ucStepTime); //获取该相位的 绿 绿删 黄 红时间  4个字节表示
			ucAllowPhaseIndex = ucPhaseIndex;
			//ACE_DEBUG((LM_DEBUG,"%s:%d PhaseId=%d  sPhaseStep[ucPhaseIndex] = %d,Green=%d,GF = %d Yellow = %d\n" ,__FILE__,__LINE__,i+1,ucPhaseIndex,sPhaseStep[ucPhaseIndex].ucStepTime[0],sPhaseStep[ucPhaseIndex].ucStepTime[1],sPhaseStep[ucPhaseIndex].ucStepTime[2])); // ADD: 20130708 测试感应控制下时间长?

			ucPhaseIndex++;
			
			//跟随相位情况   READ:20130703 1423
			GetOverlapPhaseIndex(i+1 , &ucCnt , ucOverlapPhase);
			if ( ucCnt > 0 )  //该普通相位为包含相位，遍历跟随相位
			{
				int iIndex = 0;
				while ( iIndex < ucCnt )
				{
					if ( !ExitOvelapPhase(ucOverlapPhase[iIndex]+1,ucPhaseIndex,sPhaseStep) )
					{
						GetOverlapPhaseStepTime(  
							  ucCurStageIndex 
							, ucOverlapPhase[iIndex]
						    , sPhaseStep[ucAllowPhaseIndex].ucStepTime  
							, sPhaseStep[ucPhaseIndex].ucStepTime
							, i
							, pScheduleTime->ucYellowTime);
						sPhaseStep[ucPhaseIndex].ucPhaseId = ucOverlapPhase[iIndex] + 1;
						sPhaseStep[ucPhaseIndex].bIsAllowPhase = false;
					   // ACE_DEBUG((LM_DEBUG,"%s:%d overPhaseId=%d  sPhaseStep[ucPhaseIndex] = %d,Green=%d,GF = %d Yellow = %d\n" ,__FILE__,__LINE__,ucOverlapPhase[iIndex] + 1,ucPhaseIndex,sPhaseStep[ucPhaseIndex].ucStepTime[0],sPhaseStep[ucPhaseIndex].ucStepTime[1],sPhaseStep[ucPhaseIndex].ucStepTime[2])); // ADD: 20130708 测试感应控制下时间长?

						ucPhaseIndex++;
					}

					iIndex++;
				}
			}
		}
	}// 得到该子阶段所有放行相位的 放行时间

	//ACE_DEBUG((LM_DEBUG,"%s:%d ucCurStageIndex= %d ucPhaseIndex=%d \n" ,__FILE__,__LINE__,ucCurStageIndex,ucPhaseIndex));

	//处理所有的相位 将sPhaseStep转化为步伐信息
	m_pRunData->ucStageIncludeSteps[ucCurStageIndex] = 0;	

	for ( int i=0; i<ucPhaseIndex; i++ ) //ucPhaseIndex 为放行相位数
	{
		//ACE_DEBUG((LM_DEBUG,"%s:%d i=%d \n" ,__FILE__,__LINE__,i)); 
		if ( (ucTmpStepLen = GetPhaseStepLen(&sPhaseStep[i]) ) > 0 )
		{
			ucMinStepLen = (ucMinStepLen > ucTmpStepLen ? ucTmpStepLen : ucMinStepLen);
			//ACE_DEBUG((LM_DEBUG,"%s:%d i=%d \n" ,__FILE__,__LINE__,i)); 
		}
	
	   // ACE_DEBUG((LM_DEBUG,"%s:%d i=%d,ucMinStepLen= %d ucPhaseIndex=%d \n" ,__FILE__,__LINE__,i,ucMinStepLen,ucPhaseIndex));		
		if ( ( i + 1 ) ==  ucPhaseIndex )  //构造1个完整步伐    达到最大相位数
		{
		//	ACE_DEBUG((LM_DEBUG,"%s:%d 开始构造步伐数！ \n" ,__FILE__,__LINE__));
			if ( 200 == ucMinStepLen )  //全部遍历完毕
			{
			//	 ACE_DEBUG((LM_DEBUG,"%s:%d ucStageIncludeSteps= %d \n" ,__FILE__,__LINE__,m_pRunData->ucStageIncludeSteps[ucCurStageIndex]));
				return;
			}

			Byte ucColorIndex = 0;
			m_pRunData->ucStageIncludeSteps[ucCurStageIndex]++;
			m_pRunData->sStageStepInfo[*ucCurStepIndex].ucStepLen = ucMinStepLen;				
			
			for ( int j=0; j<ucPhaseIndex; j++ ) //遍历每个相位
			{
				ucColorIndex = GetPhaseStepIndex(&sPhaseStep[j]);
				if ( 5 == ucColorIndex )
				{
					continue;
				}
				
				if ( sPhaseStep[j].bIsAllowPhase )
				{
					m_pRunData->sStageStepInfo[*ucCurStepIndex].uiAllowPhase |= 
						                  (1<<(sPhaseStep[j].ucPhaseId-1) & 0xffffffff ) ;
					pPhaseColor = m_pRunData->sStageStepInfo[*ucCurStepIndex].ucPhaseColor;
				}
				else
				{
					m_pRunData->sStageStepInfo[*ucCurStepIndex].uiOverlapPhase |= 
						                   (1<<(sPhaseStep[j].ucPhaseId-1) & 0xffffffff );
					pPhaseColor = m_pRunData->sStageStepInfo[*ucCurStepIndex].ucOverlapPhaseColor;
				}

				if ( pPhaseColor )
				{
					if ( 0 == ucColorIndex )  //green
					{
						pPhaseColor[sPhaseStep[j].ucPhaseId-1] = LAMP_COLOR_GREEN;
					}
					else if ( 1 == ucColorIndex ) //green flash
					{
						pPhaseColor[sPhaseStep[j].ucPhaseId-1] = LAMP_COLOR_GREEN;
					}
					else if ( 2 == ucColorIndex  ) //yellow
					{
						pPhaseColor[sPhaseStep[j].ucPhaseId-1] = LAMP_COLOR_YELLOW;
					}
					else  //red
					{
						pPhaseColor[sPhaseStep[j].ucPhaseId-1] = LAMP_COLOR_RED;
					}
				}

				ucSignalGrpNum = 0;
				GetSignalGroupId(sPhaseStep[j].bIsAllowPhase,sPhaseStep[j].ucPhaseId, &ucSignalGrpNum,ucSignalGrp);
				
				ucSignalGrpIndex = 0;
				while ( ucSignalGrpIndex < ucSignalGrpNum )
				{
					ucLampIndex = ( ucSignalGrp[ucSignalGrpIndex] - 1 ) * 3; //相位组的red下标
					
					if ( 0 == ucColorIndex )  //green
					{
						//if(m_pRunData->ucStageNo ==0 && m_pRunData->ucStepNo==0) // test red green conflict
						//m_pRunData->sStageStepInfo[*ucCurStepIndex].ucLampOn[ucLampIndex] = 1; //TEST :2013 0717 11 40
						ucLampIndex = ucLampIndex + 2;
					}
					else if ( 1 == ucColorIndex ) //green flash
					{
						ucLampIndex = ucLampIndex + 2;
						m_pRunData->sStageStepInfo[*ucCurStepIndex].ucLampFlash[ucLampIndex] = 1;
					}
					else if ( 2 == ucColorIndex  ) //yellow
					{
						ucLampIndex = ucLampIndex + 1;
					}
					m_pRunData->sStageStepInfo[*ucCurStepIndex].ucLampOn[ucLampIndex] = 1;
					
					ucSignalGrpIndex++;
				}

				sPhaseStep[j].ucStepTime[ucColorIndex] -= ucMinStepLen;
				
			} //end for
			
			SetRedOtherLamp(m_pRunData->sStageStepInfo[*ucCurStepIndex].ucLampOn);

			ucMinStepLen = 200;
			(*ucCurStepIndex)++;
			if ( *ucCurStepIndex > MAX_STEP )
			{
				return;
			}
			i = -1;  //重新开始查找下一轮步伐
			
		}
	
	}  //end for
	
}



/*********************************************************************************
Function:       CManaKernel::ExitOvelapPhase
Description:    当前是否已存在该跟随相位			
Input:          ucOverlapPhaseId  ： 相位号
*      			ucPhaseCnt ：相位数
*       		pPhaseStep : 相位步伐信息
Output:         
Return:         true - 存在  false - 不存在
**********************************************************************************/
bool CManaKernel::ExitOvelapPhase(Byte ucOverlapPhaseId,Byte ucPhaseCnt,SPhaseStep* pPhaseStep)
{
	for ( Byte ucIndex=0; ucIndex<ucPhaseCnt; ucIndex++ )
	{
		if ( !pPhaseStep[ucIndex].bIsAllowPhase && (ucOverlapPhaseId == pPhaseStep[ucIndex].ucPhaseId))
		{
			return true;
		}
	}
	return false;
}


/*********************************************************************************
Function:       CManaKernel::SetRedOtherLamp
Description:    将未设置灯色信息到信号灯组强制设置为红灯亮			
Input:          无
Output:         ucLampOn  - 信号灯亮灭数组指针
Return:         无
**********************************************************************************/
void CManaKernel::SetRedOtherLamp(Byte* ucLampOn)
{
	bool bRed = false;

	for ( int i=0; i<MAX_LAMP; i=i+3 )
	{
		bRed = true;
		for (int j=i; j<(i+3); j++)
		{
			if ( 1 == ucLampOn[j] )        //该灯组有被置位
			{
				bRed = false;
				break;
			}
		}
		if ( bRed && IsInChannel(i/3+1) )  //该灯组(通道)有被定义
		{
			ucLampOn[i] = 1;         	   //该灯组强制亮红灯
		}
	}
}


/*********************************************************************************
Function:       CManaKernel::ExitStageStretchPhase
Description:    判断阶段的相位组是否存在弹性相位			
Input:          pScheduleTime - 子阶段信息
Output:         无
Return:         true -弹性相位   false -非弹性相位
**********************************************************************************/
bool CManaKernel::ExitStageStretchPhase(SScheduleTime* pScheduleTime)
{
	for (Byte ucIndex=0; ucIndex<MAX_PHASE; ucIndex++ )
	{
		if ( ( pScheduleTime->usAllowPhase>>ucIndex ) & 0x1 )  //放行相位情况
		{
			if ( (m_pTscConfig->sPhase[ucIndex].ucType>>5) & 0x1 )
			{
				return true;
			}
		}
	}
	return false;
}


/*********************************************************************************
Function:       CManaKernel::GetPhaseStepTime
Description:    根据相位号，获取该相位的绿、绿闪、黄、全红时间(普通相位)			
Input:          ucPhaseId:相位号  pScheduleTime:子阶段信息
Output:         pTime:各个时间
Return:         无
**********************************************************************************/
void CManaKernel::GetPhaseStepTime(Byte ucPhaseId,SScheduleTime* pScheduleTime,Byte* pTime)
{
	pTime[2] = pScheduleTime->ucYellowTime;  //yellow
	pTime[3] = pScheduleTime->ucRedTime;     //red

	if ( /*( (m_pTscConfig->sPhase[ucPhaseId].ucType>>5) & 0x1 )*/ 
		ExitStageStretchPhase(pScheduleTime)
		&& ( (CTRL_VEHACTUATED == m_pRunData->uiCtrl)
		|| (CTRL_MAIN_PRIORITY == m_pRunData->uiCtrl ) 
		|| (CTRL_SECOND_PRIORITY == m_pRunData->uiCtrl) ) 
		/*&& (m_sPhaseDet[ucPhaseId].iRoadwayCnt > 0)*/ )  //感应控制  阶段的相位组存在弹性相位 
	{
		//pTime[0] = m_pTscConfig->sPhase[ucPhaseId].ucMinGreen;  //min green
		pTime[0] = GetStageMinGreen(pScheduleTime->usAllowPhase);
		pTime[1] = m_pTscConfig->sPhase[ucPhaseId].ucGreenFlash; //green flash
	}
	else 
	{
		if ( 0 == pScheduleTime->ucGreenTime )  //阶段绿灯为0
		{
			pTime[1] = 0;  //green flash
			pTime[0] = 0;  //green
		}
		else if ( pScheduleTime->ucGreenTime > m_pTscConfig->sPhase[ucPhaseId].ucGreenFlash )
		{
			pTime[1] = m_pTscConfig->sPhase[ucPhaseId].ucGreenFlash;  //green flash
			pTime[0] = pScheduleTime->ucGreenTime - pTime[1];         //green
		}
		else
		{
			pTime[1] = m_pTscConfig->sPhase[ucPhaseId].ucGreenFlash;  //green flash
			pTime[0] = pScheduleTime->ucGreenTime;         //green
		}
	}
	if(m_iTimePatternId ==251 || m_iTimePatternId ==250) //特殊相位组合方案无绿闪
	{
			pTime[1] = 0;  //green flash
			pTime[0] = pScheduleTime->ucGreenTime;         //green
	}
	if ( (m_pTscConfig->sPhase[ucPhaseId].ucOption>>1) & 0x1 )      //行人相位
	{
		pTime[2] = 0;    //行人相位没有黄灯
	}
	//ACE_DEBUG((LM_DEBUG,"%s:%d ucPhaseId=%d  green time=%d,gf time = %d yellow time = %d\n" ,__FILE__,__LINE__,ucPhaseId+1,pTime[0],pTime[1],pTime[2]));
}


/*********************************************************************************
Function:       CManaKernel::ExitStageStretchPhase
Description:    获取跟随相位的绿、绿闪、黄、全红时间			
Input:           ucCurStageIndex  - 当前阶段配时下标  
*      			 ucOverlapPhaseId - 跟随相位下标 0 1 2 .... 7 
*				 pIncludeTime     - 跟随相位的包含相位的绿 绿闪 黄 全红时间
Output:         pTime - 绿、绿闪、黄、全红时间
Return:         无
**********************************************************************************/
void CManaKernel::GetOverlapPhaseStepTime(  
											    Byte ucCurStageIndex
											  , Byte ucOverlapPhaseId 
											  , Byte* pIncludeTime
											  , Byte* pOverlapTime
											  , Byte ucPhaseIndex
											  , Byte ucStageYellow )
{
	bool bExitNextStage             = false;                //包含相位存在下一个阶段里
	Byte ucCnt                      = 0;
	Byte ucNextStageIndex           = ucCurStageIndex + 1;  //下一个阶段下标
	unsigned short usNextAllowPhase = 0;                    //下一个相位组
	int  iIndex                     = 0;
	int  jIndex                     = 0;
	Byte ucOverlapPhase[MAX_OVERLAP_PHASE] = {0};

	if ( ucNextStageIndex >= m_pRunData->ucStageCount )
	{
		ucNextStageIndex = 0;
	}

	usNextAllowPhase = m_pRunData->sScheduleTime[ucNextStageIndex].usAllowPhase;
	
	while ( iIndex < MAX_PHASE  ) 
	{
		if ( ( usNextAllowPhase >> iIndex ) & 1 )
		{
			GetOverlapPhaseIndex(iIndex+1 , &ucCnt , ucOverlapPhase);  //获取下一个阶段放行相位的跟随相位
			if ( ucCnt > 0 )
			{
				jIndex = 0;
				while ( jIndex < ucCnt )
				{
					if ( ucOverlapPhaseId == ucOverlapPhase[jIndex] )
					{
						bExitNextStage = true;
						break;
					}
					jIndex++;
				}

			}
		}
		
		if ( bExitNextStage )
		{
			break;
		}

        iIndex++;
	}


	if ( bExitNextStage )   //跟随相位存在于下一个阶段
	{
		pOverlapTime[0] = pIncludeTime[0] + pIncludeTime[1] + pIncludeTime[2] + pIncludeTime[3];  //green
		//可以再根据尾部绿灯、尾部黄灯、尾部红灯 后续可能扩展设置 .......
		pOverlapTime[1] = 0;  //green flash
		pOverlapTime[2] = 0;  //yellow
		pOverlapTime[3] = 0;  //red

		if ( (m_pTscConfig->sPhase[ucPhaseIndex].ucOption>>1) & 0x1 )      //行人相位
		{
			pOverlapTime[0] += ucStageYellow;  //行人相位 补上黄闪的时间
		}
	}
	else  //跟随相位在该阶段正常结束，即需要经过正常的过度步
	{
		Byte ucTailGreen = 0 ;
		Byte ucTailYw = 0 ;
		Byte ucTailRed = 0 ;
		for(int iDex = 0 ; iDex<MAX_OVERLAP_PHASE;iDex++)
		{
			if((m_pTscConfig->sOverlapPhase)[iDex].ucId ==  ucOverlapPhaseId+1)
			{
				ucTailGreen = m_pTscConfig->sOverlapPhase[iDex].ucTailGreen ;
				ucTailYw =  m_pTscConfig->sOverlapPhase[iDex].ucTailYellow ;
				ucTailRed = m_pTscConfig->sOverlapPhase[iDex].ucTailRed ;
				//ACE_DEBUG((LM_DEBUG,"%s:%d ucTailGreen =%d ,ucTailYw = %d ,ucTailRed = %d  \n" ,__FILE__,__LINE__,ucTailGreen,ucTailYw,ucTailRed));
				break ;
				
			}

		}
		if(ucTailGreen != 0)
		{
			pOverlapTime[0] = pIncludeTime[0]+pIncludeTime[1]+ucTailGreen;  //green
			pOverlapTime[1] = 0;  //green flash
			pOverlapTime[2] = ucTailYw;  //yellow
			pOverlapTime[3] = ucTailRed; //red
		}
		else
		{
			pOverlapTime[0] = pIncludeTime[0];  //green
			pOverlapTime[1] = pIncludeTime[1];  //green flash
			pOverlapTime[2] = pIncludeTime[2];  //yellow
			pOverlapTime[3] = pIncludeTime[3]; //red

		}
	}
}


/*********************************************************************************
Function:       CManaKernel::GetPhaseStepLen
Description:    获取相位在某个步伐的应截取的长度			
Input:          pPhaseStep:相位步伐信息指针
Output:         无
Return:         相位截取步伐的大小
**********************************************************************************/
Byte CManaKernel::GetPhaseStepLen( SPhaseStep* pPhaseStep)
{
	int i = 0;
	while ( i < 4 )
	{
		//ACE_DEBUG((LM_DEBUG,"%s:%d i= %d pPhaseStep->ucStepTime[%d]=%d \n" ,__FILE__,__LINE__,i,i,pPhaseStep->ucStepTime[i]));
		if ( pPhaseStep->ucStepTime[i] >0 )
		{
			//ACE_DEBUG((LM_DEBUG,"%s:%d i= %d pPhaseStep->ucStepTime[%d]=%d \n" ,__FILE__,__LINE__,i,i,pPhaseStep->ucStepTime[i]));
			return pPhaseStep->ucStepTime[i];
		}
		i++;
	}
	return 0;
}


/*********************************************************************************
Function:       CManaKernel::GetPhaseStepIndex
Description:    返回相位的下标，即判断该相位是否已经全部截取完毕			
Input:          pPhaseStep:相位步伐信息指针
Output:         无
Return:         0-3：截取下标号 5:截取完毕
**********************************************************************************/
Byte CManaKernel::GetPhaseStepIndex( SPhaseStep* pPhaseStep )
{
	int i = 0;

	while ( i < 4 )
	{
		if ( pPhaseStep->ucStepTime[i] >0 )  //0:green 1:green flash 2:yellow 3:red 
		{
			return i;
		}
		i++;
	}
	return 5;
}


/*********************************************************************************
Function:       CManaKernel::GetSignalGroupId
Description:    根据相位号获取通道			
Input:          bIsAllowPhase - 是否为放行相位 false - 跟随相位
*        		ucPhaseId     - 相位ID 
*       		pNum          - 通道个数 
*       		pSignalGroup  - 各个通道号
Output:         pNum:通道个数 pSignalGroup:各个通道号
Return:         0-3：截取下标号 5:截取完毕
**********************************************************************************/
void CManaKernel::GetSignalGroupId(bool bIsAllowPhase , Byte ucPhaseId,Byte* pNum,Byte* pSignalGroup)
{
	*pNum = 0;
	for (int i=0; i<MAX_CHANNEL; i++ )
	{
		if ( 0 == m_pTscConfig->sChannel[i].ucId )
		{
			continue;
		}
		else 
		{
			if ( m_pTscConfig->sChannel[i].ucSourcePhase == ucPhaseId )
			{
				if ( ( bIsAllowPhase && ( CHANNEL_OVERLAP == m_pTscConfig->sChannel[i].ucType ) ) 
					|| ( !bIsAllowPhase && ( CHANNEL_OVERLAP != m_pTscConfig->sChannel[i].ucType ) ) )
				{
					continue;
				}

				if ( *pNum > MAX_CHANNEL )
				{
					continue;
				}
				pSignalGroup[*pNum] = m_pTscConfig->sChannel[i].ucId;
				(*pNum)++;
			}
		}
		
	}
}


/*********************************************************************************
Function:       CManaKernel::SwitchStatus
Description:    信号机工作状态切换			
Input:          uiWorkStatus - 新工作状态
Output:         无
Return:         无
**********************************************************************************/
void CManaKernel::SwitchStatus(unsigned int uiWorkStatus)
{
#ifndef TSC_DEBUG
	ACE_DEBUG((LM_DEBUG,"%s:%d New WorkStatus:%d,Old WorkStatus:%d\n",__FILE__,__LINE__,uiWorkStatus,m_pRunData->uiWorkStatus));
#endif
	if ( uiWorkStatus == m_pRunData->uiWorkStatus )
	{
		return;
	}

	if ( ( SIGNALOFF == m_pRunData->uiWorkStatus ) && ( STANDARD == uiWorkStatus ) )  //关灯切换到标准 需要经过黄闪
	{
		m_pRunData->uiWorkStatus = FLASH;
	}
	else if ( (FLASH == m_pRunData->uiWorkStatus) && ( STANDARD == uiWorkStatus ) )  //黄闪切换到标准 需要经过全红 
	{
		m_bWaitStandard          = true;
		m_pRunData->uiWorkStatus = ALLRED;
	}
	else
	{
		m_pRunData->uiWorkStatus = uiWorkStatus;
	}

	m_pRunData->bNeedUpdate = true;   /*去掉手动时需要重新加载通道信息等一系列信息*/
	ResetRunData(0);  //正常的重新获取动态参数

	CLampBoard::CreateInstance()->SetLamp(m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucLampOn	,m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucLampFlash);
}


/*********************************************************************************
Function:       CManaKernel::SwitchCtrl
Description:    信号机控制模式切换		
Input:          uiCtrl - 新控制模式
Output:         无
Return:         无
**********************************************************************************/
void CManaKernel::SwitchCtrl(unsigned int uiCtrl)
{
#ifndef TSC_DEBUG
	ACE_DEBUG((LM_DEBUG,"%s:%d SwitchCtrl: New Ctrl:%d,Old Ctrl:%d\n",__FILE__,__LINE__,uiCtrl,m_pRunData->uiCtrl));
#endif
	if ( CTRL_UTCS == uiCtrl )
	{
		m_pRunData->uiUtcsHeartBeat = 0;
	}
	if ( uiCtrl == m_pRunData->uiCtrl )
	{
		return;
	}
	
	if ( (CTRL_MANUAL == m_pRunData->uiCtrl) && m_bSpePhase )  //当前的控制方式为手动特定相位，准备去除手动
	{
		SThreadMsg sTscMsgSts;
		sTscMsgSts.ulType             = TSC_MSG_SWITCH_STATUS;  
		sTscMsgSts.ucMsgOpt           = 0;
		sTscMsgSts.uiMsgDataLen       = 1;
		sTscMsgSts.pDataBuf           = ACE_OS::malloc(1);
		*((Byte*)sTscMsgSts.pDataBuf) = FLASH;  //闪光
		CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
	}	

	if((CTRL_VEHACTUATED == uiCtrl) || (CTRL_MAIN_PRIORITY == uiCtrl) || (CTRL_SECOND_PRIORITY == uiCtrl) || (CTRL_ACTIVATE == uiCtrl)) 
	{
		if ( !CDetector::CreateInstance()->HaveDetBoard())   //MOD: 201309231450
		{			
			if(m_pRunData->uiCtrl == CTRL_SCHEDULE )         //防止重复写入控制方式更改日志
				return ;
			else
				uiCtrl    = CTRL_SCHEDULE;			
			CMainBoardLed::CreateInstance()->DoModeLed(true,true);								
			ACE_DEBUG((LM_DEBUG,"%s:%d when no DecBoard ,new uiCtrl = %d \n" ,__FILE__,__LINE__,uiCtrl));
		}
		else if ( CTRL_ACTIVATE != uiCtrl )
		{
			m_pRunData->bNeedUpdate = true;   //需要获取检测器信息
			ACE_DEBUG((LM_DEBUG,"%s:%d CTRL_VEHACTUATED == uiCtrl,m_pRunData->bNeedUpdate == true \n" ,__FILE__,__LINE__));
		}
	}
	if ( uiCtrl == CTRL_LAST_CTRL )  //上次控制方式
	{		
		unsigned int uiCtrlTmp = m_pRunData->uiCtrl ;
		m_pRunData->uiCtrl      = m_pRunData->uiOldCtrl;
		//m_pRunData->uiOldCtrl   = uiCtrlTmp;
		m_pRunData->uiOldCtrl   = uiCtrlTmp;
		ACE_DEBUG((LM_DEBUG,"%s:%d uiCtrl = %d ,old_ctrl =%d new_ctrl = %d \n" ,__FILE__,__LINE__,uiCtrl,m_pRunData->uiOldCtrl,m_pRunData->uiCtrl));
	}
	else
	{
		m_pRunData->uiOldCtrl = m_pRunData->uiCtrl;
		m_pRunData->uiCtrl    = uiCtrl;
		ACE_DEBUG((LM_DEBUG,"%s:%d m_pRunData->uiOldCtrl =%d m_pRunData->uiCtrl = %d \n" ,__FILE__,__LINE__,m_pRunData->uiOldCtrl,m_pRunData->uiCtrl));
	}

	if ( uiCtrl == CTRL_PANEL || uiCtrl == CTRL_MANUAL )               //如果是切换到手动或者面板控制则发送黑屏倒计时
	{             
		if ( m_pTscConfig->sSpecFun[FUN_COUNT_DOWN].ucValue != 0  )
			CGaCountDown::CreateInstance()->sendblack();
	}
	
	if ( CTRL_PANEL == m_pRunData->uiOldCtrl && m_iTimePatternId != 0)  //MOD:20130928 将这个判断从上面移动到这里
	{		
		m_iTimePatternId = 0;
		ACE_DEBUG((LM_DEBUG,"%s:%d After CTRL_PANEL to auto m_iTimePatternId = %d \n" ,__FILE__,__LINE__,m_iTimePatternId));
	}
	ACE_DEBUG((LM_DEBUG,"%s:%d oldctrl = %d newctrl = %d\n"	,__FILE__,__LINE__,m_pRunData->uiOldCtrl,m_pRunData->uiCtrl));
	SndMsgLog(LOG_TYPE_OTHER,1,m_pRunData->uiOldCtrl,m_pRunData->uiCtrl,0); //日志记录控制方式切换 ADD?201311041530
}



/*********************************************************************************
Function:       CManaKernel::SetUpdateBit
Description:    设置数据库更新属性为真		
Input:          uiCtrl - 新控制模式
Output:         无
Return:         无
**********************************************************************************/
void CManaKernel::SetUpdateBit()
{
#ifndef NO_CHANGEBY_DATABASE
	m_pRunData->bNeedUpdate = true;
#endif
}

/*********************************************************************************
Function:       CManaKernel::ChangePatter
Description:    切换信号机的配时方案，用于切换到特殊方案
Input:          无
Output:         无
Return:         无
**********************************************************************************/
void CManaKernel::ChangePatter()
{
	if ( IsLongStep(m_pRunData->ucStepNo) )
	{
		int iMinGreen = GetMaxStageMinGreen(m_pRunData->ucStepNo);
		if ( m_pRunData->ucElapseTime < iMinGreen )
		{			
			ACE_OS::sleep(iMinGreen-m_pRunData->ucElapseTime);  //走完最小绿
		}
	}
	//CGbtMsgQueue::CreateInstance()->SendTscCommand(OBJECT_SWITCH_MANUALCONTROL, 0);	
	ResetRunData(0);
	CLampBoard::CreateInstance()->SetLamp(m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucLampOn
		,m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucLampFlash);
	ACE_DEBUG((LM_DEBUG,"%s:%d Change pattern\n" , __FILE__,__LINE__));
		
}


/*********************************************************************************
Function:       CManaKernel::LockStage
Description:    锁定阶段
Input:          ucStage - 阶段号
Output:         无
Return:         无
**********************************************************************************/
void CManaKernel::LockStage(Byte ucStage)
{
	Byte ucCurStepNo = m_pRunData->ucStepNo;
	int iMinGreen    = 0;
	if( ucStage < 0 || ucStage >= m_pRunData->ucStageCount)
	{
		return;
	}

	ACE_DEBUG((LM_DEBUG,"%s:%d ucStage:%d m_pRunData->ucStageNo:%d LongStep:%d\n" , __FILE__,__LINE__,ucStage , m_pRunData->ucStageNo , IsLongStep(m_pRunData->ucStepNo) ));

	m_bSpePhase = false;

	if ( ucStage == m_pRunData->ucStageNo && (IsLongStep(m_pRunData->ucStepNo)) )  //锁定阶段就是当前阶段，且阶段处于绿灯阶段
	{
		return;
	}

	//保证走完最小绿
	if ( IsLongStep(m_pRunData->ucStepNo) )
	{
		iMinGreen = GetMaxStageMinGreen(m_pRunData->ucStepNo);
		if ( m_pRunData->ucElapseTime < iMinGreen )
		{
			//return;
			ACE_OS::sleep(iMinGreen-m_pRunData->ucElapseTime);  //走完最小绿
		}
	}
	
	//过渡步
	while ( true )
	{
		m_pRunData->ucStepNo++;
		if ( m_pRunData->ucStepNo >= m_pRunData->ucStepNum )
		{
			m_pRunData->ucStepNo = 0;
		}

		if ( !IsLongStep(m_pRunData->ucStepNo) )  //非长步
		{
			m_pRunData->ucElapseTime = 0;
			m_pRunData->ucStepTime = m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucStepLen;
			m_pRunData->ucStageNo = StepToStage(m_pRunData->ucStepNo,NULL); //根据步伐号获取阶段号

			CLampBoard::CreateInstance()->SetLamp(m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucLampOn
				,m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucLampFlash);
			ACE_OS::sleep(m_pRunData->ucStepTime);

		}
		else
		{
			break;
		}

		if ( ucCurStepNo == m_pRunData->ucStepNo )
		{
			break;
		}
	}	
	
	m_pRunData->ucStageNo = ucStage;
	m_pRunData->ucStepNo = StageToStep(m_pRunData->ucStageNo);  //根据阶段号获取步伐号
	m_pRunData->ucStepTime = m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucStepLen;
	m_pRunData->ucElapseTime = 0;
	m_pRunData->ucRunTime = m_pRunData->ucStepTime;

	CLampBoard::CreateInstance()->SetLamp(m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucLampOn
		,m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucLampFlash);
	m_bNextPhase = false ;
}


/*********************************************************************************
Function:       CManaKernel::LockNextStage
Description:    切换到下一个阶段锁定
Input:          无
Output:         无
Return:         无
**********************************************************************************/
void CManaKernel::LockNextStage()
{	
	int iCurStageNo = m_pRunData->ucStageNo;	
	SThreadMsg sTscMsgSts;	
	m_bSpePhase = false; 
	iCurStageNo++;

	if ( iCurStageNo >= m_pRunData->ucStageCount )
	{
		iCurStageNo  = 0;
	}

	ACE_DEBUG((LM_DEBUG,"%s:%d iCurStageNo:%d cnt:%d\n",__FILE__,__LINE__,iCurStageNo,m_pRunData->ucStageCount));

	ACE_OS::memset( &sTscMsgSts , 0 , sizeof(SThreadMsg) );
	sTscMsgSts.ulType       = TSC_MSG_LOCK_STAGE;  
	sTscMsgSts.ucMsgOpt     = 0;
	sTscMsgSts.uiMsgDataLen = 1;
	sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
	*((Byte*)sTscMsgSts.pDataBuf) = iCurStageNo; 
	CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));

}


/*********************************************************************************
Function:       CManaKernel::LockStep
Description:    锁定步伐
Input:          ucStep - 0:下一步 other:指定步伐
Output:         无
Return:         无
**********************************************************************************/
void CManaKernel::LockStep(Byte ucStep)
{
	//保证走完最小绿
	int iMinGreen = 0;

	if ( STANDARD != m_pRunData->uiWorkStatus )  //黄闪、全红以及关灯没有下一步
	{
		return;
	}
	
	m_bSpePhase   = false; 

	if ( IsLongStep(m_pRunData->ucStepNo) )
	{
		iMinGreen = GetMaxStageMinGreen(m_pRunData->ucStepNo);

		if ( m_pRunData->ucElapseTime < iMinGreen )
		{
			return;
		}
	}
		
	if ( 0 == ucStep )
	{
		m_pRunData->ucStepNo++;
		
		//ACE_DEBUG((LM_DEBUG,"*******%d %d \n",m_pRunData->ucStepNo,m_pRunData->ucStepNum));

		if ( m_pRunData->ucStepNo >= m_pRunData->ucStepNum )
		{
			m_pRunData->ucStepNo  = 0;
		}
	}
	else if ( (ucStep>0) && (ucStep<m_pRunData->ucStepNum) )
	{
		m_pRunData->ucStepNo = ucStep - 1;
	}

	//从已有的步伐表快速加载下一步
	m_pRunData->ucStepTime = m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucStepLen;
	m_pRunData->ucElapseTime = 0;
	m_pRunData->ucRunTime = m_pRunData->ucStepTime;
	m_pRunData->ucStageNo = StepToStage(m_pRunData->ucStepNo,NULL); //根据步伐号获取阶段号

	CLampBoard::CreateInstance()->SetLamp(m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucLampOn	,m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucLampFlash);
	
}


/*********************************************************************************
Function:       CManaKernel::LockStep
Description:    锁定相位命令
Input:          PhaseId  - 锁定相位号
Output:         无
Return:         无
**********************************************************************************/
void CManaKernel::LockPhase(Byte PhaseId)
{
	int iMinGreen         = 0;
	Byte ucLampIndex      = 0;
	Byte ucSignalGrpNum   = 0;
	Byte ucSignalGrpIndex = 0;
	Byte ucSignalGrp[MAX_CHANNEL] = {0};

	if ( !m_bSpePhase )
	{
		//长步保证走完最小绿
		if ( IsLongStep(m_pRunData->ucStepNo) )
		{
			iMinGreen = GetMaxStageMinGreen(m_pRunData->ucStepNo);
			if ( m_pRunData->ucElapseTime < iMinGreen )
			{
#ifndef WINDOWS
				sleep(iMinGreen-m_pRunData->ucElapseTime); 
#endif
			}
		}

		//过渡步
		while ( true )
		{
			m_pRunData->ucStepNo++;
			if ( m_pRunData->ucStepNo >= m_pRunData->ucStepNum )
			{
				m_pRunData->ucStepNo = 0;
			}
			if ( !IsLongStep(m_pRunData->ucStepNo) )  //非长步
			{
				m_pRunData->ucElapseTime = 0;
				m_pRunData->ucStepTime   = m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucStepLen;
				m_pRunData->ucStageNo    = StepToStage(m_pRunData->ucStepNo,NULL); //根据步伐号获取阶段号

				CLampBoard::CreateInstance()->SetLamp(m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucLampOn
					,m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucLampFlash);
#ifndef WINDOWS
				sleep(m_pRunData->ucStepTime);
#endif
			}
			else
			{
				m_pRunData->ucStepNo--;
				break;
			}
		}
	}

	m_bSpePhase = true;
	ACE_OS::memset(m_ucLampOn , 0 , MAX_LAMP);
	ACE_OS::memset(m_ucLampFlash , 0 , MAX_LAMP);

	GetSignalGroupId(true , PhaseId , &ucSignalGrpNum , ucSignalGrp);

	ucSignalGrpIndex = 0;
	while ( ucSignalGrpIndex < ucSignalGrpNum )  //red yellow green
	{
		ucLampIndex = ( ucSignalGrp[ucSignalGrpIndex] - 1 ) * 3; //相位组的red下标
		ucLampIndex = ucLampIndex + 2;  //green
		m_ucLampOn[ucLampIndex] = 1;
		ucSignalGrpIndex++;
	}
	SetRedOtherLamp(m_ucLampOn);
	CLampBoard::CreateInstance()->SetLamp(m_ucLampOn,m_ucLampFlash);
}


/*********************************************************************************
Function:       CManaKernel::ReadTscEvent
Description:    事件读取
Input:          无
Output:         无
Return:         无
**********************************************************************************/
void CManaKernel::ReadTscEvent()
{
	//待补充
}


/*********************************************************************************
Function:       CManaKernel::ReadTscEvent
Description:    信号机校时接口,用于手动校时和GPS校时处理
Input:          ucType - 校时类型
				pValue - 时间值指针
Output:         无
Return:         无
**********************************************************************************/
void CManaKernel::CorrectTime(Byte ucType,Byte* pValue)
{
	int iIndex = 0;
	int fd     = 0;
	Ulong iTotalSec = 0;
	ACE_Time_Value tvCurTime;
	ACE_Date_Time  tvDateTime;

	while ( iIndex < 4 )
	{
		iTotalSec |=  ((pValue[iIndex]&0xff)<<(8*(3-iIndex)));
		iIndex++;
	}

	//ACE_DEBUG((LM_DEBUG,"******iTotalSec:%d\n",iTotalSec));

	if ( ucType == OBJECT_LOCAL_TIME ) //本地时间
	{
		iTotalSec = iTotalSec + 8 * 3600;
	}
	else
	{
		iTotalSec = iTotalSec - 8 * 3600;

	}
	tvCurTime.sec(iTotalSec);
	tvDateTime.update(tvCurTime);
	
	if (!IsRightDate((Uint)tvDateTime.year(),(Byte)tvDateTime.month(),(Byte)tvDateTime.day()))
	{
		ACE_DEBUG((LM_DEBUG,"%s:%d error date:%d-%d-%d\n",__FILE__,__LINE__,tvDateTime.year(),tvDateTime.month(),tvDateTime.day()));
		return;
	}

	CTimerManager::CreateInstance()->CloseAllTimer();

#ifdef WINDOWS
	SYSTEMTIME st;
	st.wYear   = (WORD)tvDateTime.year();
	st.wMonth  = (WORD)tvDateTime.month();
	st.wDay    = (WORD)tvDateTime.day();
	st.wHour   = (WORD)tvDateTime.hour();
	st.wMinute = (WORD)tvDateTime.minute();
	st.wSecond = (WORD)tvDateTime.second(); 
	st.wMilliseconds = 0; 
	//SetSystemTime(&st); 
#else
	struct tm now;
	time_t ti = iTotalSec;

	now.tm_year = tvDateTime.year();
	now.tm_mon  = tvDateTime.month();
	now.tm_mday = tvDateTime.day();
	now.tm_hour = tvDateTime.hour();
	now.tm_min  = tvDateTime.minute();
	now.tm_sec  = tvDateTime.second();

	now.tm_year -= 1900;
	now.tm_mon--;
	now.tm_zone = 0;
	
	ti = mktime(&now);
	//ACE_DEBUG((LM_DEBUG, "%s:%d iTotalSec =%d\n",__FILE__,__LINE__,iTotalSec));
	//ACE_DEBUG((LM_DEBUG, "%s:%d ti =%d\n",__FILE__,__LINE__,ti));
   	//ACE_DEBUG((LM_DEBUG, "%s:%d  %4d-%02d-%02d %02d:%02d:%02d \n",__FILE__,__LINE__,now.tm_year,now.tm_mon,now.tm_mday,now.tm_hour,now.tm_min,now.tm_sec));
	//ACE_DEBUG((LM_DEBUG, "22222222222\n"));

	stime(&ti);   //设置系统时间	
	fd = open(DEV_RTC, O_WRONLY, 0);
	if(fd > 0)
	{
		ioctl(fd, RTC_SET_TIME, &now);
		close(fd);
	}
#endif
	
	CTimerManager::CreateInstance()->StartAllTimer();

	SetUpdateBit();
	CWirelessCoord::CreateInstance()->ForceAssort();

	SThreadMsg sTscMsg;
	sTscMsg.ulType       = TSC_MSG_LOG_WRITE;
	sTscMsg.ucMsgOpt     = LOG_TYPE_CORRECT_TIME;
	sTscMsg.uiMsgDataLen = 4;
	sTscMsg.pDataBuf     = ACE_OS::malloc(4);
	((Byte*)(sTscMsg.pDataBuf))[3] = iTotalSec &0xff ;
	((Byte*)(sTscMsg.pDataBuf))[2] = (iTotalSec>>8) &0xff ;
	((Byte*)(sTscMsg.pDataBuf))[1] = (iTotalSec>>16) &0xff ;
	((Byte*)(sTscMsg.pDataBuf))[0] = (iTotalSec>>24) &0xff ;
	CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));
}


/*********************************************************************************
Function:       CManaKernel::GetTscStatus
Description:    信号机状态获取
Input:          ucDealDataIndex - 待处理客户端下标
Output:         无
Return:         无
**********************************************************************************/
void CManaKernel::GetTscStatus(Byte ucDealDataIndex)
{
	GBT_DB::TblDetector tblDetector;
	SThreadMsg sGbtMsg;
	sGbtMsg.ulType       = GBT_MSG_TSC_STATUS;
	sGbtMsg.ucMsgOpt     = ucDealDataIndex;
	sGbtMsg.uiMsgDataLen = sizeof(STscStatus);
	sGbtMsg.pDataBuf     = ACE_OS::malloc(sizeof(STscStatus));
	ACE_OS::memset(sGbtMsg.pDataBuf,0,sizeof(STscStatus));
	
	((STscStatus*)(sGbtMsg.pDataBuf))->bStartFlash   = m_pRunData->bStartFlash;
	((STscStatus*)(sGbtMsg.pDataBuf))->uiWorkStatus  = m_pRunData->uiWorkStatus;
	((STscStatus*)(sGbtMsg.pDataBuf))->uiCtrl        = m_pRunData->uiCtrl;
	((STscStatus*)(sGbtMsg.pDataBuf))->ucStepNo      = m_pRunData->ucStepNo + 1;
	((STscStatus*)(sGbtMsg.pDataBuf))->ucStageNo     = m_pRunData->ucStageNo + 1;
	((STscStatus*)(sGbtMsg.pDataBuf))->ucActiveSchNo = m_pRunData->ucScheduleId;

	((STscStatus*)(sGbtMsg.pDataBuf))->ucTscAlarm2    = 0;
	if ( m_bRestart )
	{
		((STscStatus*)(sGbtMsg.pDataBuf))->ucTscAlarm2 |= (1<<5);  /*信号机停止运行*/
	}

	((STscStatus*)(sGbtMsg.pDataBuf))->ucTscAlarm1    = 0;
	if ( FLASH == m_pRunData->uiWorkStatus )
	{
		((STscStatus*)(sGbtMsg.pDataBuf))->ucTscAlarm1 |= (1<<5);   /*本地闪光控制*/  
	}
	if ( CTRL_UTCS != m_pRunData->uiCtrl )
	{
		((STscStatus*)(sGbtMsg.pDataBuf))->ucTscAlarm1 |= (1<<6);   /*本地单点控制*/  
	}

	/*
	ACE_DEBUG((LM_DEBUG,"@@@@@@@@@@@@@@ %d %d %d",m_pRunData->uiWorkStatus,m_pRunData->uiCtrl
		,((STscStatus*)(sGbtMsg.pDataBuf))->ucTscWarn1));
		*/

	((STscStatus*)(sGbtMsg.pDataBuf))->ucTscAlarmSummary  = 0;
	if ( CDetector::CreateInstance()->IsDetError() )
	{
		((STscStatus*)(sGbtMsg.pDataBuf))->ucTscAlarmSummary |= (1<<5); /*检测器故障*/
	}
	if ( m_bRestart )
	{
		((STscStatus*)(sGbtMsg.pDataBuf))->ucTscAlarmSummary |= (1<<7); /*信号机即将停止运行*/
	}

	/*检测器数据*/
	/*
	((STscStatus*)(sGbtMsg.pDataBuf))->ucActiveDetCnt = tblDetector.GetCount();
	GetDetectorSts(((STscStatus*)(sGbtMsg.pDataBuf))->sDetSts);
	GetDetectorData(((STscStatus*)(sGbtMsg.pDataBuf))->sDetData);
	GetDetectorAlarm(((STscStatus*)(sGbtMsg.pDataBuf))->sDetAlarm);
	*/
	((STscStatus*)(sGbtMsg.pDataBuf))->ucActiveDetCnt =  CDetector::CreateInstance()->GetActiveDetSum();
	CDetector::CreateInstance()->GetDetStatus(((STscStatus*)(sGbtMsg.pDataBuf))->sDetSts);
	CDetector::CreateInstance()->GetDetData(((STscStatus*)(sGbtMsg.pDataBuf))->sDetData);
	CDetector::CreateInstance()->GetDetAlarm(((STscStatus*)(sGbtMsg.pDataBuf))->sDetAlarm);

	GetCurStageLen(((STscStatus*)(sGbtMsg.pDataBuf))->ucCurStageLen);
	GetCurKeyGreenLen(((STscStatus*)(sGbtMsg.pDataBuf))->ucCurKeyGreen);

	GetPhaseStatus(((STscStatus*)(sGbtMsg.pDataBuf))->sPhaseSts);
	GetOverlapPhaseStatus(& (((STscStatus*)(sGbtMsg.pDataBuf))->sOverlapPhaseSts) );
	GetChannelStatus(((STscStatus*)(sGbtMsg.pDataBuf))->sChannelSts);
	CGbtMsgQueue::CreateInstance()->SendGbtMsg(&sGbtMsg,sizeof(SThreadMsg));
}


/*********************************************************************************
Function:       CManaKernel::GetTscStatus
Description:    信号机扩展状态获取
Input:          ucDealDataIndex - 待处理客户端下标
Output:         无
Return:         无
**********************************************************************************/
void CManaKernel::GetTscExStatus(Byte ucDealDataIndex)
{
	Byte ucTmp             = 0;
	Byte ucStageTotalTime  = 0;
	Byte ucStageElapseTime = 0;
	Byte ucChanIndex       = 0;
	Ulong ulChanOut[4]     = {0,0,0,0};
	Byte* pLampOn          = NULL; 
	Byte* pLampFlash       = NULL; 
	SThreadMsg sGbtMsg;
	sGbtMsg.ulType         = GBT_MSG_TSC_EXSTATUS;
	sGbtMsg.ucMsgOpt       = ucDealDataIndex;
	sGbtMsg.uiMsgDataLen   = 25;
	sGbtMsg.pDataBuf       = ACE_OS::malloc(25);
	ACE_OS::memset(sGbtMsg.pDataBuf,0,25);

	//((Byte*)sGbtMsg.pDataBuf)[0] = 1;

	ucTmp |= (m_pRunData->ucWorkMode      & 0x3);  //bit0-bit1
	ucTmp |= (m_pRunData->uiWorkStatus<<2 & 0xC);  //bit2-bit3
	ucTmp |= (m_pRunData->uiCtrl<<4       & 0xF0); //bit4-bit7
	((Byte*)sGbtMsg.pDataBuf)[0] = ucTmp;

	//ACE_DEBUG((LM_DEBUG,"m_pRunData->uiCtrl:%d ucTmp:%d\n",m_pRunData->uiCtrl,ucTmp));

	((Byte*)sGbtMsg.pDataBuf)[1] = m_pRunData->ucScheduleId;
	((Byte*)sGbtMsg.pDataBuf)[2] = m_pRunData->ucTimePatternId;
	((Byte*)sGbtMsg.pDataBuf)[3] = m_pRunData->ucScheduleTimeId;
	((Byte*)sGbtMsg.pDataBuf)[4] = m_pRunData->ucStageCount;
	((Byte*)sGbtMsg.pDataBuf)[5] = m_pRunData->ucStageNo + 1;
	GetStageTime(&ucStageTotalTime,&ucStageElapseTime);
	((Byte*)sGbtMsg.pDataBuf)[6] = ucStageTotalTime;   //阶段总时间
	((Byte*)sGbtMsg.pDataBuf)[7] = ucStageElapseTime;  //阶段已经运行时间

	if ( m_bSpePhase )  //指定相位
	{
		pLampOn    = m_ucLampOn;
		pLampFlash = m_ucLampFlash;
	}
	else
	{
		pLampOn    = m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucLampOn;
		pLampFlash = m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucLampFlash;
	}

	//打包通道状态
	for ( int i=0; i<MAX_LAMP; i++ )
	{
		ucChanIndex = i / 3;  

		if ( pLampOn[i] == 0 )  //非灯亮
		{
			continue;
		}

		switch ( i%3 )
		{
			case LAMP_COLOR_RED:
				ulChanOut[0] |= 1<<ucChanIndex;
				break;
			case LAMP_COLOR_YELLOW:
				ulChanOut[1] |= 1<<ucChanIndex;
				break;
			case LAMP_COLOR_GREEN:
				ulChanOut[2] |= 1<<ucChanIndex;
				break;
			default:
				break;
		}

		if ( 1 == pLampFlash[i] )  //通道闪烁输出状态
		{
			ulChanOut[3] |= 1<<ucChanIndex;
		}
	}

	ACE_OS::memcpy((Byte*)(sGbtMsg.pDataBuf)+8 , ulChanOut , 16);
	((Byte*)sGbtMsg.pDataBuf)[24] = m_pRunData->ucCycle;

	CGbtMsgQueue::CreateInstance()->SendGbtMsg(&sGbtMsg,sizeof(SThreadMsg));
}


/*********************************************************************************
Function:       CManaKernel::GetCurStageLen
Description:    获取当前方案各阶段时长
Input:          
Output:         pCurStageLen - 阶段时长指针
Return:         无
**********************************************************************************/
void CManaKernel::GetCurStageLen(Byte* pCurStageLen)
{
	Byte ucIndex = 0;
	for (Byte ucStageIndex=0; ucStageIndex<MAX_SON_SCHEDULE; ucStageIndex++ )
	{
		if ( m_pRunData->sScheduleTime[ucStageIndex].ucId != 0 )
		{
			pCurStageLen[ucIndex++] = m_pRunData->sScheduleTime[ucStageIndex].ucRedTime 
										+ m_pRunData->sScheduleTime[ucStageIndex].ucGreenTime
										+ m_pRunData->sScheduleTime[ucStageIndex].ucYellowTime;
		}
	}
}


/*********************************************************************************
Function:       CManaKernel::GetCurKeyGreenLen
Description:    当前方案各关键相位绿灯时长
Input:          
Output:         pCurKeyGreen - 关键相位时长数组指针
Return:         无
**********************************************************************************/
void CManaKernel::GetCurKeyGreenLen(Byte* pCurKeyGreen)
{
	Byte ucIndex = 0;
	for (Byte ucStageIndex=0; ucStageIndex<MAX_SON_SCHEDULE; ucStageIndex++ )
	{
		if ( m_pRunData->sScheduleTime[ucStageIndex].ucId != 0 )
		{
			pCurKeyGreen[ucIndex++] = m_pRunData->sScheduleTime[ucStageIndex].ucGreenTime;
		}
	}
}


/*********************************************************************************
Function:       CManaKernel::GetDetectorSts
Description:    获取检测器状态
Input:          
Output:         pDetStatus - 检测器状态指针
Return:         无
**********************************************************************************/
/*
void CManaKernel::GetDetectorSts(SDetectorStsPara* pDetStatus)
{
	Byte ucDetCnt = 0;
	GBT_DB::TblDetector tblDetector;
	GBT_DB::Detector*   pDet = NULL;

	CDbInstance::m_cGbtTscDb.QueryDetector(tblDetector);
	pDet = tblDetector.GetData(ucDetCnt);
	
	ACE_OS::memset(pDetStatus,0,8*sizeof(SDetectorStsPara));

	while ( ucDetCnt > 0 )
	{
		pDetStatus->ucId = pDet->ucDetectorId;
		ucDetCnt--;
	}
}
*/


/*********************************************************************************
Function:       CManaKernel::GetDetectorData
Description:    获取检测器参数
Input:          
Output:         pDetStatus - 检测器参数指针
Return:         无
**********************************************************************************/
/*
void CManaKernel::GetDetectorData(SDetectorDataPara* pDetData)
{
	Byte ucDetCnt = 0;
	GBT_DB::TblDetector tblDetector;
	GBT_DB::Detector*   pDet = NULL;

	CDbInstance::m_cGbtTscDb.QueryDetector(tblDetector);
	pDet = tblDetector.GetData(ucDetCnt);

	ACE_OS::memset(pDetData,0,48*sizeof(SDetectorDataPara));

	while ( ucDetCnt > 0 )
	{
		pDetData->ucId = pDet->ucDetectorId;
		ucDetCnt--;
	}
}
*/


/*********************************************************************************
Function:       CManaKernel::GetDetectorAlarm
Description:    获取车辆检测器告警参数
Input:          
Output:         pDetStatus - 检测器警告参数指针
Return:         无
**********************************************************************************/
/*
void CManaKernel::GetDetectorAlarm(SDetectorAlarm* pDetAlarm)
{
	Byte ucDetCnt = 0;
	GBT_DB::TblDetector tblDetector;
	GBT_DB::Detector*   pDet = NULL;

	CDbInstance::m_cGbtTscDb.QueryDetector(tblDetector);
	pDet = tblDetector.GetData(ucDetCnt);

	ACE_OS::memset(pDetAlarm,0,48*sizeof(SDetectorAlarm));

	while ( ucDetCnt > 0 )
	{
		pDetAlarm->ucId = pDet->ucDetectorId;
		ucDetCnt--;
	}
}
*/


/*********************************************************************************
Function:       CManaKernel::GetPhaseStatus
Description:    获取当前的相位状态信息
Input:          无
Output:         pPhaseStatus - 相位状态信息参数指针
Return:         无
**********************************************************************************/
void CManaKernel::GetPhaseStatus(SPhaseSts* pPhaseStatus)
{
	Byte ucCurStepNo  = m_pRunData->ucStepNo;
	Uint uiWorkStatus = m_pRunData->uiWorkStatus;

	for ( int i=0; i<(MAX_PHASE/8); i++ )
	{
		pPhaseStatus[i].ucId = i + 1;

		switch ( uiWorkStatus )
		{
		case SIGNALOFF: //关灯
			continue;
		case ALLRED:    //全红
			pPhaseStatus[i].ucRed    = 0xff;
			continue;
		case FLASH:     //闪光
			pPhaseStatus[i].ucYellow = 0xff;
			continue;
		default:
			break;
		}

		for ( int j = 0; j < 8; j++ )
		{ 
			switch ( m_pRunData->sStageStepInfo[ucCurStepNo].ucPhaseColor[i*8+j] ) 
			{
			case LAMP_COLOR_RED:
				pPhaseStatus[i].ucRed |= (1<<j) & 0xff;
				break;
			case LAMP_COLOR_YELLOW:
				pPhaseStatus[i].ucYellow |= (1<<j) & 0xff;
				break;
			case LAMP_COLOR_GREEN:
				pPhaseStatus[i].ucGreen |= (1<<j) & 0xff;
				break;
			default:
				pPhaseStatus[i].ucRed |= (1<<j) & 0xff;
				break;
			}
		}
	}
}


/*********************************************************************************
Function:       CManaKernel::GetOverlapPhaseStatus
Description:    获取跟随相位状态
Input:          无
Output:         pOverlapPhaseSts - 跟随相位状态信息参数指针
Return:         无
**********************************************************************************/
void CManaKernel::GetOverlapPhaseStatus(SOverlapPhaseSts* pOverlapPhaseSts)
{
	Byte ucCurStepNo  = m_pRunData->ucStepNo;
	Uint uiWorkStatus = m_pRunData->uiWorkStatus;
	
	pOverlapPhaseSts->ucId = 1;

	switch ( uiWorkStatus )
	{
	case SIGNALOFF: //关灯
		return;
	case ALLRED:    //全红
		pOverlapPhaseSts->ucRed    = 0xff;
		return;
	case FLASH:     //闪光
		pOverlapPhaseSts->ucYellow = 0xff;
		return;
	default:
		break;
	}

	for ( int i=0; i<MAX_OVERLAP_PHASE; i++ )
	{
		switch ( m_pRunData->sStageStepInfo[ucCurStepNo].ucOverlapPhaseColor[i] ) 
		{
			case LAMP_COLOR_RED:
				pOverlapPhaseSts->ucRed |= (1<<i) & 0xff;
				break;
			case LAMP_COLOR_YELLOW:
				pOverlapPhaseSts->ucYellow |= (1<<i) & 0xff;
				break;
			case LAMP_COLOR_GREEN:
				pOverlapPhaseSts->ucGreen |= (1<<i) & 0xff;
				break;
			default:
				pOverlapPhaseSts->ucRed |= (1<<i) & 0xff;
				break;
		}
	}
}



/*********************************************************************************
Function:       CManaKernel::GetChannelStatus
Description:    通道状态信息获取
Input:          无
Output:         pChannelSts - 通道状态信息参数指针
Return:         无
**********************************************************************************/
void CManaKernel::GetChannelStatus(SChannelSts* pChannelSts)
{
	Byte ucPhaseNo    = 0;
	Byte ucPhaseType  = 0;
	Byte ucIndex      = 0;
	Byte* pPhaseColor = NULL;
	Byte ucCurStepNo = m_pRunData->ucStepNo;
	Uint uiWorkStatus = m_pRunData->uiWorkStatus;
	
	while ( ucIndex < 2 )
	{
		switch ( uiWorkStatus )
		{
		case SIGNALOFF: //关灯
			ucIndex++;
			continue;
		case ALLRED:    //全红
			pChannelSts[ucIndex].ucRed = 0xff;
			ucIndex++;
			continue;
		case FLASH:     //闪光
			pChannelSts[ucIndex].ucYellow = 0xff;
			ucIndex++;
			continue;
		default:
			break;
		}

		pChannelSts[ucIndex].ucId = ucIndex + 1;
		for ( int i=0; i<8; i++ )
		{
			//ucPhaseNo = GetPhaseIdWithChannel(8*ucIndex+i+1);
			ucPhaseNo   = m_pTscConfig->sChannel[8*ucIndex+i].ucSourcePhase;
			ucPhaseType = m_pTscConfig->sChannel[8*ucIndex+i].ucType;
			if ( CHANNEL_OVERLAP == ucPhaseType )
			{
				pPhaseColor = m_pRunData->sStageStepInfo[ucCurStepNo].ucOverlapPhaseColor;
			}
			else
			{
				pPhaseColor = m_pRunData->sStageStepInfo[ucCurStepNo].ucPhaseColor;
			}
				
			if ( (ucPhaseNo>0) && (ucPhaseNo<MAX_PHASE) )  //存在该通道信息
			{
				switch ( pPhaseColor[ucPhaseNo-1] )  
				{
				case LAMP_COLOR_RED:
					pChannelSts[ucIndex].ucRed |= (1<<i);
					break;
				case LAMP_COLOR_YELLOW:
					pChannelSts[ucIndex].ucYellow |= (1<<i);
					break;
				case LAMP_COLOR_GREEN:
					pChannelSts[ucIndex].ucGreen |= (1<<i);
					break;
				default:
					pChannelSts[ucIndex].ucRed |= (1<<i);
					break;
				}
			}
		}
		ucIndex++;
	}
}


/*********************************************************************************
Function:       CManaKernel::GetPhaseIdWithChannel
Description:    根据通道号获取通道对应关联相位号
Input:          ucChannelNo -通道信号灯组号
Output:         无
Return:         相位号，若获取错误返回 MAX_PHASE+1(错误)
**********************************************************************************/
Byte CManaKernel::GetPhaseIdWithChannel(Byte ucChannelNo)
{
	for (Byte ucIndex=0; ucIndex<MAX_CHANNEL; ucIndex++ )
	{
		if ( m_pTscConfig->sChannel[ucIndex].ucId == 0 )
		{
			break;
		}
		else if (  m_pTscConfig->sChannel[ucIndex].ucId == ucChannelNo )
		{
			return m_pTscConfig->sChannel[ucIndex].ucSourcePhase;
		}
	}

	return MAX_PHASE+1;
}




/*********************************************************************************
Function:       CManaKernel::SetRestart
Description:    设置重启属性为真值
Input:          无
Output:         无
Return:         无
**********************************************************************************/
void CManaKernel::SetRestart()
{
	m_bRestart = true;
}


/*********************************************************************************
Function:       CManaKernel::GetRestart
Description:    获取重启参数属性值
Input:          无
Output:         无
Return:         重启参数属性值
**********************************************************************************/
bool CManaKernel::GetRestart()
{
	return m_bRestart;
}


/*********************************************************************************
Function:       CManaKernel::RestartTsc
Description:    重启信号机,1s进入一次
Input:          无
Output:         无
Return:         无
**********************************************************************************/
void CManaKernel::RestartTsc()
{
	static int iTick = 0;

	if ( !m_bRestart )
	{
		return;
	}

	iTick++;
	if ( iTick > 60 )
	{
		system("killall Gb.aiton");
		
		ACE_OS::sleep(2);
		system("./gb.aiton");
		
	}
}


/*********************************************************************************
Function:       CManaKernel::IsLongStep
Description:    判断当前步伐是否为长步，即绿灯亮起
Input:          iStepNo  - 步伐号
Output:         无
Return:         true - 长步  false - 非长步
**********************************************************************************/
bool CManaKernel::IsLongStep(int iStepNo)
{
	bool bGreen      = false;
	bool bGreenFlash = false;
	int iIndex       = 0;

	while ( iIndex < MAX_LAMP )
	{
		if ( LAMP_COLOR_RED == iIndex % 3 )
		{
			iIndex++;
			continue;
		}		
		if ( ( LAMP_COLOR_YELLOW == iIndex % 3 )&& (1 == m_pRunData->sStageStepInfo[iStepNo].ucLampOn[iIndex]) )  //存在黄灯
		{
			return false;
		}

		if ( 1 == m_pRunData->sStageStepInfo[iStepNo].ucLampOn[iIndex] )  //存在绿灯亮
		{
			if ( 0 == m_pRunData->sStageStepInfo[iStepNo].ucLampFlash[iIndex] ) //绿灯不闪
			{
				bGreen = true;
				//break ; //ADD: 2013 0702 1723 应该跳出循环
				
			}
			else  //绿闪 
			{
				bGreenFlash = true;
			}

			break ; ////应该跳出来 ADD?20130702 17 21
		}
		iIndex++;
	}

	if ( bGreen && !bGreenFlash ) //存在绿灯 并且没有(黄或黄闪或绿闪)
	{
		return true;
	}

	return false;   
}



/*********************************************************************************
Function:       CManaKernel::GetStageMinGreen
Description:    获取阶段相位组的最小绿
Input:          usAllowPhase  - 放行相位组合序列
Output:         无
Return:         最小绿值
**********************************************************************************/
int CManaKernel::GetStageMinGreen(Ushort usAllowPhase)
{
	int iMinGreen = 0; 

	for (Byte i=0; i<MAX_PHASE; i++ )
	{
		if ( ( usAllowPhase >> i ) & 0x1 )
		{
			iMinGreen = (iMinGreen < m_pTscConfig->sPhase[i].ucMinGreen) ?m_pTscConfig->sPhase[i].ucMinGreen:iMinGreen;

		}
	}

	return iMinGreen;
}



/*********************************************************************************
Function:       CManaKernel::GetCurStepMinGreen
Description:    获取当前步伐的最小绿(各个相位的最小绿)，正常在阶段的第一步获取
Input:          iStepNo  - 步伐号
Output:         iMaxGreenTime:步伐最大绿 iAddGreenTime:最小单位绿灯延长时间
Return:         当前步伐的最小绿
**********************************************************************************/
int CManaKernel::GetCurStepMinGreen(int iStepNo , int* iMaxGreenTime, int* iAddGreenTime)
{
	int iIndex       = 0;
	int iMinGreen    = 0;
	int iAddGreen    = 200;
	int iMaxGreen    = 0;
	int iTmpMinGreen = 0;
	int iTmpAddGreen = 0;
	int iTmpMaxGreen = 0;
	unsigned int uiAllowPhase = m_pRunData->sStageStepInfo[iStepNo].uiAllowPhase;
	//unsigned int uiOverlapPhase = m_pRunData->sStageStepInfo[iStepNo].uiOverlapPhase;

	while ( iIndex < 32 )
	{
		if ( /*(*/ (uiAllowPhase >> iIndex) & 1 /*) || ( (uiOverlapPhase >> iIndex) & 1 )*/ )
		{
			//if ( m_pTscConfig->sPhase[iIndex].ucType>>5 & 1  )  /*弹性相位*/
			//{
				iTmpMinGreen = m_pTscConfig->sPhase[iIndex].ucMinGreen;
				iTmpAddGreen = m_pTscConfig->sPhase[iIndex].ucGreenDelayUnit;
				iTmpMaxGreen = m_pTscConfig->sPhase[iIndex].ucMaxGreen1;
			//}
			/*
			else if ( m_pTscConfig->sPhase[iIndex].ucType>>6 & 1  ) //待定相位
			{
				iTmpMinGreen = m_pTscConfig->sPhase[iIndex].ucFixedGreen;
			}
			*/
			/*
			else if ( m_pTscConfig->sPhase[iIndex].ucType>>7 & 1 )  //固定相位 
			{
				iTmpMinGreen = m_pRunData->sScheduleTime[StepToStage(m_pRunData->ucStepNo,NULL)].ucGreenTime 
					               - m_pTscConfig->sPhase[iIndex].ucGreenFlash;      //green
			}
			else
			{
				continue;
			}
			*/

			iMinGreen = iTmpMinGreen < iMinGreen    ? iMinGreen    : iTmpMinGreen; //找大
			if ( iTmpAddGreen > 0 )
			{
				iAddGreen = iAddGreen    < iTmpAddGreen ? iAddGreen    : iTmpAddGreen; //找小
			}
			iMaxGreen = iMaxGreen    < iTmpMaxGreen ? iTmpMaxGreen : iMaxGreen;    //找大
		} 
		iIndex++;
	}
	
	if ( iMaxGreenTime != NULL )
	{
		*iMaxGreenTime = iMaxGreen;
	}
	if ( iAddGreenTime != NULL )
	{
		*iAddGreenTime = iAddGreen;
	}

	return iMinGreen;
}


/*********************************************************************************
Function:       CManaKernel::AddRunTime
Description:    感应控制，增加绿灯运行时间
Input:          iAddTime     - 相位组各个相位增加的最小时间
				ucPhaseIndex - 有车的相位index 0 - 15
Output:         无
Return:         无
**********************************************************************************/
void CManaKernel::AddRunTime(int iAddTime,Byte ucPhaseIndex)
{
	//int iStepRunTime = 0;
	
	//ACE_DEBUG((LM_DEBUG,"%s %d  the Tbl_Phase[%d] Phase has car \n",__FILE__,__LINE__,ucPhaseIndex));//ADD?20130709 912
	if ( ucPhaseIndex >= MAX_PHASE )
		return;
	int iLeftRunTime = m_pRunData->ucStepTime-m_pRunData->ucElapseTime ;
	int iStepUnitTime = m_pTscConfig->sPhase[ucPhaseIndex].ucGreenDelayUnit ;
	int iPhaseFlashTime = m_pTscConfig->sPhase[ucPhaseIndex].ucGreenFlash ;   
	
	
	if( iLeftRunTime <=CNTDOWN_TIME)
	{
		//ACE_DEBUG((LM_DEBUG,"%s:%d cntdown time<=8!\n",__FILE__,__LINE__));
		m_pRunData->b8cndtown = false ;
		return ;		
	}
	
	#ifdef SPECIAL_CNTDOWN
	{
		if((m_pRunData->ucElapseTime > m_pRunData->ucStepTime-CNTDOWN_TIME-iStepUnitTime) && (m_pRunData->ucElapseTime <= m_pRunData->ucStepTime-CNTDOWN_TIME))
	#else	
		if((m_pRunData->ucElapseTime >= m_pRunData->ucStepTime-iStepUnitTime) && (m_pRunData->ucElapseTime <= m_pRunData->ucStepTime-1))
	
	#endif
	{		
		m_pRunData->ucStepTime = m_pRunData->ucStepTime+iStepUnitTime ;  
		//ACE_DEBUG((LM_DEBUG,"%s:%d Add steptime!\n",__FILE__,__LINE__));
	}
	else
	{		
		return ;
	}
  }
	iCntFlashTime = iPhaseFlashTime ;
	if(m_pRunData->ucStepTime >m_iMaxStepTime)
		m_pRunData->ucStepTime = m_iMaxStepTime ;	
	
	/*
	if ( m_bAddTimeCount)
	{
		//ACE_DEBUG((LM_DEBUG,"%d do count down start************************\n",__LINE__));
		CwpmGetCntDownSecStep();
		//ACE_DEBUG((LM_DEBUG,"%d do count down end**************************\n",__LINE__));
		m_bAddTimeCount = false;  //避免发送重复的倒计时信息
	}
	*/
	/*
	
	m_pRunData->ucStepTime = iStepRunTime < m_iMinStepTime ? m_iMinStepTime : iStepRunTime;

	if ( m_pRunData->ucStepTime > m_iMaxStepTime )
	{
		m_pRunData->ucStepTime = m_iMaxStepTime;
	}
	*/	

	//ACE_DEBUG((LM_DEBUG,"%s:%d:after ucElapseTime:%d,ucStepTime:%d m_iMaxStepTime:%d\n\n",__FILE__,__LINE__
	//	,m_pRunData->ucElapseTime,m_pRunData->ucStepTime,m_iMaxStepTime));
}


/*********************************************************************************
Function:       CManaKernel::GetStageDetector
Description:    感应控制，获取阶段的对应线圈 去除在后阶段还会出现的相位,依据相同的跟
				随相位去除前一个阶段的的相位
Input:          iStageNo     - 阶段号
Output:         无
Return:         无
**********************************************************************************/
void CManaKernel::GetStageDetector(int iStageNo)
{
	Byte ucDelPhase          = 0;
	Byte ucOverlapPhaseIndex = 0;
	Byte ucPhaseIndex        = 0;
	int iNextStage           = iStageNo + 1;
	int iNextStep            = 0;
	int iCurStepNo           = StageToStep(iStageNo);
	Uint iCurAllowPhase      = m_pRunData->sStageStepInfo[iCurStepNo].uiAllowPhase;
	int iNextAllowPhase      = 0;
	Uint iCurOverlapPhase    = m_pRunData->sStageStepInfo[iCurStepNo].uiOverlapPhase;
	Uint iNextOverlapPhase   = 0;
	unsigned int uiTmp       = 0;
 
	if ( iNextStage > m_pRunData->ucStageCount )
	{
		iNextStage = 0;
	}

	iNextStep         = StageToStep(iNextStage);
	iNextAllowPhase   = m_pRunData->sStageStepInfo[iNextStep].uiAllowPhase;
	iNextOverlapPhase = m_pRunData->sStageStepInfo[iNextStep].uiOverlapPhase;
	
	//普通相位
	while ( ucPhaseIndex < MAX_PHASE )
	{
		if ( (iCurAllowPhase>>ucPhaseIndex & 1) 
			&& (iNextAllowPhase>>ucPhaseIndex & 1) )  //下一个阶段也存在该相位，去除该相位
		{
			uiTmp = 0;
			uiTmp |= ~(1<<ucPhaseIndex);
			iCurAllowPhase  = iCurAllowPhase & uiTmp;
		}
		ucPhaseIndex++;
	}

	//跟随相位
	while ( ucOverlapPhaseIndex < MAX_OVERLAP_PHASE )
	{
		if ( (iCurOverlapPhase>>ucOverlapPhaseIndex & 1) 
		  && (iNextOverlapPhase>>ucOverlapPhaseIndex & 1) )  //下一个阶段也存在该相位，去除该相位
		{
			ucDelPhase = OverlapPhaseToPhase(iCurAllowPhase,ucOverlapPhaseIndex);  //根据跟随相位获取放行相位的下标
			if ( ucDelPhase < MAX_PHASE )
			{
				uiTmp = 0;
				uiTmp |= ~(1<<ucDelPhase);
				iCurAllowPhase  = iCurAllowPhase & uiTmp;
			}
		}

		ucOverlapPhaseIndex++;
	}

	m_uiStagePhase[iStageNo] |= iCurAllowPhase;
}


/*********************************************************************************
Function:       CManaKernel::GetAllStageDetector
Description:    感应控制，获取所有阶段的对应线圈，一个周期一次
Input:          iStageNo     - 阶段号
Output:         无
Return:         无
**********************************************************************************/
void CManaKernel::GetAllStageDetector()
{
	int iIndex = 0;

	ACE_OS::memset(m_uiStagePhase,0,MAX_SON_SCHEDULE*sizeof(unsigned int));

	while ( iIndex < m_pRunData->ucStageCount )
	{
		GetStageDetector(iIndex);
		iIndex++;
	}
}


/*********************************************************************************
Function:       CManaKernel::GetMaxStageMinGreen
Description:    获取某步伐各个相位的最大最小绿，用于阶段切换时保证走完最小绿
Input:          iStepNo - 步伐号
Output:         无
Return:         无
**********************************************************************************/
int CManaKernel::GetMaxStageMinGreen(int iStepNo)
{
	int iIndex       = 0;
	int iMinGreen    = 0;
	int iTmpMinGreen = 0;
	unsigned int uiAllowPhase = m_pRunData->sStageStepInfo[iStepNo].uiAllowPhase;
	unsigned int uiOverlapPhase = m_pRunData->sStageStepInfo[iStepNo].uiOverlapPhase;

	while ( iIndex < 32 )
	{
		if ( ( (uiAllowPhase >> iIndex) & 1 ) || ( (uiOverlapPhase >> iIndex) & 1 ) )
		{
			iTmpMinGreen = m_pTscConfig->sPhase[iIndex].ucMinGreen;	
			iMinGreen = iTmpMinGreen < iMinGreen ? iMinGreen : iTmpMinGreen; 
		} 
		iIndex++;
	}

	return iMinGreen;
}


/*********************************************************************************
Function:       CManaKernel::SpecTimePattern
Description:    指定特定的配时方案号
Input:          iTimePatternId - 指定的配时方案号
Output:         无
Return:         无
**********************************************************************************/
void CManaKernel::SpecTimePattern(int iTimePatternId )
{
	if ( iTimePatternId != m_pRunData->ucTimePatternId )   //下个周期会采用此方案号
	{
		SetUpdateBit();
	}
	m_iTimePatternId = iTimePatternId;

	ACE_DEBUG((LM_DEBUG,"%s:%d \n@@@@@@@@@@%d\n",__FILE__,__LINE__,m_iTimePatternId));
}


/*********************************************************************************
Function:       CManaKernel::GetVehilePara
Description:    获取允许感应控制起作用的值
Input:          无
Output:         pbVehile     - 是否为感应控制        bDefStep    - 目前切入新的步伐 
*       	    piAdjustTime - 每次的调整时间        piCurPhase  - 当前允许执行的相位组 
*       	    piNextPhase  - 下个允许执行的相位组  psPhaseDet  - 相位与检测器的关系  
Return:         无
**********************************************************************************/
void CManaKernel::GetVehilePara(bool* pbVehile
									 , bool* bDefStep
									 , int* piAdjustTime 
									 , Uint* piCurPhase 
									 , Uint* piNextPhase
									 , SPhaseDetector* psPhaseDet)
{
	Byte ucNextStage        = 0;
	Uint ucNextPhaseTmp     = 0;
	int  iIndex             = 0;
	static Byte ucCurStepNo = 222;

	//ACE_DEBUG((LM_DEBUG,"\n %s:%d m_bVehile:%d ucCurStepNo:%d \n" ,__FILE__,__LINE__,m_bVehile,ucCurStepNo));

	*pbVehile     = m_bVehile;
	if ( !m_bVehile )
	{
		return;
	}

	if ( ucCurStepNo == m_pRunData->ucStepNo )
	{
		return;
	}
	else
	{
		*bDefStep = true;
		ucCurStepNo = m_pRunData->ucStepNo;
	}

	//*pbVehile     = m_bVehile;
	*piAdjustTime = m_iAdjustTime;
	*piCurPhase   = m_uiStagePhase[m_pRunData->ucStageNo];
	ACE_OS::memcpy(psPhaseDet , m_sPhaseDet , MAX_PHASE*sizeof(SPhaseDetector) );

	if ( CTRL_VEHACTUATED == m_pRunData->uiCtrl )
	{
		return;
	}
	
	/*获取下一个相位步，只在半感应控制用到*/
	iIndex = m_pRunData->ucStepNo + 1;  //寻找下一个相位步
	while ( iIndex < m_pRunData->ucStepNum )
	{
		if ( IsLongStep(iIndex) )
		{
			ucNextStage    = StepToStage(iIndex,NULL);
			ucNextPhaseTmp = m_uiStagePhase[ucNextStage];
			if ( IsMainPhaseGrp(*piCurPhase) != IsMainPhaseGrp(ucNextPhaseTmp) ) //主 次 车道相结合
			{
				*piNextPhase = ucNextPhaseTmp;
				//ACE_DEBUG((LM_DEBUG,"\n %s:%d *piCurPhase:%x *piNextPhase:%x \n" ,__FILE__,__LINE__,*piCurPhase ,*piNextPhase ));
				return;
			}
		}
		iIndex++;
	}
	
	*piNextPhase = m_uiStagePhase[0];	

	//ACE_DEBUG((LM_DEBUG,"\n %s:%d *piCurPhase:%x *piNextPhase:%x \n" ,__FILE__,__LINE__,*piCurPhase ,*piNextPhase ));
}


/*********************************************************************************
Function:       CManaKernel::GetVehilePara
Description:    判断某个相位组是否为主相位组，即主车道相位，用于半感应控制
Input:          uiPhase -相位组合序列
Output:         无
Return:         true - 主车道 false - 非主车道
**********************************************************************************/
bool CManaKernel::IsMainPhaseGrp(Uint uiPhase)
{
	int iIndex    = 0;
	int iDetCnt   = 0;
	int iDetIndex = 0;
	int iDetId    = 0;

	while ( iIndex < MAX_PHASE )
	{
		if ( uiPhase>>iIndex & 1 )
		{
			iDetCnt = m_sPhaseDet[iIndex].iRoadwayCnt;

			iDetIndex = 0;
			while ( iDetIndex < iDetCnt )
			{
				iDetId = m_sPhaseDet[iIndex].iDetectorId[iDetIndex] - 1;
				if ( (m_pTscConfig->sDetector[iDetId].ucOptionFlag>>1) & 1 )  //该通道设置为关键相位
				{
					return true;
				}
				iDetIndex++;
			}
		}

		iIndex++;
	}

	return false;
}


/*********************************************************************************
Function:       CManaKernel::IsInChannel
Description:    判断该通道是否被启用
Input:          iChannelId : 通道号
Output:         无
Return:         true - 启用 false - 未启用
**********************************************************************************/
bool CManaKernel::IsInChannel(int iChannelId)
{
	int iIndex = 0;

	while ( iIndex < MAX_CHANNEL )
	{
		if ( ( iChannelId == m_pTscConfig->sChannel[iIndex].ucId ) 
			 && (m_pTscConfig->sChannel[iIndex].ucSourcePhase != 0) )
		{
			return true;
		}
		iIndex++;
	}
	return false;
}


/*********************************************************************************
Function:       CManaKernel::InConflictPhase
Description:    判断相位是否冲突，即出现绿冲突
Input:          无
Output:         无
Return:         true - 冲突 false - 未冲突
**********************************************************************************/
bool CManaKernel::InConflictPhase()
{
	int  iIndex = 0;
	int  iCurStepNo   = m_pRunData->ucStepNo;
	Uint uiAllowPhase = m_pRunData->sStageStepInfo[iCurStepNo].uiAllowPhase;
	Uint uiCollision  = 0;

	GBT_DB::Collision* pConflictPhase = m_pTscConfig->sPhaseConflict;

	while ( iIndex < MAX_PHASE )
	{
		if ( (uiAllowPhase >> iIndex) & 1 )  //存在该相位
		{
			uiCollision = (pConflictPhase+iIndex)->usCollisionFlag;
			if ( (uiAllowPhase & uiCollision) != 0 )
			{
				return true;
			}
		}
		iIndex++;
	}
	return false;
}


/*********************************************************************************
Function:       CManaKernel::DealGreenConflict
Description:    处理出现的绿冲突
Input:          ucdata   - 参数值
Output:         无
Return:         无
**********************************************************************************/
void CManaKernel::DealGreenConflict(Byte ucdata)
{
	if ( m_pTscConfig->sSpecFun[FUN_SERIOUS_FLASH].ucValue != 0 || 1)
	{
		CLampBoard::CreateInstance()->SetSeriousFlash(true);
		//CFlashMac::CreateInstance()->SetHardwareFlash(true);

		SThreadMsg sTscMsg;
		sTscMsg.ulType       = TSC_MSG_SWITCH_CTRL;  
		sTscMsg.ucMsgOpt     = 0;
		sTscMsg.uiMsgDataLen = 1;
		sTscMsg.pDataBuf     = ACE_OS::malloc(1);
		*((Byte*)sTscMsg.pDataBuf) = CTRL_MANUAL;  //手动
		CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));

		SThreadMsg sTscMsgSts;
		sTscMsgSts.ulType       = TSC_MSG_SWITCH_STATUS;  
		sTscMsgSts.ucMsgOpt     = 0;
		sTscMsgSts.uiMsgDataLen = 1;
		sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
		*((Byte*)sTscMsgSts.pDataBuf) = FLASH;  //黄闪
		CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
	}
	
}


/*********************************************************************************
Function:       CManaKernel::AllotActiveTime
Description:    自适应控制根据各个检测器上个周期的占有率，分配绿灯时间
Input:          无
Output:         无
Return:         无
**********************************************************************************/
void CManaKernel::AllotActiveTime()
{
	int iIndex      = 0;
	int jIndex      = 0;
	int zIndex      = 0;
	int iStagePhase = 0;
	int iStepNo     = 0; 
	int iDetSum     = 0;
	unsigned int iDetId       = 0;
	unsigned long iRecordSum  = 0;
	int iGreenTimeSum  = 0;
	int iStepGreenTime = 0;
	unsigned long ulTotalHaveCarTime = 0; 
	unsigned int iStageHaveCarTime[MAX_SON_SCHEDULE] = {0};   //各个阶段在配置的检测器的有车时间
	GBT_DB::VehicleStat*   pVehicleStat = NULL;
	GBT_DB::TblVehicleStat tblVehicleStat;

	(CDbInstance::m_cGbtTscDb).QueryVehicleStat(tblVehicleStat);
	pVehicleStat = tblVehicleStat.GetData(iRecordSum);

	if ( iRecordSum <= 0 )
	{
		return;
	}

	while ( iIndex < m_pRunData->ucStageCount )
	{
		iStepNo     = StageToStep(iIndex);
		iStagePhase = 
				m_pRunData->sStageStepInfo[iStepNo].uiAllowPhase 
				| (m_pRunData->sStageStepInfo[iStepNo].uiOverlapPhase<<MAX_PHASE);

		jIndex = 0;
		while ( jIndex < (MAX_PHASE+MAX_OVERLAP_PHASE) )
		{
			if ( (iStagePhase>>jIndex) & 1 )
			{
				iDetSum = m_sPhaseDet[jIndex].iRoadwayCnt;
				zIndex  = 0;
				//while ( zIndex < iDetSum )
				//{
					iDetId = m_sPhaseDet[jIndex].iDetectorId[zIndex];
					if (  iDetId < iRecordSum )
					{
						iStageHaveCarTime[iIndex] += (pVehicleStat+iDetId-1)->ucOccupancy;     //获取占有率
					}
					//zIndex++;
				//}
				 
			}
			jIndex++;
		}
		
		iGreenTimeSum      += m_pRunData->sStageStepInfo[iStepNo].ucStepLen;   //绿灯总运行时间
		ulTotalHaveCarTime += iStageHaveCarTime[iIndex];       //总有车时间      
		iStagePhase = 0;
		iIndex++;
	}

	iIndex = 0;
	while ( iIndex < m_pRunData->ucStageCount )
	{
		iStepNo        = StageToStep(iIndex);
		iStepGreenTime = iGreenTimeSum * iStageHaveCarTime[iIndex] / ulTotalHaveCarTime + 1 ;

		if ( iStepGreenTime < MIN_GREEN_TIME )
		{
			iStepGreenTime = MIN_GREEN_TIME;
		}

		m_pRunData->sStageStepInfo[iStepNo].ucStepLen = iStepGreenTime;  //修改阶段的绿灯时间
		iIndex++;
	}
}


/*********************************************************************************
Function:       CManaKernel::UtcsAdjustCycle
Description:    协调控制，根据中心提供的公共周期时长进行周期调整，多减少补
Input:          无
Output:         无
Return:         无
**********************************************************************************/
void CManaKernel::UtcsAdjustCycle()
{
	bool bPlus         = false;
	int iAdjustCnt     = 0;
	int iMinGreen      = 0;
	int iMaxGreen      = 0;
	int iAdjustPerStep = 0;
	Byte ucStageCnt    = m_pRunData->ucStageCount;
	Byte ucAdjustGreenLen[MAX_STEP] = {0};

	if ( m_pRunData->ucCycle > m_ucUtcsComCycle )  //-
	{
		bPlus      = false;
		iAdjustCnt = m_pRunData->ucCycle - m_ucUtcsComCycle;
	}
	else if ( m_pRunData->ucCycle < m_ucUtcsComCycle )  //+
	{
		bPlus      = true;
		iAdjustCnt = m_ucUtcsComCycle - m_pRunData->ucCycle;
	}
	else
	{
		return;
	}

	//平分调整周期
	for ( int i = 0; i < MAX_STEP && i < m_pRunData->ucStepNum; i++ ) 
	{
		if ( !IsLongStep(i) )
		{
			continue;
		}

		ucAdjustGreenLen[i] = iAdjustCnt / ucStageCnt;
	}
	iAdjustCnt -= ucStageCnt * ( iAdjustCnt / ucStageCnt );

	//调整约束
	for ( int i = 0; i < MAX_STEP && i < m_pRunData->ucStepNum; i++ ) 
	{
		if ( !IsLongStep(i) )
		{
			continue;
		}

		iMinGreen = GetCurStepMinGreen(i , &iMaxGreen , NULL);

		if ( bPlus )
		{
			iAdjustPerStep = iMaxGreen - m_pRunData->sStageStepInfo[i].ucStepLen;	
		}
		else
		{
			iAdjustPerStep = m_pRunData->sStageStepInfo[i].ucStepLen - iMinGreen;
		}

		if ( iAdjustPerStep < 0 )  //预防最小绿或最大绿的设置错误
		{
			iAdjustPerStep = m_pRunData->sStageStepInfo[i].ucStepLen / MAX_ADJUST_CYCLE;
		}

		if ( ucAdjustGreenLen[i] > iAdjustPerStep )  //调整范围过大
		{
			iAdjustCnt         += ( ucAdjustGreenLen[i] - iAdjustPerStep );
			ucAdjustGreenLen[i] = iAdjustPerStep;
		}
		else
		{
			iAdjustPerStep = iAdjustPerStep - ucAdjustGreenLen[i];
		}

		if ( iAdjustPerStep > iAdjustCnt )  //调整完毕
		{
			iAdjustPerStep = iAdjustCnt;
		}

		if ( bPlus ) 
		{
			m_pRunData->sStageStepInfo[i].ucStepLen += ( ucAdjustGreenLen[i] + iAdjustPerStep );
		}
		else
		{
			m_pRunData->sStageStepInfo[i].ucStepLen -= ( ucAdjustGreenLen[i] + iAdjustPerStep );
		}
		iAdjustCnt -= iAdjustPerStep;

		ACE_DEBUG((LM_DEBUG,"After cooridate m_pRunData->sStageStepInfo[%d].ucStepLen:%d\n",i,m_pRunData->sStageStepInfo[i].ucStepLen));
	}

	m_pRunData->ucCycle = m_ucUtcsComCycle;
}


/*********************************************************************************
Function:       CManaKernel::GetUseLampBoard
Description:    根据通道ID计算启用的灯控板
Input:          无
Output:         bUseLampBoard: true-启用 false-没有启用
Return:         无
**********************************************************************************/
void CManaKernel::GetUseLampBoard(bool* bUseLampBoard)
{
	int iIndex = 0;

	if ( NULL == bUseLampBoard )
	{
		return;
	}

	while ( iIndex < MAX_LAMP_BOARD )
	{
		if ( GetUseLampBoard(iIndex) )  //该灯控板有启用的通道
		{
			*(bUseLampBoard+iIndex) = true;
		}
		iIndex++;
	}
}


/*********************************************************************************
Function:       CManaKernel::GetUseLampBoard
Description:    判断灯控板是否存在配置信息
Input:          iLampBoard - 灯控板 0 , 1 , 2 , 3
Output:         无
Return:         无
**********************************************************************************/
bool CManaKernel::GetUseLampBoard(int iLampBoard)
{
	int iChannel = 0;
	iChannel = iLampBoard * 4 + 1;
	for ( iChannel=iLampBoard * 4 + 1; iChannel<(iLampBoard+1) * 4 + 1; iChannel++ )
	{
		if ( IsInChannel(iChannel)  )
		{
			return true;
		}
	}

	return false;
}


/*********************************************************************************
Function:       CManaKernel::InPhaseDetector
Description:    判断检测器是否在当前的配置
Input:          ucDetId -检测器ID号
Output:         无
Return:         true - 在当前配置  false -不在当前配置
**********************************************************************************/
bool CManaKernel::InPhaseDetector(Byte ucDetId)
{
	Byte ucPhaseIndex = 0;
	Byte ucDetIndex   = 0;
	int  iRoadwayCnt  = 0;

	while ( ucPhaseIndex < MAX_PHASE )
	{
		ucDetIndex  = 0;
		iRoadwayCnt = m_sPhaseDet[ucPhaseIndex].iRoadwayCnt;
		while ( ucDetIndex < iRoadwayCnt)
		{
			if ( ucDetId == m_sPhaseDet[ucPhaseIndex].iDetectorId[ucDetIndex] )
			{
				return true;
			}
			ucDetIndex++;
		}
		ucPhaseIndex++;
	}

	return false;
}

bool CManaKernel::IsVehile()
     {
		return m_bVehile ;
    }

/**************************************************************
Function:        CManaKernel::SndMsgLog
Description:     添加日志消息函数，用于发送各类写日志消息				
Input:          ucLogType  日志类型
				ucLogVau1  日志值1
				ucLogVau2  日志值2
				ucLogVau3  日志值3
				ucLogVau4  日志值4              
Output:         无
Return:         无
***************************************************************/
void CManaKernel::SndMsgLog(Byte ucLogType,Byte ucLogVau1,Byte ucLogVau2,Byte ucLogVau3,Byte ucLogVau4)
{
	SThreadMsg sTscMsg;
	sTscMsg.ulType       = TSC_MSG_LOG_WRITE;
	sTscMsg.ucMsgOpt     = ucLogType;
	sTscMsg.uiMsgDataLen = 4;
	sTscMsg.pDataBuf     = ACE_OS::malloc(4);
	((Byte*)(sTscMsg.pDataBuf))[0] = ucLogVau1; 
	((Byte*)(sTscMsg.pDataBuf))[1] = ucLogVau2;	
	((Byte*)(sTscMsg.pDataBuf))[2] = ucLogVau3; 
	((Byte*)(sTscMsg.pDataBuf))[3] =ucLogVau4;														
	CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));
}

/**************************************************************
Function:        CManaKernel::ValidSoftWare
Description:    验证软件的合法性			
Input:          无    
Output:         无
Return:         无
***************************************************************/
void CManaKernel::ValidSoftWare()
	{
		char SerialNum1[32]={0};
		char SerialNum2[32]={0};
		
		(CDbInstance::m_cGbtTscDb).GetEypSerial(SerialNum1);
		GetSysEnyDevId(SerialNum2);

		//ACE_DEBUG((LM_DEBUG,"%s:%d SerialNum1:%s SerialNum2:%s\n",__FILE__,__LINE__,SerialNum1,SerialNum2));
		if(ACE_OS::strcmp(SerialNum1,SerialNum2)==0)
			bValidSoftWare = true ;
		else
			bValidSoftWare = false ;
	}
