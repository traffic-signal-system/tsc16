/*================================================================================================== 
 * ��Ŀ����: ��ȡ�����ļ��� 
 *     ����: �ṩ��ͷ���,ʵ�ִ������ļ���ȡ����ֵ 
 *     ����: ³�ʻ� 
 *     ��ϵ: renhualu@live.com 
 * ����޸�: 2013-4-15 
 *     �汾: v1.0.2 
  ==================================================================================================*/  
  
#include <sys/timeb.h>  
  
#include <string>  
#include <iostream>  
#include <map>  
  
#include <ace/OS.h>  
  
#include "Configure.h"  
Configure* Configure::CreateInstance()
{
	static Configure con;
	return &con;
}
Configure::Configure():impExp_(NULL)
{
 
}
 
Configure::~Configure()
{
       if (impExp_)
              delete impExp_;
       impExp_ = NULL;
}
 
int Configure::open(const ACE_TCHAR * filename)
{
       if (this->config.open() == -1)
              return -1;
      
       this->impExp_=new ACE_Ini_ImpExp(config);
 
       return this->impExp_->import_config(filename);
}
 
int Configure::GetString(const char *szSection, const char *szKey, ACE_TString &strValue)
{
       if (config.open_section(config.root_section(),ACE_TEXT(szSection),0,this->root_key_)==-1)
              return -1;
       return config.get_string_value(this->root_key_,szKey,strValue);
}
 
int Configure::GetInteger(const char* szSection,const char* szKey,int& nValue)
{
       ACE_TString strValue;
       if (config.open_section(config.root_section(),ACE_TEXT(szSection),0,this->root_key_)==-1)
              return -1;
       if (config.get_string_value(this->root_key_,szKey,strValue) == -1)
              return -1;
       nValue = ACE_OS::atoi(strValue.c_str());
       if (nValue == 0 && strValue != "0")

              return -1;
       return nValue;
}
 
int Configure::close()
{
       if (impExp_)
       {
              delete impExp_;
              impExp_ = NULL;
       }
       return 0;
} 
