
/***************************************************************
Copyright(c) 2013  AITON. All rights reserved.
Author:     AITON
FileName:   ComFunc.cpp
Date:       2013-9-25
Description:通用处理函数
Version:    V1.0
History:    201310081456  yiodng testmem()
***************************************************************/
#include<sys/ioctl.h>
#include <fcntl.h>

#include "ComFunc.h"
#include "ManaKernel.h"
#include "DbInstance.h"

/**************************************************************
Function:       IsLeapYear
Description:    判断是否时润年
Input:          ucYear  年份              
Output:         无
Return:         false-非润年 true-润年
***************************************************************/
bool IsLeapYear(Uint ucYear)
{
	if ( ( 0==ucYear%4 && ucYear%100!=0)||(0==ucYear%400) )
	{
		return true;
	}
	return false;
}

/**************************************************************
Function:       IsRightDate
Description:    判断日期到合法性
Input:          ucYear  年份
				ucMonth 月份 
				ucDay   日期             
Output:         无
Return:         false-非润年 true-润年
***************************************************************/
bool IsRightDate(Uint ucYear,Byte ucMonth,Byte ucDay)
{
	Byte ucMaxDay = 31;

	if ( ucYear < 2000 || ucYear >= 2038 )
	{
		return false;
	}
	if ( ucMonth < 1 || ucMonth > 12 )
	{
		return false;
	}
	
	switch ( ucMonth )
	{
		case 2:
			if ( IsLeapYear(ucYear) )  //闰年
			{
				ucDay = 29;
			}
			else
			{
				ucDay = 28;
			}
			break;
		case 4:
			ucMaxDay = 30;
			break;
		case 6:
			ucMaxDay = 30;
			break;
		case 9:
			ucMaxDay = 30;
			break;
		case 11:
			ucMaxDay = 30;
			break;
		default:
			break;
	}

	if ( ucDay > ucMaxDay )
	{
		return false;
	}
	return true;

}

/**************************************************************
Function:       getCurrTime
Description:    获取当前时间，这个时间获取函数主要用于信号机定时器，
				避免修改时间到时候造成定时器停止
Input:          无   
Output:         无
Return:         ACE时间
***************************************************************/
ACE_Time_Value getCurrTime()
{
	ACE_Time_Value tv;
	struct timespec ts = {0};
	::clock_gettime(CLOCK_MONOTONIC, &ts);
	ACE_hrtime_t hrt = static_cast<ACE_hrtime_t> (ts.tv_sec) * ACE_U_ONE_SECOND_IN_NSECS+ static_cast<ACE_hrtime_t> (ts.tv_nsec);
	ACE_High_Res_Timer::hrtime_to_tv (tv, hrt);
	return tv;
}

/**************************************************************
Function:       GetCurTime
Description:    获取当前时间，用于除定时器时间获取外其他地方
Input:          无   
Output:         无
Return:         ACE时间
***************************************************************/
ACE_Time_Value GetCurTime()
{
#ifdef LINUX
	time_t ti;
	struct tm rtctm;
	int fd = open(DEV_RTC, O_WRONLY, 0);
	int ret = -1;
	if(fd>0)
	{
		ret = ioctl(fd, RTC_RD_TIME, &rtctm);
		close(fd);
	}
	ti = mktime(&rtctm);
	return ACE_Time_Value(ti);
#else
	return ACE_OS::gettimeofday();
#endif
}

/**************************************************************
Function:       CopyFile
Description:    复制文件
Input:          无   
Output:         无
Return:         无
***************************************************************/
void CopyFile(char src[], char dest[])
{
#ifdef LINUX	
	int sd, dd;
	char buf[100];
	int n;

	if (strcmp(dest, src) == 0)
		return;

	sd = open(src, O_RDONLY);
	dd = open(dest, O_CREAT | O_WRONLY | O_TRUNC, 0644);
	if (sd == -1 || dd == -1)
		goto out;

	while ((n = read(sd, buf, sizeof(buf))) > 0) {
		write(dd, buf, n);
	}

out:
	if (sd != -1)
		close(sd);
	if (dd != -1)
		close(dd);
#endif
}


/**************************************************************
Function:       CopyFile
Description:    调整文件的大小，超过iMaxFileLine时，删除掉一半，
				防止文件过大
Input:          无   
Output:         无
Return:         无
***************************************************************/
void AdjustFileSize(char* file,int iMaxFileLine)
{
#ifdef LINUX
	FILE *fp;
	FILE *fp2;
	int line = 0;
	int ch;
	if ((fp = fopen(file, "r")) == NULL)
	{
		return;
	}
	while ( !feof(fp) ) 
	{
		ch = fgetc(fp);
		if (ch == '\n')
		{
			line++;
		}
	}
	if ( line < iMaxFileLine ) 
	{
		fclose(fp);
		return;
	}
	fseek(fp, 0, SEEK_SET);
	while (!feof(fp) && ( line > iMaxFileLine / 2 ) ) 
	{
		ch = fgetc(fp);
		if (ch == '\n')
		{
			line--;
		}
	}
	if ( (fp2 = fopen("tmp.txt", "w")) != NULL) 
	{
		while ((ch = fgetc(fp)) != EOF) 
		{
			fputc(ch, fp2);
		}
		fclose(fp2);
		fclose(fp);
		CopyFile((char*)"tmp.txt", file);
		remove((char*)"tmp.txt");
	} 
	else 
	{
		fclose(fp);
	}
#endif
}


