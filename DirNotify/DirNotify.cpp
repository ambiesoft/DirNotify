// DirNotify.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include <process.h>
#include <string>
#include <vector>

#include "../../lsMisc/CreateSimpleWindow.h"
#include "../../lsMisc/GetLastErrorString.h"
#include "../../lsMisc/showballoon.h"

#include "DirNotify.h"

using namespace std;
using namespace Ambiesoft;

enum {
	WM_APP_FILECHANGED = WM_APP + 1,
};
struct Data
{
	HWND h_;
} data;
void FatalExit(LPCTSTR pError, DWORD dwLE = GetLastError())
{
	wstring error = GetLastErrorString(dwLE);
	MessageBox(NULL, error.c_str(), APPNAME, MB_ICONERROR);
	ExitProcess(-1);
}

void __cdecl start_address(void *)
{
	wstring dir = L"Z:\\Desktop";
	HANDLE hDir = CreateFile(dir.c_str(),
		GENERIC_READ,
		FILE_SHARE_READ, // share
		NULL, // security
		OPEN_EXISTING, // disposition
		FILE_FLAG_BACKUP_SEMANTICS,
		NULL // template
		);
	if (INVALID_HANDLE_VALUE == hDir)
		FatalExit(L"Failed to open dir");

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
			FatalExit(L"Failed to ReadDirectoryChangesW");
		SendMessage(data.h_, WM_APP_FILECHANGED, (WPARAM)dir.c_str(), (LPARAM)buff);
	}
	//HANDLE hChange = FindFirstChangeNotification(
	//	dir.c_str(),                   // directory to watch 
	//	FALSE,                         // do not watch subtree 
	//	FILE_NOTIFY_CHANGE_LAST_WRITE); // watch file name changes 
	//if (INVALID_HANDLE_VALUE == hChange)
	//	FatalExit(L"Failed to create ChangeNotification");

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
	//			FatalExit(L"Failed to FindNextChangeNotification");
	//	}
	//}
}

void OnChanged(LPCTSTR pDir, FILE_NOTIFY_INFORMATION* fni)
{
	wstring message;
	message += I18N(L"LastWrite Changed");
	message += L":\r\n";
	
	vector<WCHAR> text;
	text.assign(fni->FileName, fni->FileName + fni->FileNameLength / sizeof(WCHAR));
	text.push_back(L'\0');

	message += (LPCWSTR)&text[0];
	showballoon(NULL, //data.h_,
		APPNAME,
		message,
		NULL,
		5000,
		1,
		FALSE, // no messageloop in this function
		1);

}
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_APP_FILECHANGED:
		OnChanged((LPCTSTR)wParam, (FILE_NOTIFY_INFORMATION*)lParam);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	HWND hWnd = CreateSimpleWindow(NULL, NULL, NULL, WndProc);
	if (!hWnd)
		FatalExit(L"Failed to create window");
	data.h_ = hWnd;

	HANDLE hThread = (HANDLE)_beginthread(start_address, 0, NULL);
	if (!hThread)
		FatalExit(L"Failed to create thread");

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		// if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}

