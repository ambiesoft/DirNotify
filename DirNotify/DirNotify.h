#pragma once

#include "resource.h"

enum {
	WM_APP_FILECHANGED = WM_APP + 1,
	WM_APP_TRAY_NOTIFY,
	WM_APP_ACTIVATE,
	WM_APP_REFRESH_DESKTOP,
	WM_APP_AFTER_NOTIFIED,
};

struct GlobalData
{
	HWND h_;
	std::vector<std::wstring> dirs_;
};

struct ThreadData
{
	wchar_t* pDir_;
};

void ExitFatal(LPCTSTR pError, DWORD dwLE = GetLastError());
void ExitFatal(const std::wstring& error, DWORD dwLE = GetLastError());
void InitMonitors();
HANDLE InitMonitor(LPCWSTR pDir);

extern HICON ghTrayIcon;
extern GlobalData gdata;