#pragma once

#define EXPORT __declspec(dllexport)
#define IMPORT __declspec(dllimport)

#ifdef _USRDLL
#define CLASS_API EXPORT
#else
#define CLASS_API IMPORT
#endif