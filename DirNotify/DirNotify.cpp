// DirNotify.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "../../lsMisc/CHandle.h"
#include "../../lsMisc/SessionGlobalMemory/SessionGlobalMemory.h"

#include "DirNotify.h"

using namespace std;
using namespace Ambiesoft;
using namespace Ambiesoft::stdosd;

HICON ghTrayIcon;
GlobalData gdata;
CSessionGlobalMemory<HWND> sgHwnd("DirNotifyWindow");

void ExitFatal(LPCTSTR pError, DWORD dwLE)
{
	wstring error = I18N(pError);
	error += L"\r\n";
	error += GetLastErrorString(dwLE);
	MessageBox(NULL, error.c_str(), APP_NAME, MB_ICONERROR);
	ExitProcess(-1);
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

	PopupTrayIcon(gdata.h_, WM_APP_TRAY_NOTIFY, ghTrayIcon, APP_NAME, message.c_str());
}

UINT WM_TASKBARCREATED;

vector<wstring> GetAllFiles(const wstring& dir)
{
	vector<wstring> files;
	WIN32_FIND_DATA wfd;
	HANDLE hFind = FindFirstFile(stdCombinePath(dir, L"*.*").c_str(), &wfd);
	if (hFind == INVALID_HANDLE_VALUE)
		return files;
	do
	{
		if (wfd.cFileName[0] == L'.' && wfd.cFileName[1] == 0)
			continue;
		if (wfd.cFileName[0] == L'.' && wfd.cFileName[1] == L'.' && wfd.cFileName[2] == 0)
			continue;
		if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;
		if (wfd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
			continue;
		files.push_back(stdCombinePath(dir, wfd.cFileName));
	} while (FindNextFile(hFind, &wfd));
	FindClose(hFind);
	return files;
}

vector<wstring> gMenuFiles;
HMENU CreateFileMenu()
{
	HMENU hSubMenu = CreatePopupMenu();

	gMenuFiles = GetAllFiles(StdGetDesktopDirectory());
	size_t maxcount = IDC_FILE_END - IDC_FILE_START + 1;
	if (gMenuFiles.size() > maxcount)
		gMenuFiles.resize(maxcount);
	
	int index = 0;
	for (auto&& file : gMenuFiles)
		AppendMenu(hSubMenu, MF_BYCOMMAND, IDC_FILE_START + (index++), file.c_str());

	return hSubMenu;
}
HMENU CreateTrayPopupMenu()
{
	HMENU hSubMenu = CreatePopupMenu();
#ifdef _DEBUG
	AppendMenu(hSubMenu, MF_BYCOMMAND, IDC_START, L"TEST");
	AppendMenu(hSubMenu, MF_SEPARATOR, 0, NULL);
#endif
	AppendMenu(hSubMenu, MF_POPUP, (UINT_PTR)CreateFileMenu(), L"&Open");
	AppendMenu(hSubMenu, MF_SEPARATOR, 0, NULL);

	AppendMenu(hSubMenu, MF_BYCOMMAND, IDC_ABOUT, (L"&About..."));
	AppendMenu(hSubMenu, MF_BYCOMMAND, IDC_SHOWHELP, (L"&Help..."));
	AppendMenu(hSubMenu, MF_SEPARATOR, 0, NULL);

	AppendMenu(hSubMenu, MF_BYCOMMAND, IDC_QUIT, (L"&Exit"));
	
	i18nChangeMenuText(hSubMenu);

	return hSubMenu;
}

void OnFileMenu(HWND hWnd, WORD cmd)
{
	DASSERT(IDC_FILE_START <= cmd&&cmd <= IDC_FILE_END);
	int index = cmd - IDC_FILE_START;

	wstring file = gMenuFiles[index];
	OpenCommon(hWnd, file.c_str());
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
	{
		DestroyWindow(hWnd);
	}
	break;

	default:
	{
		if (IDC_FILE_START <= cmd && cmd <= IDC_FILE_END)
			OnFileMenu(hWnd,cmd);
	}
	break;
	}

}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
	{
		WM_TASKBARCREATED = RegisterWindowMessage(_T("TaskbarCreated"));
		DASSERT(sgHwnd == nullptr);
		sgHwnd = hWnd;
	}
	break;

	case WM_APP_ACTIVATE:
	{
		InitMonitor(hWnd);
	}
	break;

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
			HMENU hSubMenu = CreateTrayPopupMenu();


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
		if(message == WM_TASKBARCREATED)
		{
			InitMonitor(hWnd);
		}

		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}



CHandle hDupCheck;
bool CheckDuplicateInstance()
{
	DASSERT(!hDupCheck);
	hDupCheck = CreateMutex(NULL, TRUE, L"DirNotify_Mutex");
	return GetLastError() != ERROR_ALREADY_EXISTS;
}


int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	InitHighDPISupport();
	i18nInitLangmap(hInstance, NULL, _T(""));

	if (!CheckDuplicateInstance())
	{
		if (sgHwnd == nullptr)
			ExitFatal(L"This is duplicated instance but failed to find previous one.");
		else
		{
			PostMessage(sgHwnd, WM_APP_ACTIVATE, 0, 0);
		}
		return 1;
	}

	gdata.dir_ = StdGetDesktopDirectory();
	if (!PathIsDirectory(gdata.dir_.c_str()))
		ExitFatal(L"Failed to get desktop directory");

	ghTrayIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_DirNotify));
	DVERIFY(ghTrayIcon);

	HWND hWnd = CreateSimpleWindow(NULL, NULL, NULL, WndProc);
	if (!hWnd)
		ExitFatal(L"Failed to create window");
	gdata.h_ = hWnd;
	SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)ghTrayIcon);
	SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)ghTrayIcon);

	InitMonitor(hWnd);


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

