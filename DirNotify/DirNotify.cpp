// DirNotify.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include <thread>

#include "../../lsMisc/CHandle.h"
#include "../../lsMisc/GetAllFile.h"
#include "../../lsMisc/SessionGlobalMemory/SessionGlobalMemory.h"
#include "../../lsMisc/CommandLineParser.h"
#include "../../lsMisc/GetVersionString.h"

#include "gitrev.h"
#include "DirNotify.h"

using namespace std;
using namespace Ambiesoft;
using namespace Ambiesoft::stdosd;

HICON ghTrayIcon;
GlobalData gdata;
CSessionGlobalMemory<HWND> sgHwnd("DirNotifyWindow");
DWORD gMainThreadId = GetCurrentThreadId();
bool IsMainThread()
{
	return GetCurrentThreadId() == gMainThreadId;
}

class MessageBod
{
	vector<wstring> messages_;
	size_t curID_ = -1;
public:
	size_t curID() const { return curID_; }
	void setMessage(wstring&& message) {
		assert(IsMainThread());
		curID_ = messages_.size();
		messages_.emplace_back(message);
	}
	bool IsCurID(size_t id) const {
		return curID_ == id;
	}
	const wstring& getMessage(size_t id) const {
		return messages_[id];
	}
	void clear() {
		messages_.clear();
		curID_ = -1;
	}
} gMessagePod;

void ExitFatal(LPCTSTR pError, DWORD dwLE)
{
	wstring error = pError;
	if (dwLE != NOERROR)
	{
		error += L"\r\n";
		error += GetLastErrorString(dwLE);
	}
	MessageBox(NULL, error.c_str(), APP_NAME, MB_ICONERROR);
	ExitProcess(-1);
}
void ExitFatal(const std::wstring& error, DWORD dwLE)
{
	ExitFatal(error.c_str(), dwLE);
}



void OnChanged(HWND hWnd, LPCTSTR pDir, FILE_NOTIFY_INFORMATION* fni)
{
	wstring file;
	{
		vector<WCHAR> text;
		text.assign(fni->FileName, fni->FileName + fni->FileNameLength / sizeof(WCHAR));
		text.push_back(L'\0');
		file = (LPCWSTR)&text[0];
	}

	const wstring filefull = stdCombinePath(pDir, file);

	WIN32_FIND_DATA wfd;
	if (!FindClose(FindFirstFile(filefull.c_str(), &wfd)))
	{
		DTRACE(L"FindFirstFile failed");
		return;
	}
	// skip if file size is zero
	if (wfd.nFileSizeHigh == 0 && wfd.nFileSizeLow == 0)
	{
		DTRACE(stdFormat(L"Canceled '%s':File size is zero", filefull.c_str()));
		return;
	}

	// check as if notify is too early
	DTRACE(stdFormat(L"'%s' has been changed.", filefull.c_str()));
	//constexpr DWORD MinTickDelta = 2000;
	//assert(IsMainThread());
	//static DWORD lastTick = 0;
	//const DWORD curTick = GetTickCount();
	//const DWORD tickDelta = curTick - lastTick;
	//if (tickDelta < MinTickDelta)
	//{
	//	// Too early
	//	DTRACE(stdFormat(L"Canceled:TickDelta(%d) is too short.", tickDelta));
	//	return;
	//}
	//lastTick = curTick;

	// Does this lock file?
	//if (IsFileOpen(filefull.c_str()))
	//	return;


	{
		wstring message;
		message += I18N(L"LastWrite Changed");
		message += wstring() + L" '" + pDir + L"'";
		message += L"\r\n";

		message += stdFormat(I18N(L"Size=%d bytes"), wfd.nFileSizeLow);
		message += L"\r\n";

		message += file;

		gMessagePod.setMessage(move(message));
	}

	DTRACE_WITHCOUT(L"NotifyCounter", L"Notify!");

	std::thread afterrun([](HWND h, size_t messageId) {
		Sleep(3 * 1000);
		PostMessage(h, WM_APP_AFTER_NOTIFIED, (WPARAM)messageId, 0);
		}, hWnd, gMessagePod.curID());
	afterrun.detach();
}

UINT WM_TASKBARCREATED;


