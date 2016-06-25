#include "command_queue.h"

command *command_queue::pop()								// return directly, NULL if no command in queue
{
	CAutoLock lck(&m_cs);
	if (m_queue.empty())
		return NULL;

	command *o = (command*)malloc(sizeof(command));
	*o= *m_queue.begin();
	m_queue.pop_front();

	return o;
}
command *command_queue::wait(int max_wait_time)				// 
{
	int l = timeGetTime();
	while (timeGetTime()-l < max_wait_time || max_wait_time == -1)
	{
		command *o = pop();

		if (o != NULL)
			return o;

		Sleep(1);
	}

	return NULL;
}
void command_queue::insert(command c)
{
	CAutoLock lck(&m_cs);

	m_queue.push_back(c);
}

class remove_by_type_compare
{
public:
	bool operator() (const command& value) 
	{
		return value.command_type == m_command_type;
	}
	remove_by_type_compare(int command_type)
	{
		m_command_type = command_type;
	}
private:
	int m_command_type;
};

void command_queue::remove_by_type(int command_type)
{
	CAutoLock lck(&m_cs);

	m_queue.remove_if(remove_by_type_compare(command_type));
}
