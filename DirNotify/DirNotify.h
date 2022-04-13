#pragma once

#include "resource.h"

enum {
	WM_APP_FILECHANGED = WM_APP + 1,
	WM_APP_TRAY_NOTIFY,
	WM_APP_ACTIVATE,
	WM_APP_REFRESH_DESKTOP,
	WM_APP_AFTER_NOTIFIED,
	WM_APP_DIRREMOVED,
};

struct MonitorInfo
{
	bool monitorFile_;
	bool monitorDir_;
	bool monitorSub_;
	std::wstring dir_;

	MonitorInfo() : monitorFile_(false),monitorDir_(false),monitorSub_(false) {	}
};
struct GlobalData
{
	HWND h_;
	bool isSound_;
	std::wstring wavFile_;
	std::vector<MonitorInfo> monitorInfos_;
};

struct ThreadData
{
	wchar_t* pDir_;
};

void ExitFatal(LPCTSTR pError, DWORD dwLE = GetLastError());
void ExitFatal(const std::wstring& error, DWORD dwLE = GetLastError());
void InitMonitors();
HANDLE InitMonitor(MonitorInfo* pMI);

extern HICON ghTrayIcon;
extern GlobalData gdata;