#pragma once
#include "MediaStreamApply/LogObserver.h"
#include <Windows.h>
#include <stdlib.h>
#include <stdio.h>
#define LOG_MODE_OUT 0x1
#define LOG_MODE_PRINT 0x2

class WinLogObserver
	: public LogObserver
{
public:
	virtual void OnLogMessageA(const char* type, const char * value) 
	{
		if ((mode & LOG_MODE_OUT ) == LOG_MODE_OUT) {
			OutputDebugStringA(type);
			OutputDebugStringA(":");
			OutputDebugStringA(value);
			OutputDebugStringA("\n");
		}
		if ((mode & LOG_MODE_PRINT) == LOG_MODE_PRINT) {
			fprintf(stdout, "%s:%s\n", type, value);
		}
	}
	virtual void OnLogMessageW(const wchar_t* type, const wchar_t * value)
	{
		if ((mode & LOG_MODE_OUT) == LOG_MODE_OUT) {
			OutputDebugStringW(type);
			OutputDebugStringW(L":");
			OutputDebugStringW(value);
			OutputDebugStringW(L"\n");
		}
		if ((mode & LOG_MODE_PRINT) == LOG_MODE_PRINT) {
			fwprintf(stdout, L"%s:%s\n", type, value);
		}
	}
public:
	int mode = LOG_MODE_PRINT;
};