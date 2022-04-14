#include "stdafx.h"
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

	DTRACE(I18N(L"TEST"));

	wstring exeDirNotify = stdCombinePath(stdGetParentDirectory(stdGetModuleFileName()),
		stdFormat(L"%s\\DirNotify.exe", Is64BitWindows() ? L"x64":L"x86"));
	if (!PathFileExists(exeDirNotify.c_str()))
		ExitFatal(stdFormat(I18N(L"'%s' does not exit"), exeDirNotify.c_str()));

	CCommandLineString cmd;
	wstring dummy, args;
	CKernelHandle hProcess;
	cmd.ExplodeExeAndArg(GetCommandLine(), dummy, args);
	if (!OpenCommon(NULL,
		exeDirNotify.c_str(),
		args.c_str(),
		stdGetCurrentDirectory().c_str(),
		&hProcess))
	{
		ExitFatal(stdFormat(I18N(L"Failed to open '%s'"), exeDirNotify.c_str()));
	}

	// WaitForSingleObject(hProcess, INFINITE);
	return 0;
}