#include <Windows.h>
#include <Tlhelp32.h>
#pragma comment(lib, "shlwapi.lib")

int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) 
{
	int argc = 1;
	LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);

	if (argc<2)
		return 0;

	const wchar_t *exe_name = wcsrchr(argv[1], L'\\');
	exe_name = exe_name == NULL ? argv[1] : exe_name+1;

	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 pe32; 
	pe32.dwSize = sizeof(PROCESSENTRY32);
	if (Process32First(hSnap, &pe32))
	{
		do
		{
			if (!wcsicmp(pe32.szExeFile, exe_name))
			{
				HANDLE process = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
				TerminateProcess(process, -1);
				CloseHandle(process);
			}
		} while (Process32Next(hSnap, &pe32));
	}

	SHELLEXECUTEINFO ShExecInfo = {0};
	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	ShExecInfo.hwnd = NULL;
	ShExecInfo.lpFile = argv[1];
	ShExecInfo.lpParameters = NULL;	
	ShExecInfo.lpDirectory = NULL;
	ShExecInfo.nShow = SW_SHOW;
	ShExecInfo.hInstApp = NULL;	
	ShExecInfo.lpVerb = L"open";

	ShellExecuteEx(&ShExecInfo);

	return false;
}