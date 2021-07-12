// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <Shlobj.h>
#include <Shlwapi.h>
#include <process.h>

#include <assert.h>
#include <string>
#include <vector>
#include <map>


#include "../../lsMisc/CreateSimpleWindow.h"
#include "../../lsMisc/GetLastErrorString.h"
#include "../../lsMisc/TrayIcon.h"
#include "../../lsMisc/HighDPI.h"
#include "../../lsMisc/I18N.h"
#include "../../lsMisc/OpenCommon.h"
#include "../../lsMisc/IsFileOpen.h"
#include "../../lsMisc/stdosd/stdosd.h"
#include "../../lsMisc/dverify.h"

#ifndef NDEBUG
inline void DTRACE(const std::wstring& str)
{
	OutputDebugString(str.c_str());
	OutputDebugString(L"\r\n");
}
inline void DTRACE(DWORD dw)
{
	OutputDebugString((L"LastError = " + std::to_wstring(dw)).c_str());
}

inline void DTRACE_WITHCOUT(const std::wstring& counterName, const std::wstring& str)
{
	thread_local std::map<std::wstring, size_t> counterMap;
	OutputDebugString(std::to_wstring(counterMap[counterName]++).c_str());
	OutputDebugString(L" ");
	OutputDebugString(str.c_str());
	OutputDebugString(L"\r\n");
}
#else
#define DTRACE(s) (void)0
#define DTRACE_WITHCOUT(n,s) (void)0
#endif

#define APP_NAME L"DirNotify"

