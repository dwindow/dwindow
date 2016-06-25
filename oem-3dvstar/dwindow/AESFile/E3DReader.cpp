#include <stdio.h>
#include "E3DReader.h"

void log_line(char *format, ...)
{
#ifdef DEBUG
	char tmp[10240];
	va_list valist;
	va_start(valist, format);
	wvsprintfA(tmp, format, valist);
	va_end(valist);
	static FILE * f = fopen("F:\\e3d.log", "wb");
	if (f)
	{
		fprintf(f, "%s(%d)\r\n", tmp, GetCurrentThreadId());
		fflush(f);
	}
#endif
}


__int64 str2int64(const char *str)
{
	__int64 rtn = 0;
	int len = strlen(str);
	for(int i=0; i<min(8,len); i++)
		((char*)&rtn)[i] = str[i];

	return rtn;
}


file_reader::file_reader()
{
	m_file_size = 0;
	m_block_size = 0;
	m_cache_pos = -1;
	m_is_encrypted = false;
	m_key_ok = false;
	m_file = INVALID_HANDLE_VALUE;
	m_block_cache = NULL;
}

file_reader::~file_reader()
{
	if (m_block_cache)
		free(m_block_cache);
}

void file_reader::set_key(unsigned char*key)
{
	if (!m_is_encrypted)
		log_line("warning: not a encrypted file, ignored and set key.\n");

	m_codec.set_key(key, 256);
	m_codec.encrypt(m_keyhint, m_keyhint);
	if (memcmp(m_keyhint, m_keyhint+16, 16))
	{
		log_line("key error.\n");
		m_key_ok = false;
	}
	else
	{
		log_line("key ok.\n");
		m_key_ok = true;
	}

	m_codec.decrypt(m_keyhint, m_keyhint);
}

void file_reader::SetFile(HANDLE file)
{
	// init
	memset(m_remux_sig, 0, 128);
	LARGE_INTEGER li;
	li.QuadPart = 0;
	::SetFilePointerEx(file, li, &li, SEEK_CUR);
	m_file = file;
	m_is_encrypted = false;
	memset(m_hash, 0, 32);
	if (m_block_cache)
		free(m_block_cache);

	// read parameter
	DWORD byte_read = 0;
	__int64 eightcc;
	if (!::ReadFile(m_file, &eightcc, 8, &byte_read, NULL) || byte_read != 8)
		goto rewind;
	if (eightcc != str2int64("my12doom"))
		goto rewind;

	__int64 root_size;
	if (!::ReadFile(m_file, &root_size, 8, &byte_read, NULL) || byte_read != 8)
		goto rewind;
	m_start_in_file = root_size + 16;


	__int64 total_byte_read = 0;
	do
	{
		__int64 leaf_size;
		if (!::ReadFile(m_file, &eightcc, 8, &byte_read, NULL) || byte_read != 8)
			goto rewind;
		if (!::ReadFile(m_file, &leaf_size, 8, &byte_read, NULL) || byte_read != 8)
			goto rewind;
		
		byte_read = 0;
		if (eightcc == str2int64("filesize"))
		{
			if (!::ReadFile(m_file, &m_file_size, 8, &byte_read, NULL) || byte_read != 8)
				goto rewind;
		}
		else if (eightcc == str2int64("blocksize"))
		{
			if (!::ReadFile(m_file, &m_block_size, 8, &byte_read, NULL) || byte_read != 8)
				goto rewind;
		}
		else if (eightcc == str2int64("keyhint"))
		{
			if (!::ReadFile(m_file, m_keyhint, 32, &byte_read, NULL) || byte_read != 32)
				goto rewind;
		}
		else if (eightcc == str2int64("layout"))
		{
			if (!::ReadFile(m_file, &m_layout, 8, &byte_read, NULL) || byte_read != 8)
				goto rewind;
		}
		else if (eightcc == str2int64("srchash"))
		{
			if (!::ReadFile(m_file, m_hash, 20, &byte_read, NULL) || byte_read != 20)
				goto rewind;
		}
		else if (eightcc == str2int64("remux"))
		{
			if (!::ReadFile(m_file, m_remux_sig, 128, &byte_read, NULL) || byte_read != 128)
				goto rewind;
		}

		SetFilePointer(m_file, leaf_size - byte_read, NULL, SEEK_CUR);
		total_byte_read += 16 + leaf_size;

	} while (total_byte_read < root_size);

	if (m_file_size == 0 ||  m_block_size == 0)
		goto rewind;

	// set member
	m_file_pos_cache = m_start_in_file;
	m_is_encrypted = true;
	m_cache_pos = -1;
	m_pos = 0;
	m_block_cache = (unsigned char*)malloc(m_block_size);
	return;

rewind:
	::SetFilePointerEx(file, li, NULL, SEEK_SET);
}

