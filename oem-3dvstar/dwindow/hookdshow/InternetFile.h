#pragma once
#include "..\httppost\httppost.h"
#include "CFileBuffer.h"

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

	bool m_ready;
	wchar_t m_URL[MAX_PATH];
	__int64 m_size;
	__int64 m_pos;

	httppost *m_http;
};