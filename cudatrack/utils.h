#pragma once

#define _CRT_SECURE_NO_WARNINGS

#ifdef _MSC_VER
#pragma warning(disable: 4244) // conversion from 'int' to 'float', possible loss of data
#endif

#include <string>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <stdexcept>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstddef>

typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned long ulong;
typedef unsigned char uchar;

/*
 * Portable definition for SNPRINTF, VSNPRINTF, STRCASECMP and STRNCASECMP
 */
#ifdef _MSC_VER
	#if _MSC_VER > 1310
		#define SNPRINTF _snprintf_s
		#define VSNPRINTF _vsnprintf_s
	#else
		#define SNPRINTF _snprintf
		#define VSNPRINTF _vsnprintf
	#endif
	#define STRCASECMP _stricmp
	#define STRNCASECMP _strnicmp
	#define ALLOCA(size) _alloca(size) // allocates memory on stack
#else
	#define STRCASECMP strcasecmp
	#define STRNCASECMP strncasecmp
	#define SNPRINTF snprintf
	#define VSNPRINTF vsnprintf
	#define ALLOCA(size) alloca(size)
#endif
#define ALLOCA_ARRAY(T, N) (new(ALLOCA(sizeof(T) * N)) T[N])

#define DLL_CALLCONV __cdecl
#define CDLL_EXPORT extern "C" __declspec(dllexport) 
#define DLL_EXPORT __declspec(dllexport) 

DLL_EXPORT void dbgout(std::string s);
DLL_EXPORT std::string SPrintf(const char *fmt, ...);
DLL_EXPORT void dbgprintf(const char *fmt,...);

#pragma pack(push, 4)
struct vector2f {
	float x,y;
};
#pragma pack(pop)

ushort* floatToNormalizedUShort(float *data, uint w,uint h);

