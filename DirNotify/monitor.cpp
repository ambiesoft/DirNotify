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
		ExitFatal(L"Failed to open dir");

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
	//HANDLE hChange = FindFirstChangeNotification(
	//	dir.c_str(),                   // directory to watch 
	//	FALSE,                         // do not watch subtree 
	//	FILE_NOTIFY_CHANGE_LAST_WRITE); // watch file name changes 
	//if (INVALID_HANDLE_VALUE == hChange)
	//	ExitFatal(L"Failed to create ChangeNotification");

	//while (true)
	//{
	//	switch (WaitForSingleObject(hChange, INFINITE))
	//	{
	//	case WAIT_OBJECT_0:
	//		ReadDirectoryChangesW(hDir,
	//			buff,
	//			BUFFLEN,
	//			FALSE, // subtree
	//			FILE_NOTIFY_CHANGE_LAST_WRITE,
	//			&dwLen,
	//			NULL, NULL);

	//		SendMessage(data.h_, WM_APP_FILECHANGED, (WPARAM)buff, 0);
	//		if (!FindNextChangeNotification(hChange))
	//			ExitFatal(L"Failed to FindNextChangeNotification");
	//	}
	//}
}

void InitMonitor(HWND hWnd)
{
	static HANDLE shThread;
	if (shThread)
		TerminateThread(shThread, -1);
	shThread = (HANDLE)_beginthread(start_address, 0, NULL);
	if (!shThread)
		ExitFatal(L"Failed to create thread");

	DVERIFY(AddTrayIcon(hWnd, WM_APP_TRAY_NOTIFY, ghTrayIcon, APP_NAME));

}