BOOL file_reader::ReadFile(LPVOID lpBuffer, DWORD nToRead, LPDWORD nRead, LPOVERLAPPED lpOverlap)
{
	log_line("ReadFile %d", nToRead);
	if (!m_is_encrypted || !m_key_ok)
		return ::ReadFile(m_file, lpBuffer, nToRead, nRead, lpOverlap);

	// caculate size
	DWORD got = 0;
	nToRead = (DWORD)min(nToRead, m_file_size - m_pos);

	if (nToRead<=0 || m_pos<0)
		return FALSE;

	unsigned char*tmp = (unsigned char*)lpBuffer;
	LARGE_INTEGER li;
	DWORD byte_read = 0;
	if ( (m_pos % m_block_size))		// head
	{
		// cache miss
		if (m_cache_pos != m_pos / m_block_size * m_block_size) 
		{
			m_cache_pos = m_pos / m_block_size * m_block_size;

			li.QuadPart = m_start_in_file + m_cache_pos;
			if (li.QuadPart != m_file_pos_cache)
			{
				m_file_pos_cache = li.QuadPart;
				::SetFilePointerEx(m_file, li, NULL, SEEK_SET);
			}

			::ReadFile(m_file, m_block_cache, m_block_size, &byte_read, NULL);
			m_file_pos_cache += m_block_size;		// assume read success
			for(int i=0; i<m_block_size; i+=16)
				m_codec.decrypt(m_block_cache+i, m_block_cache+i);
		}

		// copy from cache
		int byte_to_get_from_cache = min(m_cache_pos + m_block_size - m_pos, nToRead);
		memcpy(tmp, m_block_cache + m_pos - m_cache_pos, byte_to_get_from_cache);
		nToRead -= byte_to_get_from_cache;
		tmp += byte_to_get_from_cache;
		m_pos += byte_to_get_from_cache;
		got += byte_to_get_from_cache;
	}

	// main body
	if (nToRead > m_block_size)
	{
		li.QuadPart = m_pos + m_start_in_file;
		if (li.QuadPart != m_file_pos_cache)
		{
			m_file_pos_cache = li.QuadPart;
			::SetFilePointerEx(m_file, li, NULL, SEEK_SET);
		}
		::ReadFile(m_file, tmp, nToRead / m_block_size * m_block_size, &byte_read, NULL);
		byte_read = nToRead / m_block_size * m_block_size, &byte_read; // assume read success
		m_file_pos_cache += byte_read;
		for(int i=0; i<nToRead / m_block_size * m_block_size; i+=16)
			m_codec.decrypt(tmp+i, tmp+i);

		nToRead -= byte_read;
		tmp += byte_read;
		m_pos += byte_read;
		got += byte_read;
	}

	// tail
	if (nToRead > 0)
	{
		m_cache_pos = m_pos;
		li.QuadPart = m_pos + m_start_in_file;
		if (li.QuadPart != m_file_pos_cache)
		{
			m_file_pos_cache = li.QuadPart;
			::SetFilePointerEx(m_file, li, NULL, SEEK_SET);
		}
		::ReadFile(m_file, m_block_cache, m_block_size, &byte_read, NULL);
		m_file_pos_cache += byte_read;
		for(int i=0; i<m_block_size; i+=16)
			m_codec.decrypt(m_block_cache+i, m_block_cache+i);
		memcpy(tmp, m_block_cache, nToRead);

		m_pos += nToRead;
		got += nToRead;
	}

	if (nRead)
		*nRead = got;

	return TRUE;
}

