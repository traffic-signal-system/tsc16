
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
	Byte m_ucManualSts;  //ÊÖ¿Ø×´Ì¬
	Byte m_ucManual;     //0×Ô¶¯ÔËĞĞ 1ÊÖ¶¯¿ØÖÆ
	Byte m_ucRedFlash ; //ÊÖ¶¯È«ºì¿ØÖÆ»òÕß»ÆÉÁ¿ØÖÆ 1-È«ºì»ò»ÆÉ 0-È«ºì»ÆÉÁ¹éÎ»
	Byte m_ucLastManual;     //Ö®Ç°ÊÇ·ñÎªÊÖ¶¯µÄ
	Byte m_ucLastManualSts;  //Ö®Ç°µÄÊÖ¶¯×´Ì¬
	void OpenDev();
	int m_iManualFd;
	Byte ManualKey ;  //µ±Ç°´¦ÓÚÊÖ¿ØÊ±£¬Ñ¡ÔñµÄÊÖ¿Ø²Ù×÷°´Å¥

};

#endif /* MANUAL_H_ */
