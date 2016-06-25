#pragma once
#include "list"
#include <streams.h>

typedef struct command_struct
{
	int command_type;
	void *data;
	int size;
} command;

class command_queue
{
public:
	command *pop();								// return directly, NULL if no command in queue
	command *wait(int max_wait_time);			// 
	void insert(command c);
	void remove_by_type(int command_type);

protected:
	CCritSec m_cs;
	std::list<command> m_queue;
};