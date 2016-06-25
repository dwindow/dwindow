#include "InternetFile.h"
#include <assert.h>
#include <stdio.h>

#define safe_close(x) {if(x){InternetCloseHandle(x);x=NULL;}}
#define rtn_null(x) {if ((x) == NULL){Close();return FALSE}}
#define safe_delete(x) {if(x){delete x;x=NULL;}}

#define countof(x) (sizeof(x)/sizeof(x[0]))

InternetFile::InternetFile()
:m_http(NULL)
{
	Close();
}

InternetFile::~InternetFile()
{
	Close();
}

BOOL InternetFile::Open(const wchar_t *URL, int max_buffer, __int64 startpos /*=0*/)
{
	Close();

	wchar_t tmp[8] = {0};
	for(unsigned int i=0; i<7 && i<wcslen(URL); i++)
		tmp[i] = towlower(URL[i]);

	if (wcscmp(tmp, L"http://") != 0)
		return FALSE;

	m_http = new httppost(URL);
	if (startpos>0)
	{
		wchar_t tmp[200] = {0};
		wsprintfW(tmp, L"bytes=%I64d-", startpos);
		m_http->addHeader(L"Range", tmp);
	}
	int reponse_code = m_http->send_request();
	if (reponse_code > 299 || reponse_code < 200)
	{
// 		Close();
		return TRUE;
	}

	m_size = startpos + m_http->get_content_length();

	wcscpy_s(m_URL, URL);
	m_ready = true;
	m_pos = startpos;
	return TRUE;
}

BOOL InternetFile::Close()
{
	m_ready = false;
	m_URL[0] = NULL;
	m_size = 0;
	m_pos = 0;

	safe_delete(m_http);

	return TRUE;
}

BOOL InternetFile::ReadFile(LPVOID lpBuffer, DWORD nToRead, LPDWORD nRead, LPOVERLAPPED lpOverlap)
{
	if (!m_http)
		return FALSE;

	char tmp[2048];
	sprintf(tmp, "ReadFile %08x @ %d\n", this, (int)m_pos);
	OutputDebugStringA(tmp);

	if (nToRead > 200)
	{
		printf("");
	}

	DWORD left = nToRead;
	BYTE *p = (BYTE*)lpBuffer;
	*nRead = 0;

	int l = GetTickCount();

	*nRead = m_http->read_content(lpBuffer, nToRead);

	int ms = GetTickCount() - l;
	sprintf(tmp, "%fK Byte/s\n", (float)nToRead/ms);
	OutputDebugStringA(tmp);

// 	assert(*nRead == nToRead);
	m_pos += *nRead;

	return TRUE;
}

BOOL InternetFile::GetFileSizeEx(PLARGE_INTEGER lpFileSize)
{
	lpFileSize->QuadPart = m_size;
	return TRUE;
}
BOOL InternetFile::SetFilePointerEx(__in LARGE_INTEGER liDistanceToMove, __out_opt PLARGE_INTEGER lpNewFilePointer, __in DWORD dwMoveMethod)
{
	switch(dwMoveMethod)
	{
		case SEEK_SET:
			lpNewFilePointer->QuadPart = liDistanceToMove.QuadPart;
			break;
		case SEEK_CUR:
			lpNewFilePointer->QuadPart = m_pos + liDistanceToMove.QuadPart;
			break;
		case SEEK_END:
			lpNewFilePointer->QuadPart = m_size + liDistanceToMove.QuadPart;
			break;
	}

	lpNewFilePointer->QuadPart = max(0, lpNewFilePointer->QuadPart);

  	if (lpNewFilePointer->QuadPart != m_pos)
	{
		char tmp[2048];
		sprintf(tmp, "SetFilePointerEx %d method %d\n", (int)lpNewFilePointer->QuadPart, dwMoveMethod);
		OutputDebugStringA(tmp);

		__int64 size = m_size;
		wchar_t URL[MAX_PATH];
		wcscpy(URL, m_URL);
		Close();
		Open(URL, 1024, lpNewFilePointer->QuadPart);
		m_size = size;
		m_pos = lpNewFilePointer->QuadPart;
	}

	return TRUE;
}
