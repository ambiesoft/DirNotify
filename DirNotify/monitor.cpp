#include "stdafx.h"
#include "DirNotify.h"


using namespace Ambiesoft;


void __cdecl start_address(void * pvoid)
{
	DASSERT(!pvoid);
	GlobalData* pData = &gdata; // static_cast<Data*>(pvoid);
	static HANDLE shDir;
	if (shDir)
		CloseHandle(shDir);
	shDir = CreateFile(pData->dir_.c_str(),
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, // share
		NULL, // security
		OPEN_EXISTING, // disposition
		FILE_FLAG_BACKUP_SEMANTICS,
		NULL // template
		);
	if (INVALID_HANDLE_VALUE == shDir)
		ExitFatal(L"Failed to open directory.");

	const int BUFFLEN = 4096;
	char buff[BUFFLEN];
	DWORD dwLen;
	while (true)
	{
		if (!ReadDirectoryChangesW(shDir,
			buff,
			BUFFLEN,
			FALSE, // subtree
			FILE_NOTIFY_CHANGE_LAST_WRITE,
			&dwLen,
			NULL, NULL))
			ExitFatal(L"Failed to ReadDirectoryChangesW");
		SendMessage(pData->h_, WM_APP_FILECHANGED, (WPARAM)pData->dir_.c_str(), (LPARAM)buff);
	}
}

void InitMonitor(HWND hWnd)
{
	static HANDLE shThread;
	if (shThread)
		TerminateThread(shThread, -1);
	shThread = (HANDLE)_beginthread(start_address, 0, NULL);
	if (!shThread)
		ExitFatal(I18N(L"Failed to create thread."));

	RemoveTrayIcon(hWnd, WM_APP_TRAY_NOTIFY);

	do
	{
		if (AddTrayIcon(hWnd, WM_APP_TRAY_NOTIFY, ghTrayIcon, APP_NAME))
			return;

		int nRet = MessageBox(hWnd, I18N(L"Failed to Add Tray Icon."), APP_NAME,
			MB_ICONERROR | MB_ABORTRETRYIGNORE);
		if (nRet == IDABORT)
			ExitProcess(-1);
		else if (nRet == IDRETRY)
			continue;
		else
			break;
	} while (true);
}