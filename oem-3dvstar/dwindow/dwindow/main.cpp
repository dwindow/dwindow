#include "dx_player.h"
#include "login.h"
#include "..\lua\my12doom_lua.h"
#include <locale.h>
#include <Dbghelp.h>
#include "..\renderer_prototype\my12doomRenderer_lua.h"
#include "dwindow_log.h"
#include "zip.h"
#include "runnable.h"
#include <atlbase.h>
#include "ui_lua.h"
#include "dshow_lua.h"

#pragma comment(lib, "DbgHelp")

// main window
int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
LONG WINAPI my_handler(struct _EXCEPTION_POINTERS *ExceptionInfo);
extern HRESULT dwindow_dll_go(HINSTANCE inst, HWND owner, Iplayer *p);
namespace zhuzhu
{
	extern HRESULT dwindow_dll_go_zhuzhu(HINSTANCE inst, HWND owner, Iplayer *p);
}
void DisableSetUnhandledExceptionFilter();
int enable_hookdshow();
int disable_hookdshow();

static int writer(lua_State* L, const void* p, size_t size, void* u)
{
	return (fwrite(p,size,1,(FILE*)u)!=1) && (size!=0);
}

#include <Shlobj.h>
#pragma comment(lib, "Shell32.lib")
int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) 
{
 	SetUnhandledExceptionFilter(my_handler);
	DisableSetUnhandledExceptionFilter();
	CoInitialize(NULL);

	int argc = 1;
	LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);


	RECT screen1;
	RECT screen2;
	if (FAILED(get_monitors_rect(&screen1, &screen2)))
	{
		MessageBoxW(0, C(L"System initialization failed : monitor detection error, the program will exit now."), L"Error", MB_OK);
		return -1;
	}


	dwindow_lua_init();
	ui_lua_init();
	player_lua_init();
	my12doomRenderer_lua_init();
	my12doomRenderer_lua_loadscript();

	check_login();
	save_passkey();
	BasicRsaCheck();

	// always do File Association
	update_file_association();


	HWND pre_instance = GET_CONST("SingleInstance") ? FindWindowA("DWindowClass", NULL) : NULL;
	wchar_t *argvv[] = {L"", L"compile", L"D:\\private\\dwindow_UI\\dwindow.lua", L"D:\\private\\dwindow_UI\\dwindow.bin"};
	//argv = argvv;
	//argc=4;

	if (argc == 2 && wcsicmp(argv[1], L"association") == 0)
		ExitProcess(0);

	if (argc == 4 && wcsicmp(argv[1], L"compile") == 0)
	{
		// the lua compiler
		USES_CONVERSION;
		luaState L;

		if (luaL_loadfile(L, W2A(argv[2])))
		{
			const char * result;
			result = lua_tostring(L, -1);
			printf("failed loading lua script: %s\n", result);
		}
		else
		{
			FILE * f = _wfopen(argv[3], L"wb");
			if (!f)
			{
				wprintf(L"error opening output %s\n", argv[3]);
			}
			else
			{
				lua_dump2(L, writer, f, 1);
				wprintf(L"compile OK: %s\n", argv[3]);
				fclose(f);
			}
		}

		ExitProcess(0);
	}

	else if (argc == 2 && wcsicmp(argv[1], L"uninstall") == 0)
	{
		luaState L;
		lua_getglobal(L, "core");
		lua_getfield(L, -1, "GetConfigFile");
		lua_mypcall(L, 0, 1, 0);
		UTF82W configfile(lua_tostring(L, -1));
		_wremove(configfile);
		ExitProcess(0);
	}

	if (pre_instance)
	{
		SendMessageW(pre_instance, WM_SYSCOMMAND, (WPARAM)SC_RESTORE, 0);
		SetForegroundWindow(pre_instance);
		if (argc>1)
		{
			if (wcsicmp(argv[1], L"cmd") != 0)
			{
				COPYDATASTRUCT copy = {WM_LOADFILE, wcslen(argv[1])*2+2, argv[1]};
				SendMessageW(pre_instance, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&copy);
			}
			else
			{
				wchar_t cmd[20480] = {0};
				for(int i=2; i<argc; i++)
				{
					wcscat(cmd, argv[i]);
					if (i!=argc-1)wcscat(cmd, L"|");
				}
				COPYDATASTRUCT copy = {WM_DWINDOW_COMMAND, wcslen(cmd)*2+2, cmd};
				SendMessageW(pre_instance, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&copy);
			}
		}
		return 0;
	}	

	enable_hookdshow();
	dx_player *test = new dx_player(hinstance);
	BringWindowToTop(test->m_hwnd1);

	if (argc>1)
	{
		test->reset_and_loadfile(argv[1], false);
		SetFocus(test->m_hwnd1);
	}

	setlocale(LC_ALL, "chs");

	while (test->init_done_flag != 0x12345678)
		Sleep(100);
	SendMessageW(test->m_hwnd1, WM_SYSCOMMAND, (WPARAM)SC_RESTORE, 0);
	SetForegroundWindow(test->m_hwnd1);

	DWORD rights = get_passkey_rights();

	if (rights & USER_RIGHTS_THEATER_BOX)
		zhuzhu::dwindow_dll_go_zhuzhu(hinstance, NULL, test);
	else
	while (!test->is_closed())
		Sleep(100);

	lua_save_settings();

 	HANDLE killer = CreateThread(NULL, NULL, killer_thread2, new DWORD(3000), NULL, NULL);
	bar_logout();
	delete test;
 	WaitForSingleObject(killer, 3000);
	return 0;
}

