#include "GaCountDown.h"
#include "Rs485.h"
#include "ManaKernel.h"
#include "DbInstance.h"

//与上位机通信协议 -- 相位与方向表中方向的代号
Byte CGaCountDown::m_ucGaDirVector[GA_MAX_DIRECT][GA_MAX_LANE] = 
{
	// 左, 直, 右, 人行
	{ 33,  34,  36,  40},  //北
	{ 65,  66,  68,  72},  //东
	{ 97,  98, 100, 104},  //南
	{129, 130, 132, 136}   //西	
};


/**************************************************************
Function:       CGaCountDown::CGaCountDown
Description:    CGaCountDown，用于倒计时类初始化处理				
Input:          无              
Output:         无
Return:         无
***************************************************************/
CGaCountDown::CGaCountDown()
{
	ACE_OS::memset(m_sGaSendBuf , 0 , GA_MAX_SEND_BUF );

	ACE_DEBUG((LM_DEBUG,"%s:%d Init GaCountDown object ok !\n",__FILE__,__LINE__));
}

/**************************************************************
Function:       CGaCountDown::~CGaCountDown
Description:    CGaCountDown类	析构函数	
Input:          无              
Output:         无
Return:         无
***************************************************************/
CGaCountDown::~CGaCountDown()
{
	ACE_DEBUG((LM_DEBUG,"%s:%d Destruct GaCountDown object ok !\n",__FILE__,__LINE__));
}

/**************************************************************
Function:       CGaCountDown::CreateInstance
Description:    创建CGaCountDown静态对象
Input:          无              
Output:         无
Return:         静态对象指针
***************************************************************/
CGaCountDown* CGaCountDown::CreateInstance()
{
	static CGaCountDown oGaCountDown;

	return &oGaCountDown;
}


/**************************************************************
Function:       CGaCountDown::GaGetCntDownInfo
Description:    判断各个方向/属性(左、直、右与人行)是否需要倒计时
				以及计算对应的倒计时秒数和灯色信息
Input:          无              
Output:         无
Return:         无
***************************************************************/
void CGaCountDown::GaGetCntDownInfo()
{
	CManaKernel* pCWorkParaManager = CManaKernel::CreateInstance();
	STscRunData* pRunData               = CManaKernel::CreateInstance()->m_pRunData;
	Byte ucCurStep                      = pRunData->ucStepNo;
	SStepInfo*   pStepInfo              = pRunData->sStageStepInfo + ucCurStep;
	
	bool bIsAllowPhase  = false;
	Byte ucPhaseId      = 0;
	Byte ucSignalGrpNum = 0;
	Byte ucLightLamp    = 0;
	Byte ucDirIndex     = 0;
	Byte ucLaneIndex    = 0;
	Byte ucSignalGrp[MAX_CHANNEL] = {0};

	for ( ucDirIndex=0; ucDirIndex<GA_MAX_DIRECT; ucDirIndex++ )
	{
		for ( ucLaneIndex=0; ucLaneIndex<GA_MAX_LANE; ucLaneIndex++ )
		{
			//方向+属性-->相位类型+相位id  
			if ( m_sGaPhaseToDirec[ucDirIndex][ucLaneIndex].ucOverlapPhase != 0 )
			{
				ucPhaseId     = m_sGaPhaseToDirec[ucDirIndex][ucLaneIndex].ucOverlapPhase;
				bIsAllowPhase = false;
			}
			else
			{
				ucPhaseId     = m_sGaPhaseToDirec[ucDirIndex][ucLaneIndex].ucPhase;
				bIsAllowPhase = true;
			}
			
			//相位类型+相位id-->通道信息(ryg)
			ucSignalGrpNum = 0;
			pCWorkParaManager->GetSignalGroupId(bIsAllowPhase ,ucPhaseId,&ucSignalGrpNum,ucSignalGrp);

			if ( ucSignalGrpNum > 0 ) //相位对应通道个数大于0
			{
				ucLightLamp = (ucSignalGrp[0] - 1) * 3;  //ucLightLamp 通道组号，下面判断当前通道组号亮什么灯。
				if ( 1 == pStepInfo->ucLampOn[ucLightLamp] )
				{
					m_ucGaColor[ucDirIndex][ucLaneIndex] = GA_COLOR_RED;
				}
				else if ( 1 == pStepInfo->ucLampOn[ucLightLamp+1] )
				{
					m_ucGaColor[ucDirIndex][ucLaneIndex] = GA_COLOR_YELLOW;
					ucLightLamp = ucLightLamp + 1;
				}
				else if ( 1 == pStepInfo->ucLampOn[ucLightLamp+2] )
				{
					m_ucGaColor[ucDirIndex][ucLaneIndex] = GA_COLOR_GREEN;
					ucLightLamp = ucLightLamp + 2;
				}
				m_bGaNeedCntDown[ucDirIndex][ucLaneIndex] = true; //相位方向上有放行相位和通道 或者有跟随相位有跟随通道
				m_ucGaRuntime[ucDirIndex][ucLaneIndex]    = GaGetCntTime(ucLightLamp); //获取剩余时间
			}
			else
			{
				m_bGaNeedCntDown[ucDirIndex][ucLaneIndex] = false;//该方向上无相位无通道则无倒计时，跟随相位无跟随通道则无倒计时
			}
		}
	}
}


