#include <stdio.h>
#include <Windows.h>
#include <atlbase.h>
#include <DShow.h>
#include "rijndael.h"

#include "detours\detours.h"
#pragma comment(lib, "detours\\detoured.lib")
#pragma comment(lib, "detours\\detours.lib")

//#define __in
//#define __out
//#define __out_opt
//#define __inout_opt

// just hook a few mostly used file functions
// GetFileSize&Ex
// SetFilePointer&Ex
// ReadFile
// CreateFileW ("A" version is just a call to W version)
static DWORD (WINAPI * TrueGetFileSize)(__in HANDLE hFile, __out  LPDWORD lpFileSizeHigh) = GetFileSize;
static BOOL (WINAPI * TrueGetFileSizeEx)(__in HANDLE hFile, __out  PLARGE_INTEGER lpFileSize) = GetFileSizeEx;
static DWORD (WINAPI *TrueSetFilePointer)(__in HANDLE hFile, __in LONG liDistanceToMove, __inout_opt PLONG lpDistanceToMoveHigh, __in DWORD dwMoveMethod) = SetFilePointer;
static BOOL (WINAPI *TrueSetFilePointerEx)(__in HANDLE hFile, __in LARGE_INTEGER liDistanceToMove, __out_opt PLARGE_INTEGER lpNewFilePointer, __in DWORD dwMoveMethod) = SetFilePointerEx;
static BOOL (WINAPI * TrueReadFile)(__in HANDLE hFile, __out LPVOID lpBuffer, __in DWORD nNumberOfBytesToRead, __out_opt LPDWORD lpNumberOfBytesRead, __inout_opt LPOVERLAPPED lpOverlapped) = ReadFile;
static HANDLE (WINAPI * TrueCreateFileW)(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,	DWORD dwFlagsAndAttributes,	HANDLE hTemplateFile) = CreateFileW;


class simple_file
{
public:
	simple_file(LPCWSTR name);
	LONGLONG size();
	LONGLONG seek(LONGLONG distance, int method);
	LONGLONG read(void *buf, LONGLONG size);
	LONGLONG tell();
protected:
	HANDLE m_file;
};

int hooked_count = 0;
HANDLE hooked_file[5];
simple_file file(L"Z:\\00019_sub.mkv");

DWORD WINAPI MineGetFileSize(__in HANDLE hFile, __out_opt LPDWORD lpFileSizeHigh)
{
	if (hFile == hooked_file)
	{
		LARGE_INTEGER size;
		size.QuadPart = file.size();
		if (lpFileSizeHigh) *lpFileSizeHigh = size.HighPart;
		return size.LowPart;
	}
	DWORD rv = TrueGetFileSize(hFile, lpFileSizeHigh);
	return rv;
}
BOOL WINAPI MineGetFileSizeEx(__in HANDLE hFile, __out  PLARGE_INTEGER lpFileSize)
{
	if (hFile == hooked_file)
	{
		lpFileSize->QuadPart = file.size();
		return TRUE;
	}
	BOOL rv = TrueGetFileSizeEx(hFile, lpFileSize);
	return rv;
}
DWORD WINAPI MineSetFilePointer(__in HANDLE hFile, __in LONG liDistanceToMove, __inout_opt PLONG lpDistanceToMoveHigh, __in DWORD dwMoveMethod)
{
	if (hFile == hooked_file)
	{
		LARGE_INTEGER newpos;
		newpos.HighPart = 0;
		newpos.LowPart = liDistanceToMove;
		if (lpDistanceToMoveHigh)
			newpos.HighPart = *lpDistanceToMoveHigh;

		newpos.QuadPart = file.seek(newpos.QuadPart, dwMoveMethod);
		if (lpDistanceToMoveHigh)
			*lpDistanceToMoveHigh = newpos.HighPart;

		return newpos.LowPart;
	}
	DWORD rv = TrueSetFilePointer(hFile, liDistanceToMove, lpDistanceToMoveHigh, dwMoveMethod);
	return rv;
}
BOOL WINAPI MineSetFilePointerEx(__in HANDLE hFile, __in LARGE_INTEGER liDistanceToMove, __out_opt PLARGE_INTEGER lpNewFilePointer, __in DWORD dwMoveMethod)
{
	for(int i=0; i<5; i++)
	if (hFile == hooked_file[i])
	{
		LARGE_INTEGER newpos;
		newpos.QuadPart = file.seek(liDistanceToMove.QuadPart, dwMoveMethod);
		if (lpNewFilePointer)
			*lpNewFilePointer = newpos;
		return TRUE;
	}
	BOOL rv = TrueSetFilePointerEx(hFile, liDistanceToMove, lpNewFilePointer, dwMoveMethod);
	return rv;
}
BOOL WINAPI MineReadFile(__in HANDLE hFile, __out LPVOID lpBuffer, __in DWORD nNumberOfBytesToRead, __out_opt LPDWORD lpNumberOfBytesRead, __inout_opt LPOVERLAPPED lpOverlapped)
{
	if (hFile == hooked_file)
	{
		LONGLONG read = file.read(lpBuffer, nNumberOfBytesToRead);
		if (lpNumberOfBytesRead)
			*lpNumberOfBytesRead = read;

		return TRUE;
	}

	BOOL rv = TrueReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
	return rv;
}

