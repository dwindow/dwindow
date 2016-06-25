#pragma once
#include <Windows.h>
#include <wininet.h>
#include "CFileBuffer.h"
#pragma comment(lib,"wininet.lib")

const int buffer_size = 4096;
const int buffer_count = 256*10;

class InternetFile
{
public:
	InternetFile();
	~InternetFile();
	BOOL ReadFile(LPVOID lpBuffer, DWORD nToRead, LPDWORD nRead, LPOVERLAPPED lpOverlap);
	BOOL GetFileSizeEx(PLARGE_INTEGER lpFileSize);
	BOOL SetFilePointerEx(__in LARGE_INTEGER liDistanceToMove, __out_opt PLARGE_INTEGER lpNewFilePointer, __in DWORD dwMoveMethod);
	BOOL Open(const wchar_t *URL, int max_buffer, __int64 startpos=0);
	BOOL Close();
	bool IsReady(){return m_ready;}

protected:

	static DWORD WINAPI downloading_thread_entry(LPVOID p){return ((InternetFile*)p)->downloading_thread();}
	DWORD downloading_thread();
	HANDLE m_downloading_thread;
	bool m_downloading_thread_exit;
	bool m_downloading_thread_state;	// 0 = down/not started, 1 = running

	CFileBuffer *m_buffer[buffer_count];
	__int64 m_buffer_start;
	__int64 m_downloading_pos;
	myCCritSec m_buffer_lock;
	int get_from_buffer(void *buf, __int64 start, int size);
	int increase_buffers();



	bool m_ready;
	wchar_t m_URL[MAX_PATH];
	__int64 m_size;
	__int64 m_pos;
	HINTERNET m_hInternet;
	HINTERNET m_hFile;
	HINTERNET m_hConnect;
	HINTERNET m_hRequest;
};