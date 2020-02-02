
#pragma once

#ifdef _DEBUG
#define DASSERT(s) assert(s)
#define DVERIFY(s) DASSERT(s)
#else
#define DASSERT(s) (void)0
#define DVERIFY(s) s
#endif

#define APP_NAME L"DirNotify"