#pragma once

#include "..\dwindow\runnable.h"
#include <wchar.h>
#include <list>
#include <string>
#include "CCritSec.h"

class inet_worker_manager;
class disk_manager;
class disk_manager;

#ifndef LINUX
#else
	typedef long long __int64;
	typedef unsigned long DWORD;
	typedef void * HANDLE;
	typedef unsigned char BYTE;
	typedef union _LARGE_INTEGER {
		struct {
			DWORD LowPart;
			long HighPart;
		};
		struct {
			DWORD LowPart;
			long HighPart;
		} u;
		__int64 QuadPart;
	} LARGE_INTEGER;
	#include <wchar.h>
	DWORD GetTickCount();
#endif

typedef struct
{
	__int64 start;
	__int64 end;
} fragment;
fragment cross_fragment(fragment a, fragment b);
int subtract_fragment(fragment frag, fragment subtractor, fragment o[2]);

class inet_worker : public Irunnable
{
public:
	inet_worker(const wchar_t *URL, __int64 start, inet_worker_manager *manager);
	~inet_worker();

	void run();
	int hint(__int64 pos);				// 3 reasons for worker to die: 
										// hint timeout, network error, reached a point that was done by other threads
										// so the manager need to hint workers periodically.

										// return value:
										// -1: not responsible currently and after hint()
										// 0: OK
	void signal_quit();

	int get_timeout_left();				// get time to timeout

	bool responsible(__int64 pos);		// return if this worker is CURRENTLY responsible for the pos

	__int64 m_pos;
	__int64 m_maxpos;					// modified by hint();
protected:
	friend class disk_manager;
	DWORD m_last_inet_time;
	bool m_exit_signaled;
	void *m_inet_file;
	inet_worker_manager *m_manager;
	std::wstring m_URL;
};

class inet_worker_manager
{
public:
	inet_worker_manager(const wchar_t *URL, disk_manager *manager);
	~inet_worker_manager();

	int hint(fragment pos, bool open_new_worker_if_necessary, bool debug = false);			// hint the workers to continue their work, and launch new worker if necessary

protected:
	friend class inet_worker;
	friend class disk_manager;

	myCCritSec m_worker_cs;
	std::list<inet_worker*> m_active_workers;
	thread_pool m_worker_pool;
	std::wstring m_URL;
	disk_manager *m_manager;
};

typedef struct
{
	fragment frag;
	enum
	{
		disk,
		net,
		read,
		preread,
	} type;
	int tick;
}debug_info;

class disk_fragment
{
public:
	disk_fragment(const wchar_t*file, __int64 startpos);
	~disk_fragment();

	int put(void *buf, int size);

	// return value:
	// 0: all requests completed.
	// 1/2: some request completed, 1/2 fragments left, output to fragment_left
	// -1: no requests completed.
	int get(void *buf, fragment pos, fragment fragment_left[2]);

	// this function subtract pos by {m_start, m_pos}
	int remaining(fragment pos, fragment out[2]);

	// this function subtract pos by {m_start, m_pos}, assuming there is only one piece left.
	// and shift *pointer if not NULL
	fragment remaining2(fragment pos, void**pointer/* = NULL*/);

	__int64 tell(){return m_pos;}

protected:
	friend class disk_manager;
	myCCritSec m_cs;
	__int64 m_pos;
	__int64 m_start;
#ifdef LINUX
	int m_file;
#else
	HANDLE m_file;
#endif
};

class disk_manager
{
public:
	disk_manager(const wchar_t *configfile);
	disk_manager():m_worker_manager(NULL){disk_manager(NULL);}
	~disk_manager();

	int setURL(const wchar_t *URL);
	int get(void *buf, fragment &pos);
	int pre_read(fragment &pos);
	__int64 getsize(){return m_filesize;}
	std::list<debug_info> debug();

protected:
	friend class disk_fragment;
	friend class inet_worker_manager;
	friend class inet_worker;

	// return:
	// -1 on error (unlikely and not handled currently)
	// 0 on total success
	// 1 on success but some fragment has already exists in some fragments
	int feed(void *buf, fragment pos);

	std::list<disk_fragment*> m_fragments;
	std::list<debug_info> m_access_info;
	inet_worker_manager *m_worker_manager;
	std::wstring m_URL;
	std::wstring m_config_file;
	myCCritSec m_fragments_cs;
	myCCritSec m_access_lock;
	__int64 m_filesize;
};
