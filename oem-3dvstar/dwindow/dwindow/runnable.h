#pragma once
#include <Windows.h>
#include <queue>
#include <list>

class Irunnable
{
public:
	virtual void run() = 0;
	virtual void signal_quit(){}
	virtual void join(){}
	virtual ~Irunnable(){}
};

class thread_pool
{
public:
	thread_pool(int n);
	~thread_pool();
	int submmit(Irunnable *task);
	int remove(Irunnable *task);
	int destroy();		// abandon all unstarted worker, wait for all working worker to finish, and reject any further works.

protected:
	std::queue<Irunnable*> m_tasks;
	std::list<Irunnable*> m_running_tasks;
	CRITICAL_SECTION m_cs;
	HANDLE m_task_event;
	HANDLE m_quit_event;
	bool m_destroyed;
	int m_thread_count;
	HANDLE *m_threads;

	static DWORD WINAPI worker_thread(LPVOID p);
};