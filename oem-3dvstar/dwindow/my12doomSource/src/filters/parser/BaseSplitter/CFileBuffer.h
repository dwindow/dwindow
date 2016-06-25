#pragma once
// this is only a linear buffer, no seek support
// and only 1 reader and 1 writer support.

#include <Windows.h>
#include "CCritSec.h"


class CFileBuffer
{
public:
	BYTE* m_the_buffer;
	int m_buffer_size;
	int m_data_size;//=0
	int m_data_start;//=0;

	bool no_more_data;
	bool no_more_remove;		// true: insert will direct return 0, no blocking

	myCCritSec cs;

	CFileBuffer(int buffer_size);
	~CFileBuffer();
	int reset();
	int wait_for_data(int size);
	int remove_data(int size);
	int insert(int size, const BYTE*buf, int time_out = INFINITE);
	int wait_for_free_space(int size, int time_out = INFINITE);
	int remove(int size, BYTE *buf);
	int block_remove(int size, BYTE *buf);

private:	
	void mPut(const unsigned char *in, int size, int start);
	void mGet(unsigned char *in, int size, int start);
	void resort();
};