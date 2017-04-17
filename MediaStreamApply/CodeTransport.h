#pragma once

#include <tchar.h>
#include <Windows.h>
#include <string>
#include <vector>

//unicode 转为 ascii  
static std::string WideByte2Acsi(std::wstring wstrcode)
{
	int asciisize = ::WideCharToMultiByte(CP_OEMCP, 0, wstrcode.c_str(), -1, NULL, 0, NULL, NULL);
	if (asciisize == ERROR_NO_UNICODE_TRANSLATION)
	{
		throw std::exception("Invalid UTF-8 sequence.");
	}
	if (asciisize == 0)
	{
		throw std::exception("Error in conversion.");
	}
	std::vector <char> resultstring(asciisize);
	int convresult = ::WideCharToMultiByte(CP_OEMCP, 0, wstrcode.c_str(), -1, &resultstring[0], asciisize, NULL, NULL);

	if (convresult != asciisize)
	{
		throw std::exception("La falla!");
	}

	return std::string(&resultstring[0]);
}

//ascii 转 Unicode  
static std::wstring Acsi2WideByte(std::string strascii)
{
	int widesize = MultiByteToWideChar(CP_ACP, 0, (char*)strascii.c_str(), -1, NULL, 0);
	if (widesize == ERROR_NO_UNICODE_TRANSLATION)
	{
		throw std::exception("Invalid UTF-8 sequence.");
	}
	if (widesize == 0)
	{
		throw std::exception("Error in conversion.");
	}
	std::vector<wchar_t> resultstring(widesize);
	int convresult = MultiByteToWideChar(CP_ACP, 0, (char*)strascii.c_str(), -1, &resultstring[0], widesize);


	if (convresult != widesize)
	{
		throw std::exception("La falla!");
	}

	return std::wstring(&resultstring[0]);
}

static std::wstring Utf82Unicode(const std::string utf8string)
{
	int widesize = ::MultiByteToWideChar(CP_UTF8, 0, utf8string.c_str(), -1, NULL, 0);
	if (widesize == ERROR_NO_UNICODE_TRANSLATION)
	{
		throw std::exception("Invalid UTF-8 sequence.");
	}
	if (widesize == 0)
	{
		throw std::exception("Error in conversion.");
	}

	std::vector<wchar_t> resultstring(widesize);

	int convresult = ::MultiByteToWideChar(CP_UTF8, 0, utf8string.c_str(), -1, &resultstring[0], widesize);

	if (convresult != widesize)
	{
		throw std::exception("La falla!");
	}

	return std::wstring(&resultstring[0]);
}

//utf-8 转 ascii  
static std::string UTF82ASCII(std::string strUtf8Code)
{
	std::string strRet("");
	//先把 utf8 转为 unicode  
	std::wstring wstr = Utf82Unicode(strUtf8Code);
	//最后把 unicode 转为 ascii  
	strRet = WideByte2Acsi(wstr);
	return strRet;
}

//Unicode 转 Utf8  
static std::string Unicode2Utf8(const std::wstring widestring)
{
	int utf8size = ::WideCharToMultiByte(CP_UTF8, 0, widestring.c_str(), -1, NULL, 0, NULL, NULL);
	if (utf8size == 0)
	{
		throw std::exception("Error in conversion.");
	}

	std::vector <char> resultstring(utf8size);

	int convresult = ::WideCharToMultiByte(CP_UTF8, 0, widestring.c_str(), -1, &resultstring[0], utf8size, NULL, NULL);

	if (convresult != utf8size)
	{
		throw std::exception("La falla!");
	}

	return std::string(&resultstring[0]);
}

//ascii 转 Utf8  
static std::string ASCII2UTF8(std::string strAsciiCode)
{
	std::string strRet("");
	//先把 ascii 转为 unicode  
	std::wstring wstr = Acsi2WideByte(strAsciiCode);
	//最后把 unicode 转为 utf8  
	strRet = Unicode2Utf8(wstr);
	return strRet;
}

#ifdef _UNICODE
#define AToString(A) Acsi2WideByte(A)
#define WToString(A) (A)
#else
#define AToString(A) (A)
#define WToString(A) WideByte2Acsi(A)
#endif

#ifdef _UNICODE
#define toWString(A) (A)
#else
#define toWString(A) Acsi2WideByte(A)
#endif

#ifdef _UNICODE
#define toString(A) WideByte2Acsi(A)
#else
#define toString(A) (A)
#endif

#ifdef _UNICODE
#define Utf82String Utf82Unicode
#else
#define Utf82String UTF82ASCII
#endif

#ifdef _UNICODE
#define String2Utf8 Unicode2Utf8
#else
#define String2Utf8 ASCII2UTF_8
#endif