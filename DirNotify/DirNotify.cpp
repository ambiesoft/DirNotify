// DirNotify.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include <thread>

#include "../../lsMisc/CHandle.h"
#include "../../lsMisc/GetAllFile.h"
#include "../../lsMisc/SessionGlobalMemory/SessionGlobalMemory.h"
#include "../../lsMisc/CommandLineParser.h"
#include "../../lsMisc/GetVersionString.h"
#include "../../lsMisc/CreateCompleteDirectory.h"

#include "gitrev.h"
#include "DirNotify.h"

// PlaySound
#pragma comment(lib,"Winmm.lib")

using namespace std;
using namespace Ambiesoft;
using namespace Ambiesoft::stdosd;

HICON ghTrayIcon;
GlobalData gdata;
vector<pair<time_t,wstring>> gNotifyHistory;
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
	wstring newFile;
	if(fni->NextEntryOffset != 0)
	{
		fni = (FILE_NOTIFY_INFORMATION*) ((BYTE*)fni + fni->NextEntryOffset);
		vector<WCHAR> text;
		text.assign(fni->FileName, fni->FileName + fni->FileNameLength / sizeof(WCHAR));
		text.push_back(L'\0');
		newFile = (LPCWSTR)&text[0];
	}
	const wstring filefull = stdCombinePath(pDir, file);
	if (stdFileExists(filefull))
	{
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

		wstring message;
		message += I18N(L"LastWrite Changed");
		message += wstring() + L" '" + pDir + L"'";
		message += L"\r\n";

		message += stdFormat(I18N(L"Size=%d bytes"), wfd.nFileSizeLow);
		message += L"\r\n";

		message += file;
		gMessagePod.setMessage(move(message));
	}
	else
	{
		if(newFile.empty())
		{
			wstringstream message;
			if (stdDirectoryExists(filefull))
				message << I18N(L"Directory Created");
			else
				message << I18N(L"Directory Removed");
			message << L"\r\n";
			message << filefull;
			gMessagePod.setMessage(message.str());
		}
		else
		{
			wstringstream message;
			message << I18N(L"Directory Changed");
			message << L"\r\n";
			message << file << L" -> " << newFile;
			gMessagePod.setMessage(message.str());
		}
	}



	DTRACE_WITHCOUT(L"NotifyCounter", L"Notify!");

	std::thread afterrun([](HWND h, size_t messageId) {
		Sleep(3 * 1000);
		PostMessage(h, WM_APP_AFTER_NOTIFIED, (WPARAM)messageId, 0);
		}, hWnd, gMessagePod.curID());
	afterrun.detach();
}