HANDLE WINAPI MineCreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
	HANDLE rv = TrueCreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, 
		dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);

	if (wcsstr(lpFileName, L"pack.mkv"))
	{
		hooked_file[hooked_count++] = rv;
	}
	return rv;
}
static DWORD (WINAPI *TrueGetModuleFileNameA)(HMODULE hModule, LPSTR lpFilename, DWORD nSize) = GetModuleFileNameA;
DWORD WINAPI MineGetModuleFileNameA(HMODULE hModule, LPSTR lpFilename, DWORD nSize)
{
	DWORD rtn = TrueGetModuleFileNameA(hModule, lpFilename, nSize);
	if (hModule == NULL)
		strcat(lpFilename, "StereoPlayer.exe");
	return rtn;
}
int HookIt()
{
	DetourRestoreAfterWith();
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(PVOID&)TrueGetModuleFileNameA, MineGetModuleFileNameA);

	DetourAttach(&(PVOID&)TrueGetFileSize, MineGetFileSize);
	DetourAttach(&(PVOID&)TrueGetFileSizeEx, MineGetFileSizeEx);
	DetourAttach(&(PVOID&)TrueSetFilePointer, MineSetFilePointer);
	DetourAttach(&(PVOID&)TrueSetFilePointerEx, MineSetFilePointerEx);
	DetourAttach(&(PVOID&)TrueReadFile, MineReadFile);
	DetourAttach(&(PVOID&)TrueCreateFileW, MineCreateFileW);
	LONG error = DetourTransactionCommit();

	return 0;
}
DWORD WINAPI thread_func(LPVOID lpThreadParameter)
{
	char name[MAX_PATH];
	return GetModuleFileNameA(NULL, name, MAX_PATH);
}

int main()
{
	HookIt();


	// basic file test
	/*
	FILE *f = fopen("F:\\pack.txt", "rb");
	char read[50];
	fread(read, 1, 50, f);
	fseek(f, 0, SEEK_END);
	int size = ftell(f);
	fclose(f);
	*/

	// dshow test
	CoInitialize(NULL);
	CComPtr<IGraphBuilder> gb;
	gb.CoCreateInstance(CLSID_FilterGraph);
	HRESULT hr = gb->RenderFile(L"Z:\\pack.mkv", NULL);
	CComQIPtr<IMediaControl, &IID_IMediaControl> mc(gb);
	CComQIPtr<IMediaSeeking, &IID_IMediaSeeking> ms(gb);
	REFERENCE_TIME duration;
	ms->GetDuration(&duration);
	mc->Run();
	Sleep(180*1000);



	// AES test
	AESCryptor codec;
	u1byte key[32];
	u1byte data[16] = "Hello World";
	u1byte encoded[16];
	u1byte decoded[16];

	codec.set_key(key, 256);
	codec.encrypt(data, encoded);

	int cycle = 10000000;
	int l = GetTickCount();
	for(int i=0; i<cycle; i++)
	{
		encoded[0] ++;
		codec.decrypt(encoded, decoded);
		decoded[0] = 1;
	}

	printf((char*)decoded);
	printf("speed: %d Kbyte/s.\n", (__int64)cycle*16/(GetTickCount()-l));

	return 0;
}


// simple_file
simple_file::simple_file(LPCWSTR name)
{
	m_file = TrueCreateFileW(name, GENERIC_READ | GENERIC_WRITE, 7, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
}

LONGLONG simple_file::seek(LONGLONG distance, int method)
{
	LARGE_INTEGER distance2;
	distance2.QuadPart = distance;

	LARGE_INTEGER newpos = distance2;
	TrueSetFilePointerEx(m_file, distance2, &newpos, method);
	//newpos.LowPart = SetFilePointer(m_file, distance2.LowPart, &distance2.HighPart, method);

	return newpos.QuadPart;
}

LONGLONG simple_file::size()
{
	LARGE_INTEGER lsize;
	lsize.LowPart = TrueGetFileSize(m_file, (LPDWORD)&lsize.HighPart);
	return lsize.QuadPart;
}

LONGLONG simple_file::read(void *buf, LONGLONG size)
{
	LARGE_INTEGER rtn;
	rtn.QuadPart = size;

	BOOL res = TrueReadFile(m_file, buf, (DWORD)size, &rtn.LowPart, NULL);

	return rtn.QuadPart;
}

LONGLONG simple_file::tell()
{
	return seek(0, SEEK_CUR);
}