int main()
{
	WinMain(GetModuleHandle(NULL), 0, "", SW_SHOW);
}

LONG WINAPI my_handler(struct _EXCEPTION_POINTERS *ExceptionInfo)
{
	// mini dump
	wchar_t dump_file[MAX_PATH];
	wcscpy(dump_file, dwindow_log_get_filename());
	wcscpy(wcsrchr(dump_file, '\\'), L"\\DWindowDumpFile.dmp");
	HANDLE lhDumpFile = CreateFileW(dump_file, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL ,NULL);

	MINIDUMP_EXCEPTION_INFORMATION loExceptionInfo;
	loExceptionInfo.ExceptionPointers = ExceptionInfo;
	loExceptionInfo.ThreadId = GetCurrentThreadId();
	loExceptionInfo.ClientPointers = TRUE;
	MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),lhDumpFile, MiniDumpNormal, &loExceptionInfo, NULL, NULL);

	CloseHandle(lhDumpFile);

	// create zip file
	wchar_t zip[MAX_PATH];
	HZIP hz; DWORD writ;
	wcscpy(zip, dwindow_log_get_filename());
	wcscpy(wcsrchr(zip, '\\'), L"\\DWindowReport.zip");
	hz = CreateZip(zip,0);
	ZipAdd(hz,_T("Log.txt"), dwindow_log_get_filename());
	ZipAdd(hz,_T("DWindowDumpFile.dmp"), dump_file);
	CloseZip(hz);

	// upload it
	report_file(zip);

#ifdef ZHUZHU
	restart_this_program();
#endif

	// reset and suicide
	wchar_t description[1024];
	swprintf(description, C(L"Ooops....something bad happened. \n"
							L"Press Retry to debug or Cancel to RESTART progress.\n"
							L"Or continue at your own risk, the program will become UNSTABLE.\n"
							L"\nMini Dump File:\n%s"), dump_file);
	int o = MessageBoxW(NULL, description, C(L"Error"), MB_CANCELTRYCONTINUE | MB_ICONERROR);
	if (o == IDCANCEL)
		restart_this_program();
	else if (o == IDCONTINUE)
		TerminateThread(GetCurrentThread(), -1);
	DebugBreak();
	return EXCEPTION_CONTINUE_SEARCH;
}

void DisableSetUnhandledExceptionFilter()
{
	void *addr = (void*)GetProcAddress(LoadLibrary(_T("kernel32.dll")),
		"SetUnhandledExceptionFilter");
	if (addr)
	{
		unsigned char code[16];
		int size = 0;
		code[size++] = 0x33;
		code[size++] = 0xC0;
		code[size++] = 0xC2;
		code[size++] = 0x04;
		code[size++] = 0x00;

		DWORD dwOldFlag, dwTempFlag;
		VirtualProtect(addr, size, PAGE_READWRITE, &dwOldFlag);
		WriteProcessMemory(GetCurrentProcess(), addr, code, size, NULL);
		VirtualProtect(addr, size, dwOldFlag, &dwTempFlag);
	}
}