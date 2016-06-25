#include "InternetFile.h"
#include <assert.h>
#include <stdio.h>

#define safe_close(x) {if(x){InternetCloseHandle(x);x=NULL;}}
#define rtn_null(x) {if ((x) == NULL){Close();return FALSE}}

#define countof(x) (sizeof(x)/sizeof(x[0]))

InternetFile::InternetFile()
{
	m_hFile = NULL;
	m_hInternet = NULL;
	m_hConnect = NULL;
	m_hRequest = NULL;
	m_buffer_start = 0;

	for(int i=0; i<countof(m_buffer); i++)
		m_buffer[i] = new CFileBuffer(buffer_size);

	Close();
}

InternetFile::~InternetFile()
{
	Close();

	for(int i=0; i<countof(m_buffer); i++)
		delete m_buffer[i];
}

BOOL InternetFile::Open(const wchar_t *URL, int max_buffer, __int64 startpos /*=0*/)
{
	Close();

	wchar_t tmp[8] = {0};
	for(unsigned int i=0; i<7 && i<wcslen(URL); i++)
		tmp[i] = towlower(URL[i]);

	if (wcscmp(tmp, L"http://") != 0)
		return FALSE;


	m_hInternet = InternetOpenW(L"Testing",INTERNET_OPEN_TYPE_PRECONFIG,NULL,NULL,0);
	if (!m_hInternet)
		return FALSE;

	if (startpos == 0)
	{
		m_hFile = InternetOpenUrlW(m_hInternet, URL,NULL,0,0,0);
		if (!m_hFile)
		{
			Close();
			return FALSE;
		}

		LARGE_INTEGER li = {0};
		li.LowPart = InternetSetFilePointer(m_hFile, 0, &li.HighPart, SEEK_END, 0);
		if (li.LowPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
		{
			Close();
			return FALSE;
		}

		InternetSetFilePointer(m_hFile, 0, 0, SEEK_SET, 0);

		m_size = li.QuadPart;
	}
	else
	{
		wchar_t host[1024];
		wchar_t get_request[1024];

		wcscpy(get_request, wcschr(URL+7, L'/'));
		wcscpy(host, URL+7);
		*((wchar_t*)wcschr(host, L'/')) = NULL;

		int port = INTERNET_DEFAULT_HTTP_PORT;
		wchar_t *p = (wchar_t*)wcschr(host, L':');
		if (p)
		{
			p[0] = NULL;
			port = _wtoi(p+1);
		}

		m_hConnect = InternetConnectW(m_hInternet, host, port,
							NULL, NULL, INTERNET_SERVICE_HTTP, NULL, NULL);
		if (!m_hConnect)
		{
			Close();
			return FALSE;
		}

		const wchar_t* rgpszAcceptTypes[] = {L"*/*", NULL};
		m_hRequest = HttpOpenRequestW(m_hConnect, NULL, get_request, NULL, NULL, rgpszAcceptTypes, NULL, NULL);
		if (!m_hRequest)
		{
			Close();
			return FALSE;
		}

		wchar_t tmp[200] = {0};
		if (startpos>0)
			wsprintfW(tmp, L"Range: bytes=%I64d-", startpos);
		if (!HttpSendRequestW(m_hRequest, startpos>0?tmp:NULL, wcslen(tmp), NULL, NULL))
		{
			Close();
			return FALSE;
		}
	}
	wcscpy_s(m_URL, URL);
	m_ready = true;
	m_pos = startpos;
	for(int i=0; i<countof(m_buffer); i++)
		m_buffer[i]->reset();
	m_downloading_thread_exit = false;
	m_downloading_thread = CreateThread(NULL, NULL, downloading_thread_entry, this, NULL, NULL);

	return TRUE;
}

BOOL InternetFile::Close()
{
	m_ready = false;
	m_URL[0] = NULL;
	m_size = 0;
	m_pos = 0;
	m_downloading_thread_exit = true;
	m_downloading_thread_state = 0;

	WaitForSingleObject(m_downloading_thread, INFINITE);

	safe_close(m_hRequest);
	safe_close(m_hConnect);
	safe_close(m_hFile);
	safe_close(m_hInternet);

	return TRUE;
}

BOOL InternetFile::ReadFile(LPVOID lpBuffer, DWORD nToRead, LPDWORD nRead, LPOVERLAPPED lpOverlap)
{
	DWORD left = nToRead;
	BYTE *p = (BYTE*)lpBuffer;
	*nRead = 0;

	int l = GetTickCount();
	while (left > 0)
	{

		int this_round_got = 0;

		for(int i=(m_pos-m_buffer_start)/buffer_size; 
			i<=min(countof(m_buffer)-1, (m_pos+left-m_buffer_start)/buffer_size); i++)
		{
			__int64 L = m_buffer_start + buffer_size * i;
			__int64 R = m_buffer_start + buffer_size *(i+1);
			int size = min(buffer_size, left);
			size = min(size, R-m_pos);

			if (size>0 && m_pos >= L)
			{
				int got = m_buffer[i]->wait_for_data(m_pos+size-L);
				if (got == 0)	// downloading thread down
					break;
				got += L - m_pos;

				memcpy(p, m_buffer[i]->m_the_buffer + m_buffer[i]->m_data_start + m_pos - L, got);

				*nRead += got;
				m_pos += got;
				p += got;
				left -= got;
				this_round_got += got;
			}
		}

		if (this_round_got == 0)
		{
			if (m_downloading_thread_state == 0 && m_pos < m_size)
			{
				LARGE_INTEGER li;
				li.QuadPart = m_pos;
				wchar_t URL[MAX_PATH];
				wcscpy(URL, m_URL);
				SetFilePointerEx(li, &li, SEEK_SET);
				wcscpy(m_URL, URL);
				Sleep(1);
				continue;
			}
			else
			{
				break;
			}
		}
	}

	myCAutoLock lck(&m_buffer_lock);
	for(int i=0; i< (m_pos - m_buffer_start - buffer_count*buffer_size/2) / buffer_size; i++)
		increase_buffers();

	int ms = GetTickCount() - l;
	char tmp[200];
	sprintf(tmp, "%fK Byte/s\n", (float)nToRead/ms);
	OutputDebugStringA(tmp);

	return *nRead == 0 ? FALSE : TRUE;
}

int InternetFile::increase_buffers()
{
	myCAutoLock lck(&m_buffer_lock);

	CFileBuffer *p = m_buffer[0];
	for(int i=0; i<countof(m_buffer)-1; i++)
		m_buffer[i] = m_buffer[i+1];
	m_buffer[countof(m_buffer)-1] = p;
	p->reset();

	m_buffer_start += buffer_size;

	return 0;
}

BOOL InternetFile::GetFileSizeEx(PLARGE_INTEGER lpFileSize)
{
	lpFileSize->QuadPart = m_size;
	return TRUE;
}
BOOL InternetFile::SetFilePointerEx(__in LARGE_INTEGER liDistanceToMove, __out_opt PLARGE_INTEGER lpNewFilePointer, __in DWORD dwMoveMethod)
{
	assert(dwMoveMethod != SEEK_END);

	switch(dwMoveMethod)
	{
		case SEEK_SET:
			lpNewFilePointer->QuadPart = liDistanceToMove.QuadPart;
			break;
		case SEEK_CUR:
			lpNewFilePointer->QuadPart += liDistanceToMove.QuadPart;
			break;
		case SEEK_END:
			lpNewFilePointer->QuadPart = m_size + liDistanceToMove.QuadPart;
			break;
	}

 	if (lpNewFilePointer->QuadPart != m_pos)
	{
 		if (lpNewFilePointer->QuadPart < m_buffer_start || lpNewFilePointer->QuadPart >= m_buffer_start + buffer_size * buffer_count)
		{
			__int64 size = m_size;
			wchar_t URL[MAX_PATH];
			wcscpy(URL, m_URL);
			Close();
			m_buffer_start = lpNewFilePointer->QuadPart / buffer_size * buffer_size;
			Open(URL, 1024, m_buffer_start);
			m_size = size;
		}
	}

	m_pos = lpNewFilePointer->QuadPart;

	return TRUE;
}

DWORD InternetFile::downloading_thread()
{
	const int block_size = 4096;
	BYTE buf[block_size];
	__int64 internet_pos = m_buffer_start;
	m_downloading_thread_state = 1;

	while(!m_downloading_thread_exit)
	{
		DWORD nRead = 0;
		BOOL succ =  InternetReadFile(m_hRequest ? m_hRequest : m_hFile, buf, block_size, &nRead);

		if (!succ)
			break;

wait:
		m_buffer_lock.Lock();
		int n = (internet_pos - m_buffer_start) / buffer_size;

		if (n<0 || m_downloading_thread_exit)
		{
			m_buffer_lock.Unlock();
			break;
		}

		if (n>=buffer_count)
		{
			m_buffer_lock.Unlock();
			Sleep(1);
			goto wait;
		}

		if (m_buffer[n]->wait_for_free_space(nRead, 0) < 0)
		{
			m_buffer_lock.Unlock();
			Sleep(1);
			goto wait;
		}

		int insert_result = m_buffer[n]->insert(nRead, buf, 1);
		m_buffer_lock.Unlock();

		if (insert_result < 0)
			goto wait;

		if (nRead != block_size)
			break;

		internet_pos += nRead;
		m_downloading_pos = internet_pos;
	}

	m_buffer_lock.Lock();
	for(int i=0; i<countof(m_buffer); i++)
	{
		m_buffer[i]->no_more_remove = true;
		m_buffer[i]->no_more_data = true;
	}
	m_buffer_lock.Unlock();

	m_downloading_thread_state = 0;

	return 0;
}