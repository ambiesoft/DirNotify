// DirNotify.cpp : Defines the entry point for the application.
//

#include "stdafx.h"


#include "DirNotify.h"

using namespace std;
using namespace Ambiesoft;
using namespace Ambiesoft::stdosd;

HICON ghTrayIcon;

enum {
	WM_APP_FILECHANGED = WM_APP + 1,
	WM_APP_TRAY_NOTIFY,
};
struct Data
{
	HWND h_;
	wstring dir_;
} data;
void FatalExit(LPCTSTR pError, DWORD dwLE = GetLastError())
{
	wstring error = I18N(pError);
	error += L"\r\n";
	error += GetLastErrorString(dwLE);
	MessageBox(NULL, error.c_str(), APP_NAME, MB_ICONERROR);
	ExitProcess(-1);
}

void __cdecl start_address(void *)
{
	HANDLE hDir = CreateFile(data.dir_.c_str(),
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
		SendMessage(data.h_, WM_APP_FILECHANGED, (WPARAM)data.dir_.c_str(), (LPARAM)buff);
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
	wstring file;
	{
		vector<WCHAR> text;
		text.assign(fni->FileName, fni->FileName + fni->FileNameLength / sizeof(WCHAR));
		text.push_back(L'\0');
		file = (LPCWSTR)&text[0];
	}

	wstring filefull = stdCombinePath(pDir, file);
	if (IsFileOpen(filefull.c_str()))
		return;
	WIN32_FIND_DATA wfd;
	if (!FindClose(FindFirstFile(filefull.c_str(), &wfd)))
		return;
	
	wstring message;
	message += I18N(L"LastWrite Changed");
	message += L":\r\n";
	
	message += stdFormat(I18N(L"Size=%d"), wfd.nFileSizeLow);
	message += L"\r\n";

	message += file;
	//showballoon(NULL, //data.h_,
	//	APPNAME,
	//	message,
	//	NULL,
	//	5000,
	//	1,
	//	FALSE, // no messageloop in this function
	//	1);

	PopupTrayIcon(data.h_, WM_APP_TRAY_NOTIFY, ghTrayIcon, APP_NAME, message.c_str());
}

void OnCommand(HWND hWnd, WORD cmd)
{
	switch (cmd)
	{
	case IDC_ABOUT:
	{
		tstring message;
		message += APP_NAME L" " APP_VERSION;
		message += _T("\r\n\r\n");
		MessageBox(NULL,
			message.c_str(),
			APP_NAME,
			MB_ICONINFORMATION);
	}
	break;

	case IDC_SHOWHELP:
	{
		if (_wcsicmp(Ambiesoft::i18nGetCurrentLang(), L"jpn") == 0)
		{
			OpenCommon(hWnd, L"https://github.com/ambiesoft/DirNotify/blob/master/README.md");
		}
		else
		{
			OpenCommon(hWnd, L"https://github.com/ambiesoft/DirNotify/blob/master/README.md");
		}
	}
	break;

	case IDC_QUIT:
		DestroyWindow(hWnd);
		break;
	}

}
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_APP_TRAY_NOTIFY:
		switch (lParam)
		{
		case WM_LBUTTONUP:
		{
			SetForegroundWindow(hWnd);
		}
		break;

		case WM_RBUTTONUP:
		{
			POINT apos;
			HMENU hSubMenu = CreatePopupMenu();
#ifdef _DEBUG
			AppendMenu(hSubMenu, MF_BYCOMMAND, IDC_START, L"TEST");
			AppendMenu(hSubMenu, MF_SEPARATOR, 0, NULL);
#endif
			AppendMenu(hSubMenu, MF_BYCOMMAND, IDC_ABOUT, (L"&About..."));
			AppendMenu(hSubMenu, MF_BYCOMMAND, IDC_SHOWHELP, (L"&Help..."));
			AppendMenu(hSubMenu, MF_SEPARATOR, 0, NULL);

			AppendMenu(hSubMenu, MF_BYCOMMAND, IDC_QUIT, (L"&Exit"));

			i18nChangeMenuText(hSubMenu);

			SetForegroundWindow(hWnd);
			GetCursorPos((LPPOINT)&apos);

			TrackPopupMenu(hSubMenu,
				TPM_TOPALIGN | TPM_RIGHTBUTTON | TPM_LEFTBUTTON,
				apos.x, apos.y, 0, hWnd, NULL);

			DestroyMenu(hSubMenu);
		}
		break;

		}
		break;

	case WM_APP_FILECHANGED:
		OnChanged((LPCTSTR)wParam, (FILE_NOTIFY_INFORMATION*)lParam);
		break;
	case WM_COMMAND:
		OnCommand(hWnd, LOWORD(wParam));
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

wstring GetDesktopDirectory()
{
	WCHAR path[MAX_PATH];
	if (SHGetFolderPath(NULL, CSIDL_DESKTOP, NULL, 0, path))
		return L"";
	return path;
}
int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	
	data.dir_ = GetDesktopDirectory();
	if (!PathIsDirectory(data.dir_.c_str()))
		FatalExit(L"Failed to get desktop directory");

	InitHighDPISupport();
	i18nInitLangmap(hInstance, NULL, _T(""));
	ghTrayIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_DirNotify));
	DVERIFY(ghTrayIcon);

	HWND hWnd = CreateSimpleWindow(NULL, NULL, NULL, WndProc);
	if (!hWnd)
		FatalExit(L"Failed to create window");
	data.h_ = hWnd;
	SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)ghTrayIcon);
	SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)ghTrayIcon);

	HANDLE hThread = (HANDLE)_beginthread(start_address, 0, NULL);
	if (!hThread)
		FatalExit(L"Failed to create thread");

	DVERIFY(AddTrayIcon(hWnd, WM_APP_TRAY_NOTIFY, ghTrayIcon, APP_NAME));
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		// if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	DVERIFY(RemoveTrayIcon(hWnd, WM_APP_TRAY_NOTIFY));
	return (int)msg.wParam;
}