vector<wstring> gMenuFiles;
HMENU CreateFileMenu()
{
	HMENU hSubMenu = CreatePopupMenu();

	gMenuFiles = GetAllFiles(stdGetDesktopDirectory(), GETALLFILES_SORT_LASTWRITETIME);
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
	
	const int indexOpenMenu = GetMenuItemCount(hSubMenu);

	AppendMenu(hSubMenu, MF_SEPARATOR, 0, NULL);

	AppendMenu(hSubMenu, MF_BYCOMMAND, IDC_ABOUT, (L"&About..."));
	AppendMenu(hSubMenu, MF_BYCOMMAND, IDC_SHOWHELP, (L"&Help..."));
	AppendMenu(hSubMenu, MF_SEPARATOR, 0, NULL);

	AppendMenu(hSubMenu, MF_BYCOMMAND, IDC_QUIT, (L"&Exit"));
	
	i18nChangeMenuText(hSubMenu);

	// should not I18Ned
	InsertMenu(hSubMenu, indexOpenMenu, MF_POPUP|MF_BYPOSITION, (UINT_PTR)CreateFileMenu(), 
		I18N(L"&Open"));

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
		message += APP_NAME L" v" + GetVersionString(stdGetModuleFileName().c_str(),3);
		message += _T("\r\n\r\n");
		message += I18N(L"Directories in monitor:");
		message += L"\r\n";
		for (auto&& dir : gdata.dirs_)
		{
			message += dir;
			message += L"\r\n";
		}
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
		InitMonitors();
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
		OnChanged(hWnd, (LPCTSTR)wParam, (FILE_NOTIFY_INFORMATION*)lParam);
		break;
	case WM_APP_REFRESH_DESKTOP:
		SHChangeNotify(0x8000000, 0x1000, 0, 0);
		break;
	case WM_APP_AFTER_NOTIFIED:
		{
			size_t thisMessageID = (size_t)wParam;
			if (!gMessagePod.IsCurID(thisMessageID))
				break;
			wstring message = gMessagePod.getMessage(thisMessageID);
			gMessagePod.clear();
			// if (stdIsSamePath(stdGetDesktopDirectory(), pDir))
			{
				std::thread thd([&] {
					Sleep(3 * 1000);
					PostMessage(hWnd, WM_APP_REFRESH_DESKTOP, 0, 0);
					});
				thd.detach();
			}
			
			DVERIFY_LE(PopupTrayIcon(gdata.h_, WM_APP_TRAY_NOTIFY, ghTrayIcon, APP_NAME, message.c_str()));
		}
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
			InitMonitors();
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

	CCommandLineParser parser;
	bool isDesktop = false;
	parser.AddOption(L"-desktop", 0, &isDesktop,
		ArgEncodingFlags::ArgEncodingFlags_Default,
		I18N(L"Monitor Desktop directory"));

	bool isHelp = false;
	parser.AddOptionRange({ L"-h",L"--help",L"/h",L"/help" },
		0,
		&isHelp,
		ArgEncodingFlags::ArgEncodingFlags_Default,
		I18N(L"Show Help"));

	bool isVersion = false;
	parser.AddOptionRange({ L"-v",L"--version",L"/v",L"/version" },
		0,
		&isVersion,
		ArgEncodingFlags::ArgEncodingFlags_Default,
		I18N(L"Show Version"));

	COption opDir(wstring(), ArgCount::ArgCount_Infinite,
		ArgEncodingFlags::ArgEncodingFlags_Default,
		I18N(L"Directories to monitor"));
	parser.AddOption(&opDir);

	parser.Parse();

	if (parser.hadUnknownOption())
	{
		ExitFatal(I18N(L"Unknown Options:") + parser.getUnknowOptionStrings());
	}
	if (isHelp)
	{
		MessageBox(NULL, parser.getHelpMessage().c_str(), APP_NAME, MB_ICONINFORMATION);
		return 0;
	}
	if (isVersion)
	{
		wstring message = APP_NAME L" v" + GetVersionString(stdGetModuleFileName().c_str(), 3);
		message += L"\r\n\r\n";
		message += L"GitRev:\r\n";
		message += stdStringReplace(GITREV::GetHashMessage(), L"\n", L"\r\n");
		MessageBox(NULL, message.c_str(), APP_NAME, MB_ICONINFORMATION);
		return 0;
	}
	if (!CheckDuplicateInstance())
	{
		if (sgHwnd == nullptr)
			ExitFatal(I18N(L"This is duplicated instance but failed to find previous one."));
		else
		{
			PostMessage(sgHwnd, WM_APP_ACTIVATE, 0, 0);
		}
		return 1;
	}

	if (isDesktop)
	{
		gdata.dirs_.push_back(stdGetDesktopDirectory());
	}
	for (size_t i = 0; i < opDir.getValueCount(); ++i)
	{
		gdata.dirs_.push_back(stdExpandEnvironmentStrings(opDir.getValue(i)));
	}
	if (gdata.dirs_.empty())
	{
		ExitFatal(I18N(L"No dirs"));
	}

	for (auto&& dir : gdata.dirs_)
	{
		if (!PathIsDirectory(dir.c_str()))
		{
			if (IDYES == MessageBox(NULL,
				stdFormat(I18N(L"Directory '%s' does not exist. Do you want to create it?"), dir.c_str()).c_str(),
				APP_NAME,
				MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2))
			{
				CreateDirectory(dir.c_str(), NULL);
			}
		}
	}
	for (auto&& dir : gdata.dirs_)
	{
		if (!PathIsDirectory(dir.c_str()))
		{
			ExitFatal(stdFormat(I18N(L"'%s' is not a directory."), dir.c_str()));
		}
	}

	ghTrayIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_DirNotify));
	DVERIFY(ghTrayIcon);

	HWND hWnd = CreateSimpleWindow(NULL, NULL, NULL, WndProc);
	if (!hWnd)
		ExitFatal(I18N(L"Failed to create window."));
	gdata.h_ = hWnd;
	SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)ghTrayIcon);
	SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)ghTrayIcon);

	InitMonitors();


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

