// DirNotify.cpp : Defines the entry point for the application.
//

#include "stdafx.h"

#include "gitrev.h"
#include "DirNotify.h"
#include "NotifyPod.h"
#include "PopMessage.h"

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

vector<NotifyPod> gNotifyPods;

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

wstring getActionString(const int action) {
	switch (action) {
	case FILE_ACTION_ADDED: return I18N(L"Added");
	case FILE_ACTION_REMOVED: return I18N(L"Removed");
	case FILE_ACTION_MODIFIED: return I18N(L"Modified");
	case FILE_ACTION_RENAMED_OLD_NAME: return I18N(L"Renamed");
	case FILE_ACTION_RENAMED_NEW_NAME: return I18N(L"Renamed");
	}
	return L"UNKNOWN";
};
wstring getFileOrDirectoryString(DWORD fileAttribute)
{
	if (fileAttribute==FILE_ATTRIBUTE_NORMAL)
		return I18N(L"File");
	if (fileAttribute == FILE_ATTRIBUTE_DIRECTORY)
		return I18N(L"Directory");
	return I18N(L"File or Directory");
}
void OnChanged(HWND hWnd, LPCTSTR pDir, vector<NotifyPair>* pNotifyPairs)
{
#ifdef _DEBUG
	{
		wstringstream message;
		for (size_t i = 0; i < pNotifyPairs->size(); ++i)
		{
			message << L"-------------------Start-----------------\r\n";
			message << pDir << L":" << i << L":" << getActionString( (*pNotifyPairs)[i].getAction() ) << L":" << (*pNotifyPairs)[i].getData() << L"\r\n";
			message << L"-------------------End-----------------\r\n";
		}
		DTRACE(message.str());
	}
#endif
	if (pNotifyPairs->empty())
		return;

	gNotifyPods.push_back(NotifyPod(pDir, *pNotifyPairs));

	std::thread afterrun([](HWND h) {
		Sleep(3 * 1000);
		PostMessage(h, WM_APP_AFTER_NOTIFIED, 0, 0);
		}, hWnd);
	afterrun.detach();
}

void doShowPopup(HWND hWnd, 
	const vector<PopMessage>& popMessages, 
	bool isDesktop,
	DWORD dwInfoFlags = NIIF_INFO)
{
	if (popMessages.size() == 0)
		return;

	if(isDesktop)
	{
		std::thread thd([&] {
			Sleep(3 * 1000);
			PostMessage(hWnd, WM_APP_REFRESH_DESKTOP, 0, 0);
			});
		thd.detach();
	}

	for (auto&& pm : popMessages)
	{
		gNotifyHistory.push_back(pair<time_t, wstring>(time(nullptr), pm.ToString()));
	}

	// Get continuous message from tail
	wstring fixedFirstLine = popMessages[popMessages.size() - 1].getFirstLine();
	vector<wstring> secondLines;
	for (auto&& pm : popMessages)
	{
		if (pm.getFirstLine() != fixedFirstLine)
		{
			secondLines.clear();
			continue;
		}
		secondLines.push_back(pm.getSecondLine());
	}

	DVERIFY_LE(PopupTrayIcon(gdata.h_, 
		WM_APP_TRAY_NOTIFY,
		ghTrayIcon, APP_NAME,
		(fixedFirstLine + L"\r\n" + stdosd::stdJoinStrings(secondLines, L"\r\n", L"", L"")).c_str(),
		dwInfoFlags));
	if (gdata.isSound_)
	{
		PlaySound(gdata.wavFile_.c_str(), nullptr, SND_FILENAME | SND_ASYNC);
	}
}

void OnAfterNotified(HWND hWnd, const size_t thisMessageID)
{
	if (gNotifyPods.size() == 0)
		return;
	for (auto&& pod : gNotifyPods)
		pod.refinePods();

	vector<PopMessage> popMessages;
	bool isDesktop = false;
	auto fnProcessPod = [&]()
		{
			for (auto&& pod : gNotifyPods)
			{
				isDesktop |= stdIsSamePath(stdGetDesktopDirectory(), pod.getDir());
				if (pod.getCount() == 1)
				{
					const wstring data = pod.getData(0);
					const wstring full = stdCombinePath(pod.getDir(), data);
					const int action = pod.getAction(0);
					const wstring actionString = getActionString(action);
					const wstring strFileOrDirectory = getFileOrDirectoryString(pod.getFileAttribute(0));
					switch (action)
					{
					case FILE_ACTION_ADDED:
					case FILE_ACTION_REMOVED:
					case FILE_ACTION_MODIFIED:
						if (action == FILE_ACTION_MODIFIED && stdDirectoryExists(full))
						{
							// Skip directory modification message
						}
						else
						{
							popMessages.emplace_back(PopMessage(
								boost::str(boost::wformat(I18N(L"%1% has been %2%")) %
									strFileOrDirectory % actionString),
								full));
						}
					}
				}
				else if (pod.getCount() == 2)
				{
					if (pod.getAction(0) == FILE_ACTION_RENAMED_OLD_NAME &&
						pod.getAction(1) == FILE_ACTION_RENAMED_NEW_NAME )
					{
						// Renamed
						const wstring dataFrom = pod.getData(0);
						const wstring fullFrom = stdCombinePath(pod.getDir(), dataFrom);
						const wstring dataTo = pod.getData(1);
						const wstring fullTo = stdCombinePath(pod.getDir(), dataTo);
						const wstring actionString = I18N(L"Renamed");
						const wstring strFileOrDirectory = getFileOrDirectoryString(pod.getFileAttribute(0));

						popMessages.emplace_back(PopMessage(
							boost::str(boost::wformat(I18N(L"%1% has been %2% in %3%")) %
								strFileOrDirectory %
								actionString %
								pod.getDir()),
							boost::str(boost::wformat(I18N(L"%1% -> %2%")) %
								dataFrom % dataTo)));
					}
				}
			}
		};

	fnProcessPod();

	doShowPopup(hWnd, popMessages, isDesktop);

	gNotifyPods.clear();
}

