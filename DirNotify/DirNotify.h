#pragma once

#include "resource.h"
#include "MonitorInfo.h"

enum {
	WM_APP_FILECHANGED = WM_APP + 1,
	WM_APP_TRAY_NOTIFY,
	WM_APP_ACTIVATE,
	WM_APP_REFRESH_DESKTOP,
	WM_APP_AFTER_NOTIFIED,
	WM_APP_MONITOR_DIR_REMOVED,
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

class NotifyPair
{
	DWORD action_;
	std::wstring data_;
	DWORD fileOrDirectory_;
public:
	NotifyPair(DWORD ford, DWORD action, const std::wstring& data):
		fileOrDirectory_(ford), action_(action), data_(data){}

	std::wstring getData() const {
		return data_;
	}
	DWORD getAction() const {
		return action_;
	}
	DWORD getFileAttribute() const {
		return fileOrDirectory_;
	}
};
inline bool operator<(const NotifyPair& left, const NotifyPair& right)
{
	std::pair<DWORD, std::wstring> leftPair(left.getAction(), left.getData());
	std::pair<DWORD, std::wstring> rightPair(right.getAction(), right.getData());
	return leftPair < rightPair;
}