/**************************************************************
Function:       CGaCountDown::GaSendStepPer
Description:    每一次步伐开始发送一次
Input:          无              
Output:         无
Return:         无
***************************************************************/
void CGaCountDown::GaSendStepPer()
{
	GaGetCntDownInfo();
	GaSetSendBuffer();
	//Byte buff[6]="Andy\n";
	for ( int iDir=0; iDir<GA_MAX_DIRECT; iDir++ )
	{
		CRs485::CreateInstance()->Send(m_sGaSendBuf[iDir], GA_MAX_SEND_BUF);
		//CRs485::CreateInstance()->Send(buff, GA_MAX_SEND_BUF);
	}
}


/**************************************************************
Function:       CGaCountDown::GaSetSendBuffer
Description:    构造发送数据
Input:          无              
Output:         无
Return:         无
***************************************************************/
void CGaCountDown::GaSetSendBuffer()
{
	Byte ucDirIndex  = 0;
	Byte ucColor     = GA_COLOR_RED;
	Byte ucCountTime = 0;

	STscConfig* pTscCfg = CManaKernel::CreateInstance()->m_pTscConfig;

	char sDirec[4][48] = {"NORTH","EAST","SOUTH","WEST"};
	char sColor[3][48] = {"RED","YELLOW","GREEN"};

	if ( ( (pTscCfg->sSpecFun[FUN_PRINT_FLAGII].ucValue) & 1 ) != 0 )
	{
		ACE_DEBUG((LM_DEBUG,"\n%s:%d ",GACOUNTDOWN_CPP, __LINE__));
	}

	for ( ucDirIndex=0; ucDirIndex<GA_MAX_DIRECT; ucDirIndex++ )
	{
		//获取某一方向倒计时牌的颜色与秒数
		ComputeColorCount(ucDirIndex,ucColor,ucCountTime);
		//ACE_DEBUG((LM_DEBUG,"%s:%d  Direc=%d color=%d colortime=%d \n",GACOUNTDOWN_CPP, __LINE__,ucDirIndex,ucColor,ucCountTime));
		if ( ( (pTscCfg->sSpecFun[FUN_PRINT_FLAGII].ucValue) & 1 ) != 0 )
		{
			ACE_DEBUG((LM_DEBUG," %s-%s-%d",sDirec[ucDirIndex],sColor[ucColor],ucCountTime));
		}
	
		//打包数据
		PackSendBuf(ucDirIndex,ucColor,ucCountTime);
	}

	if ( ( (pTscCfg->sSpecFun[FUN_PRINT_FLAGII].ucValue) & 1 ) != 0 )
	{
		ACE_DEBUG((LM_DEBUG,"\n"));
	}
	
}


