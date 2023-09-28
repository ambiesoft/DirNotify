#include "stdafx.h"

#include "../../lsMisc/CHandle.h"

#include "DirNotify.h"


using namespace Ambiesoft;
using namespace Ambiesoft::stdosd;
using namespace std;


void __cdecl MonitorEntryPoint(void * pvoid)
{
	DASSERT(pvoid);
	MonitorInfo* pMI = (MonitorInfo*)pvoid;

	CFileHandle dir(CreateFile(pMI->getDir().c_str(),
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, // share
		NULL, // security
		OPEN_EXISTING, // disposition
		FILE_FLAG_BACKUP_SEMANTICS,
		NULL // template
	));
	if (!dir)
		ExitFatal(stdFormat(I18N(L"Failed to open directory '%s'."), pMI->getDir().c_str()));

	const int BUFFLEN = 4096;
	char buff[BUFFLEN];
	DWORD dwLen;
	DWORD dwNotifyFilter = 0;
	if (pMI->isMonitorFile()) {
		dwNotifyFilter |= FILE_NOTIFY_CHANGE_LAST_WRITE;
		dwNotifyFilter |= FILE_NOTIFY_CHANGE_FILE_NAME;
	}
	if(pMI->isMonitorDirectory())
		dwNotifyFilter |= FILE_NOTIFY_CHANGE_DIR_NAME;
	DASSERT(dwNotifyFilter != 0);
	while (true)
	{
		if (!ReadDirectoryChangesW(dir,
			buff,
			BUFFLEN,
			pMI->isSubTree() ? TRUE : FALSE,
			dwNotifyFilter,
			&dwLen,
			NULL, NULL))
		{
			// ExitFatal(L"Failed to ReadDirectoryChangesW");
			SendMessage(gdata.h_, WM_APP_MONITOR_DIR_REMOVED, (WPARAM)pMI->getDir().c_str(), (LPARAM)buff);
			break;
		}

		vector<NotifyPair> data;
		for(FILE_NOTIFY_INFORMATION* fni = (FILE_NOTIFY_INFORMATION*)buff;;)
		{
			vector<WCHAR> text;
			text.assign(fni->FileName, fni->FileName + fni->FileNameLength / sizeof(WCHAR));
			text.push_back(L'\0');
			wstring file = (LPCWSTR)&text[0];

			NotifyPair entry( pMI->isMonitorDirectory() ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL,
				fni->Action,file );
			data.push_back(entry);
			
			if (fni->NextEntryOffset == 0)
				break;
			fni = (FILE_NOTIFY_INFORMATION*)((BYTE*)fni + fni->NextEntryOffset);
		} 
			
		SendMessage(gdata.h_, WM_APP_FILECHANGED, (WPARAM)pMI->getDir().c_str(), (LPARAM)&data);
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
	HANDLE hThread = (HANDLE)_beginthread(MonitorEntryPoint, 0, (void*)pMI);
	if (!hThread)
		ExitFatal(I18N(L"Failed to create thread."));

	return hThread;
}