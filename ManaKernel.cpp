/***********************************************************************************
Copyright(c) 2013  AITON. All rights reserved.
Author:     AITON
FileName:   ManaKernel.cpp
Date:       2013-1-1
Description:�źŻ�����ҵ�������ļ�
Version:    V1.0
History:    201308010930  ���48�����ݼ����������ݿ��ͱ���
			201309251030 �޸���λ��ͻ���λ�ã�����һ�����Ƶ���һ�������м��
			201309251120 �����־��Ϣ���ͺ���������ͬ����־��Ϣͳһ��һ����������
			201309251030 �޸���λ��ͻ���λ�ã�����һ�����Ƶ���һ�������м��
			201309281025 Add "ComFunc.h" headfile
		    201401101130 �޸Ľ׶β�����ȡ��ʱ���Ӧ�������̵���С�̣���Ϊȥ������λ��С�̴���
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
�źŵ���ɫ����ö��
*/
enum
{
	LAMP_COLOR_RED = 0,  //��ɫ��
	LAMP_COLOR_YELLOW ,  //��ɫ��
	LAMP_COLOR_GREEN     //��ɫ��
};

/**************************************************************
Function:       CManaKernel::CManaKernel
Description:    �źŻ�����CManaKernel�๹�캯������ʼ�����ı���
Input:          ucMaxTick                 
Output:         ��
Return:         ��
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

	m_pTscConfig = new STscConfig;      //�źŻ�������Ϣ
	m_pRunData   = new STscRunData;     //�źŻ���̬������Ϣ
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
Description:    CManaKernel�������������ͷ������
Input:          ��               
Output:         ��
Return:         ��
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
Description:    ����CManaKernel�ྲ̬����		
Input:          ��              
Output:         ��
Return:         CManaKernel��̬����ָ��
***************************************************************/
CManaKernel* CManaKernel::CreateInstance()
{
	static CManaKernel cWorkParaManager; 

	return &cWorkParaManager;
}


/**************************************************************
Function:       CManaKernel::InitWorkPara
Description:    ��ʼ��CManaKernel�����ಿ�ֹ������������ݿ�	
Input:          ��              
Output:         ��
Return:         ��
***************************************************************/
void CManaKernel::InitWorkPara()
{	
	ACE_DEBUG((LM_DEBUG,"%s:%d ϵͳ����,��ʼ���źŻ�����! \n",__FILE__,__LINE__)); //ADD: 20130523 1053	
	// ����Ĭ�ϵ�����
	SignalDefaultData sdd;
	sdd.InsertGbDefaultData();//����Ĭ��ֵ�����ݿ� ������ݿ��Ϊ��
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
	//ValidSoftWare();  //ADD: 201310221530 ��֤����ĺϷ���
}




/**************************************************************
Function:       CManaKernel::SelectDataFromDb
Description:    ��Sqlite���ݿ��ȡ�źż�ȫ����������	
Input:          ��              
Output:         ��
Return:         ��
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
	
	(CDbInstance::m_cGbtTscDb).QueryDetPara(sDetPara);               //������ṹ�����,��tbl_system tbl_constant ���в�ѯ
	CDetector::CreateInstance()->SetStatCycle(sDetPara.ucDataCycle);  //���ݲɼ���������

	ACE_OS::memset(m_pTscConfig , 0 , sizeof(STscConfig) ); //�źŻ���̬���в���ȫ����ʼ��Ϊ0
	
	
	/*************��λ�뷽�����ñ�****************/  //ADD:201310181706
	(CDbInstance::m_cGbtTscDb).QueryPhaseToDirec(tblPhaseToDirec);
	pPhaseToDirec = tblPhaseToDirec.GetData(ucCount);
	iIndex = 0;
	while(iIndex<ucCount)
	{
		ACE_OS::memcpy(m_pTscConfig->sPhaseToDirec+iIndex,pPhaseToDirec,sizeof(GBT_DB::PhaseToDirec));
		pPhaseToDirec++;
		iIndex++;
	}

	/*************ͨ�����ݼ�����ñ��****************/  //ADD:20130801 15 35
	(CDbInstance::m_cGbtTscDb).QueryChannelChk(tblChannelChk);
	pChannelChk = tblChannelChk.GetData(ucCount);
	iIndex = 0;
	while(iIndex<ucCount)
	{
		ACE_OS::memcpy(m_pTscConfig->sChannelChk+iIndex,pChannelChk,sizeof(GBT_DB::ChannelChk));
		pChannelChk++;
		iIndex++;

	}

    /*************���ȼƻ���****************/
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


	/*************һ���ʱ�α��ȡ****************/
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

	/*************��ʱ���������****************/
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

	

	/************�׶���ʱ��*************/
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

	/********************��λ��*****************/
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

	/********************������λ��****************/
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

	/********************��ͻ��λ��****************/
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
	
	/********************ͨ����*******************/
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
	

	/**********************��Ԫ���� �������� ȫ��ʱ��*******************/
	(CDbInstance::m_cGbtTscDb).GetUnitCtrlPara(ucStartFlash, ucStartAllRed, ucRemoteCtrlFlag, ucFlashFreq);
	m_pTscConfig->sUnit.ucStartFlashTime  = ucStartFlash;
	m_pTscConfig->sUnit.ucStartAllRedTime = ucStartAllRed;
	
	/**********************����ģʽ ������׼����*******************/
	(CDbInstance::m_cGbtTscDb).GetDegradeCfg(m_pTscConfig->DegradeMode, m_pTscConfig->DegradePattern);

	/**********************��������ʱ�� Э����λ��********************/
	if ( false == (CDbInstance::m_cGbtTscDb).GetGlobalCycle(m_ucUtcsComCycle) )
	{
		m_ucUtcsComCycle = 0; 
	}
	if ( false == (CDbInstance::m_cGbtTscDb).GetCoorPhaseOffset(m_ucUtscOffset) )
	{
		m_ucUtscOffset   = 0;
	}

	
	/*********************���������***************************/
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
			//CDetector::CreateInstance()->m_ucDetError[pDetector->ucDetectorId-1] = DET_NORMAL ; //�����ڼ����ʱ��Ĭ�Ϲ���״̬����
		}
		
		pDetector++;
		iIndex++;
	}
	//ACE_OS::memset(m_pTscConfig->sDetector+iIndex , 0 , sizeof(GBT_DB::Detector));
	SetDetectorPhase();  //���������λ��ϵ
	
	/*********************���⹦�ܿ�����***********************/ //�����źŻ����⹦�ܳ�ʼ������
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
	
	/********************��ȡ���õĵƿذ����********************/
	//GetUseLampBoard(bLampBoardExit);
	//CLamp::CreateInstance()->GetLampBoardExit(bLampBoardExit);

	/********************��ȡģ��� ���������������***********/
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
	
	 
	/********************��ȡ�������չ��**********************/
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

	//CAdaptive::CreateInstance()->SynPara();  //����Ӧ���Ʋ���

	if ( m_pTscConfig->sSpecFun[FUN_COUNT_DOWN].ucValue != 0 )  //����ʱ�豸����
	{
		#ifdef GA_COUNT_DOWN
		CGaCountDown::CreateInstance()->GaUpdateCntDownCfg();
		#endif
	}


	
}


