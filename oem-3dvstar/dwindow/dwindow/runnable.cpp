#include "runnable.h"

thread_pool::thread_pool(int n)
:m_destroyed(false)
,m_thread_count(n)
{
	InitializeCriticalSection(&m_cs);
	m_task_event = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_quit_event = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_threads = new HANDLE[n];
	for(int i=0; i<n; i++)
		m_threads[i] = CreateThread(NULL, NULL, worker_thread, this, NULL, NULL);
}

thread_pool::~thread_pool()
{
	destroy();
	DeleteCriticalSection(&m_cs);
}

int thread_pool::destroy()
{
	EnterCriticalSection(&m_cs);

	m_destroyed = true;

	while (!m_tasks.empty())
	{
		Irunnable *p = m_tasks.front();
		m_tasks.pop();

		p->signal_quit();
		delete p;
	}

	for(std::list<Irunnable*>::iterator i = m_running_tasks.begin(); i != m_running_tasks.end(); ++i)
		(*i)->signal_quit();

	LeaveCriticalSection(&m_cs);

	SetEvent(m_quit_event);
	WaitForMultipleObjects(m_thread_count, m_threads, TRUE, INFINITE);
	CloseHandle(m_task_event);
	CloseHandle(m_quit_event);


	delete [] m_threads;

	return 0;
}

DWORD thread_pool::worker_thread(LPVOID p)
{
	thread_pool *k = (thread_pool*)p;

	while(true)
	{
		EnterCriticalSection(&k->m_cs);
		if (k->m_tasks.size() == 0)
		{
			LeaveCriticalSection(&k->m_cs);
			HANDLE events[2] = {k->m_task_event, k->m_quit_event};
			DWORD o = WaitForMultipleObjects(2, events, FALSE, INFINITE);
			if (o != WAIT_OBJECT_0)
				break;
			continue;
		}

		Irunnable *task = k->m_tasks.front();
		k->m_tasks.pop();

		k->m_running_tasks.push_back(task);
		LeaveCriticalSection(&k->m_cs);

		task->run();

		EnterCriticalSection(&k->m_cs);
		k->m_running_tasks.remove(task);
		LeaveCriticalSection(&k->m_cs);

		delete task;
	}

	return 0;
}

int thread_pool::submmit(Irunnable *task)
{
	EnterCriticalSection(&m_cs);
	if (!m_destroyed)
		m_tasks.push(task);
	LeaveCriticalSection(&m_cs);
	SetEvent(m_task_event);
	return 0;
}

