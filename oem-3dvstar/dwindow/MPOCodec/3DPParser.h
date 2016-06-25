#pragma once
#include <stdio.h>

class _3DPParser
{
public:
	// the only function
	// returns number of sub pictures found, 0 or 2
	// returns 0 on error, 2 on success
	// other member variables are valid on success, undefined on error
	// don't delete or free any variables, they should be handled by deconstructor
	int parseFile(const wchar_t *path);
	_3DPParser();
	~_3DPParser();

	int width;
	int height;

	char *m_datas[2];
	int m_sizes[2];

private:
	void swapendian(void *p, int size);
};