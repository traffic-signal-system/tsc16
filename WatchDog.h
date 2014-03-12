#ifndef _WATCHDOG_H_
#define _WATCHDOG_H_

#include "ComStruct.h"

#define DEV_WATCHDOG  "/dev/watchdog"

class WatchDog
{
public:
	static WatchDog* CreateInstance();
	void OpenWatchdog();
	void CloseWatchdog();
	void FillWatchdog(char cData);
private:
	WatchDog();
	~WatchDog();

	int m_watchdogFd;
};

#endif  //_WATCHDOG_H_
