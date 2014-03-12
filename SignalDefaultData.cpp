#include "SignalDefaultData.h"
#include "GbtDb.h"
#include "DbInstance.h"

SignalDefaultData::SignalDefaultData()
{
}
void SignalDefaultData::InsertNtcipDefaultData()
{}
void SignalDefaultData::InsertGbDefaultData()
{
	GBT_DB::TblPlan tblPlan;
	GBT_DB::Plan    sPlan;

	GBT_DB::TblSchedule tblSchedule;
	GBT_DB::Schedule    sSchedule;

	GBT_DB::TblPattern tblPattern;
	GBT_DB::Pattern    sPattern;

	GBT_DB::TblStagePattern tblStage;
	GBT_DB::StagePattern    sStage;

	GBT_DB::TblPhase tblPhase;
	GBT_DB::Phase    sPhase;

	GBT_DB::TblOverlapPhase tblOverlapPhase;
	GBT_DB::OverlapPhase    sOverlapPhase;

	GBT_DB::TblChannel tblChannel;
	GBT_DB::Channel    sChannel;

	GBT_DB::TblDetector tblDetector;
	GBT_DB::Detector    sDetector;

	GBT_DB::TblCollision tblCollision;
	GBT_DB::Collision    sCollision;

	GBT_DB::TblModule tblModule;
	GBT_DB::Module    sModule;

	GBT_DB::TblSpecFun tblSpecFun;
	GBT_DB::SpecFun    sSpecFun;

	GBT_DB::TblDetExtend tblDetExtend;
	GBT_DB::DetExtend    sDetExtend;
	
	GBT_DB::TblPhaseToDirec tblPhaseToDirec;
	GBT_DB::PhaseToDirec sPhaseToDirec;

	GBT_DB::TblCntDownDev tblCntDownDev;
	GBT_DB::CntDownDev sCntDownDev;

	GBT_DB::TblAdaptPara tblAdaptPara;
	GBT_DB::AdaptPara sAdaptPara;
	
	
	GBT_DB::TblChannelChk tblChannelChk;
	GBT_DB::ChannelChk    sChannelChk;


	/********************ͨ�����ݼ�����ñ�*****************************/
	(CDbInstance::m_cGbtTscDb).QueryChannelChk(tblChannelChk);
	if(0 == tblChannelChk.GetCount())
	{
		for(int i = 0 ;i<MAX_LAMP;i++)
		{
			sChannelChk.ucSubChannelId= i+1;
			sChannelChk.ucIsCheck = 0 ;
			(CDbInstance::m_cGbtTscDb).AddChannelChk(sChannelChk.ucSubChannelId,sChannelChk);
		}
	}
	/**************���Ĭ�ϵ��ȼƻ�����***********/
	(CDbInstance::m_cGbtTscDb).QueryPlan(tblPlan);
	if ( 0 == tblPlan.GetCount() )
	{
		sPlan.ucId         = 1;
		sPlan.usMonthFlag  = 0xffff;               
		sPlan.ucWeekFlag   = 0xff;               
		sPlan.ulDayFlag    = 0xffffffff;               
		sPlan.ucScheduleId = 1;

		(CDbInstance::m_cGbtTscDb).AddPlan(1,sPlan);   
	}

	/**************���Ĭ��ʱ�α�����***********/
	(CDbInstance::m_cGbtTscDb).QuerySchedule(tblSchedule);
	if ( 0 == tblSchedule.GetCount() )
	{
		sSchedule.ucScheduleId = 1;                
		sSchedule.ucEvtId      = 1;                     
		sSchedule.ucBgnHour    = 8;                   
		sSchedule.ucBgnMinute  = 0;                
		sSchedule.ucCtrlMode   = 0;  //0:����   1:�ص� 2:���� 3:ȫ��  4�����߰��Ӧ 5:���߰��Ӧ 6:ȫ��Ӧ
		                             //7:�޵����� 8:����Ӧ����  12:����
		sSchedule.ucPatternNo  = 1;              
		sSchedule.ucAuxOut     = 0;                   
		sSchedule.ucSpecialOut = 0;               
		(CDbInstance::m_cGbtTscDb).AddSchedule(sSchedule.ucScheduleId,sSchedule.ucEvtId,sSchedule);
	}

		/**************��ʱ������***********/
	(CDbInstance::m_cGbtTscDb).QueryPattern(tblPattern);
	if ( 0 == tblPattern.GetCount() )
	{
		sPattern.ucPatternId = 1; 
		sPattern.ucCycleTime = 120; 
		sPattern.ucOffset    = 100;      
		sPattern.ucCoorPhase = 15;             
		sPattern.ucStagePatternId = 1;  
		(CDbInstance::m_cGbtTscDb).AddPattern(sPattern.ucPatternId,sPattern);
		
		sPattern.ucPatternId = 2;             
		sPattern.ucStagePatternId = 1;  
		(CDbInstance::m_cGbtTscDb).AddPattern(sPattern.ucPatternId,sPattern);

		sPattern.ucPatternId = 3;             
		sPattern.ucStagePatternId = 1;  
		(CDbInstance::m_cGbtTscDb).AddPattern(sPattern.ucPatternId,sPattern);
	}

#if 1  //��ͨ��λ��ʱ��
	/**************�׶���ʱ������***********/
	(CDbInstance::m_cGbtTscDb).QueryStagePattern(tblStage);
	if ( 0 == tblStage.GetCount() )
	{
		sStage.ucStagePatternId = 1;      
		sStage.ucStageNo        = 1;            
		sStage.usAllowPhase     = 257;
		sStage.ucGreenTime      = 13;           
		sStage.ucYellowTime     = 3;         
		sStage.ucRedTime        = 0;             
		sStage.ucOption         = 0;
		(CDbInstance::m_cGbtTscDb).AddStagePattern(sStage.ucStagePatternId,
			sStage.ucStageNo, sStage);

		sStage.ucStageNo        = 2;            
		sStage.usAllowPhase     = 34438;
		sStage.ucGreenTime      = 14;          
		(CDbInstance::m_cGbtTscDb).AddStagePattern(sStage.ucStagePatternId,
			sStage.ucStageNo, sStage);

		sStage.ucStageNo        = 3;            
		sStage.usAllowPhase     = 4112;
		sStage.ucGreenTime      = 15;          
		(CDbInstance::m_cGbtTscDb).AddStagePattern(sStage.ucStagePatternId,
			sStage.ucStageNo, sStage);

		sStage.ucStageNo        = 4;            
		sStage.usAllowPhase     = 26728;
		sStage.ucGreenTime      = 16;          
		(CDbInstance::m_cGbtTscDb).AddStagePattern(sStage.ucStagePatternId,
			sStage.ucStageNo, sStage);
	}

	/**************��λ��***********/
	(CDbInstance::m_cGbtTscDb).QueryPhase(tblPhase);
	if ( 0 == tblPhase.GetCount() )
	{
		sPhase.ucPhaseId         = 1;                 
		sPhase.ucPedestrianGreen = 0;           
		sPhase.ucPedestrianClear = 0;          
		sPhase.ucMinGreen        = 7;                
		sPhase.ucGreenDelayUnit  = 3;          
		sPhase.ucMaxGreen1       = 40;                
		sPhase.ucMaxGreen2       = 60;                
		sPhase.ucFixGreen        = 12;                 
		sPhase.ucGreenFlash      = 3; 
		//sPhase.ucPhaseTypeFlag   = 0x40;    //������λ
		//sPhase.ucPhaseTypeFlag   = 0x80;  //�̶���λ
		sPhase.ucPhaseTypeFlag   = 32;    //������λ
		sPhase.ucPhaseOption     = 0;               
		sPhase.ucExtend          = 0;
		sPhase.ucPedestrianGreen = 0; 
		sPhase.ucGreenFlash      = 2; 

		for ( int i=1; i<(MAX_PHASE+1); i++ )
		{
			sPhase.ucPhaseId = i; 

			if ( (4 == sPhase.ucPhaseId) || (8 == sPhase.ucPhaseId) 
				|| (12 == sPhase.ucPhaseId) || (16 == sPhase.ucPhaseId)  )  //�0�6�0�9�0�4�0�4
			{
				sPhase.ucPhaseOption = 2; 
			}
			else
			{
				sPhase.ucPhaseOption = 0; 
			}
			
			(CDbInstance::m_cGbtTscDb).AddPhase(sPhase.ucPhaseId, sPhase);
		}
	}

	/**************������λ��***********/
	(CDbInstance::m_cGbtTscDb).QueryOverlapPhase(tblOverlapPhase);
	ACE_OS::memset(&sOverlapPhase,0,sizeof(GBT_DB::OverlapPhase));
	if ( 0 == tblOverlapPhase.GetCount() )
	{
		sOverlapPhase.ucOverlapPhaseId = 1;         
		sOverlapPhase.ucIncldPhaseCnt  = 0;
		sOverlapPhase.ucTailGreen      = 0;
		sOverlapPhase.ucTailYellow     = 0;
		sOverlapPhase.ucTailRed        = 0;
		(sOverlapPhase.ucIncldPhase)[0] = 0; 
		(sOverlapPhase.ucIncldPhase)[1] = 0;
		(CDbInstance::m_cGbtTscDb).AddOverlapPhase(sOverlapPhase.ucOverlapPhaseId,sOverlapPhase);
	}

	/**************��ͻ��λ��***********/
	(CDbInstance::m_cGbtTscDb).QueryCollision(tblCollision);
	ACE_OS::memset(&sCollision , 0 , sizeof(GBT_DB::Collision) );
	if ( 0 == tblCollision.GetCount() )
	{
		/*
		sCollision.ucPhaseId       = 1;
		sCollision.usCollisionFlag = 0x4;
		(CDbInstance::m_cGbtTscDb).AddCollision(sCollision.ucPhaseId, sCollision);
		*/
	}
	
	/**************ͨ����***********/
	(CDbInstance::m_cGbtTscDb).QueryChannel(tblChannel);
	ACE_OS::memset(&sChannel,0,sizeof(GBT_DB::Channel));
	sChannel.ucCtrlType = 1;
	if ( 0 == tblChannel.GetCount() )
	{
		for (int i=0; i<16; i++ )
		{
			sChannel.ucChannelId = 1 + i; 
			sChannel.ucCtrlSrc   = 1 + i; 
			if((i+1)%4 != 0)
				sChannel.ucCtrlType  = CHANNEL_VEHICLE;
			else
				sChannel.ucCtrlType  = CHANNEL_FOOT;
			(CDbInstance::m_cGbtTscDb).AddChannel(sChannel.ucChannelId,sChannel);
		}
	}
#else  //������λ

	/**************���Ĭ�Ͻ׶���ʱ��***********/
	(CDbInstance::m_cGbtTscDb).QueryStagePattern(tblStage);
	if ( 0 == tblStage.GetCount() )
	{
		sStage.ucStagePatternId = 1;      
		sStage.ucStageNo        = 1;            
		sStage.usAllowPhase     = 0x1;
		sStage.ucGreenTime      = 13;           
		sStage.ucYellowTime     = 3;         
		sStage.ucRedTime        = 0;             
		sStage.ucOption         = 0;
		(CDbInstance::m_cGbtTscDb).AddStagePattern(sStage.ucStagePatternId,
			sStage.ucStageNo, sStage);

		sStage.ucStageNo        = 2;            
		sStage.usAllowPhase     = 0x8;
		sStage.ucGreenTime      = 15;          
		(CDbInstance::m_cGbtTscDb).AddStagePattern(sStage.ucStagePatternId,
			sStage.ucStageNo, sStage);

		sStage.ucStageNo        = 2;            
		sStage.usAllowPhase     = 0x6;
		sStage.ucGreenTime      = 17;          
		(CDbInstance::m_cGbtTscDb).AddStagePattern(sStage.ucStagePatternId,
			sStage.ucStageNo, sStage);
	}

	/**************�����λ����***********/
	(CDbInstance::m_cGbtTscDb).QueryPhase(tblPhase);
	if ( 0 == tblPhase.GetCount() )
	{
		sPhase.ucPhaseId         = 1;                 
		sPhase.ucPedestrianGreen = 0;           
		sPhase.ucPedestrianClear = 0;          
		sPhase.ucMinGreen        = 7;                
		sPhase.ucGreenDelayUnit  = 3;          
		sPhase.ucMaxGreen1       = 40;                
		sPhase.ucMaxGreen2       = 60;                
		sPhase.ucFixGreen        = 12;                 
		sPhase.ucGreenFlash      = 3;               
		sPhase.ucPhaseTypeFlag   = 0x80;             
		sPhase.ucPhaseOption     = 0;               
		sPhase.ucExtend          = 0;                    

		for ( int i=1; i<(MAX_PHASE+1); i++ )
		{
			sPhase.ucPhaseId = i; 
			(CDbInstance::m_cGbtTscDb).AddPhase(sPhase.ucPhaseId, sPhase);
		}
	}

	/**************���Ĭ�ϸ�����λ����***********/
	(CDbInstance::m_cGbtTscDb).QueryOverlapPhase(tblOverlapPhase);
	ACE_OS::memset(&sOverlapPhase,0,sizeof(GBT_DB::OverlapPhase));
	if ( 0 == tblOverlapPhase.GetCount() )
	{
		sOverlapPhase.ucOverlapPhaseId = 1;         
		sOverlapPhase.ucIncldPhaseCnt = 2;
		sOverlapPhase.ucTailGreen     = 0;
		sOverlapPhase.ucTailYellow    = 0;
		sOverlapPhase.ucTailRed       = 0;
		(sOverlapPhase.ucIncldPhase)[0] = 1; 
		(sOverlapPhase.ucIncldPhase)[1] = 4;
		(CDbInstance::m_cGbtTscDb).AddOverlapPhase(sOverlapPhase.ucOverlapPhaseId,sOverlapPhase);

		sOverlapPhase.ucOverlapPhaseId   = 2;         
		(sOverlapPhase.ucIncldPhase)[0] = 3; 
		(sOverlapPhase.ucIncldPhase)[1] = 4;
		(CDbInstance::m_cGbtTscDb).AddOverlapPhase(sOverlapPhase.ucOverlapPhaseId,sOverlapPhase);

		sOverlapPhase.ucOverlapPhaseId   = 3;                 
		(sOverlapPhase.ucIncldPhase)[0] = 2; 
		(sOverlapPhase.ucIncldPhase)[1] = 4;
		(CDbInstance::m_cGbtTscDb).AddOverlapPhase(sOverlapPhase.ucOverlapPhaseId,sOverlapPhase);
	}

	/**************���Ĭ����λ��ͻ����***********/
	(CDbInstance::m_cGbtTscDb).QueryCollision(tblCollision);
	ACE_OS::memset(&sCollision , 0 , sizeof(GBT_DB::Collision) );
	if ( 0 == tblCollision.GetCount() )
	{
		;
		/*
		sCollision.ucPhaseId       = 1;
		sCollision.usCollisionFlag = 0x4;
		(CDbInstance::m_cGbtTscDb).AddCollision(sCollision.ucPhaseId, sCollision);
		*/
	}
	
	/**************���Ĭ��ͨ������***********/
	(CDbInstance::m_cGbtTscDb).QueryChannel(tblChannel);
	ACE_OS::memset(&sChannel,0,sizeof(GBT_DB::Channel));
	sChannel.ucCtrlType = 1;
	if ( 0 == tblChannel.GetCount() )
	{
		for (int i=0; i<3; i++ )
		{
			sChannel.ucChannelId = 1 + i; 
			sChannel.ucCtrlSrc   = 1 + i; 
			sChannel.ucCtrlType  = CHANNEL_VEHICLE;
			(CDbInstance::m_cGbtTscDb).AddChannel(sChannel.ucChannelId,sChannel);
		}

		for (int i=3; i<6; i++ )
		{
			sChannel.ucChannelId = 1 + i; 
			sChannel.ucCtrlSrc   = 1 + i; 
			sChannel.ucCtrlType  = CHANNEL_OVERLAP;
			(CDbInstance::m_cGbtTscDb).AddChannel(sChannel.ucChannelId,sChannel);
		}
		
	}
#endif
	
	/**************************���Ĭ��Ӳ��ģ��************************/
	(CDbInstance::m_cGbtTscDb).QueryModule(tblModule);

	if ( 0 == tblModule.GetCount() )
	{		
		sModule.ucModuleId = 1;
		sModule.strDevNode.SetString("DET-0-1");
		sModule.strModel.SetString("hd");
		sModule.strVersion.SetString("V1.0");   
		sModule.ucType = 2;
		(CDbInstance::m_cGbtTscDb).ModModule(sModule.ucModuleId,  sModule);

		sModule.ucModuleId = 2;
		sModule.strDevNode.SetString("DET-1-17");							
		sModule.strModel.SetString("hd");
		sModule.strVersion.SetString("V1.0");   
		sModule.ucType = 2;
		(CDbInstance::m_cGbtTscDb).ModModule(sModule.ucModuleId,  sModule);
		sModule.ucModuleId = 3;
		sModule.strDevNode.SetString("DET-2-0");
		sModule.strModel.SetString("hd");
		sModule.strVersion.SetString("V1.0");   
		sModule.ucType = 2;
		(CDbInstance::m_cGbtTscDb).ModModule(sModule.ucModuleId,  sModule);

		sModule.ucModuleId = 4;
		sModule.strDevNode.SetString("DET-3-0");							
		sModule.strModel.SetString("hd");
		sModule.strVersion.SetString("V1.0");   
		sModule.ucType = 2;
		(CDbInstance::m_cGbtTscDb).ModModule(sModule.ucModuleId,  sModule);
	}

	/************************���Ĭ���ض�����**************************/
	(CDbInstance::m_cGbtTscDb).QuerySpecFun(tblSpecFun);

	if ( tblSpecFun.GetCount() == 0 )
	{
		sSpecFun.ucFunType = FUN_SERIOUS_FLASH + 1;
		sSpecFun.ucValue   = 0;
		(CDbInstance::m_cGbtTscDb).ModSpecFun(sSpecFun.ucFunType , sSpecFun.ucValue);
		
		sSpecFun.ucFunType = FUN_COUNT_DOWN + 1;
		sSpecFun.ucValue   = 0;
		(CDbInstance::m_cGbtTscDb).ModSpecFun(sSpecFun.ucFunType , sSpecFun.ucValue);

		sSpecFun.ucFunType = FUN_GPS + 1;
		(CDbInstance::m_cGbtTscDb).ModSpecFun(sSpecFun.ucFunType , sSpecFun.ucValue);
 
		sSpecFun.ucFunType = FUN_MSG_ALARM + 1;
		(CDbInstance::m_cGbtTscDb).ModSpecFun(sSpecFun.ucFunType , sSpecFun.ucValue);

		
	   (CDbInstance::m_cGbtTscDb).ModSpecFun((FUN_CROSS_TYPE     + 1) , 0);  //0-tsc 1-psc1  2-psc2
	   (CDbInstance::m_cGbtTscDb).ModSpecFun((FUN_STAND_STAGEID  + 1) , 1);
	   (CDbInstance::m_cGbtTscDb).ModSpecFun((FUN_CORSS1_STAGEID + 1) , 2);
	   (CDbInstance::m_cGbtTscDb).ModSpecFun((FUN_CROSS2_STAGEID + 1) , 3);

	   (CDbInstance::m_cGbtTscDb).ModSpecFun((FUN_TEMPERATURE    + 1) , 0);
	   (CDbInstance::m_cGbtTscDb).ModSpecFun((FUN_VOLTAGE        + 1) , 0);
	   (CDbInstance::m_cGbtTscDb).ModSpecFun((FUN_DOOR           + 1) , 0);
	   (CDbInstance::m_cGbtTscDb).ModSpecFun((FUN_COMMU_PARA     + 1) , 0);
	   (CDbInstance::m_cGbtTscDb).ModSpecFun((FUN_PORT_LOW       + 1) , 59);
	   (CDbInstance::m_cGbtTscDb).ModSpecFun((FUN_PORT_HIGH      + 1) , 21);

	   (CDbInstance::m_cGbtTscDb).ModSpecFun((FUN_PRINT_FLAG    + 1) , 0);
	   (CDbInstance::m_cGbtTscDb).ModSpecFun((FUN_PRINT_FLAGII  + 1) , 0);
		(CDbInstance::m_cGbtTscDb).ModSpecFun((FUN_CAM           + 1) , 0);
	   (CDbInstance::m_cGbtTscDb).ModSpecFun((FUN_3G            + 1) , 0);
	   (CDbInstance::m_cGbtTscDb).ModSpecFun((FUN_WLAN          + 1) , 0);
	}

	/*******************���Ĭ�ϼ������չ����*******************/
	(CDbInstance::m_cGbtTscDb).QueryDetExtend(tblDetExtend);

	if ( tblDetExtend.GetCount() == 0 )
	{
		sDetExtend.ucId         = 1;
		sDetExtend.ucSensi      = 6;
		sDetExtend.ucGrpNo      = 1;
		sDetExtend.ucPro        = 100;
		sDetExtend.ucOcuDefault = 0;
		sDetExtend.usCarFlow    = 0;
		(CDbInstance::m_cGbtTscDb).AddDetExtend(sDetExtend);

		sDetExtend.ucId         = 2;
		sDetExtend.ucSensi      = 7;
		sDetExtend.ucGrpNo      = 1;
		sDetExtend.ucPro        = 100;
		sDetExtend.ucOcuDefault = 0;
		sDetExtend.usCarFlow    = 0;
		(CDbInstance::m_cGbtTscDb).AddDetExtend(sDetExtend);
	}

	/*******************�����Ĭ�Ϸ�������λ��Ӧ����*******************/
	(CDbInstance::m_cGbtTscDb).QueryPhaseToDirec(tblPhaseToDirec);

	if ( tblPhaseToDirec.GetCount() == 0 )
	{
		Byte tmp[MAX_PHASE] = { 33,  34,  36,  40 ,  //����
		                        65,  66,  68,  72 ,  //�0�9�0�0
		                        97,  98, 100, 104 ,  //�0�2�0�3
		                        129, 130, 132, 136}; //�0�2��	
		for (int i=0; i<MAX_PHASE; i++)
		{
			sPhaseToDirec.ucId    = tmp[i];
			sPhaseToDirec.ucPhase = i + 1;
			sPhaseToDirec.ucOverlapPhase = 0;
			sPhaseToDirec.ucRoadCnt      = 1;
			(CDbInstance::m_cGbtTscDb).AddPhaseToDirec(sPhaseToDirec.ucId,sPhaseToDirec);
		}
	}

	/*******************���������Ӧ����*******************/
	(CDbInstance::m_cGbtTscDb).QueryAdaptPara(tblAdaptPara);

	if ( tblAdaptPara.GetCount() == 0 )
	{
		sAdaptPara.ucType       = 1;
		sAdaptPara.ucFirstPro   = 20;
		sAdaptPara.ucSecPro     = 30;
		sAdaptPara.ucThirdPro   = 50;
		sAdaptPara.ucOcuPro     = 70;
		sAdaptPara.ucCarFlowPro = 30;
		sAdaptPara.ucSmoothPara = 0;
		
		(CDbInstance::m_cGbtTscDb).AddAdaptPara(sAdaptPara);
	}

	/*******************��ӵ���ʱ�豸����*******************/
	(CDbInstance::m_cGbtTscDb).QueryCntDownDev(tblCntDownDev);

	if ( tblCntDownDev.GetCount() == 0 )
	{
		sCntDownDev.ucDevId = 1;
		sCntDownDev.usPhase = 1;
		sCntDownDev.ucOverlapPhase = 0;
		sCntDownDev.ucMode  = 3;
		(CDbInstance::m_cGbtTscDb).AddCntDownDev(sCntDownDev);
	}

}
