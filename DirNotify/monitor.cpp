#include "stdafx.h"
#include "DirNotify.h"


using namespace Ambiesoft;
using namespace Ambiesoft::stdosd;
using namespace std;


void __cdecl start_address(void * pvoid)
{
	DASSERT(pvoid);
	LPCWSTR pDir = (LPWSTR)pvoid;

	HANDLE hDir = CreateFile(pDir,
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, // share
		NULL, // security
		OPEN_EXISTING, // disposition
		FILE_FLAG_BACKUP_SEMANTICS,
		NULL // template
		);
	if (INVALID_HANDLE_VALUE == hDir)
		ExitFatal(L"Failed to open directory.");

	const int BUFFLEN = 4096;
	char buff[BUFFLEN];
	DWORD dwLen;
	while (true)
	{
		if (!ReadDirectoryChangesW(hDir,
			buff,
			BUFFLEN,
			FALSE, // subtree
			FILE_NOTIFY_CHANGE_LAST_WRITE,
			&dwLen,
			NULL, NULL))
			ExitFatal(L"Failed to ReadDirectoryChangesW");
		SendMessage(gdata.h_, WM_APP_FILECHANGED, (WPARAM)pDir, (LPARAM)buff);
	}
}

void InitMonitors()
{
	static std::vector<HANDLE> threads;
	for(HANDLE h : threads)
		TerminateThread(h, -1);
	threads.clear();
	for (auto&& dir : gdata.dirs_)
		threads.push_back(InitMonitor(dir.c_str()));

	RemoveTrayIcon(gdata.h_, WM_APP_TRAY_NOTIFY);
	do
	{
		wstring trayMessage = stdFormat(L"%s | %s",
			stdFormat(I18N(L"Watching %d dirs"), gdata.dirs_.size()).c_str(),
			APP_NAME);

		if (AddTrayIcon(gdata.h_, WM_APP_TRAY_NOTIFY, ghTrayIcon, trayMessage.c_str()))
			return;

		int nRet = MessageBox(gdata.h_, I18N(L"Failed to Add Tray Icon."), APP_NAME,
			MB_ICONERROR | MB_ABORTRETRYIGNORE);
		if (nRet == IDABORT)
			ExitProcess(-1);
		else if (nRet == IDRETRY)
			continue;
		else
			break;
	} while (true);
}
HANDLE InitMonitor(LPCWSTR pDir)
{
	HANDLE hThread = (HANDLE)_beginthread(start_address, 0, (void*)pDir);
	if (!hThread)
		ExitFatal(I18N(L"Failed to create thread."));

	return hThread;
}