/**************************************************************
Function:       CGaCountDown::ComputeColorCount
Description:    根据方向计算某方向的倒计时颜色和总秒数,不考虑人行
				倒计时优先级：有绿倒绿 无绿倒黄 同绿同黄同红倒最小
Input:          ucDirIndex-方向(0 1 2 3)              
Output:         ucColor-颜色(r y g)
		        ucCountTime-秒数
Return:         false-失败  true-成功
***************************************************************/
bool CGaCountDown::ComputeColorCount( Byte ucDirIndex , Byte& ucColor , Byte& ucCountTime )
{
	bool bRed           = false;
	bool bYellow        = false;
	bool bGreen         = false;
	Byte ucLaneIndex    = 0;
	Byte ucMinRedCnt    = 254;
	Byte ucMinYellowCnt = 254;
	Byte ucMinGreenCnt  = 254;

	if ( ucDirIndex >= GA_MAX_DIRECT )
	{
		return false;
	}

	//左 直  右  不算人行
	for ( ucLaneIndex=0; ucLaneIndex<GA_LANE_FOOT; ucLaneIndex++ )
	{
		if ( false == m_bGaNeedCntDown[ucDirIndex][ucLaneIndex] ) 
		{
			continue;
		}
		switch ( m_ucGaColor[ucDirIndex][ucLaneIndex] )
		{
			case GA_COLOR_GREEN:
				bGreen = true;
				if ( ucMinGreenCnt > m_ucGaRuntime[ucDirIndex][ucLaneIndex] )
				{
					ucMinGreenCnt = m_ucGaRuntime[ucDirIndex][ucLaneIndex]; 
				}
				break;
			case GA_COLOR_YELLOW:
				bYellow = true;
				if ( ucMinYellowCnt > m_ucGaRuntime[ucDirIndex][ucLaneIndex] )
				{
					ucMinYellowCnt = m_ucGaRuntime[ucDirIndex][ucLaneIndex]; 
				}
				break;
			case GA_COLOR_RED:
				bRed = true;
				if ( ucMinRedCnt > m_ucGaRuntime[ucDirIndex][ucLaneIndex] )
				{
					ucMinRedCnt = m_ucGaRuntime[ucDirIndex][ucLaneIndex]; 
				}
				break;
			default:
				break;
		}
	}

	if ( bGreen )  //绿灯倒计时
	{
		ucColor     = GA_COLOR_GREEN;
		ucCountTime = ucMinGreenCnt;
	}
	else if ( bYellow )  //如不倒黄灯 在此处做限制
	{
		ucColor     = GA_COLOR_YELLOW;
		ucCountTime = ucMinYellowCnt;
	}
	else if ( bRed )
	{
		ucColor     = GA_COLOR_RED;
		ucCountTime = ucMinRedCnt;
	}
	else
	{
		return false;
	}

	return true;
}


/**************************************************************
Function:       CGaCountDown::PackSendBuf
Description:    打包发送的数据
Input:          ucDirIndex-方向(0 1 2 3)              
Output:         ucColor-颜色(0r 1y 2g)
*        		ucCountTime-秒数
Return:         false-失败  true-成功
***************************************************************/
bool CGaCountDown::PackSendBuf( Byte ucDirIndex , Byte ucColor , Byte ucCountTime )
{
	const Byte ucFrameHead = 0xFE;    //规定帧头
	static Byte sColorGaDef[4] = {3,2,1,0};  //index: 0-r 1-y 2-g 3-无
	                                         //value: 3-r 2-y 1-g 0-无
	static Byte sDirGaDef[8]= {0,1,2,3,4,5,6,7};  //index：0-北 1-东 2-南 3-西 4 5 6 7
				                      			  //value: 0-北 1-东 2-南 3-西 4 5 6 7
	Byte ucColorDir = 0;
	Byte ucThousand = 0;
	Byte ucHundred  = 0;
	Byte ucTen      = 0;
	Byte ucEntries  = 0;
	Byte ucCheckSum = 0;

	if ( ucDirIndex > GA_MAX_DIRECT	|| ucColor > 4 )
	{
		return false;
	}

	ACE_OS::memset(m_sGaSendBuf[ucDirIndex] , 0 , GA_MAX_SEND_BUF);

	m_sGaSendBuf[ucDirIndex][0] = ucFrameHead;   //帧头

	ucColorDir |= sColorGaDef[ucColor];       //D0-D1 表示颜色
	ucColorDir |= (sDirGaDef[ucDirIndex]<<2); //D2-D4 表示方向
	ucColorDir |= 0x20;                       //D5位为1  D6-D7扩展
	m_sGaSendBuf[ucDirIndex][1] = ucColorDir;

	ucThousand = ucCountTime / 1000;
	ucHundred  = (ucCountTime %1000)/100;
	ucTen      = ((ucCountTime%1000)%100)/10;
	ucEntries  = ucCountTime % 10;
	m_sGaSendBuf[ucDirIndex][2] |= ucHundred;
	m_sGaSendBuf[ucDirIndex][2] |= (ucThousand<<4);
	m_sGaSendBuf[ucDirIndex][3] |= ucEntries;
	m_sGaSendBuf[ucDirIndex][3] |= (ucTen<<4);

	ucCheckSum = 0x7f & (m_sGaSendBuf[ucDirIndex][1] ^ m_sGaSendBuf[ucDirIndex][2] ^ m_sGaSendBuf[ucDirIndex][3]);
	m_sGaSendBuf[ucDirIndex][4] = ucCheckSum;
	//ACE_DEBUG((LM_DEBUG,"%s:%d ContDown data :Direc:%d  FE %02X %02X %02X  %02X \n" ,__FILE__,__LINE__,
	//ucDirIndex ,m_sGaSendBuf[ucDirIndex][1],m_sGaSendBuf[ucDirIndex][2],m_sGaSendBuf[ucDirIndex][3],
	//m_sGaSendBuf[ucDirIndex][4]));


	return true;
}