void OnMonitorDirRemoved(HWND hWnd, LPCTSTR pDir, FILE_NOTIFY_INFORMATION* fni)
{
	PopMessage popMessage(
		L"Monitor directory Removed",
		pDir);

	doShowPopup(hWnd, 
		{ popMessage },
		stdIsSamePath(stdGetDesktopDirectory(), pDir),
		NIIF_WARNING);
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
		wstring message;
		message += APP_NAME L" v" + GetVersionString(stdGetModuleFileName().c_str(),3);
		message += L"\r\n\r\n";
		message += I18N(L"Directories in monitor:");
		message += L"\r\n";
		for (auto&& mis : gdata.monitorInfos_)
		{
			message += boost::str(boost::wformat(I18N(L"'%1%' Target=%2% Subtree=%3%")) %
				mis.getDir() % mis.getMonitarTagetAsString() % mis.isSubTreeAsString());
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
		OnChanged(hWnd, (LPCTSTR)wParam, (vector<NotifyPair>*)lParam);
		break;
	case WM_APP_MONITOR_DIR_REMOVED:
		OnMonitorDirRemoved(hWnd, (LPCTSTR)wParam, (FILE_NOTIFY_INFORMATION*)lParam);
		break;
	case WM_APP_REFRESH_DESKTOP:
		SHChangeNotify(0x8000000, 0x1000, 0, 0);
		break;
	case WM_APP_AFTER_NOTIFIED:
		OnAfterNotified(hWnd, (size_t)wParam);
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
	parser.AddOptionRange({ L"-h", L"--help", L"/?", L"/h", L"/help" },
		0,
		&isHelp,
		ArgEncodingFlags::ArgEncodingFlags_Default,
		I18N(L"Show Help"));

	bool isVersion = false;
	parser.AddOptionRange({ L"-v", L"--version", L"/v", L"/version" },
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
	if (IsDuplicateInstance(L"DirNotify_Mutex"))
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

	if (isMonitorFileOfDesktop)
	{
		MonitorInfo mi(stdGetDesktopDirectory(), true, false, false);
		gdata.monitorInfos_.push_back(mi);
	}
	if (isMonitorDirOfDesktop)
	{
		MonitorInfo mi(stdGetDesktopDirectory(), false, true, false);
		gdata.monitorInfos_.push_back(mi);
	}
	if (isMonitorFileOfDesktopAndSub)
	{
		MonitorInfo mi(stdGetDesktopDirectory(), true, false, true);
		gdata.monitorInfos_.push_back(mi);
	}
	if (isMonitorDirOfDesktopAndSub)
	{
		MonitorInfo mi(stdGetDesktopDirectory(), false, true, true);
		gdata.monitorInfos_.push_back(mi);
	}

	for (size_t i = 0; i < opMonitorFile.getValueCount(); ++i)
	{
		MonitorInfo mi(stdExpandEnvironmentStrings(opMonitorFile.getValue(i)), true, false, false);
		gdata.monitorInfos_.push_back(mi);
	}
	for (size_t i = 0; i < opMonitorDir.getValueCount(); ++i)
	{
		MonitorInfo mi(stdExpandEnvironmentStrings(opMonitorDir.getValue(i)), false, true, false);
		gdata.monitorInfos_.push_back(mi);
	}
	for (size_t i = 0; i < opMonitorFileSub.getValueCount(); ++i)
	{
		MonitorInfo mi(stdExpandEnvironmentStrings(opMonitorFileSub.getValue(i)), true, false, true);
		gdata.monitorInfos_.push_back(mi);
	}
	for (size_t i = 0; i < opMonitorDirSub.getValueCount(); ++i)
	{
		MonitorInfo mi(stdExpandEnvironmentStrings(opMonitorDirSub.getValue(i)), false, true, true);
		gdata.monitorInfos_.push_back(mi);
	}

	if (gdata.monitorInfos_.empty())
	{
		ExitFatal(I18N(L"No Directories specified"));
	}

#ifdef _DEBUG
	for (auto&& mi : gdata.monitorInfos_)
	{
		// Only monitor one of file or directory
		DASSERT(!(mi.isMonitorDirectory() && mi.isMonitorFile()));
	}
#endif

	for (auto&& mi : gdata.monitorInfos_)
	{
		if (PathFileExists(mi.getDir().c_str()) && !PathIsDirectory(mi.getDir().c_str()))
		{
			ExitFatal(stdFormat(I18N(L"'%s' is a file. A directory must be provided."), mi.getDir().c_str()));
		}
		if (!PathIsDirectory(mi.getDir().c_str()))
		{
			if (IDYES == MessageBox(NULL,
				stdFormat(I18N(L"Directory '%s' does not exist. Do you want to create it?"), mi.getDir().c_str()).c_str(),
				APP_NAME,
				MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2))
			{
				CreateCompleteDirectory(mi.getDir().c_str());
			}
		}
	}
	for (auto&& mi : gdata.monitorInfos_)
	{
		if (!PathIsDirectory(mi.getDir().c_str()))
		{
			ExitFatal(stdFormat(I18N(L"'%s' is not a directory."), mi.getDir().c_str()));
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