/**************************************************************
Function:       RecordTscStartTime
Description:    记录系统开始运行时间，并写入日志			
Input:          无        
Output:         无
Return:         无
***************************************************************/
void RecordTscStartTime()
{
#ifdef LINUX
	FILE* file = NULL;
	char tmp[64] = {0};
	struct tm *now;
	time_t ti;

	file = fopen("TscRun.log","a+");
	if ( NULL == file )
	{
		return;
	}
	ti = time(NULL);
	now = localtime(&ti);
	sprintf(tmp,"%d-%d-%d %d %d:%02d:%02d TscGb restart.\n", now->tm_year + 1900,
		now->tm_mon + 1, now->tm_mday, now->tm_wday, now->tm_hour,
		now->tm_min, now->tm_sec);
	fputs(tmp,file);
	fclose(file);
	AdjustFileSize((char*)"TscRun.log",100);
#endif
	//unsigned long mRestart = 0 ;
	//(CDbInstance::m_cGbtTscDb).GetSystemData("ucDownloadFlag",mRestart);
	CManaKernel::CreateInstance()->SndMsgLog(LOG_TYPE_REBOOT,0,0,0,0);	
}

/**************************************************************
Function:       TestMem
Description:    测试系统内存稳定性	
Input:          无              
Output:         无
Return:         无
***************************************************************/
void TestMem(char* funName,int iLine)
{
#ifdef LINUX
	static int iMemCnt = 0;
	int iCurMemCnt     = 0;
	//FILE* fp = NULL;
	system("ps | grep Gb.aiton > mem.txt");

	int sd;
	char buf[100] = {0};
	char cTmp[36] = {0};
	int n;

	sd = open("mem.txt", O_RDONLY);
	n = read(sd, buf, sizeof(buf));
	close(sd);

	ACE_OS::memcpy(cTmp,buf+15,5);

	iCurMemCnt = atoi(cTmp);


	if ( iCurMemCnt != iMemCnt )  
	{
		iMemCnt = iCurMemCnt;
		
		FILE* file = NULL;
		char tmp[64] = {0};
		struct tm *now;
		time_t ti;

		file = fopen("moreMem.txt","a+");
		if ( NULL == file )
		{
			return;
		}

		ti = time(NULL);
		now = localtime(&ti);
		sprintf(tmp,"%d-%d-%d %d %d:%02d:%02d %s:%d memcnt:%d.\n", now->tm_year + 1900,
			now->tm_mon + 1, now->tm_mday, now->tm_wday, now->tm_hour,
			now->tm_min, now->tm_sec,funName,iLine,iCurMemCnt);
		fputs(tmp,file);
		fclose(file);
	}

	//ACE_DEBUG((LM_DEBUG,"buf:%s cTmp:%s iMemCnt:%d \n",buf,cTmp,iMemCnt));
#endif
}


/**************************************************************
Function:       CompareTime
Description:    时间大小比较
Input:          ucHour1 ：比较时间1的小时值 
			    ucMin1  ：比较时间1的分钟值  
			    ucHour1 ：比较时间2的小时值 
			    ucMin1  ：比较时间2的分钟值    
Output:         无
Return:         0:time2比time1小   1：time2比time1大,或相等 
***************************************************************/
bool CompareTime(Byte ucHour1,Byte ucMin1,Byte ucHour2,Byte ucMin2)
{
	if ( ucHour2 > ucHour1 )
	{		
		return 1;
	}
	else if ( ucHour2 == ucHour1 )
	{
		if ( ucMin2 >= ucMin1 )
		{			
			return 1;
		}
	}	
	return 0;
}


/**************************************************************
Function:       GetEypDevID
Description:    对字符串异或加密处理
Input:          src ：待加密字符串指针 
			    dec  ：加密后字符串指针 
Output:         无
Return:         -1 -加密字符串失败 0 - 加密字串串成功
***************************************************************/
int GetEypChar(char *src ,char *dec)
{  
   if(src == NULL || dec ==NULL)
   		return -1 ;
   char *p=src; 
   int i=0; 
   while(*p!='\0') 
   { 
     dec[i]=0x01 ^ *p; /* 异或处理加密*/
     p++; 
     i++; 
  } 
 	dec[i]='\0';
 	return 0 ;
}  


/**************************************************************

Function:       GetStrHwAddr
Description:    获取网卡物理地址字符串
Input:          StrHwAddr ：网卡物理地址字符串 
Output:         无
Return:         -1 获取失败  0 -获取成功
***************************************************************/
int GetStrHwAddr(char * strHwAddr)
{
        //char strHwAddr[32];
        int fdReq = -1;
        struct ifreq typIfReq;
        char *hw;
        memset(&typIfReq, 0x00, sizeof(typIfReq));
        fdReq = socket(AF_INET, SOCK_STREAM, 0);
        strcpy(typIfReq.ifr_name, "eth0");
        if(ioctl(fdReq, SIOCGIFHWADDR, &typIfReq) < 0)
        {
            printf("fail to get hwaddr %s %d\n", typIfReq.ifr_name, fdReq);
            close(fdReq);
            return -1;
        }
        hw = typIfReq.ifr_hwaddr.sa_data;
        sprintf(strHwAddr, "%02x%02x%02x%02x%02x%02x", *hw, *(hw+1), *(hw+2), *(hw+3), *(hw+4), *(hw+5));
        close(fdReq);  
       
        return 0;
}

/**************************************************************
Function:       SaveEnyDevId
Description:    保存设置设备ID加密存储
Input:          SysEnyDevId ：系统加密ID指针 
Output:         无
Return:         -1 - 产生加密设备ID失败 0 -产生加密设备ID成功
***************************************************************/
int GetSysEnyDevId(char *SysEnyDevId)
{
    char strHwAddr[32]={0};
    char enyDevId[32] = {0};
    if(GetStrHwAddr(strHwAddr) == -1)
       return -1;
    if(GetEypChar(strHwAddr ,enyDevId)==-1)
       return -1;     
     ACE_OS::strcpy(SysEnyDevId,enyDevId); 
      
     return 0;
}