/**************************************************************
Function:       CGaCountDown::GaGetCntTime
Description:    根据通道获取该通道同一灯色剩余的持续时间
Input:          ucSignalId: 通道Id  1-16            
Output:         无
Return:         该灯色的剩余时间
***************************************************************/
Byte CGaCountDown::GaGetCntTime(Byte ucSignalId)
{
	STscRunData* pRunData  = CManaKernel::CreateInstance()->m_pRunData;
	Byte ucCurStep         = pRunData->ucStepNo;	
	SStepInfo*   pStepInfo = NULL;	
	Byte ucRuntime         = 0;
	
	if ( pRunData->ucStepTime >= pRunData->ucElapseTime )
	{
		ucRuntime = pRunData->ucStepTime - pRunData->ucElapseTime;
	}
	else
	{
		ucRuntime = 0;
	}

	for (int iStep = ((ucCurStep+1) % pRunData->ucStepNum); iStep != ucCurStep; //其他步伐该灯色还会亮，比如通道的红灯可能会除了通道组绿灯步外其他步伐时候也一直亮
		                         iStep = (iStep + 1) % pRunData->ucStepNum) 
	{
		pStepInfo = pRunData->sStageStepInfo + iStep;
		if ( 1 == pStepInfo->ucLampOn[ucSignalId] ) 
		{
			ucRuntime += pStepInfo->ucStepLen;
		} 
		else 
		{
			break;
		}
	}

	return ucRuntime;
}


/**************************************************************
Function:       CGaCountDown::GaUpdateCntDownCfg
Description:    更新倒计时配置参数 数据库更新时需要调用
Input:          无         
Output:         无
Return:         无
***************************************************************/
void CGaCountDown::GaUpdateCntDownCfg()
{
	Byte ucRecordCnt = 0;
	Byte ucDirIndex  = 0;
	Byte ucLaneIndex = 0;
	int iIndex       = 0;

	GBT_DB::PhaseToDirec*   pPhaseToDirec = NULL;    
	GBT_DB::TblPhaseToDirec tTblPhaseToDirec;

	/******方向与相位对应表******/
	(CDbInstance::m_cGbtTscDb).QueryPhaseToDirec(tTblPhaseToDirec);
	pPhaseToDirec = tTblPhaseToDirec.GetData(ucRecordCnt);
	ACE_OS::memset(m_sGaPhaseToDirec  , 0   , (GA_MAX_DIRECT*GA_MAX_LANE)*sizeof(GBT_DB::PhaseToDirec));
	iIndex = 0;
	while ( iIndex < ucRecordCnt )
	{
		GaGetDirLane(pPhaseToDirec[iIndex].ucId , ucDirIndex , ucLaneIndex );
		if ( ucDirIndex < GA_MAX_DIRECT && ucLaneIndex < GA_MAX_LANE )
		{
			ACE_OS::memcpy(&m_sGaPhaseToDirec[ucDirIndex][ucLaneIndex]
			                       ,&pPhaseToDirec[iIndex],sizeof(GBT_DB::PhaseToDirec));
		}
		iIndex++;
	}
}