/**************************************************************
Function:       CManaKernel::SetDetectorPhase
Description:    ���ü��������λ�Ķ�Ӧ��ϵ	
Input:          ��              
Output:         ��
Return:         ��
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
Description:    ��û�����ݿ������£�ģ���źŻ��ľ�̬���ò���	
Input:          ��              
Output:         ��
Return:         ��
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
	m_pTscConfig->sScheduleTime[0][0].usAllowPhase = 0x50;  //�ϱ���ת
	m_pTscConfig->sScheduleTime[0][0].ucGreenTime  =  13;
	m_pTscConfig->sScheduleTime[0][0].ucYellowTime = 3;
	m_pTscConfig->sScheduleTime[0][0].ucRedTime    = 3;
	m_pTscConfig->sScheduleTime[0][1].ucId = 1;
	m_pTscConfig->sScheduleTime[0][1].ucScheduleId = 2;
	m_pTscConfig->sScheduleTime[0][1].usAllowPhase = 0x5; //�ϱ�ֱ��(�����ϱ���ת������)
	m_pTscConfig->sScheduleTime[0][1].ucGreenTime  =  17;
	m_pTscConfig->sScheduleTime[0][1].ucYellowTime = 3;
	m_pTscConfig->sScheduleTime[0][1].ucRedTime    = 3;
	
	m_pTscConfig->sScheduleTime[0][2].ucId = 1;
	m_pTscConfig->sScheduleTime[0][2].ucScheduleId = 3;
	m_pTscConfig->sScheduleTime[0][2].usAllowPhase = 0xA0;  //������ת
	m_pTscConfig->sScheduleTime[0][2].ucGreenTime  =  13;
	m_pTscConfig->sScheduleTime[0][2].ucYellowTime = 3;
	m_pTscConfig->sScheduleTime[0][2].ucRedTime    = 3;
	m_pTscConfig->sScheduleTime[0][3].ucId = 1;
	m_pTscConfig->sScheduleTime[0][3].ucScheduleId = 4;
	m_pTscConfig->sScheduleTime[0][3].usAllowPhase = 0xA; //����ֱ��(���涫����ת������)
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

	m_pTscConfig->sPhase[0].ucId         = 1;  //�� ֱ��
	m_pTscConfig->sPhase[0].ucPedestrianGreen  = 0;
	m_pTscConfig->sPhase[0].ucFixedGreen = 12;
	m_pTscConfig->sPhase[0].ucType       = 0x80;
	m_pTscConfig->sPhase[0].ucGreenFlash = 3;
	
	m_pTscConfig->sPhase[1].ucId = 2;  //�� ֱ��
	m_pTscConfig->sPhase[1].ucPedestrianGreen  = 0;
	m_pTscConfig->sPhase[1].ucFixedGreen = 12;
	m_pTscConfig->sPhase[1].ucType       = 0x80;
	m_pTscConfig->sPhase[1].ucGreenFlash = 3;
	
	m_pTscConfig->sPhase[2].ucId = 3; //�� ֱ��
	m_pTscConfig->sPhase[2].ucPedestrianGreen  = 0;
	m_pTscConfig->sPhase[2].ucFixedGreen = 10;
	m_pTscConfig->sPhase[2].ucType       = 0x80;
	m_pTscConfig->sPhase[2].ucGreenFlash = 3;

	m_pTscConfig->sPhase[3].ucId = 4;  //�� ֱ��
	m_pTscConfig->sPhase[3].ucPedestrianGreen  = 0;
	m_pTscConfig->sPhase[3].ucFixedGreen = 12;
	m_pTscConfig->sPhase[3].ucType       = 0x80;
	m_pTscConfig->sPhase[3].ucGreenFlash = 3;

	m_pTscConfig->sPhase[4].ucId = 5;  //�� ��ת
	m_pTscConfig->sPhase[4].ucPedestrianGreen  = 0;
	m_pTscConfig->sPhase[4].ucFixedGreen = 12;
	m_pTscConfig->sPhase[4].ucType       = 0x80;
	m_pTscConfig->sPhase[4].ucGreenFlash = 3;

	m_pTscConfig->sPhase[5].ucId = 6; //�� ��ת
	m_pTscConfig->sPhase[5].ucPedestrianGreen  = 0;
	m_pTscConfig->sPhase[5].ucFixedGreen = 12;
	m_pTscConfig->sPhase[5].ucType       = 0x80;
	m_pTscConfig->sPhase[5].ucGreenFlash = 3;

	m_pTscConfig->sPhase[6].ucId = 7;  //�� ��ת
	m_pTscConfig->sPhase[6].ucPedestrianGreen  = 0;
	m_pTscConfig->sPhase[6].ucFixedGreen = 12;
	m_pTscConfig->sPhase[6].ucType       = 0x80;
	m_pTscConfig->sPhase[6].ucGreenFlash = 3;

	m_pTscConfig->sPhase[7].ucId = 8;  //�� ��ת
	m_pTscConfig->sPhase[7].ucPedestrianGreen  = 0;
	m_pTscConfig->sPhase[7].ucFixedGreen = 12;
	m_pTscConfig->sPhase[7].ucType       = 0x80;
	m_pTscConfig->sPhase[7].ucGreenFlash = 3;

	for (int i=0; i<4; i++)  //��ת
	{ 
		m_pTscConfig->sPhase[8+i].ucId         = 9 + i;  
		m_pTscConfig->sPhase[8+i].ucPedestrianGreen  = 0;
		m_pTscConfig->sPhase[8+i].ucFixedGreen = 12;
		m_pTscConfig->sPhase[8+i].ucType       = 0x80;
		m_pTscConfig->sPhase[8+i].ucGreenFlash = 3;
	}

	for (int i=0; i<4; i++)  //����
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

	//��ͻ��λ�ݲ���
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
Description:    �����źŻ���̬���ò���,�����ݿ���ظ��������Ϣ��
				�ڴ���,�����ݿ�õ�����ʱ����һ�����ڽ�������øú���
Input:          ��              
Output:         ��
Return:         ��
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
Description:    1sִ��һ�Σ�����ʱ��++ ��Ҫ�������ж��Ƿ�����һ
				��ʱ�䣬�ǵĻ��ͷ���TSC��Ϣ���������ǵĻ��򽫵�ǰ��
				ʣ��ʱ�����һ��
Input:          ��              
Output:         ��
Return:         ��
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
			if(iCntFlashTime>0)            //����ʱ�串��
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

	if ( m_bWaitStandard )  //�źŻ������ڻ���-->ȫ��  �ֶ��ֽ���
	{
		if ( CTRL_PANEL == m_pRunData->uiCtrl )  //����ֶ�����
		{
			return;
		}
	}
	else if ( !m_bWaitStandard )
	{
		if ( CTRL_MANUAL == m_pRunData->uiCtrl )  //�ֶ�����
		{
			return;
		}

		if ( ( CTRL_PANEL == m_pRunData->uiCtrl ) && ( (SIGNALOFF == m_pRunData->uiWorkStatus ) || (ALLRED == m_pRunData->uiWorkStatus) 
			|| (FLASH == m_pRunData->uiWorkStatus ) ) )  //���صơ�ȫ�졢����
		{
			return;
		}

		if ( (CTRL_PANEL == m_pRunData->uiCtrl) && (0 == m_iTimePatternId || (250 == m_iTimePatternId)))  //�������ҷ�ָ����ʱ����
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
			|| ( CTRL_MANUAL == m_pRunData->uiWorkStatus ) )//�����жϵ�״̬ȡֵ����?
		{
			;   //
		}
		else if ( m_pRunData->ucElapseTime != m_pRunData->ucStepTime - 1 )  //���ⲽ�������1S�Լ���һ����ʼ�ĵ�1S�ظ�
		{
			;   //
		}
	}

}


/**************************************************************
Function:       CManaKernel::GoNextStep
Description:    һ���������꣬�л�����һ������
Input:          ��              
Output:         ��
Return:         ��
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
	//MOD:201309251030 �޸���λ��ͻ���λ�ã�����һ�����Ƶ���һ�������м��
	if ( InConflictPhase()  )  //��λ��ͻ�� ����ͬʱ�����̵�
	{			
		SndMsgLog(LOG_TYPE_GREEN_CONFIG,3,0,0,0); // 3��ʾ��λ��ͻ ADD��201309251130
		//DealGreenConflict(1);
		ACE_DEBUG((LM_DEBUG,"%s:%d InConflictPhase !\n",__FILE__,__LINE__));
		CFlashMac::CreateInstance()->FlashForceStart(2) ; //�̳�ͻ��ǿ�ƻ���˸
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

		//�����еĲ�������ټ�����һ��
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
				if ( bStageFirstStep && IsLongStep(m_pRunData->ucStepNo) )   //������Ϊ�׶εĵ�һ��
				{
					//m_pRunData->ucStepTime = 
						   //CAdaptive::CreateInstance()->GetStageGreenLen(m_pRunData->ucStepNo,ucStageNo);
				}
				else
				{
					m_pRunData->ucStepTime = m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucStepLen;
				}
				break;

			case CTRL_VEHACTUATED:   //��Ӧ����״̬�²����ı�
			case CTRL_MAIN_PRIORITY: 
			case CTRL_SECOND_PRIORITY: 
				StepToStage(m_pRunData->ucStepNo,&bStageFirstStep);
				if ( bStageFirstStep && IsLongStep(m_pRunData->ucStepNo) )   //������Ϊ�׶εĵ�һ�� �������ǵ�ǰ���̵���
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
		m_pRunData->ucStageNo = StepToStage(m_pRunData->ucStepNo,NULL); //���ݲ����Ż�ȡ�׶κ�

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
Description:    һ����������
Input:          ��              
Output:         ��
Return:         ��
***************************************************************/
void CManaKernel::OverCycle()
{
	bool bStageFirstStep      = false;
	Byte ucStageNo            = 0;
	int iCurTimePatternId     = 1;
	int iStepLength[MAX_STEP] = {0};
	
	if( m_iTimePatternId == 251) //���ʱ��ʱ��λ��Ϸ����򷵻�ԭ������״̬
	{
			m_iTimePatternId = 0 ;
			GetRunDataStandard(); //�����ع��춯̬����
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
	
	CDetector::CreateInstance()->GetAllWorkSts();     //һ�����ڼ��һ�μ��������״̬���ڸ�Ӧ������������

	if ( FLASH == m_pRunData->uiWorkStatus )  //������ ����ȫ��
	{	
		if ( m_bSpeStatusTblSchedule )  //ʱ�α���Ļ���״̬
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
		if ( m_bSpeStatusTblSchedule )  //ʱ�α���Ĺصƻ���ȫ��
		{
			m_pRunData->uiOldWorkStatus = m_pRunData->uiWorkStatus;
			m_pRunData->uiWorkStatus    = STANDARD;
			m_bVirtualStandard         = true;
		}


		if ( ALLRED == m_pRunData->uiWorkStatus )  //ȫ���� �����׼
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
			(CDbInstance::m_cGbtTscDb).SetSystemData("ucDownloadFlag",0);  // ��������
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
				if ( (Byte)m_pRunData->uiUtcsHeartBeat >= m_pRunData->ucCycle )  //1������û���յ���������ָ��
				{					
						bUTS = false ;
						bDegrade = true ;
						m_pRunData->uiOldCtrl = CTRL_UTCS;
						
						if(m_pTscConfig->DegradeMode > 0)
						{
							if(m_pTscConfig->DegradeMode >= 5)
							{
								m_pRunData->uiCtrl    = CTRL_SCHEDULE;	
								SndMsgLog(LOG_TYPE_OTHER,1,m_pRunData->uiOldCtrl,m_pRunData->uiCtrl,1); //��־��¼���Ʒ�ʽ�л� ADD?201311041530													
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
							SndMsgLog(LOG_TYPE_OTHER,1,CTRL_UTCS,CTRL_SCHEDULE,1); //��־��¼���Ʒ�ʽ�л� ADD?201311041530
							ACE_DEBUG((LM_DEBUG,"%s:%d oldctrl = %d newctrl =%d \n"	,__FILE__,__LINE__,CTRL_UTCS,CTRL_SCHEDULE));
						}
						CWirelessCoord::CreateInstance()->SetDegrade() ;   //������������  ADD��201311121430
						CMainBoardLed::CreateInstance()->DoModeLed(true,true);
						ACE_DEBUG((LM_DEBUG,"%s:%d too long no utcs command��\n"	,__FILE__,__LINE__));
				}
				break;
			case CTRL_VEHACTUATED:
			case CTRL_MAIN_PRIORITY:   
			case CTRL_SECOND_PRIORITY:
				if ( !CDetector::CreateInstance()->HaveDetBoard() || CTRL_VEHACTUATED != m_pRunData->uiScheduleCtrl )/*|| !m_bPhaseDetCfg*/  /*�����ڼ������*/
				{
					ACE_DEBUG((LM_DEBUG,"%s:%d When change cycle,no DetBoard or uiCtrl != VEHACTUATED\n",__FILE__,__LINE__));
					m_pRunData->uiOldCtrl = m_pRunData->uiCtrl;					
					m_pRunData->uiCtrl    = CTRL_SCHEDULE;
					
					m_pRunData->bNeedUpdate = true;	
					bDegrade = true ;	
					CMainBoardLed::CreateInstance()->DoModeLed(true,true);				
					SndMsgLog(LOG_TYPE_OTHER,1,m_pRunData->uiOldCtrl,m_pRunData->uiCtrl,1); //��־��¼���Ʒ�ʽ�л� ADD?201311041530
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
					SndMsgLog(LOG_TYPE_OTHER,1,m_pRunData->uiOldCtrl,m_pRunData->uiCtrl,1); //��־��¼���Ʒ�ʽ�л� ADD?201311041530
				}
				*/
				break;
			default:
				iCurTimePatternId = m_pRunData->ucTimePatternId > 0 ? m_pRunData->ucTimePatternId : 1;
				if ( CDetector::CreateInstance()->HaveDetBoard() /*&& m_bPhaseDetCfg*/ 
					&& CTRL_VEHACTUATED == m_pRunData->uiScheduleCtrl )  //���ڼ�������������ü��������
				{
					ACE_DEBUG((LM_DEBUG,"%s:%d  switch break ",__FILE__,__LINE__));
					m_pRunData->uiOldCtrl = m_pRunData->uiCtrl;
					m_pRunData->uiCtrl    = CTRL_VEHACTUATED;
					
					m_pRunData->bNeedUpdate = true;
					ACE_DEBUG((LM_DEBUG,"%s:%d oldctrl = %d newctrl = %d\n"	,__FILE__,__LINE__,m_pRunData->uiCtrl, CTRL_VEHACTUATED));
					SndMsgLog(LOG_TYPE_OTHER,1,m_pRunData->uiOldCtrl,m_pRunData->uiCtrl,1); //��־��¼���Ʒ�ʽ�л� ADD?201311041530
					break;
				}
				else if ( CTRL_WIRELESS == m_pRunData->uiScheduleCtrl && m_pTscConfig->sTimePattern[iCurTimePatternId-1].ucPhaseOffset < 99 
					&& CManaKernel::CreateInstance()->m_pTscConfig->sSpecFun[FUN_GPS].ucValue != 0 )  //������λ��ҿ���gps
				{
					m_pRunData->uiOldCtrl = m_pRunData->uiCtrl;
					m_pRunData->uiCtrl    = CTRL_WIRELESS;
					SndMsgLog(LOG_TYPE_OTHER,1,m_pRunData->uiOldCtrl,m_pRunData->uiCtrl,1); //��־��¼���Ʒ�ʽ�л� ADD?201311041530
				}
				
				break;
			}
		}
		
		
		ResetRunData(0);  //���������»�ȡ��̬����
		
		m_pRunData->bNeedUpdate = false;

		if ( m_pRunData->bOldLock )
		{
			m_pRunData->bOldLock = false;
			ACE_DEBUG((LM_DEBUG,"%s:%d  begin setcyclestepinfo\n",__FILE__,__LINE__));
			SetCycleStepInfo(0); //���������������ڵĲ�����Ϣ����
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
				/*&& (m_ucUtscOffset != 0)*/ )   //���ر�׼ʱ ��ʹ��λ��Ϊ0Ҳ��������ʱ���ĵ���
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
			if ( IsLongStep(m_pRunData->ucStepNo) && bStageFirstStep )   //������Ϊ�׶εĵ�һ��
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
			if ( bStageFirstStep && IsLongStep(m_pRunData->ucStepNo) )   //������Ϊ�׶εĵ�һ��
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
Description:    ��������λ
Input:          ��              
Output:         ��
Return:         ��
***************************************************************/
void CManaKernel::SetCycleBit(bool bSetCycle)
{
	//ACE_Guard<ACE_Thread_Mutex>  guard(m_mutexSetCycle);
	m_bCycleBit = bSetCycle;
}


/**************************************************************
Function:       CManaKernel::CwpmGetCntDownSecStep
Description:    ���͵���ʱ��Ϣ
Input:          ��              
Output:         ��
Return:         ��
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
		//ACE_DEBUG((LM_DEBUG,"%s:%d �������ʱ��StepNum=%d��\n" ,__FILE__,__LINE__,m_pRunData->ucStepNo));
		CGaCountDown::CreateInstance()->GaSendStepPer();
		#endif

	}

	
}


/**************************************************************
Function:       CManaKernel::ResetRunData
Description:    ���������źŻ���̬����,����һ������һ��,�ı��źŻ�
				״̬��ʱ��Ҳ�����
Input:          ucTime :  ������ȫ���ʱ��           
Output:         ��
Return:         ��
***************************************************************/
void CManaKernel::ResetRunData(Byte ucTime)
{
#ifdef CHECK_MEMERY
	TestMem(__FILE__,__LINE__);
#endif

	if ( FLASH == m_pRunData->uiWorkStatus )// ��ʼ��ֵΪ CTRL_SCHEDULE;
	{
		if ( m_bSpeStatusTblSchedule )
		{
			m_pRunData->ucStepTime = MAX_SPESTATUS_CYCLE;
		}
		else if ( 0 == ucTime )
		{
			m_pRunData->ucStepTime = m_pTscConfig->sUnit.ucStartFlashTime;
			//ACE_DEBUG((LM_DEBUG,"%s:%d ����������� 10�뿪ʼ\n" ,__FILE__,__LINE__));
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
		
		if ( m_bSpePhase )  //�ض���λ����
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
			/******************* ����֮ǰ������Ϩ���źţ��������������**************************************/
			CLampBoard::CreateInstance()->SetLamp(m_pRunData->sStageStepInfo[0].ucLampOn, m_pRunData->sStageStepInfo[0].ucLampFlash);
			CLampBoard::CreateInstance()->SendLamp();
			/******************* ����֮ǰ������Ϩ���źţ��������������**************************************/
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
	else if ( m_pRunData->uiWorkStatus == ALLRED )  //ȫ��
	{
		if ( m_bSpeStatusTblSchedule )
		{
			m_pRunData->ucStepTime = MAX_SPESTATUS_CYCLE;
		}
		else if ( 0 == ucTime )
		{
			m_pRunData->ucStepTime = m_pTscConfig->sUnit.ucStartAllRedTime;
		//	ACE_DEBUG((LM_DEBUG,"%s:%d �������������ȫ�� 5�뿪ʼ\n" ,__FILE__,__LINE__));
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
	else if ( m_pRunData->uiWorkStatus == SIGNALOFF )  //�ص�
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
	else if ( m_pRunData->uiWorkStatus == STANDARD )  //��׼
	{
		

		ACE_DEBUG((LM_DEBUG,"%s:%d ResetRunDate ��ʼ�����׼״̬ ���춯̬����,call GetRunDataStandard()\n" ,__FILE__,__LINE__));
		GetRunDataStandard(); //�����ع��춯̬����
		m_bSpeStatus = false;
		
	}

#ifdef CHECK_MEMERY
	TestMem(__FILE__,__LINE__);
#endif
}


/**************************************************************
Function:       CManaKernel::GetRunDataStandard
Description:    ��׼����£������߼��Ļ�ȡ��̬��Ϣ ������Ҫ���¼�
				��ʱ�α�  ��ȡ��ʱ������ ��ȡ�׶���ʱ���,��ȡ��
				����Ϣ
Input:          ��          
Output:         ��
Return:         ��
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

	if ( m_pRunData->bNeedUpdate || (0==tvTime.hour() && tvTime.day() !=ucDay ) )  //��һ��,������Ҫ���¼���ʱ�α���Ϣ
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
		ucCurScheduleId = GetScheduleId((Byte)tvTime.month(),(Byte)tvTime.day(),(Byte)tvTime.weekday());  //��ȡ��ǰ��ʱ�α��  
		
		if ( m_pRunData->bNeedUpdate || (ucCurScheduleId != m_pRunData->ucScheduleId) )
		{
			//���´����ݿ����1��ʱ�α���Ϣ 
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

	ucCurScheduleId         = GetScheduleId((Byte)tvTime.month(),(Byte)tvTime.day(),(Byte)tvTime.weekday()); //��ʱ�����ȱ��õ����ʱ�α��
	
	//����ʱ����Ϣ���»�ȡ��ʱ������
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
		ucCurTimePatternId = GetTimePatternId( ucCurScheduleId , &ucCurCtrl , &ucCurStatus );// �������ݵ�ǰʱ������������ʱ������

	m_pRunData->ucScheduleId    = ucCurScheduleId; //ʱ�α����Ʒ�ʽ�� ������Ŀ��Ʊ�������
	m_pRunData->uiScheduleCtrl  = ucCurCtrl;   //��¼ʱ�α�Ŀ��Ʒ�ʽ  ��Ӧ���Ʒ�ʽ��Ҫ���ø�Ӧ����
	//ACE_DEBUG((LM_DEBUG,"%s:%d ucCurCtrl = %d m_pRunData->uiCtrl = %d\n" ,__FILE__,__LINE__,ucCurCtrl,m_pRunData->uiCtrl));	
	if ( ucCurCtrl != m_pRunData->uiCtrl )  //���Ʒ�ʽ�ı�
	{
		//if ( (CTRL_SCHEDULE == ucCurCtrl) 
			//&& ( CTRL_WIRELESS == m_pRunData->uiCtrl || CTRL_VEHACTUATED == m_pRunData->uiCtrl 
			//  || CTRL_MAIN_PRIORITY == m_pRunData->uiCtrl || CTRL_SECOND_PRIORITY == m_pRunData->uiCtrl 
			//  || CTRL_UTCS == m_pRunData->uiCtrl ) )
		//{
			//ACE_DEBUG((LM_DEBUG,"%s:%d �Ӹ�Ӧ�������ʱ�ο���ת��\n" ,__FILE__,__LINE__));
		//}
		//else
		//{
			if(!(ucCurCtrl ==CTRL_UTCS && bUTS == false) && m_pRunData->uiCtrl != CTRL_UTCS) //���ʱ�α���ΪCTRL_UTC

			{
				ACE_DEBUG((LM_DEBUG,"%s:%d ʱ�α���Ŀ��Ʒ�ʽ�����˸ı�\n" ,__FILE__,__LINE__));
				SwitchCtrl(ucCurCtrl); //ʱ�α���Ŀ��Ʒ�ʽ�����˸ı�
			}
			
		//}
	}
	ACE_DEBUG((LM_DEBUG,"%s:%d m_iTimePatternId1 = %d \n" ,__FILE__,__LINE__,m_iTimePatternId));
	//��ȡ�׶���ʱ��
	//if ( m_pRunData->bNeedUpdate || (ucCurTimePatternId != m_pRunData->ucTimePatternId) || m_iTimePatternId == 251)//��ʱ�����Ų�ͬ����Ϊ251���ⷽ����
	 if(m_pRunData->bNeedUpdate || ucCurTimePatternId != m_pRunData->ucTimePatternId || m_iTimePatternId == 251 || m_iTimePatternId == 250)
	{
		Byte ucCurScheduleTimeId = GetScheduleTimeId(ucCurTimePatternId,m_ucUtcsComCycle,m_ucUtscOffset); //��ȡ��ʱ�׶α��
ACE_DEBUG((LM_DEBUG,"%s:%d m_iTimePatternId2 = %d \n" ,__FILE__,__LINE__,m_iTimePatternId));
		m_pRunData->ucTimePatternId = ucCurTimePatternId;
				
		if ( m_pRunData->bNeedUpdate|| (ucCurScheduleTimeId != m_pRunData->ucScheduleTimeId) ||m_iTimePatternId == 251 || m_iTimePatternId == 250)
		{
			m_pRunData->ucScheduleTimeId = ucCurScheduleTimeId;

			//���¼��ض�̬���ݵĽ׶���ʱ��ϢsScheduleTime
			if(!GetSonScheduleTime(ucCurScheduleTimeId))
			{
				bDegrade = true ;
				CGbtMsgQueue::CreateInstance()->SendTscCommand(OBJECT_SWITCH_SYSTEMCONTROL,254);
				ACE_DEBUG((LM_DEBUG,"%s:%d �׶���ʱΪ�գ�����������\n" ,__FILE__,__LINE__));	
				CMainBoardLed::CreateInstance()->DoModeLed(true,true);
				return ;	
			}
			ACE_DEBUG((LM_DEBUG,"%s:%d �����������ڸ��׶β�����Ϣ�������ǵ�ǰ��ʱ�����Ž׶���ʱ�Ų�ͬ�������ݿ��и���\n" ,__FILE__,__LINE__));		
			//���¹����������ڵĲ�����Ϣ
			SetCycleStepInfo(0);
			
		}
	}
	else if ( m_bSpeStatus && STANDARD == ucCurStatus)  //������״̬-->STANDARD ��Ҫ���»�ȡ����
	{
		Byte ucCurScheduleTimeId = GetScheduleTimeId(ucCurTimePatternId,m_ucUtcsComCycle,m_ucUtscOffset);

		m_pRunData->ucTimePatternId = ucCurTimePatternId;
		m_pRunData->ucScheduleTimeId = ucCurScheduleTimeId;

		//���¼��ض�̬���ݵĽ׶���ʱ��ϢsScheduleTime
		GetSonScheduleTime(ucCurScheduleTimeId); // READ: 2013 0704 0915 ��ȡ��ǰ�ӽ׶����� �ӽ׶�����
		
		ACE_DEBUG((LM_DEBUG,"%s:%d �����������ڸ��׶β�����Ϣ������������״̬�����׼״̬���ҵ�ǰ�źŻ�����״̬�Ǳ�׼״̬\n" ,__FILE__,__LINE__));
		//���¹����������ڵĲ�����Ϣ
		SetCycleStepInfo(0);
		
	}

	if ( ucCurStatus != m_pRunData->uiWorkStatus && m_pRunData->uiCtrl != CTRL_UTCS)  //����״̬�ı� ��������ʱ������
	{
		SwitchStatus(ucCurStatus);
	}
	else if ( m_bVirtualStandard && (STANDARD == ucCurStatus) )  //STANDARD(FLASH��ALLRED��SINGLEALL) --> STANDARD
	{
		m_pRunData->uiWorkStatus = m_pRunData->uiOldWorkStatus;
		m_bVirtualStandard      = false;
		SwitchStatus(ucCurStatus);
		ACE_DEBUG((LM_DEBUG,"%s:%d m_bVirtualStandard && (STANDARD == ucCurStatus\n" ,__FILE__,__LINE__));
	}

}


/**************************************************************
Function:       CManaKernel::GetScheduleId
Description:    �����¡��ա����ڻ�ȡ��ǰ��ʱ�α��
Input:          ucWeek 0:���� 1����1......6����6 ��������
				ucMonth ��1-12 �����·�  
				ucDay   ��1-31 ��������        
Output:         ��
Return:         ���յ�ʱ�α�ţ���û�в�ѯ���򷵻�0
***************************************************************/
Byte CManaKernel::GetScheduleId(Byte ucMonth,Byte ucDay , Byte ucWeek)
{
	//���� �ڼ��� �ض��� + �ض��� + ��ȫѡ
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
			break;  //û�и����ʱ�䷽��
		}
	}
	//���ܣ�ȫ��   + ȫ��   + �ض���
	for (Byte i=0; i<MAX_TIMEGROUP; i++ )
	{	
		if ( m_pTscConfig->sTimeGroup[i].ucId != 0 )
		{			
			if ( ( (m_pTscConfig->sTimeGroup[i].usMonth) & 0x1ffe) == 0x1ffe )
			{
				if ( (m_pTscConfig->sTimeGroup[i].uiDayWithMonth & 0xfffffffe) == 0xfffffffe )
				{
					if ( ((m_pTscConfig->sTimeGroup[i].ucDayWithWeek)>>(ucWeek+1)) & 0x1       //������λ�Ƚϵ�ʱ�� ucWeek+1
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
			break;  //û�и����ʱ�䷽��
		}
	}
	//���£��ض��� + ȫ��   + ȫ��
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
			break;  //û�и����ʱ�䷽��
		}
	}

	//�������ض��� + �ض��� + �ض���
	for (Byte i=0; i<MAX_TIMEGROUP; i++ )
	{	
		if ( m_pTscConfig->sTimeGroup[i].ucId != 0 )
		{			
			if ( (m_pTscConfig->sTimeGroup[i].usMonth>>ucMonth) &0x1 )	//�·���ȷ b0 = 0 ���� b1:1�� b12:12
			{
				if ( ( ((m_pTscConfig->sTimeGroup[i].ucDayWithWeek)>>(ucWeek+1)) & 0x1 ) //b0=0 ����b1������ b2����1 
					&& ( ((m_pTscConfig->sTimeGroup[i].uiDayWithMonth)>>ucDay) & 0x1 ) )  //b0 =1 ���� b1:1�� b2
				{
					//ACE_DEBUG((LM_DEBUG,"%s:%d ucId =%d Priority 4: Spe month,Spe day ,Spe weeks Get ScheduleId = %d from Tbl_Plan !\n" ,__FILE__,__LINE__, m_pTscConfig->sTimeGroup[i].ucId,m_pTscConfig->sTimeGroup[i].ucScheduleId));
					if (m_pTscConfig->sTimeGroup[i].ucScheduleId >0)
							return m_pTscConfig->sTimeGroup[i].ucScheduleId;
				}
			}
		}
		else
		{
			break;  //û�и����ʱ�䷽��
		}
	}	
	return 0;
}



/**************************************************************
Function:       CManaKernel::GetScheduleId
Description:    �����¡��ա����ڻ�ȡ��ǰ��ʱ�α��
Input:          ucScheduleId ��ʱ�α��       
Output:         ucCtrl	 �� ��ǰ����ģʽ
				ucStatus �� ��ǰ����״̬
				ucCycle      - ����    ��ʱδ��
		        ucOffSet     - ��λ��  ��ʱδ��
Return:         ��ʱ������ 
***************************************************************/
Byte CManaKernel::GetTimePatternId(Byte ucScheduleId , Byte* ucCtrl , Byte* ucStatus )
{	
	ACE_DEBUG((LM_DEBUG,"%s:%d m_pRunData->uiCtrl= %d m_iTimePatternId= %d\n" ,__FILE__,__LINE__,m_pRunData->uiCtrl ,m_iTimePatternId));
	//�����ǰ������״̬���Ʒ�ʽΪ�����ƻ���ϵͳ�Ż����ƣ����ҵ�ǰ���η����Ų�Ϊ0
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
			if ( m_pTscConfig->sSchedule[iIndex+1].ucId == ucScheduleId )//�ж��Ƿ������һ��ʱ��
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
		else //Ӧ����ǰȥ
		{
			bLastTem = true; //
			break;           //
		}
	}

	if ( bLastTem && (iIndex != 0 )) //���һ�ε�ʱ��׶�
	{
		
		ucCurTimePatternId =  m_pTscConfig->sSchedule[iIndex-1].ucTimePatternId;
		ucCtrlTmp = m_pTscConfig->sSchedule[iIndex-1].ucCtrl;
	}
	
	m_bSpeStatusTblSchedule = false;	 
    
	switch ( ucCtrlTmp )
	{
		case 0:     //0:��ʱ��
			*ucCtrl   = CTRL_SCHEDULE;
			*ucStatus = STANDARD;
			printf("%s:%d now:STRL_SCHEDULE!\n",__FILE__,__LINE__);
			break;
		case 1:    //1:�ص�
			*ucStatus = SIGNALOFF;
			m_bSpeStatusTblSchedule = true;
			break;
		case 2:    //2:����
			*ucStatus = FLASH;
			m_bSpeStatusTblSchedule = true;
			break;
		case 3:    //3:ȫ��
			*ucStatus = ALLRED;
			m_bSpeStatusTblSchedule = true;
			break;
		case 4:    //4�����߰��Ӧ
			*ucCtrl   = CTRL_MAIN_PRIORITY;
			*ucStatus = STANDARD;
			break;
		case 5:    //5:���߰��Ӧ
			*ucCtrl   = CTRL_SECOND_PRIORITY;
			*ucStatus = STANDARD;
			break;
		case 6:    //6:ȫ��Ӧ
			*ucCtrl   = CTRL_VEHACTUATED;
			*ucStatus = STANDARD;
			printf("%s:%d now:STRL_VEHACTUATED!\n",__FILE__,__LINE__);
			break;
		case 7:    //7:�޵�����
			*ucCtrl   = CTRL_WIRELESS;
			*ucStatus = STANDARD;
			break;
		case 8:    //8:����Ӧ����
			*ucCtrl   = CTRL_ACTIVATE;
			*ucStatus = STANDARD;
			break;
		case 12:   //12:����
			*ucCtrl   = CTRL_UTCS;
			*ucStatus = STANDARD;
			break;
		
		default:   //Ĭ�϶�ʱ��
			*ucCtrl   = CTRL_SCHEDULE;
			*ucStatus = STANDARD;
			break;
	}	
	return ucCurTimePatternId;
}


	

/**************************************************************
Function:       CManaKernel::GetScheduleTimeId
Description:    �����¡��ա����ڻ�ȡ��ǰ��ʱ�α��
Input:          ucTimePatternId - ��ʱ�������       
Output:         ucCycle         - ����
*       	    ucOffSet        - ��λ��
Return:         �׶���ʱ��� �����û�в�ѯ���Ϸ��׶���ʱ����򷵻�1
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
	return 1;  //���Ϸ��Ĳ���,������
}


/**************************************************************
Function:       CManaKernel::GetSonScheduleTime
Description:    ���ݶ�̬���ݵĽ׶���ʱ���(ucScheduleTimeId)��ȡ
				�����ӽ׶ε���Ϣ
Input:          ucScheduleTimeId - �׶���ʱ���       
Output:         ��
Return:         ��
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
Description:    ���ݽ׶κŻ�ȡ������ �ý׶κŶ�Ӧ����������
Input:          iStageNo:�׶κ�      
Output:         ��
Return:         ������
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
Description:    ���ݲ����Ż�ȡ�׶κ�
Input:          iStepNo:������      
Output:         bStageFirstStep:�Ƿ�Ϊ�׶εĵ�һ��
Return:         �׶κ�
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
Description:    ��ȡ�׶ε�Ԥ����ʱ��ͽ׶��Ѿ����е�ʱ��
Input:          ��     
Output:         pTotalTime-�׶ε�Ԥ����ʱ��  pElapseTime-�׶���
				�����е�ʱ��
Return:         ��
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
Description:    �����������ڵĲ�����Ϣ��������Ϣ�洢��
				m_pRunData->sStageStepInfo��
Input:          ucLockPhase - ������λ��     
Output:         ��
Return:         ��
***************************************************************/
void CManaKernel::SetCycleStepInfo(Byte ucLockPhase)
{
	ACE_Guard<ACE_Thread_Mutex>  guard(m_mutexRunData);

	Byte ucStepIndex  = 0;
	Byte ucStageIndex = 0;

	ACE_OS::memset(m_pRunData->sStageStepInfo,0,sizeof(m_pRunData->sStageStepInfo));

	if ( ucLockPhase != 0 )  //������λ 
	{
		//����������λ�����������ڵĲ�����Ϣ..............
		m_pRunData->ucStepNo = 0;
		m_pRunData->ucStepNum = 1;
		m_pRunData->ucStageNo = 0;
		m_pRunData->ucStageCount = 1;
		m_pRunData->bOldLock = true;
		return;
	}

	//�������ò�����Ϣ
	for (ucStageIndex=0; ucStageIndex<MAX_SON_SCHEDULE; ucStageIndex++ )//MAX_SON_SCHEDULE
	{
 		if ( m_pRunData->ucScheduleTimeId == m_pRunData->sScheduleTime[ucStageIndex].ucId )  //ucScheduleTimeId  ucId�׶���ʱ���
 			
		{
			//����һ���ӽ׶ι��첽����
			SetStepInfoWithStage(ucStageIndex,&ucStepIndex,&(m_pRunData->sScheduleTime[ucStageIndex]));// ucStageIndex �ӽ׶κ�
			// ACE_DEBUG((LM_DEBUG,"%s:%d ucStageIncludeSteps= %d \n" ,__FILE__,__LINE__,m_pRunData->ucStageIncludeSteps[ucStageIndex]));
		}
		else
		{
			break;  //ȫ�����ӽ׶α������
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
Description:    ���ݰ�����λid��ȡȫ���ĸ�����λ
				m_pRunData->sStageStepInfo��
Input:          iPhaseId      - ������λid     
Output:         ucCnt         - ������λ����
*       	    pOverlapPhase - �洢����������λ
Return:         ��
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
Description:   ���ݸ�����λ��ȡ������λindex				
Input:          iCurAllowPhase      - �׶η�����λ
*    		    ucOverlapPhaseIndex - ������λ�±� 0 1 ....7    
Output:       
Return:         return: ��Ӧ������λ��index 0 1 .....15
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
Description:    ����һ���ӽ׶����ò�����Ϣ			
Input:          ucStageIndex  : ��ǰ���ӽ׶��±�
*      			ucCurStepIndex����ǰ�ۼ����õĲ�����,��sStageStepInfo���±�
*       		pScheduleTime : �ӽ׶���Ϣ��ָ�룬Ϊ��ǰ�ӽ׶κŶ�Ӧ�׶���ʱ�������
Output:         ucCurStepIndex: ��ǰ�ۼ����õĲ�����,��sStageStepInfo���±�
Return:         ��
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
		if ( ( pScheduleTime->usAllowPhase>>i ) & 0x1 )  //������λ���,��ȡ���ӽ׶η�����λ���̻ƺ�ʱ��
		{
			sPhaseStep[ucPhaseIndex].ucPhaseId     = i + 1;
			sPhaseStep[ucPhaseIndex].bIsAllowPhase = true;
			GetPhaseStepTime(i,pScheduleTime,sPhaseStep[ucPhaseIndex].ucStepTime); //��ȡ����λ�� �� ��ɾ �� ��ʱ��  4���ֽڱ�ʾ
			ucAllowPhaseIndex = ucPhaseIndex;
			//ACE_DEBUG((LM_DEBUG,"%s:%d PhaseId=%d  sPhaseStep[ucPhaseIndex] = %d,Green=%d,GF = %d Yellow = %d\n" ,__FILE__,__LINE__,i+1,ucPhaseIndex,sPhaseStep[ucPhaseIndex].ucStepTime[0],sPhaseStep[ucPhaseIndex].ucStepTime[1],sPhaseStep[ucPhaseIndex].ucStepTime[2])); // ADD: 20130708 ���Ը�Ӧ������ʱ�䳤?

			ucPhaseIndex++;
			
			//������λ���   READ:20130703 1423
			GetOverlapPhaseIndex(i+1 , &ucCnt , ucOverlapPhase);
			if ( ucCnt > 0 )  //����ͨ��λΪ������λ������������λ
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
					   // ACE_DEBUG((LM_DEBUG,"%s:%d overPhaseId=%d  sPhaseStep[ucPhaseIndex] = %d,Green=%d,GF = %d Yellow = %d\n" ,__FILE__,__LINE__,ucOverlapPhase[iIndex] + 1,ucPhaseIndex,sPhaseStep[ucPhaseIndex].ucStepTime[0],sPhaseStep[ucPhaseIndex].ucStepTime[1],sPhaseStep[ucPhaseIndex].ucStepTime[2])); // ADD: 20130708 ���Ը�Ӧ������ʱ�䳤?

						ucPhaseIndex++;
					}

					iIndex++;
				}
			}
		}
	}// �õ����ӽ׶����з�����λ�� ����ʱ��

	//ACE_DEBUG((LM_DEBUG,"%s:%d ucCurStageIndex= %d ucPhaseIndex=%d \n" ,__FILE__,__LINE__,ucCurStageIndex,ucPhaseIndex));

	//�������е���λ ��sPhaseStepת��Ϊ������Ϣ
	m_pRunData->ucStageIncludeSteps[ucCurStageIndex] = 0;	

	for ( int i=0; i<ucPhaseIndex; i++ ) //ucPhaseIndex Ϊ������λ��
	{
		//ACE_DEBUG((LM_DEBUG,"%s:%d i=%d \n" ,__FILE__,__LINE__,i)); 
		if ( (ucTmpStepLen = GetPhaseStepLen(&sPhaseStep[i]) ) > 0 )
		{
			ucMinStepLen = (ucMinStepLen > ucTmpStepLen ? ucTmpStepLen : ucMinStepLen);
			//ACE_DEBUG((LM_DEBUG,"%s:%d i=%d \n" ,__FILE__,__LINE__,i)); 
		}
	
	   // ACE_DEBUG((LM_DEBUG,"%s:%d i=%d,ucMinStepLen= %d ucPhaseIndex=%d \n" ,__FILE__,__LINE__,i,ucMinStepLen,ucPhaseIndex));		
		if ( ( i + 1 ) ==  ucPhaseIndex )  //����1����������    �ﵽ�����λ��
		{
		//	ACE_DEBUG((LM_DEBUG,"%s:%d ��ʼ���첽������ \n" ,__FILE__,__LINE__));
			if ( 200 == ucMinStepLen )  //ȫ���������
			{
			//	 ACE_DEBUG((LM_DEBUG,"%s:%d ucStageIncludeSteps= %d \n" ,__FILE__,__LINE__,m_pRunData->ucStageIncludeSteps[ucCurStageIndex]));
				return;
			}

			Byte ucColorIndex = 0;
			m_pRunData->ucStageIncludeSteps[ucCurStageIndex]++;
			m_pRunData->sStageStepInfo[*ucCurStepIndex].ucStepLen = ucMinStepLen;				
			
			for ( int j=0; j<ucPhaseIndex; j++ ) //����ÿ����λ
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
					ucLampIndex = ( ucSignalGrp[ucSignalGrpIndex] - 1 ) * 3; //��λ���red�±�
					
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
			i = -1;  //���¿�ʼ������һ�ֲ���
			
		}
	
	}  //end for
	
}



/*********************************************************************************
Function:       CManaKernel::ExitOvelapPhase
Description:    ��ǰ�Ƿ��Ѵ��ڸø�����λ			
Input:          ucOverlapPhaseId  �� ��λ��
*      			ucPhaseCnt ����λ��
*       		pPhaseStep : ��λ������Ϣ
Output:         
Return:         true - ����  false - ������
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
Description:    ��δ���õ�ɫ��Ϣ���źŵ���ǿ������Ϊ�����			
Input:          ��
Output:         ucLampOn  - �źŵ���������ָ��
Return:         ��
**********************************************************************************/
void CManaKernel::SetRedOtherLamp(Byte* ucLampOn)
{
	bool bRed = false;

	for ( int i=0; i<MAX_LAMP; i=i+3 )
	{
		bRed = true;
		for (int j=i; j<(i+3); j++)
		{
			if ( 1 == ucLampOn[j] )        //�õ����б���λ
			{
				bRed = false;
				break;
			}
		}
		if ( bRed && IsInChannel(i/3+1) )  //�õ���(ͨ��)�б�����
		{
			ucLampOn[i] = 1;         	   //�õ���ǿ�������
		}
	}
}


/*********************************************************************************
Function:       CManaKernel::ExitStageStretchPhase
Description:    �жϽ׶ε���λ���Ƿ���ڵ�����λ			
Input:          pScheduleTime - �ӽ׶���Ϣ
Output:         ��
Return:         true -������λ   false -�ǵ�����λ
**********************************************************************************/
bool CManaKernel::ExitStageStretchPhase(SScheduleTime* pScheduleTime)
{
	for (Byte ucIndex=0; ucIndex<MAX_PHASE; ucIndex++ )
	{
		if ( ( pScheduleTime->usAllowPhase>>ucIndex ) & 0x1 )  //������λ���
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
Description:    ������λ�ţ���ȡ����λ���̡��������ơ�ȫ��ʱ��(��ͨ��λ)			
Input:          ucPhaseId:��λ��  pScheduleTime:�ӽ׶���Ϣ
Output:         pTime:����ʱ��
Return:         ��
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
		/*&& (m_sPhaseDet[ucPhaseId].iRoadwayCnt > 0)*/ )  //��Ӧ����  �׶ε���λ����ڵ�����λ 
	{
		//pTime[0] = m_pTscConfig->sPhase[ucPhaseId].ucMinGreen;  //min green
		pTime[0] = GetStageMinGreen(pScheduleTime->usAllowPhase);
		pTime[1] = m_pTscConfig->sPhase[ucPhaseId].ucGreenFlash; //green flash
	}
	else 
	{
		if ( 0 == pScheduleTime->ucGreenTime )  //�׶��̵�Ϊ0
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
	if(m_iTimePatternId ==251 || m_iTimePatternId ==250) //������λ��Ϸ���������
	{
			pTime[1] = 0;  //green flash
			pTime[0] = pScheduleTime->ucGreenTime;         //green
	}
	if ( (m_pTscConfig->sPhase[ucPhaseId].ucOption>>1) & 0x1 )      //������λ
	{
		pTime[2] = 0;    //������λû�лƵ�
	}
	//ACE_DEBUG((LM_DEBUG,"%s:%d ucPhaseId=%d  green time=%d,gf time = %d yellow time = %d\n" ,__FILE__,__LINE__,ucPhaseId+1,pTime[0],pTime[1],pTime[2]));
}


/*********************************************************************************
Function:       CManaKernel::ExitStageStretchPhase
Description:    ��ȡ������λ���̡��������ơ�ȫ��ʱ��			
Input:           ucCurStageIndex  - ��ǰ�׶���ʱ�±�  
*      			 ucOverlapPhaseId - ������λ�±� 0 1 2 .... 7 
*				 pIncludeTime     - ������λ�İ�����λ���� ���� �� ȫ��ʱ��
Output:         pTime - �̡��������ơ�ȫ��ʱ��
Return:         ��
**********************************************************************************/
void CManaKernel::GetOverlapPhaseStepTime(  
											    Byte ucCurStageIndex
											  , Byte ucOverlapPhaseId 
											  , Byte* pIncludeTime
											  , Byte* pOverlapTime
											  , Byte ucPhaseIndex
											  , Byte ucStageYellow )
{
	bool bExitNextStage             = false;                //������λ������һ���׶���
	Byte ucCnt                      = 0;
	Byte ucNextStageIndex           = ucCurStageIndex + 1;  //��һ���׶��±�
	unsigned short usNextAllowPhase = 0;                    //��һ����λ��
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
			GetOverlapPhaseIndex(iIndex+1 , &ucCnt , ucOverlapPhase);  //��ȡ��һ���׶η�����λ�ĸ�����λ
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


	if ( bExitNextStage )   //������λ��������һ���׶�
	{
		pOverlapTime[0] = pIncludeTime[0] + pIncludeTime[1] + pIncludeTime[2] + pIncludeTime[3];  //green
		//�����ٸ���β���̵ơ�β���Ƶơ�β����� ����������չ���� .......
		pOverlapTime[1] = 0;  //green flash
		pOverlapTime[2] = 0;  //yellow
		pOverlapTime[3] = 0;  //red

		if ( (m_pTscConfig->sPhase[ucPhaseIndex].ucOption>>1) & 0x1 )      //������λ
		{
			pOverlapTime[0] += ucStageYellow;  //������λ ���ϻ�����ʱ��
		}
	}
	else  //������λ�ڸý׶���������������Ҫ���������Ĺ��Ȳ�
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
Description:    ��ȡ��λ��ĳ��������Ӧ��ȡ�ĳ���			
Input:          pPhaseStep:��λ������Ϣָ��
Output:         ��
Return:         ��λ��ȡ�����Ĵ�С
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
Description:    ������λ���±꣬���жϸ���λ�Ƿ��Ѿ�ȫ����ȡ���			
Input:          pPhaseStep:��λ������Ϣָ��
Output:         ��
Return:         0-3����ȡ�±�� 5:��ȡ���
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
Description:    ������λ�Ż�ȡͨ��			
Input:          bIsAllowPhase - �Ƿ�Ϊ������λ false - ������λ
*        		ucPhaseId     - ��λID 
*       		pNum          - ͨ������ 
*       		pSignalGroup  - ����ͨ����
Output:         pNum:ͨ������ pSignalGroup:����ͨ����
Return:         0-3����ȡ�±�� 5:��ȡ���
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
Description:    �źŻ�����״̬�л�			
Input:          uiWorkStatus - �¹���״̬
Output:         ��
Return:         ��
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

	if ( ( SIGNALOFF == m_pRunData->uiWorkStatus ) && ( STANDARD == uiWorkStatus ) )  //�ص��л�����׼ ��Ҫ��������
	{
		m_pRunData->uiWorkStatus = FLASH;
	}
	else if ( (FLASH == m_pRunData->uiWorkStatus) && ( STANDARD == uiWorkStatus ) )  //�����л�����׼ ��Ҫ����ȫ�� 
	{
		m_bWaitStandard          = true;
		m_pRunData->uiWorkStatus = ALLRED;
	}
	else
	{
		m_pRunData->uiWorkStatus = uiWorkStatus;
	}

	m_pRunData->bNeedUpdate = true;   /*ȥ���ֶ�ʱ��Ҫ���¼���ͨ����Ϣ��һϵ����Ϣ*/
	ResetRunData(0);  //���������»�ȡ��̬����

	CLampBoard::CreateInstance()->SetLamp(m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucLampOn	,m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucLampFlash);
}


/*********************************************************************************
Function:       CManaKernel::SwitchCtrl
Description:    �źŻ�����ģʽ�л�		
Input:          uiCtrl - �¿���ģʽ
Output:         ��
Return:         ��
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
	
	if ( (CTRL_MANUAL == m_pRunData->uiCtrl) && m_bSpePhase )  //��ǰ�Ŀ��Ʒ�ʽΪ�ֶ��ض���λ��׼��ȥ���ֶ�
	{
		SThreadMsg sTscMsgSts;
		sTscMsgSts.ulType             = TSC_MSG_SWITCH_STATUS;  
		sTscMsgSts.ucMsgOpt           = 0;
		sTscMsgSts.uiMsgDataLen       = 1;
		sTscMsgSts.pDataBuf           = ACE_OS::malloc(1);
		*((Byte*)sTscMsgSts.pDataBuf) = FLASH;  //����
		CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
	}	

	if((CTRL_VEHACTUATED == uiCtrl) || (CTRL_MAIN_PRIORITY == uiCtrl) || (CTRL_SECOND_PRIORITY == uiCtrl) || (CTRL_ACTIVATE == uiCtrl)) 
	{
		if ( !CDetector::CreateInstance()->HaveDetBoard())   //MOD: 201309231450
		{			
			if(m_pRunData->uiCtrl == CTRL_SCHEDULE )         //��ֹ�ظ�д����Ʒ�ʽ������־
				return ;
			else
				uiCtrl    = CTRL_SCHEDULE;			
			CMainBoardLed::CreateInstance()->DoModeLed(true,true);								
			ACE_DEBUG((LM_DEBUG,"%s:%d when no DecBoard ,new uiCtrl = %d \n" ,__FILE__,__LINE__,uiCtrl));
		}
		else if ( CTRL_ACTIVATE != uiCtrl )
		{
			m_pRunData->bNeedUpdate = true;   //��Ҫ��ȡ�������Ϣ
			ACE_DEBUG((LM_DEBUG,"%s:%d CTRL_VEHACTUATED == uiCtrl,m_pRunData->bNeedUpdate == true \n" ,__FILE__,__LINE__));
		}
	}
	if ( uiCtrl == CTRL_LAST_CTRL )  //�ϴο��Ʒ�ʽ
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

	if ( uiCtrl == CTRL_PANEL || uiCtrl == CTRL_MANUAL )               //������л����ֶ��������������ͺ�������ʱ
	{             
		if ( m_pTscConfig->sSpecFun[FUN_COUNT_DOWN].ucValue != 0  )
			CGaCountDown::CreateInstance()->sendblack();
	}
	
	if ( CTRL_PANEL == m_pRunData->uiOldCtrl && m_iTimePatternId != 0)  //MOD:20130928 ������жϴ������ƶ�������
	{		
		m_iTimePatternId = 0;
		ACE_DEBUG((LM_DEBUG,"%s:%d After CTRL_PANEL to auto m_iTimePatternId = %d \n" ,__FILE__,__LINE__,m_iTimePatternId));
	}
	ACE_DEBUG((LM_DEBUG,"%s:%d oldctrl = %d newctrl = %d\n"	,__FILE__,__LINE__,m_pRunData->uiOldCtrl,m_pRunData->uiCtrl));
	SndMsgLog(LOG_TYPE_OTHER,1,m_pRunData->uiOldCtrl,m_pRunData->uiCtrl,0); //��־��¼���Ʒ�ʽ�л� ADD?201311041530
}



/*********************************************************************************
Function:       CManaKernel::SetUpdateBit
Description:    �������ݿ��������Ϊ��		
Input:          uiCtrl - �¿���ģʽ
Output:         ��
Return:         ��
**********************************************************************************/
void CManaKernel::SetUpdateBit()
{
#ifndef NO_CHANGEBY_DATABASE
	m_pRunData->bNeedUpdate = true;
#endif
}

/*********************************************************************************
Function:       CManaKernel::ChangePatter
Description:    �л��źŻ�����ʱ�����������л������ⷽ��
Input:          ��
Output:         ��
Return:         ��
**********************************************************************************/
void CManaKernel::ChangePatter()
{
	if ( IsLongStep(m_pRunData->ucStepNo) )
	{
		int iMinGreen = GetMaxStageMinGreen(m_pRunData->ucStepNo);
		if ( m_pRunData->ucElapseTime < iMinGreen )
		{			
			ACE_OS::sleep(iMinGreen-m_pRunData->ucElapseTime);  //������С��
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
Description:    �����׶�
Input:          ucStage - �׶κ�
Output:         ��
Return:         ��
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

	if ( ucStage == m_pRunData->ucStageNo && (IsLongStep(m_pRunData->ucStepNo)) )  //�����׶ξ��ǵ�ǰ�׶Σ��ҽ׶δ����̵ƽ׶�
	{
		return;
	}

	//��֤������С��
	if ( IsLongStep(m_pRunData->ucStepNo) )
	{
		iMinGreen = GetMaxStageMinGreen(m_pRunData->ucStepNo);
		if ( m_pRunData->ucElapseTime < iMinGreen )
		{
			//return;
			ACE_OS::sleep(iMinGreen-m_pRunData->ucElapseTime);  //������С��
		}
	}
	
	//���ɲ�
	while ( true )
	{
		m_pRunData->ucStepNo++;
		if ( m_pRunData->ucStepNo >= m_pRunData->ucStepNum )
		{
			m_pRunData->ucStepNo = 0;
		}

		if ( !IsLongStep(m_pRunData->ucStepNo) )  //�ǳ���
		{
			m_pRunData->ucElapseTime = 0;
			m_pRunData->ucStepTime = m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucStepLen;
			m_pRunData->ucStageNo = StepToStage(m_pRunData->ucStepNo,NULL); //���ݲ����Ż�ȡ�׶κ�

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
	m_pRunData->ucStepNo = StageToStep(m_pRunData->ucStageNo);  //���ݽ׶κŻ�ȡ������
	m_pRunData->ucStepTime = m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucStepLen;
	m_pRunData->ucElapseTime = 0;
	m_pRunData->ucRunTime = m_pRunData->ucStepTime;

	CLampBoard::CreateInstance()->SetLamp(m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucLampOn
		,m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucLampFlash);
	m_bNextPhase = false ;
}


/*********************************************************************************
Function:       CManaKernel::LockNextStage
Description:    �л�����һ���׶�����
Input:          ��
Output:         ��
Return:         ��
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
Description:    ��������
Input:          ucStep - 0:��һ�� other:ָ������
Output:         ��
Return:         ��
**********************************************************************************/
void CManaKernel::LockStep(Byte ucStep)
{
	//��֤������С��
	int iMinGreen = 0;

	if ( STANDARD != m_pRunData->uiWorkStatus )  //������ȫ���Լ��ص�û����һ��
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

	//�����еĲ�������ټ�����һ��
	m_pRunData->ucStepTime = m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucStepLen;
	m_pRunData->ucElapseTime = 0;
	m_pRunData->ucRunTime = m_pRunData->ucStepTime;
	m_pRunData->ucStageNo = StepToStage(m_pRunData->ucStepNo,NULL); //���ݲ����Ż�ȡ�׶κ�

	CLampBoard::CreateInstance()->SetLamp(m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucLampOn	,m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucLampFlash);
	
}


/*********************************************************************************
Function:       CManaKernel::LockStep
Description:    ������λ����
Input:          PhaseId  - ������λ��
Output:         ��
Return:         ��
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
		//������֤������С��
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

		//���ɲ�
		while ( true )
		{
			m_pRunData->ucStepNo++;
			if ( m_pRunData->ucStepNo >= m_pRunData->ucStepNum )
			{
				m_pRunData->ucStepNo = 0;
			}
			if ( !IsLongStep(m_pRunData->ucStepNo) )  //�ǳ���
			{
				m_pRunData->ucElapseTime = 0;
				m_pRunData->ucStepTime   = m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucStepLen;
				m_pRunData->ucStageNo    = StepToStage(m_pRunData->ucStepNo,NULL); //���ݲ����Ż�ȡ�׶κ�

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
		ucLampIndex = ( ucSignalGrp[ucSignalGrpIndex] - 1 ) * 3; //��λ���red�±�
		ucLampIndex = ucLampIndex + 2;  //green
		m_ucLampOn[ucLampIndex] = 1;
		ucSignalGrpIndex++;
	}
	SetRedOtherLamp(m_ucLampOn);
	CLampBoard::CreateInstance()->SetLamp(m_ucLampOn,m_ucLampFlash);
}


/*********************************************************************************
Function:       CManaKernel::ReadTscEvent
Description:    �¼���ȡ
Input:          ��
Output:         ��
Return:         ��
**********************************************************************************/
void CManaKernel::ReadTscEvent()
{
	//������
}


/*********************************************************************************
Function:       CManaKernel::ReadTscEvent
Description:    �źŻ�Уʱ�ӿ�,�����ֶ�Уʱ��GPSУʱ����
Input:          ucType - Уʱ����
				pValue - ʱ��ֵָ��
Output:         ��
Return:         ��
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

	if ( ucType == OBJECT_LOCAL_TIME ) //����ʱ��
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

	stime(&ti);   //����ϵͳʱ��	
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
Description:    �źŻ�״̬��ȡ
Input:          ucDealDataIndex - ������ͻ����±�
Output:         ��
Return:         ��
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
		((STscStatus*)(sGbtMsg.pDataBuf))->ucTscAlarm2 |= (1<<5);  /*�źŻ�ֹͣ����*/
	}

	((STscStatus*)(sGbtMsg.pDataBuf))->ucTscAlarm1    = 0;
	if ( FLASH == m_pRunData->uiWorkStatus )
	{
		((STscStatus*)(sGbtMsg.pDataBuf))->ucTscAlarm1 |= (1<<5);   /*�����������*/  
	}
	if ( CTRL_UTCS != m_pRunData->uiCtrl )
	{
		((STscStatus*)(sGbtMsg.pDataBuf))->ucTscAlarm1 |= (1<<6);   /*���ص������*/  
	}

	/*
	ACE_DEBUG((LM_DEBUG,"@@@@@@@@@@@@@@ %d %d %d",m_pRunData->uiWorkStatus,m_pRunData->uiCtrl
		,((STscStatus*)(sGbtMsg.pDataBuf))->ucTscWarn1));
		*/

	((STscStatus*)(sGbtMsg.pDataBuf))->ucTscAlarmSummary  = 0;
	if ( CDetector::CreateInstance()->IsDetError() )
	{
		((STscStatus*)(sGbtMsg.pDataBuf))->ucTscAlarmSummary |= (1<<5); /*���������*/
	}
	if ( m_bRestart )
	{
		((STscStatus*)(sGbtMsg.pDataBuf))->ucTscAlarmSummary |= (1<<7); /*�źŻ�����ֹͣ����*/
	}

	/*���������*/
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
Description:    �źŻ���չ״̬��ȡ
Input:          ucDealDataIndex - ������ͻ����±�
Output:         ��
Return:         ��
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
	((Byte*)sGbtMsg.pDataBuf)[6] = ucStageTotalTime;   //�׶���ʱ��
	((Byte*)sGbtMsg.pDataBuf)[7] = ucStageElapseTime;  //�׶��Ѿ�����ʱ��

	if ( m_bSpePhase )  //ָ����λ
	{
		pLampOn    = m_ucLampOn;
		pLampFlash = m_ucLampFlash;
	}
	else
	{
		pLampOn    = m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucLampOn;
		pLampFlash = m_pRunData->sStageStepInfo[m_pRunData->ucStepNo].ucLampFlash;
	}

	//���ͨ��״̬
	for ( int i=0; i<MAX_LAMP; i++ )
	{
		ucChanIndex = i / 3;  

		if ( pLampOn[i] == 0 )  //�ǵ���
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

		if ( 1 == pLampFlash[i] )  //ͨ����˸���״̬
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
Description:    ��ȡ��ǰ�������׶�ʱ��
Input:          
Output:         pCurStageLen - �׶�ʱ��ָ��
Return:         ��
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
Description:    ��ǰ�������ؼ���λ�̵�ʱ��
Input:          
Output:         pCurKeyGreen - �ؼ���λʱ������ָ��
Return:         ��
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
Description:    ��ȡ�����״̬
Input:          
Output:         pDetStatus - �����״ָ̬��
Return:         ��
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
Description:    ��ȡ���������
Input:          
Output:         pDetStatus - ���������ָ��
Return:         ��
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
Description:    ��ȡ����������澯����
Input:          
Output:         pDetStatus - ������������ָ��
Return:         ��
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
Description:    ��ȡ��ǰ����λ״̬��Ϣ
Input:          ��
Output:         pPhaseStatus - ��λ״̬��Ϣ����ָ��
Return:         ��
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
		case SIGNALOFF: //�ص�
			continue;
		case ALLRED:    //ȫ��
			pPhaseStatus[i].ucRed    = 0xff;
			continue;
		case FLASH:     //����
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
Description:    ��ȡ������λ״̬
Input:          ��
Output:         pOverlapPhaseSts - ������λ״̬��Ϣ����ָ��
Return:         ��
**********************************************************************************/
void CManaKernel::GetOverlapPhaseStatus(SOverlapPhaseSts* pOverlapPhaseSts)
{
	Byte ucCurStepNo  = m_pRunData->ucStepNo;
	Uint uiWorkStatus = m_pRunData->uiWorkStatus;
	
	pOverlapPhaseSts->ucId = 1;

	switch ( uiWorkStatus )
	{
	case SIGNALOFF: //�ص�
		return;
	case ALLRED:    //ȫ��
		pOverlapPhaseSts->ucRed    = 0xff;
		return;
	case FLASH:     //����
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
Description:    ͨ��״̬��Ϣ��ȡ
Input:          ��
Output:         pChannelSts - ͨ��״̬��Ϣ����ָ��
Return:         ��
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
		case SIGNALOFF: //�ص�
			ucIndex++;
			continue;
		case ALLRED:    //ȫ��
			pChannelSts[ucIndex].ucRed = 0xff;
			ucIndex++;
			continue;
		case FLASH:     //����
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
				
			if ( (ucPhaseNo>0) && (ucPhaseNo<MAX_PHASE) )  //���ڸ�ͨ����Ϣ
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
Description:    ����ͨ���Ż�ȡͨ����Ӧ������λ��
Input:          ucChannelNo -ͨ���źŵ����
Output:         ��
Return:         ��λ�ţ�����ȡ���󷵻� MAX_PHASE+1(����)
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
Description:    ������������Ϊ��ֵ
Input:          ��
Output:         ��
Return:         ��
**********************************************************************************/
void CManaKernel::SetRestart()
{
	m_bRestart = true;
}


/*********************************************************************************
Function:       CManaKernel::GetRestart
Description:    ��ȡ������������ֵ
Input:          ��
Output:         ��
Return:         ������������ֵ
**********************************************************************************/
bool CManaKernel::GetRestart()
{
	return m_bRestart;
}


/*********************************************************************************
Function:       CManaKernel::RestartTsc
Description:    �����źŻ�,1s����һ��
Input:          ��
Output:         ��
Return:         ��
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
Description:    �жϵ�ǰ�����Ƿ�Ϊ���������̵�����
Input:          iStepNo  - ������
Output:         ��
Return:         true - ����  false - �ǳ���
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
		if ( ( LAMP_COLOR_YELLOW == iIndex % 3 )&& (1 == m_pRunData->sStageStepInfo[iStepNo].ucLampOn[iIndex]) )  //���ڻƵ�
		{
			return false;
		}

		if ( 1 == m_pRunData->sStageStepInfo[iStepNo].ucLampOn[iIndex] )  //�����̵���
		{
			if ( 0 == m_pRunData->sStageStepInfo[iStepNo].ucLampFlash[iIndex] ) //�̵Ʋ���
			{
				bGreen = true;
				//break ; //ADD: 2013 0702 1723 Ӧ������ѭ��
				
			}
			else  //���� 
			{
				bGreenFlash = true;
			}

			break ; ////Ӧ�������� ADD?20130702 17 21
		}
		iIndex++;
	}

	if ( bGreen && !bGreenFlash ) //�����̵� ����û��(�ƻ����������)
	{
		return true;
	}

	return false;   
}



/*********************************************************************************
Function:       CManaKernel::GetStageMinGreen
Description:    ��ȡ�׶���λ�����С��
Input:          usAllowPhase  - ������λ�������
Output:         ��
Return:         ��С��ֵ
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
Description:    ��ȡ��ǰ��������С��(������λ����С��)�������ڽ׶εĵ�һ����ȡ
Input:          iStepNo  - ������
Output:         iMaxGreenTime:��������� iAddGreenTime:��С��λ�̵��ӳ�ʱ��
Return:         ��ǰ��������С��
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
			//if ( m_pTscConfig->sPhase[iIndex].ucType>>5 & 1  )  /*������λ*/
			//{
				iTmpMinGreen = m_pTscConfig->sPhase[iIndex].ucMinGreen;
				iTmpAddGreen = m_pTscConfig->sPhase[iIndex].ucGreenDelayUnit;
				iTmpMaxGreen = m_pTscConfig->sPhase[iIndex].ucMaxGreen1;
			//}
			/*
			else if ( m_pTscConfig->sPhase[iIndex].ucType>>6 & 1  ) //������λ
			{
				iTmpMinGreen = m_pTscConfig->sPhase[iIndex].ucFixedGreen;
			}
			*/
			/*
			else if ( m_pTscConfig->sPhase[iIndex].ucType>>7 & 1 )  //�̶���λ 
			{
				iTmpMinGreen = m_pRunData->sScheduleTime[StepToStage(m_pRunData->ucStepNo,NULL)].ucGreenTime 
					               - m_pTscConfig->sPhase[iIndex].ucGreenFlash;      //green
			}
			else
			{
				continue;
			}
			*/

			iMinGreen = iTmpMinGreen < iMinGreen    ? iMinGreen    : iTmpMinGreen; //�Ҵ�
			if ( iTmpAddGreen > 0 )
			{
				iAddGreen = iAddGreen    < iTmpAddGreen ? iAddGreen    : iTmpAddGreen; //��С
			}
			iMaxGreen = iMaxGreen    < iTmpMaxGreen ? iTmpMaxGreen : iMaxGreen;    //�Ҵ�
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
Description:    ��Ӧ���ƣ������̵�����ʱ��
Input:          iAddTime     - ��λ�������λ���ӵ���Сʱ��
				ucPhaseIndex - �г�����λindex 0 - 15
Output:         ��
Return:         ��
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
		m_bAddTimeCount = false;  //���ⷢ���ظ��ĵ���ʱ��Ϣ
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
Description:    ��Ӧ���ƣ���ȡ�׶εĶ�Ӧ��Ȧ ȥ���ں�׶λ�����ֵ���λ,������ͬ�ĸ�
				����λȥ��ǰһ���׶εĵ���λ
Input:          iStageNo     - �׶κ�
Output:         ��
Return:         ��
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
	
	//��ͨ��λ
	while ( ucPhaseIndex < MAX_PHASE )
	{
		if ( (iCurAllowPhase>>ucPhaseIndex & 1) 
			&& (iNextAllowPhase>>ucPhaseIndex & 1) )  //��һ���׶�Ҳ���ڸ���λ��ȥ������λ
		{
			uiTmp = 0;
			uiTmp |= ~(1<<ucPhaseIndex);
			iCurAllowPhase  = iCurAllowPhase & uiTmp;
		}
		ucPhaseIndex++;
	}

	//������λ
	while ( ucOverlapPhaseIndex < MAX_OVERLAP_PHASE )
	{
		if ( (iCurOverlapPhase>>ucOverlapPhaseIndex & 1) 
		  && (iNextOverlapPhase>>ucOverlapPhaseIndex & 1) )  //��һ���׶�Ҳ���ڸ���λ��ȥ������λ
		{
			ucDelPhase = OverlapPhaseToPhase(iCurAllowPhase,ucOverlapPhaseIndex);  //���ݸ�����λ��ȡ������λ���±�
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
Description:    ��Ӧ���ƣ���ȡ���н׶εĶ�Ӧ��Ȧ��һ������һ��
Input:          iStageNo     - �׶κ�
Output:         ��
Return:         ��
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
Description:    ��ȡĳ����������λ�������С�̣����ڽ׶��л�ʱ��֤������С��
Input:          iStepNo - ������
Output:         ��
Return:         ��
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
Description:    ָ���ض�����ʱ������
Input:          iTimePatternId - ָ������ʱ������
Output:         ��
Return:         ��
**********************************************************************************/
void CManaKernel::SpecTimePattern(int iTimePatternId )
{
	if ( iTimePatternId != m_pRunData->ucTimePatternId )   //�¸����ڻ���ô˷�����
	{
		SetUpdateBit();
	}
	m_iTimePatternId = iTimePatternId;

	ACE_DEBUG((LM_DEBUG,"%s:%d \n@@@@@@@@@@%d\n",__FILE__,__LINE__,m_iTimePatternId));
}


/*********************************************************************************
Function:       CManaKernel::GetVehilePara
Description:    ��ȡ�����Ӧ���������õ�ֵ
Input:          ��
Output:         pbVehile     - �Ƿ�Ϊ��Ӧ����        bDefStep    - Ŀǰ�����µĲ��� 
*       	    piAdjustTime - ÿ�εĵ���ʱ��        piCurPhase  - ��ǰ����ִ�е���λ�� 
*       	    piNextPhase  - �¸�����ִ�е���λ��  psPhaseDet  - ��λ�������Ĺ�ϵ  
Return:         ��
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
	
	/*��ȡ��һ����λ����ֻ�ڰ��Ӧ�����õ�*/
	iIndex = m_pRunData->ucStepNo + 1;  //Ѱ����һ����λ��
	while ( iIndex < m_pRunData->ucStepNum )
	{
		if ( IsLongStep(iIndex) )
		{
			ucNextStage    = StepToStage(iIndex,NULL);
			ucNextPhaseTmp = m_uiStagePhase[ucNextStage];
			if ( IsMainPhaseGrp(*piCurPhase) != IsMainPhaseGrp(ucNextPhaseTmp) ) //�� �� ��������
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
Description:    �ж�ĳ����λ���Ƿ�Ϊ����λ�飬����������λ�����ڰ��Ӧ����
Input:          uiPhase -��λ�������
Output:         ��
Return:         true - ������ false - ��������
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
				if ( (m_pTscConfig->sDetector[iDetId].ucOptionFlag>>1) & 1 )  //��ͨ������Ϊ�ؼ���λ
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
Description:    �жϸ�ͨ���Ƿ�����
Input:          iChannelId : ͨ����
Output:         ��
Return:         true - ���� false - δ����
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
Description:    �ж���λ�Ƿ��ͻ���������̳�ͻ
Input:          ��
Output:         ��
Return:         true - ��ͻ false - δ��ͻ
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
		if ( (uiAllowPhase >> iIndex) & 1 )  //���ڸ���λ
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
Description:    ������ֵ��̳�ͻ
Input:          ucdata   - ����ֵ
Output:         ��
Return:         ��
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
		*((Byte*)sTscMsg.pDataBuf) = CTRL_MANUAL;  //�ֶ�
		CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));

		SThreadMsg sTscMsgSts;
		sTscMsgSts.ulType       = TSC_MSG_SWITCH_STATUS;  
		sTscMsgSts.ucMsgOpt     = 0;
		sTscMsgSts.uiMsgDataLen = 1;
		sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
		*((Byte*)sTscMsgSts.pDataBuf) = FLASH;  //����
		CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
	}
	
}


/*********************************************************************************
Function:       CManaKernel::AllotActiveTime
Description:    ����Ӧ���Ƹ��ݸ���������ϸ����ڵ�ռ���ʣ������̵�ʱ��
Input:          ��
Output:         ��
Return:         ��
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
	unsigned int iStageHaveCarTime[MAX_SON_SCHEDULE] = {0};   //�����׶������õļ�������г�ʱ��
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
						iStageHaveCarTime[iIndex] += (pVehicleStat+iDetId-1)->ucOccupancy;     //��ȡռ����
					}
					//zIndex++;
				//}
				 
			}
			jIndex++;
		}
		
		iGreenTimeSum      += m_pRunData->sStageStepInfo[iStepNo].ucStepLen;   //�̵�������ʱ��
		ulTotalHaveCarTime += iStageHaveCarTime[iIndex];       //���г�ʱ��      
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

		m_pRunData->sStageStepInfo[iStepNo].ucStepLen = iStepGreenTime;  //�޸Ľ׶ε��̵�ʱ��
		iIndex++;
	}
}


/*********************************************************************************
Function:       CManaKernel::UtcsAdjustCycle
Description:    Э�����ƣ����������ṩ�Ĺ�������ʱ���������ڵ���������ٲ�
Input:          ��
Output:         ��
Return:         ��
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

	//ƽ�ֵ�������
	for ( int i = 0; i < MAX_STEP && i < m_pRunData->ucStepNum; i++ ) 
	{
		if ( !IsLongStep(i) )
		{
			continue;
		}

		ucAdjustGreenLen[i] = iAdjustCnt / ucStageCnt;
	}
	iAdjustCnt -= ucStageCnt * ( iAdjustCnt / ucStageCnt );

	//����Լ��
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

		if ( iAdjustPerStep < 0 )  //Ԥ����С�̻�����̵����ô���
		{
			iAdjustPerStep = m_pRunData->sStageStepInfo[i].ucStepLen / MAX_ADJUST_CYCLE;
		}

		if ( ucAdjustGreenLen[i] > iAdjustPerStep )  //������Χ����
		{
			iAdjustCnt         += ( ucAdjustGreenLen[i] - iAdjustPerStep );
			ucAdjustGreenLen[i] = iAdjustPerStep;
		}
		else
		{
			iAdjustPerStep = iAdjustPerStep - ucAdjustGreenLen[i];
		}

		if ( iAdjustPerStep > iAdjustCnt )  //�������
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
Description:    ����ͨ��ID�������õĵƿذ�
Input:          ��
Output:         bUseLampBoard: true-���� false-û������
Return:         ��
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
		if ( GetUseLampBoard(iIndex) )  //�õƿذ������õ�ͨ��
		{
			*(bUseLampBoard+iIndex) = true;
		}
		iIndex++;
	}
}


/*********************************************************************************
Function:       CManaKernel::GetUseLampBoard
Description:    �жϵƿذ��Ƿ����������Ϣ
Input:          iLampBoard - �ƿذ� 0 , 1 , 2 , 3
Output:         ��
Return:         ��
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
Description:    �жϼ�����Ƿ��ڵ�ǰ������
Input:          ucDetId -�����ID��
Output:         ��
Return:         true - �ڵ�ǰ����  false -���ڵ�ǰ����
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
Description:     �����־��Ϣ���������ڷ��͸���д��־��Ϣ				
Input:          ucLogType  ��־����
				ucLogVau1  ��־ֵ1
				ucLogVau2  ��־ֵ2
				ucLogVau3  ��־ֵ3
				ucLogVau4  ��־ֵ4              
Output:         ��
Return:         ��
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
Description:    ��֤����ĺϷ���			
Input:          ��    
Output:         ��
Return:         ��
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
