#ifndef _SERIALCTRL_H_
#define _SERIALCTRL_H_

class CSerialCtrl
{
public:
	static CSerialCtrl* CreateInstance();
	void OpenSerial(short iSerNum);
	int  GetSerialFd(short iSerNum);

private:
	CSerialCtrl();
	~CSerialCtrl();
	int  m_iSerial1fd;
	int  m_iSerial2fd;
	int  m_iSerial3fd;
};

#endif //_SERIALCTRL_H_

