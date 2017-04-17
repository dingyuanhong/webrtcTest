#pragma once

#include <tchar.h>

class LogObserver
{
public:
	void OnLogMessage(const TCHAR* type, const TCHAR * value) {
#ifdef _UNICODE
		OnLogMessageW(type, value);
#else
		OnLogMessageA(type, value);
#endif
	}
	virtual void OnLogMessageA(const char* type, const char * value) = 0;
	virtual void OnLogMessageW(const wchar_t* type, const wchar_t * value) = 0;
};