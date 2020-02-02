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

#include <assert.h>

#ifdef _DEBUG
#define DASSERT(s) assert(s)
#define DVERIFY(s) DASSERT(s)
#else
#define DASSERT(s) (void)0
#define DVERIFY(s) s
#endif

#define APP_NAME L"DirNotify"
#define APP_VERSION L"1.0.1"