BOOL file_reader::GetFileSizeEx(PLARGE_INTEGER lpFileSize)
{
	log_line("GetFileSizeEx");
	if (!m_is_encrypted || !m_key_ok)
		return ::GetFileSizeEx(m_file, lpFileSize);

	if (!lpFileSize)
		return FALSE;
	lpFileSize->QuadPart = m_file_size;
	return TRUE;
}

BOOL file_reader::SetFilePointerEx(__in LARGE_INTEGER liDistanceToMove, __out_opt PLARGE_INTEGER lpNewFilePointer, __in DWORD dwMoveMethod)
{
	log_line("SetFilePointerEx, %d, %d", liDistanceToMove, dwMoveMethod);
	if (!m_is_encrypted || !m_key_ok)
		return ::SetFilePointerEx(m_file, liDistanceToMove, lpNewFilePointer, dwMoveMethod);

	__int64 old_pos = m_pos;

	if (dwMoveMethod == SEEK_SET)
		m_pos = liDistanceToMove.QuadPart;
	else if (dwMoveMethod == SEEK_CUR)
	{
		m_pos += liDistanceToMove.QuadPart;
	}
	else if (dwMoveMethod == SEEK_END)
		m_pos = m_file_size;


	if (m_pos <0)
	{
		m_pos = old_pos;
		return FALSE;
	}
	if (lpNewFilePointer)
		lpNewFilePointer->QuadPart = m_pos;

	return TRUE;
}

const WCHAR* e3d_soft_key= L"Software\\DWindow\\E3D";
unsigned char * key_in_process = NULL;
HRESULT e3d_get_process_key(BYTE * key)
{
	int len = 4;
	wchar_t pid[100];
	wsprintfW(pid, L"%d", GetCurrentProcessId());

	HKEY hkey = NULL;
	int ret = RegOpenKeyExW(HKEY_CURRENT_USER, e3d_soft_key,0,STANDARD_RIGHTS_REQUIRED |KEY_READ  , &hkey);
	if (ret != ERROR_SUCCESS || hkey == NULL)
		return E_FAIL;

	unsigned char * p_key_got = NULL;
	RegQueryValueExW(hkey, pid, 0, NULL, (LPBYTE)&p_key_got, (LPDWORD)&len);

	RegCloseKey(hkey);

	if (p_key_got)
		memcpy(key, p_key_got, 32);

	return e3d_del_process_key();
}
HRESULT e3d_set_process_key(const BYTE *key)
{
	if (key_in_process)
		delete key_in_process;

	key_in_process = new unsigned char[32];
	memcpy(key_in_process, key, 32);

	int len = 4;
	wchar_t pid[100];
	wsprintfW(pid, L"%d", GetCurrentProcessId());

	HKEY hkey = NULL;
	int ret = RegCreateKeyExW(HKEY_CURRENT_USER, e3d_soft_key, 0,0,REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS | KEY_WRITE |KEY_SET_VALUE, NULL , &hkey, NULL  );
	if (ret != ERROR_SUCCESS)
		return E_FAIL;
	ret = RegSetValueExW(hkey, pid, 0, REG_BINARY, (const byte*)&key_in_process, len );
	if (ret != ERROR_SUCCESS)
		return E_FAIL;

	RegCloseKey(hkey);

	return S_OK;
}
HRESULT e3d_del_process_key()
{
	wchar_t pid[100];
	wsprintfW(pid, L"%d", GetCurrentProcessId());

	HKEY hkey = NULL;
	int ret = RegCreateKeyExW(HKEY_CURRENT_USER, e3d_soft_key, 0,0,REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS | KEY_WRITE |KEY_SET_VALUE, NULL , &hkey, NULL  );
	if (ret != ERROR_SUCCESS)
		return E_FAIL;
	ret = RegDeleteValueW(hkey, pid);
	if (ret != ERROR_SUCCESS)
		return E_FAIL;
	RegCloseKey(hkey);

	return S_OK;
}