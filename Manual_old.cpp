/*
 * Manual.cpp
 *
 *  Created on: Apr 12, 2013
 *      Author: root
 */
#include "Manual.h"
#include "TscMsgQueue.h"
#include "ace/Date_Time.h"
static int key_value = 0;
static int runtimes = 0 ;
static int deadmanual = 0 ;
Manual::Manual() 
{
	m_ucManualSts     = MAC_CTRL_NOTHING;
	m_ucManual        = 0;

	m_ucLastManual    = 0;
	m_ucLastManualSts = MAC_CTRL_NOTHING;
	runtimes = 0;
	OpenDev();
}
Manual* Manual::CreateInstance()
{
	static Manual cManual;
	return &cManual;
}
Manual::~Manual() 
{
	// TODO Auto-generated destructor stub
}
void Manual::OpenDev()
{
	m_iManualFd = open(DEV_MANUAL_KEYINT, O_RDONLY | O_NONBLOCK);
	if(-1 == m_iManualFd)
	{
		ACE_DEBUG((LM_DEBUG,"%s:%d  open DEV_MANUAL_KEYINT error!\n",__FILE__,__LINE__)); //MOD:0517 15:10
	}
	else
	{
		ACE_DEBUG((LM_DEBUG,"%s:%d  open DEV_MANUAL_KEYINT success!\n",__FILE__,__LINE__));
	
	}
}
void Manual::DoManual() //100ms调用一次
{
	runtimes++;
	//int iResult;	//result
	int ret;
    fd_set rfds;
    int last_kval = key_value;
	//int key_value[6];
	if ( m_iManualFd < 0 )
	{
		close(m_iManualFd);
		OpenDev();
	}
	
    FD_ZERO(&rfds);
      
    FD_SET(m_iManualFd,&rfds);
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 10;
	if(-1 == (ret = select(m_iManualFd + 1,&rfds,NULL,NULL,&tv)))
    {
       	ACE_DEBUG((LM_DEBUG,"select error\n"));
        	//_exit(EXIT_FAILURE);
       	return ;
   	}
	if(FD_ISSET(m_iManualFd,&rfds))
    {
       int iNum = read(m_iManualFd, &key_value, sizeof(key_value));
	   if(iNum == -1)
	   {
			ACE_DEBUG((LM_DEBUG,"read form m_iManualFd error!\n"));

	   }
		ACE_DEBUG((LM_DEBUG,"key_valuse =  %d\n",key_value));
      
       	for(int i = 0;i < 6;i ++ )
        {			
              	if((key_value & (1 << i)) != (last_kval & (1 << i)))
              	{
                     	ACE_DEBUG((LM_DEBUG,"KEY%d: %s (key_value=%d)(last_kval=%d)\n", i+1,(key_value& (1<<i))? "DOWN": "UP", key_value,last_kval));
				
				switch(i) //循环6次
				{
					case 0:
						if((key_value& (1<<i)))  //down  后执行
						{
							ACE_DEBUG((LM_DEBUG,"key 1 MAC_CTRL_NEXT_STEP\n"));
							if(m_ucManual==1)
							{
								m_ucManualSts = MAC_CTRL_NEXT_STEP;
							}else
							{
								
								;//m_ucManualSts = MAC_CTRL_ALLOFF;
							
								ACE_DEBUG((LM_DEBUG,"不是手控按下key 6 \n"));
							}
						}
						break;
					case 1:
						if((key_value& (1<<i))||(last_kval&(1<<i)))//down 后执行
						{
							ACE_DEBUG((LM_DEBUG,"key 2 manual control \n"));
							if (m_ucManual==1)
							{
								m_ucManual = 0;
								ACE_DEBUG((LM_DEBUG,"手控改为自动运行 \n"));
								m_ucManualSts = MAC_CTRL_NOTHING;
							}
							else
							{
								m_ucManual = 1;
								ACE_DEBUG((LM_DEBUG,"自动运行改为手动控制 \n"));
							}
						}
						break;
					case 2:
						if((key_value& (1<<i)) || (last_kval&(1<<i)) )
						{
							ACE_DEBUG((LM_DEBUG,"key 3 MAC_CTRL_FLASH \n"));
							if(m_ucManualSts != MAC_CTRL_FLASH)
							{
								m_ucManualSts = MAC_CTRL_FLASH;
							}
							else
							{
								m_ucManualSts = MAC_CTRL_NOTHING;
							}
						}
						break;
					case 3:
						if((key_value& (1<<i))||(last_kval&(1<<i)))
							{
							ACE_DEBUG((LM_DEBUG,"key 4 MAC_CTRL_ALLRED \n"));
							
							if(m_ucManualSts != MAC_CTRL_ALLRED)
								{
									m_ucManualSts = MAC_CTRL_ALLRED;
								}
							else
								{
									m_ucManualSts = MAC_CTRL_NOTHING;
								}
							}
						break;
					case 4:
						if((key_value& (1<<i)))
							{
							ACE_DEBUG((LM_DEBUG,"key 5  MAC_CTRL_ALLOFF \n"));
							if(m_ucManualSts != MAC_CTRL_ALLOFF)
								{
									m_ucManualSts = MAC_CTRL_ALLOFF;
								}
							else
								{
									m_ucManualSts = MAC_CTRL_NOTHING;
								}
							}
						break;
					case 5:
						if((key_value& (1<<i)))
							{
							ACE_DEBUG((LM_DEBUG,"key 6 MAC_CTRL_NEXT_PHASE \n"));
							if(m_ucManual==1)
								{
									m_ucManualSts = MAC_CTRL_NEXT_PHASE;
								}
							else
								{
								//不是手控的时候，按下5
								
									ACE_DEBUG((LM_DEBUG,"不是手控按下key 5 \n"));								
								}
						break;						
					default:
						break;
				}
				              
             		}
             	
  			}
		
       }
		   last_kval = key_value;

	}	   

	if( runtimes%20 == 0)
	{
		ACE_DEBUG((LM_DEBUG,"runtimes = %d ,%s:%d  m_ucManualSts = %d  m_ucManual = %d \n",runtimes,__FILE__,__LINE__,m_ucManualSts,m_ucManual));
	}
//	m_ucManualSts = 3;
//	m_ucManual = 0;
	SThreadMsg sTscMsg;
	SThreadMsg sTscMsgSts;
	SThreadMsg sTscMsgLog;

	ACE_OS::memset( &sTscMsg    , 0 , sizeof(SThreadMsg) );
	ACE_OS::memset( &sTscMsgSts , 0 , sizeof(SThreadMsg) );
	ACE_OS::memset( &sTscMsgLog , 0 , sizeof(SThreadMsg) );

	if ( m_ucManualSts == m_ucLastManualSts   && m_ucManual == m_ucLastManual )
	{
		//ACE_DEBUG((LM_DEBUG,"信号机控制状态未发生改变!\n"));
		if(m_ucManualSts == MAC_CTRL_NEXT_STEP && (last_kval == 0x1 || last_kval == 0x3))
		{
			m_ucManualSts == MAC_CTRL_NEXT_STEP;//步进
		}

		else if(m_ucManual ==1)
		 {
			deadmanual++ ;
			if(deadmanual >3000) //五分钟手控无变化，进入自动状态
			{
				 m_ucManual = 0 ;
				 m_ucManualSts =  MAC_CTRL_NOTHING;
				 deadmanual = 0;
			}
			else
				return ;
		 }
			
		else
			return;
	}

	ACE_DEBUG((LM_DEBUG,"%s:%d Manual:%d ManualSts:%d\n",__FILE__,__LINE__,m_ucManual,m_ucManualSts));

	
	if ( 0 == m_ucManual  && MAC_CTRL_NOTHING == m_ucManualSts )  //无操作且非手动状态下
	{
		sTscMsgSts.ulType       = TSC_MSG_SWITCH_CTRL;
		sTscMsgSts.ucMsgOpt     = 0;
		sTscMsgSts.uiMsgDataLen = 1;
		sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
		*((Byte*)sTscMsgSts.pDataBuf) = CTRL_SCHEDULE;
		CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));

		sTscMsgSts.ulType       = TSC_MSG_SWITCH_STATUS;
		sTscMsgSts.ucMsgOpt     = 0;
		sTscMsgSts.uiMsgDataLen = 1;
		sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
		*((Byte*)sTscMsgSts.pDataBuf) = STANDARD;
		CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));

		ACE_DEBUG((LM_DEBUG,"%s:%d CTRL_SCHEDULE STANDARD \n",__FILE__,__LINE__));
	}
	else if ( MAC_CTRL_ALLOFF == m_ucManualSts ) //面板关灯
	{
		if ( MAC_CTRL_ALLOFF == m_ucLastManualSts ) //当前已经是面板关灯
		{
			//return;
			 m_ucManualSts =  MAC_CTRL_NOTHING; //MOD?:2013 0602 1145
		
		}
		else
		{

		sTscMsg.ulType       = TSC_MSG_SWITCH_CTRL;
		sTscMsg.ucMsgOpt     = 0;
		sTscMsg.uiMsgDataLen = 1;
		sTscMsg.pDataBuf     = ACE_OS::malloc(1);
		*((Byte*)sTscMsg.pDataBuf) = CTRL_PANEL;
		CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));

		sTscMsgSts.ulType       = TSC_MSG_SWITCH_STATUS;
		sTscMsgSts.ucMsgOpt     = 0;
		sTscMsgSts.uiMsgDataLen = 1;
		sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
		*((Byte*)sTscMsgSts.pDataBuf) = SIGNALOFF;
		CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));

		ACE_DEBUG((LM_DEBUG,"%s:%d CTRL_PANEL SIGNALOFF \n",__FILE__,__LINE__));
		}
	}
	else if ( MAC_CTRL_ALLRED == m_ucManualSts )  //面板全红
	{
		if ( MAC_CTRL_ALLRED == m_ucLastManualSts ) //当前已经是面板全红
		{
			return;
		}

		sTscMsg.ulType       = TSC_MSG_SWITCH_CTRL;
		sTscMsg.ucMsgOpt     = 0;
		sTscMsg.uiMsgDataLen = 1;
		sTscMsg.pDataBuf     = ACE_OS::malloc(1);
		*((Byte*)sTscMsg.pDataBuf) = CTRL_PANEL;
		CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));

		sTscMsgSts.ulType       = TSC_MSG_SWITCH_STATUS;
		sTscMsgSts.ucMsgOpt     = 0;
		sTscMsgSts.uiMsgDataLen = 1;
		sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
		*((Byte*)sTscMsgSts.pDataBuf) = ALLRED;
		CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));

		ACE_DEBUG((LM_DEBUG,"%s:%d CTRL_PANEL ALLRED \n",__FILE__,__LINE__));
	}
	else if ( MAC_CTRL_FLASH == m_ucManualSts )  //面板黄闪
	{
		if ( MAC_CTRL_FLASH == m_ucLastManualSts ) //当前已经是面板黄闪
		{
			return;
		}

		sTscMsg.ulType       = TSC_MSG_SWITCH_CTRL;
		sTscMsg.ucMsgOpt     = 0;
		sTscMsg.uiMsgDataLen = 1;
		sTscMsg.pDataBuf     = ACE_OS::malloc(1);
		*((Byte*)sTscMsg.pDataBuf) = CTRL_PANEL;
		CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));

		sTscMsgSts.ulType       = TSC_MSG_SWITCH_STATUS;
		sTscMsgSts.ucMsgOpt     = 0;
		sTscMsgSts.uiMsgDataLen = 1;
		sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
		*((Byte*)sTscMsgSts.pDataBuf) = FLASH;
		CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));

		ACE_DEBUG((LM_DEBUG,"%s:%d CTRL_PANEL ALLRED \n",__FILE__,__LINE__));
	}
	else if ( 1 == m_ucManual )  //手动
	{
		if ( 0 == m_ucLastManual )  //上次非手动
		{
			sTscMsg.ulType       = TSC_MSG_SWITCH_CTRL;
			sTscMsg.ucMsgOpt     = 0;
			sTscMsg.uiMsgDataLen = 1;
			sTscMsg.pDataBuf     = ACE_OS::malloc(1);
			*((Byte*)sTscMsg.pDataBuf) = CTRL_PANEL;
			CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));
			ACE_DEBUG((LM_DEBUG,"%s:%d CTRL_PANEL \n",__FILE__,__LINE__));
		}

		if ( MAC_CTRL_NEXT_STEP == m_ucManualSts )  //步进
		{
			sTscMsgSts.ulType       = TSC_MSG_LOCK_STEP;  //按步伐前进
			sTscMsgSts.ucMsgOpt     = 0;
			sTscMsgSts.uiMsgDataLen = 1;
			sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
			*((Byte*)sTscMsgSts.pDataBuf) = 0;
			CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
			ACE_DEBUG((LM_DEBUG,"%s:%d MAC_CTRL_NEXT_STEP \n",__FILE__,__LINE__));
		}
		else if ( MAC_CTRL_NEXT_PHASE == m_ucManualSts )  //下一阶段  下一相位
		{
			sTscMsg.ulType       = TSC_MSG_NEXT_STAGE;  //按阶段前进
			sTscMsg.ucMsgOpt     = 0;
			sTscMsg.uiMsgDataLen = 1;
			sTscMsg.pDataBuf     = NULL;
			CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));
			ACE_DEBUG((LM_DEBUG,"%s:%d MAC_CTRL_NEXT_PHASE \n",__FILE__,__LINE__));
		}
		else if ( MAC_CTRL_NEXT_DIR == m_ucManualSts )  //下一方向
		{
			//待补
			ACE_DEBUG((LM_DEBUG,"%s:%d MAC_CTRL_NEXT_DIR \n",__FILE__,__LINE__));
		}
	}
	else if ( 0 == m_ucManual )  //去除手动
	{
		if ( 0 == m_ucLastManual )
		{
			return;
		}
		sTscMsgSts.ulType       = TSC_MSG_SWITCH_CTRL;
		sTscMsgSts.ucMsgOpt     = 0;
		sTscMsgSts.uiMsgDataLen = 1;
		sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
		*((Byte*)sTscMsgSts.pDataBuf) = CTRL_SCHEDULE;
		CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));

		ACE_DEBUG((LM_DEBUG,"%s:%d stop manual \n",__FILE__,__LINE__));
	}

	m_ucLastManualSts = m_ucManualSts;  //保存当前的手控状态
	m_ucLastManual    = m_ucManual;    //保存当前的控制方式

}