void OnDirRemoved(HWND hWnd, LPCTSTR pDir, FILE_NOTIFY_INFORMATION* fni)
{
	{
		wstring message;
		message += I18N(L"Directory Removed");
		message += wstring() + L" '" + pDir + L"'";
		message += L"\r\n";

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
	AppendMenu(hSubMenu, MF_BYCOMMAND, IDC_NOTIFICATION_LOG, (L"&Notification log..."));
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
		for (auto&& mis : gdata.monitorInfos_)
		{
			message += mis.dir_;
			message += L"\r\n";
		}
		MessageBox(NULL,
			message.c_str(),
			APP_NAME,
			MB_ICONINFORMATION);
	}
	break;

	case IDC_NOTIFICATION_LOG:
	{
		wstring caption = stdFormat(L"%s - %s", I18N(L"Notification log"), APP_NAME);
		wstringstream message;
		wstring separator = L"---------------------";
		if (gNotifyHistory.empty())
		{
			message << I18N(L"No notifications");
		}
		else
		{
			for (size_t i = 0; i < gNotifyHistory.size(); ++i)
			{
				time_t time = gNotifyHistory[i].first;
				//std::tm tm;
				//localtime_s(&tm, &time);
				wchar_t buffer[128] = { 0 };
				_wctime_s(buffer, _countof(buffer), &time);
				// Format: Mo, 15.06.2009 20:20:00
				// std::wcsftime(buffer, _countof(buffer), L"%a, %d.%m.%Y %H:%M:%S", &tm);
				message << buffer;
				message << gNotifyHistory[i].second << L"\r\n";
				message << separator << L"\r\n";
			}
		}
		wstring shownotifyexe = stdCombinePath(stdGetParentDirectory(stdGetModuleFileName()), L"..\\ShowFlexibleMessageBox\\ShowFlexibleMessageBox.exe");
		if (!stdFileExists(shownotifyexe))
		{
			MessageBox(gdata.h_,
				stdFormat(I18N(L"'%s' was not found."), shownotifyexe.c_str()).c_str(),
				APP_NAME,
				MB_ICONWARNING);
		}
		else
		{
			string encodedMessage = UrlEncodeStd(toStdUtf8String(message.str()).c_str());
			CDynamicSessionGlobalMemory sgDyn("dyn", (size_t32)encodedMessage.size());
			{
				sgDyn.set((const unsigned char*)encodedMessage.data());
			}

			string commandLine = stdFormat("-t \"%s\" -m \"%s\"",
				UrlEncodeStd(toStdUtf8String(caption).c_str()).c_str(),
				sgDyn.getMapName().c_str());
				
			CKernelHandle process;
			OpenCommon(gdata.h_,
				shownotifyexe.c_str(),
				toStdWstringFromUtf8(commandLine).c_str(),
				nullptr,
				&process);
			WaitForInputIdle(process, INFINITE);
		}
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
	case WM_APP_DIRREMOVED:
		OnDirRemoved(hWnd, (LPCTSTR)wParam, (FILE_NOTIFY_INFORMATION*)lParam);
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
			
			gNotifyHistory.push_back(pair<time_t,wstring>(time(nullptr), message));
			DVERIFY_LE(PopupTrayIcon(gdata.h_, WM_APP_TRAY_NOTIFY, ghTrayIcon, APP_NAME, message.c_str()));
			if (gdata.isSound_)
			{
				PlaySound(gdata.wavFile_.c_str(), nullptr, SND_FILENAME | SND_ASYNC);
			}
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



CKernelHandle hDupCheck;
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

	CCommandLineParser parser(I18N(L"Notification of file/directory changes"), APP_NAME);
	parser.setStrict();

	bool isMonitorFileOfDesktop = false;
	bool isMonitorDirOfDesktop = false;
	bool isMonitorFileOfDesktopAndSub = false;
	bool isMonitorDirOfDesktopAndSub = false;

	parser.AddOptionRange({ L"-df", L"--desktop-file" },
		ArgCount::ArgCount_Zero,
		& isMonitorFileOfDesktop,
		ArgEncodingFlags::ArgEncodingFlags_Default,
		I18N(L"Monitor Files of Desktop"));
	parser.AddOptionRange({ L"-dd", L"--desktop-directory" },
		ArgCount::ArgCount_Zero,
		& isMonitorDirOfDesktop,
		ArgEncodingFlags::ArgEncodingFlags_Default,
		I18N(L"Monitor Directories of Desktop"));
	parser.AddOptionRange({ L"-dfs", L"--desktop-file-subtree" },
		ArgCount::ArgCount_Zero,
		& isMonitorFileOfDesktopAndSub,
		ArgEncodingFlags::ArgEncodingFlags_Default,
		I18N(L"Monitor Files of Desktop and its subdirectories"));
	parser.AddOptionRange({ L"-dds", L"--desktop-directory-subtree" },
		ArgCount::ArgCount_Zero,
		& isMonitorDirOfDesktopAndSub,
		ArgEncodingFlags::ArgEncodingFlags_Default,
		I18N(L"Monitor Directories of Desktop and its subdirectories"));

	parser.AddOptionRange({ L"-s",L"--sound"},
		ArgCount::ArgCount_One,
		&gdata.isSound_,
		ArgEncodingFlags::ArgEncodingFlags_Default,
		I18N(L"Plays sound on notification. '0', 'off' or 'no' to disable it."));

	parser.AddOptionRange({ L"-w",L"--wavfile" },
		ArgCount::ArgCount_One,
		&gdata.wavFile_,
		ArgEncodingFlags::ArgEncodingFlags_Default,
		I18N(L"Wav-file to play sound on notification"));

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

	COption opMonitorFile(L"-mf",
		ArgCount::ArgCount_One,
		ArgEncodingFlags::ArgEncodingFlags_Default,
		I18N(L"Directory to monitor Files"));
	parser.AddOption(&opMonitorFile);

	COption opMonitorDir(L"-md",
		ArgCount::ArgCount_One,
		ArgEncodingFlags::ArgEncodingFlags_Default,
		I18N(L"Directory to monitor Directories"));
	parser.AddOption(&opMonitorDir);

	COption opMonitorFileSub(L"-mfs",
		ArgCount::ArgCount_One,
		ArgEncodingFlags::ArgEncodingFlags_Default,
		I18N(L"Directory and its subdirectories to monitor Files"));
	parser.AddOption(&opMonitorFileSub);

	COption opMonitorDirSub(L"-mds",
		ArgCount::ArgCount_One,
		ArgEncodingFlags::ArgEncodingFlags_Default,
		I18N(L"Directory and its subdirectories to monitor Directories"));
	parser.AddOption(&opMonitorDirSub);

	try
	{
		parser.Parse();
	}
	catch (illegal_value_type_error<wstring, bool>& ex)
	{
		ExitFatal(ex.wwhat());
	}
	catch (no_value_error<wstring>& ex)
	{
		ExitFatal(ex.wwhat());
	}

	if (parser.hadUnknownOption())
	{
		ExitFatal(I18N(L"Unknown Option(s):") + parser.getUnknowOptionStrings());
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
			// PostMessage(sgHwnd, WM_APP_ACTIVATE, 0, 0);
			MessageBox(NULL, I18N(L"Another instance is already running."), APP_NAME, MB_ICONWARNING);
		}
		return 1;
	}

	if (gdata.wavFile_.empty())
		gdata.wavFile_ = stdCombinePath(stdGetParentDirectory(stdGetModuleFileName()), L"..\\chime.wav");
	else
		gdata.wavFile_ = stdExpandEnvironmentStrings(gdata.wavFile_);
	if (gdata.isSound_)
	{
		if (!stdFileExists(gdata.wavFile_))
		{
			MessageBox(NULL, 
				stdFormat(I18N(L"Wave file '%s' does not exit."), gdata.wavFile_.c_str()).c_str(),
				APP_NAME, MB_ICONWARNING);
			return 1;
		}
	}
	if (isMonitorFileOfDesktop || isMonitorDirOfDesktop ||
		isMonitorFileOfDesktopAndSub || isMonitorDirOfDesktopAndSub)
	{
		MonitorInfo mi;
		if(isMonitorFileOfDesktop)
			mi.monitorFile_ = true;
		if (isMonitorDirOfDesktop)
			mi.monitorDir_ = true;
		if (isMonitorFileOfDesktopAndSub)
		{
			mi.monitorFile_ = true;
			mi.monitorSub_ = true;
		}
		if (isMonitorDirOfDesktopAndSub)
		{
			mi.monitorDir_ = true;
			mi.monitorSub_ = true;
		}
		mi.dir_ = stdGetDesktopDirectory();
		gdata.monitorInfos_.push_back(mi);
	}

	for (size_t i = 0; i < opMonitorFile.getValueCount(); ++i)
	{
		MonitorInfo mi;
		mi.monitorFile_ = true;
		mi.dir_ = stdExpandEnvironmentStrings(opMonitorFile.getValue(i));
		gdata.monitorInfos_.push_back(mi);
	}
	for (size_t i = 0; i < opMonitorDir.getValueCount(); ++i)
	{
		MonitorInfo mi;
		mi.monitorDir_ = true;
		mi.dir_ = stdExpandEnvironmentStrings(opMonitorDir.getValue(i));
		gdata.monitorInfos_.push_back(mi);
	}
	for (size_t i = 0; i < opMonitorFileSub.getValueCount(); ++i)
	{
		MonitorInfo mi;
		mi.monitorFile_ = true;
		mi.monitorSub_ = true;
		mi.dir_ = stdExpandEnvironmentStrings(opMonitorFileSub.getValue(i));
		gdata.monitorInfos_.push_back(mi);
	}
	for (size_t i = 0; i < opMonitorDirSub.getValueCount(); ++i)
	{
		MonitorInfo mi;
		mi.monitorDir_ = true;
		mi.monitorSub_ = true;
		mi.dir_ = stdExpandEnvironmentStrings(opMonitorDirSub.getValue(i));
		gdata.monitorInfos_.push_back(mi);
	}

	if (gdata.monitorInfos_.empty())
	{
		ExitFatal(I18N(L"No Directories specified"));
	}

	for (auto&& mi : gdata.monitorInfos_)
	{
		if (PathFileExists(mi.dir_.c_str()) && !PathIsDirectory(mi.dir_.c_str()))
		{
			ExitFatal(stdFormat(I18N(L"'%s' is a file. A directory must be provided."), mi.dir_.c_str()));
		}
		if (!PathIsDirectory(mi.dir_.c_str()))
		{
			if (IDYES == MessageBox(NULL,
				stdFormat(I18N(L"Directory '%s' does not exist. Do you want to create it?"), mi.dir_.c_str()).c_str(),
				APP_NAME,
				MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2))
			{
				CreateCompleteDirectory(mi.dir_.c_str());
			}
		}
	}
	for (auto&& mi : gdata.monitorInfos_)
	{
		if (!PathIsDirectory(mi.dir_.c_str()))
		{
			ExitFatal(stdFormat(I18N(L"'%s' is not a directory."), mi.dir_.c_str()));
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

