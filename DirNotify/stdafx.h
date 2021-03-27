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



#include "../../lsMisc/CreateSimpleWindow.h"
#include "../../lsMisc/GetLastErrorString.h"
#include "../../lsMisc/TrayIcon.h"
#include "../../lsMisc/HighDPI.h"
#include "../../lsMisc/I18N.h"
#include "../../lsMisc/OpenCommon.h"
#include "../../lsMisc/IsFileOpen.h"
#include "../../lsMisc/stdosd/stdosd.h"
#include "../../lsMisc/dverify.h"


#define APP_NAME L"DirNotify"
#define APP_VERSION L"1.0.10"

