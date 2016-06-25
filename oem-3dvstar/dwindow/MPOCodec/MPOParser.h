#pragma once
#include <stdio.h>
#include <string.h>

class MpoParser
{
public:
	// the only function
	// returns number of sub pictures found, 0 or n
	// returns 0 on error, n on success
	// other member variables are valid on success, undefined on error
	// don't delete or free any variables, they'll be handled by deconstructor
	int parseFile(const wchar_t *path);
	MpoParser();
	~MpoParser();

	char *m_datas[32];
	int m_sizes[32];
	int m_NumberOfImages;

private:
	unsigned short read2(FILE *f, bool isLittleEndian);

	unsigned int read4(FILE *f, bool isLittleEndian);
	void swapendian(void *p, int size);
	int fsize(FILE *f);
};