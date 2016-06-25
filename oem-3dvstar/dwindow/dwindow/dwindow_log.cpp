#include "dwindow_log.h"
#include <stdio.h>
#include <Windows.h>
#include <atlbase.h>
#include <streams.h>
#include <Userenv.h>
#include <time.h>

#pragma comment(lib, "Userenv.lib")

CCritSec dwindow_log_cs;
FILE * dwindow_log_file = NULL;
FILE * getfile();
void closefile();
wchar_t dwindow_file_name[MAX_PATH] = {0};

int dwindow_log_line(const wchar_t *format, ...)
{
	FILE * f = getfile();
	if (!f)
		return -1;

	wchar_t tmp[10240];
	va_list valist;
	va_start(valist, format);
	vswprintf(tmp, format, valist);
	va_end(valist);

	time_t tt = time(NULL);
	struct tm t = *localtime(&tt);
	wchar_t time_str[200];	
	wsprintfW(time_str, L"%d-%02d-%02d %02d:%02d:%02d:%03d ", t.tm_year+1900, t.tm_mon+1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, GetTickCount()%1000);
	fwrite(time_str, 2, wcslen(time_str), f);
	fwrite(tmp, 2, wcslen(tmp), f);
	fwrite(L"\r\n", 2, 2, f);
	fflush(f);

	closefile();

	return 0;
}

int dwindow_log_line(const char *format, ...)
{
	FILE * f = getfile();
	if (!f)
		return -1;

	char tmp[10240];
	va_list valist;
	va_start(valist, format);
	vsprintf(tmp, format, valist);
	va_end(valist);

	time_t tt = time(NULL);
	struct tm t = *localtime(&tt);
	wchar_t time_str[200];	
	wsprintfW(time_str, L"%d-%02d-%02d %02d:%02d:%02d:%03d ", t.tm_year, t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, GetTickCount()%1000);
	fwrite(time_str, 2, wcslen(time_str), f);

	USES_CONVERSION;
	const wchar_t *p = A2W(tmp);
	fwrite(p, 2, wcslen(p), f);

	fwrite(L"\r\n", 2, 2, f);
	fflush(f);

	closefile();

	return 0;
}


const wchar_t *dwindow_log_get_filename()
{
	dwindow_log_cs.Lock();

	if (dwindow_file_name[0] == NULL)
	{
		DWORD buf_size = MAX_PATH;
		HANDLE handle = NULL;
		OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &handle);
		GetUserProfileDirectory(handle, dwindow_file_name, &buf_size);
		CloseHandle(handle);
		wcscat(dwindow_file_name, L"\\DWindow\\");
		_wmkdir(dwindow_file_name);
		wcscat(dwindow_file_name, L"LastLog.log");
		_wremove(dwindow_file_name);
	}

	dwindow_log_cs.Unlock();

	return dwindow_file_name;
}

FILE * getfile()
{
	int s = 0;
	while(true)
	{
		dwindow_log_cs.Lock();
		if(!dwindow_log_file)
			break;
		else
		{
			dwindow_log_cs.Unlock();
			Sleep(s);
			s = 1;
		}
	}

	dwindow_log_file = _wfopen(dwindow_log_get_filename(), L"ab");
	if (dwindow_log_file == NULL)
		dwindow_log_file = _wfopen(dwindow_log_get_filename(), L"wb");

	dwindow_log_cs.Unlock();
	return dwindow_log_file;
}
void closefile()
{
	dwindow_log_cs.Lock();
	if (dwindow_log_file)
		fclose(dwindow_log_file);
	dwindow_log_file = NULL;
	dwindow_log_cs.Unlock();
}