/**************************************************************
Function:       CGaCountDown::GaUpdateCntDownCfg
Description:    获取方向与转向  如：左 直
Input:          ucTableId - 方向与相位表的id         
Output:         ucDir     - 方向
*        		ucLane    - 转向
Return:         无
***************************************************************/
void CGaCountDown::GaGetDirLane(Byte ucTableId , Byte& ucDir , Byte& ucLane)
{	

	Byte ucDirCfg        = (ucTableId >> 5) & 0x07;
	Byte ucCrossWalkFlag = (ucTableId >> 3) & 0x03;
	Byte ucVelhileFlag   = ucTableId & 0x07;

	switch ( ucDirCfg )
	{
		case 1:
			ucDir = GA_DIRECT_NORTH; //北
			break;
		case 2:
			ucDir = GA_DIRECT_EAST; //东
			break;
		case 3:
			ucDir = GA_DIRECT_SOUTH; //南
			break;
		case 4:
			ucDir =  GA_DIRECT_WEST; //西
			break;
		default:
			ucDir  = GA_MAX_DIRECT;
			break;
	}

	if ( ucCrossWalkFlag )
	{
		ucLane = GA_LANE_FOOT;
	}
	else if ( ucVelhileFlag & 0x1 )
	{
		ucLane = GA_LANE_LEFT;
	}
	else if ( ucVelhileFlag & 0x2 )
	{
		ucLane = GA_LANE_STRAIGHT;
	}
	else if ( ucVelhileFlag & 0x4 )
	{
		ucLane = GA_LANE_RIGHT;
	}
	else
	{
		ucLane = GA_MAX_LANE;
	}
}

/**************************************************************
Function:       CGaCountDown::sendblack
Description:    用于发送倒计时黑屏
Input:          无         
Output:         无
Return:         无
***************************************************************/
void CGaCountDown::sendblack()
{
	for ( int iDir=0; iDir<GA_MAX_DIRECT; iDir++ )
	{
		PackSendBuf( iDir , 3 , 99) ;			
		CRs485::CreateInstance()->Send(m_sGaSendBuf[iDir], GA_MAX_SEND_BUF);
		//CRs485::CreateInstance()->Send(buff, GA_MAX_SEND_BUF);
	}

}

/**************************************************************
Function:       CGaCountDown::send8cntdown
Description:    用于感应控制时的8秒倒计时发送
Input:          无         
Output:         无
Return:         无
***************************************************************/
void CGaCountDown:: send8cntdown()
{
	CManaKernel * p_ManaKernel = CManaKernel::CreateInstance();
	bool bSend8CntDwon[GA_MAX_DIRECT] ={false};
	Byte iStepNum = p_ManaKernel->m_pRunData->ucStepNo ;
	Uint iAllowPhase = p_ManaKernel->m_pRunData->sStageStepInfo[iStepNum].uiAllowPhase ;
	int iNdex = 0 ;
	Byte iColor = 0 ;
	Byte iCostTime = 0 ;
	ACE_OS::printf("%s:%d StepNum= %d iAllowPhase = %d \n",__FILE__,__LINE__,iStepNum,iAllowPhase);
	
	while(iNdex < MAX_PHASE)
	{
		if ( (iAllowPhase>>iNdex) & 1 )
		{
			for(int iDirec = 0 ;iDirec < GA_MAX_DIRECT ;iDirec++)
			{
				for(int iLan = 0 ;iLan < (GA_MAX_LANE-1) ;iLan++)
				{
					if((iNdex+1) == m_sGaPhaseToDirec[iDirec][iLan].ucPhase)
					{
						bSend8CntDwon[iDirec] = true ;
						ACE_OS::printf("%s:%d Phase= %d Direc = %d \n",__FILE__,__LINE__,iNdex+1,iDirec);
					}
				}
			}
				
		}

		iNdex++;
	}
	iNdex = 0 ;
	ACE_OS::printf("%s:%d Begin Send 8 cntdown \n",__FILE__,__LINE__);
	 while(iNdex < GA_MAX_DIRECT)
	 {
		if(bSend8CntDwon[iNdex] == true)
		{
			PackSendBuf( iNdex , 2 , 8 ) ;			
			CRs485::CreateInstance()->Send(m_sGaSendBuf[iNdex], GA_MAX_SEND_BUF);
		}
		else
		{	
			
			ComputeColorCount(iNdex,iColor,iCostTime);
			PackSendBuf( iNdex , 0 , iCostTime-8 ) ;			
			CRs485::CreateInstance()->Send(m_sGaSendBuf[iNdex], GA_MAX_SEND_BUF);
		}
		iNdex++;	
	 }
			
	}

	
	

