#pragma once

#include "resource.h"

enum {
	WM_APP_FILECHANGED = WM_APP + 1,
	WM_APP_TRAY_NOTIFY,
	WM_APP_ACTIVATE,
	WM_APP_REFRESH_DESKTOP,
};

struct GlobalData
{
	HWND h_;
	std::wstring dir_;
};

void ExitFatal(LPCTSTR pError, DWORD dwLE = GetLastError());
void InitMonitor(HWND hWnd);

extern HICON ghTrayIcon;
extern GlobalData gdata;