
#ifndef MANUAL_H_
#define MANUAL_H_

#include "ComDef.h"

#define DEV_MANUAL_KEYINT "/dev/tiny6410_buttons"  //MOD:0517 14:59
						 
class Manual 
{
public:
	Manual();
	virtual ~Manual();
	static Manual* CreateInstance();
	void DoManual();
private:
	Byte m_ucManualSts;  //�ֿ�״̬
	Byte m_ucManual;     //0�Զ����� 1�ֶ�����
	Byte m_ucRedFlash ; //�ֶ�ȫ����ƻ��߻������� 1-ȫ����� 0-ȫ�������λ
	Byte m_ucLastManual;     //֮ǰ�Ƿ�Ϊ�ֶ���
	Byte m_ucLastManualSts;  //֮ǰ���ֶ�״̬
	void OpenDev();
	int m_iManualFd;
	Byte ManualKey ;  //��ǰ�����ֿ�ʱ��ѡ����ֿز�����ť

};

#endif /* MANUAL_H_ */
