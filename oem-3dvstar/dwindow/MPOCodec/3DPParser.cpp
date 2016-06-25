#include "3DPParser.h"
#include <memory.h>

#pragma pack(1)
typedef struct
{
	unsigned char SOI[2];
	unsigned char dummy[12];
	unsigned int chunk_size;
	char stereo_tag[4];
	unsigned short width;
	unsigned short height;
	unsigned char one;
} _3dv_header;
#pragma pack()

const unsigned char dummy_value[12] = {0xff, 0xe2, 0x00, 0x17, 0x4d, 0x50, 0x46, 0x00, 0x49, 0x49, 0x2a, 0x00};

#define safe_delete(x) {if(x!=NULL)delete[]x;x=NULL;}


_3DPParser::_3DPParser()
{
	memset(m_datas, 0, sizeof(m_datas));
}
_3DPParser::~_3DPParser()
{
	for(int i=0; i<2; i++)
		safe_delete(m_datas[i]);
}
int _3DPParser::parseFile(const wchar_t *path)
{
	for(int i=0; i<2; i++)
		safe_delete(m_datas[i]);

	FILE *f = _wfopen(path, L"rb");
	if (!f)
		return 0;

	_3dv_header header1 = {0};
	_3dv_header header2 = {0};
	if (sizeof(header1) != fread(&header1, 1, sizeof(header1), f))
	{
		fclose(f);
		return 0;
	}

	swapendian(&header1.width, 2);
	swapendian(&header1.height, 2);
	swapendian(&header1.chunk_size, 4);
	if (memcmp(header1.dummy, dummy_value, sizeof(dummy_value)) != 0)
	{
		fclose(f);
		return 0;
	}

	fseek(f, header1.chunk_size, SEEK_SET);
	if (sizeof(header2) != fread(&header2, 1, sizeof(header2), f))
	{
		fclose(f);
		return 0;
	}

	swapendian(&header2.width, 2);
	swapendian(&header2.height, 2);
	swapendian(&header2.chunk_size, 4);
	if (memcmp(header2.dummy, dummy_value, sizeof(dummy_value)) != 0)
	{
		fclose(f);
		return 0;
	}

	
	m_datas[0] = new char[header1.chunk_size];
	m_datas[1] = new char[header2.chunk_size];
	m_sizes[0] = header1.chunk_size;
	m_sizes[1] = header2.chunk_size;

	fseek(f, 0, SEEK_SET);
	if (header1.chunk_size != fread(m_datas[0], 1, header1.chunk_size, f))
	{
		fclose(f);
		return 0;
	}

	fseek(f, header1.chunk_size, SEEK_SET);
	if (header2.chunk_size != fread(m_datas[1], 1, header2.chunk_size, f))
	{
		fclose(f);
		return 0;
	}

	width = header1.width;
	height = header1.height;

	return 2;
}


void _3DPParser::swapendian(void *p, int size)
{
	char *t = (char*)p;
	for(int i=0; i<size/2; i++)
	{
		t[i] = t[size-i-1] ^ t[i];
		t[size-i-1] = t[i] ^ t[size-i-1];
		t[i] = t[size-i-1] ^ t[i];
	}
}	