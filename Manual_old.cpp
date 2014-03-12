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
void Manual::DoManual() //100msµ÷ÓÃÒ»´Î
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
				
				switch(i) //Ñ­»·6´Î
				{
					case 0:
						if((key_value& (1<<i)))  //down  ºóÖ´ÐÐ
						{
							ACE_DEBUG((LM_DEBUG,"key 1 MAC_CTRL_NEXT_STEP\n"));
							if(m_ucManual==1)
							{
								m_ucManualSts = MAC_CTRL_NEXT_STEP;
							}else
							{
								
								;//m_ucManualSts = MAC_CTRL_ALLOFF;
							
								ACE_DEBUG((LM_DEBUG,"²»ÊÇÊÖ¿Ø°´ÏÂkey 6 \n"));
							}
						}
						break;
					case 1:
						if((key_value& (1<<i))||(last_kval&(1<<i)))//down ºóÖ´ÐÐ
						{
							ACE_DEBUG((LM_DEBUG,"key 2 manual control \n"));
							if (m_ucManual==1)
							{
								m_ucManual = 0;
								ACE_DEBUG((LM_DEBUG,"ÊÖ¿Ø¸ÄÎª×Ô¶¯ÔËÐÐ \n"));
								m_ucManualSts = MAC_CTRL_NOTHING;
							}
							else
							{
								m_ucManual = 1;
								ACE_DEBUG((LM_DEBUG,"×Ô¶¯ÔËÐÐ¸ÄÎªÊÖ¶¯¿ØÖÆ \n"));
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
								//²»ÊÇÊÖ¿ØµÄÊ±ºò£¬°´ÏÂ5
								
									ACE_DEBUG((LM_DEBUG,"²»ÊÇÊÖ¿Ø°´ÏÂkey 5 \n"));								
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
		//ACE_DEBUG((LM_DEBUG,"ÐÅºÅ»ú¿ØÖÆ×´Ì¬Î´·¢Éú¸Ä±ä!\n"));
		if(m_ucManualSts == MAC_CTRL_NEXT_STEP && (last_kval == 0x1 || last_kval == 0x3))
		{
			m_ucManualSts == MAC_CTRL_NEXT_STEP;//²½½ø
		}

		else if(m_ucManual ==1)
		 {
			deadmanual++ ;
			if(deadmanual >3000) //Îå·ÖÖÓÊÖ¿ØÎÞ±ä»¯£¬½øÈë×Ô¶¯×´Ì¬
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

	
	if ( 0 == m_ucManual  && MAC_CTRL_NOTHING == m_ucManualSts )  //ÎÞ²Ù×÷ÇÒ·ÇÊÖ¶¯×´Ì¬ÏÂ
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
	else if ( MAC_CTRL_ALLOFF == m_ucManualSts ) //Ãæ°å¹ØµÆ
	{
		if ( MAC_CTRL_ALLOFF == m_ucLastManualSts ) //µ±Ç°ÒÑ¾­ÊÇÃæ°å¹ØµÆ
		{
			//return;
			 m_ucManualSts =  MAC_CTRL_NOTHING; //MODå:2013 0602 1145
		
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
	else if ( MAC_CTRL_ALLRED == m_ucManualSts )  //Ãæ°åÈ«ºì
	{
		if ( MAC_CTRL_ALLRED == m_ucLastManualSts ) //µ±Ç°ÒÑ¾­ÊÇÃæ°åÈ«ºì
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
	else if ( MAC_CTRL_FLASH == m_ucManualSts )  //Ãæ°å»ÆÉÁ
	{
		if ( MAC_CTRL_FLASH == m_ucLastManualSts ) //µ±Ç°ÒÑ¾­ÊÇÃæ°å»ÆÉÁ
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
	else if ( 1 == m_ucManual )  //ÊÖ¶¯
	{
		if ( 0 == m_ucLastManual )  //ÉÏ´Î·ÇÊÖ¶¯
		{
			sTscMsg.ulType       = TSC_MSG_SWITCH_CTRL;
			sTscMsg.ucMsgOpt     = 0;
			sTscMsg.uiMsgDataLen = 1;
			sTscMsg.pDataBuf     = ACE_OS::malloc(1);
			*((Byte*)sTscMsg.pDataBuf) = CTRL_PANEL;
			CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));
			ACE_DEBUG((LM_DEBUG,"%s:%d CTRL_PANEL \n",__FILE__,__LINE__));
		}

		if ( MAC_CTRL_NEXT_STEP == m_ucManualSts )  //²½½ø
		{
			sTscMsgSts.ulType       = TSC_MSG_LOCK_STEP;  //°´²½·¥Ç°½ø
			sTscMsgSts.ucMsgOpt     = 0;
			sTscMsgSts.uiMsgDataLen = 1;
			sTscMsgSts.pDataBuf     = ACE_OS::malloc(1);
			*((Byte*)sTscMsgSts.pDataBuf) = 0;
			CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsgSts,sizeof(sTscMsgSts));
			ACE_DEBUG((LM_DEBUG,"%s:%d MAC_CTRL_NEXT_STEP \n",__FILE__,__LINE__));
		}
		else if ( MAC_CTRL_NEXT_PHASE == m_ucManualSts )  //ÏÂÒ»½×¶Î  ÏÂÒ»ÏàÎ»
		{
			sTscMsg.ulType       = TSC_MSG_NEXT_STAGE;  //°´½×¶ÎÇ°½ø
			sTscMsg.ucMsgOpt     = 0;
			sTscMsg.uiMsgDataLen = 1;
			sTscMsg.pDataBuf     = NULL;
			CTscMsgQueue::CreateInstance()->SendMessage(&sTscMsg,sizeof(sTscMsg));
			ACE_DEBUG((LM_DEBUG,"%s:%d MAC_CTRL_NEXT_PHASE \n",__FILE__,__LINE__));
		}
		else if ( MAC_CTRL_NEXT_DIR == m_ucManualSts )  //ÏÂÒ»·½Ïò
		{
			//´ý²¹
			ACE_DEBUG((LM_DEBUG,"%s:%d MAC_CTRL_NEXT_DIR \n",__FILE__,__LINE__));
		}
	}
	else if ( 0 == m_ucManual )  //È¥³ýÊÖ¶¯
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

	m_ucLastManualSts = m_ucManualSts;  //±£´æµ±Ç°µÄÊÖ¿Ø×´Ì¬
	m_ucLastManual    = m_ucManual;    //±£´æµ±Ç°µÄ¿ØÖÆ·½Ê½

}

