#include "stdafx.h"

#include "../../lsMisc/CHandle.h"

#include "DirNotify.h"


using namespace Ambiesoft;
using namespace Ambiesoft::stdosd;
using namespace std;


void __cdecl start_address(void * pvoid)
{
	DASSERT(pvoid);
	MonitorInfo* pMI = (MonitorInfo*)pvoid;

	CFileHandle dir(CreateFile(pMI->dir_.c_str(),
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, // share
		NULL, // security
		OPEN_EXISTING, // disposition
		FILE_FLAG_BACKUP_SEMANTICS,
		NULL // template
	));
	if (!dir)
		ExitFatal(L"Failed to open directory.");

	const int BUFFLEN = 4096;
	char buff[BUFFLEN];
	DWORD dwLen;
	DWORD dwNotifyFilter = 0;
	if (pMI->monitorFile_)
		dwNotifyFilter |= FILE_NOTIFY_CHANGE_LAST_WRITE;
	if(pMI->monitorDir_)
		dwNotifyFilter |= FILE_NOTIFY_CHANGE_DIR_NAME;
	DASSERT(dwNotifyFilter != 0);
	while (true)
	{
		if (!ReadDirectoryChangesW(dir,
			buff,
			BUFFLEN,
			pMI->monitorSub_ ? TRUE : FALSE,
			dwNotifyFilter,
			&dwLen,
			NULL, NULL))
		{
			// ExitFatal(L"Failed to ReadDirectoryChangesW");
			SendMessage(gdata.h_, WM_APP_DIRREMOVED, (WPARAM)pMI->dir_.c_str(), (LPARAM)buff);
			break;
		}
		SendMessage(gdata.h_, WM_APP_FILECHANGED, (WPARAM)pMI->dir_.c_str(), (LPARAM)buff);
	}
}

void InitMonitors()
{
	static std::vector<HANDLE> threads;
	for(HANDLE h : threads)
		TerminateThread(h, -1);
	threads.clear();
	for (auto&& mi : gdata.monitorInfos_)
		threads.push_back(InitMonitor(&mi));

	RemoveTrayIcon(gdata.h_, WM_APP_TRAY_NOTIFY);
	do
	{
		wstring trayMessage = stdFormat(L"%s | %s",
			stdFormat(I18N(L"Watching %d dirs"), gdata.monitorInfos_.size()).c_str(),
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
HANDLE InitMonitor(MonitorInfo* pMI)
{
	HANDLE hThread = (HANDLE)_beginthread(start_address, 0, (void*)pMI);
	if (!hThread)
		ExitFatal(I18N(L"Failed to create thread."));

	return hThread;
}