#ifndef _GPS_H_
#define _GPS_H_

#include "ComDef.h"
#include "ComStruct.h"

#define GPRMC "$GPRMC"
#define GPSFILE "GpsTime.info"

class CGps
{
public:
	static CGps* CreateInstance();
	static void* RunGpsData(void* arg);

	static int Extract();
	void InitGps();
	void ForceAdjust();
	static bool CheckSum(char *cMsg);
	static void SetTime(int iYear , int iMon , int iDay, int iHour, int iMin, int iSec);
	static bool m_bGpsTime ;

private:
	CGps();
	~CGps();

	static bool   m_bNeedGps;
	static int    m_iGpsFd;
	static char   m_cBuf[128];
	static time_t m_tLastTi;
};

#endif //_GPS_H_
