#include "stdafx.h"
#include <Windows.h>
#include <tchar.h>
#include <Shlwapi.h>

#include <string>

#include "../../lsMisc/stdosd/stdosd.h"
#include "../../lsMisc/Is64.h"
#include "../../lsMisc/OpenCommon.h"
#include "../../lsMisc/CommandLineString.h"
#include "../../lsMisc/CHandle.h"

#include "resource.h"

using namespace Ambiesoft::stdosd;
using namespace Ambiesoft;
using namespace std;

void ExitFatal(wstring error)
{
	MessageBox(NULL, error.c_str(), APP_NAME, MB_ICONERROR);
	ExitProcess(-1);
}
int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPTSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);


	wstring exe = stdCombinePath(stdGetParentDirectory(stdGetModuleFileName()),
		stdFormat(L"%s\\DirNotify.exe", Is64BitWindows() ? L"x64":L"x86"));
	if (!PathFileExists(exe.c_str()))
		ExitFatal(stdFormat(L"'%s' does not exit", exe.c_str()));

	CCommandLineString cmd;
	wstring dummy, args;
	CKernelHandle hProcess;
	cmd.ExplodeExeAndArg(GetCommandLine(), dummy, args);
	if (!OpenCommon(NULL,
		exe.c_str(),
		args.c_str(),
		stdGetCurrentDirectory().c_str(),
		&hProcess))
		ExitFatal(L"Failed to open");

	// WaitForSingleObject(hProcess, INFINITE);
	return 0;
}