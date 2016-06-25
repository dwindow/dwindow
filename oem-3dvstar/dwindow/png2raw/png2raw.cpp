// png2raw.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <Windows.h>
#include "include/il/il.h"
#include <conio.h>

#pragma comment( lib, "lib/DevIL.lib" )

int _tmain(int argc, _TCHAR* argv[])
{
	if(argc<2)
	{
		printf("png2raw [png] [raw]");
		return -1;
	}
	wchar_t out[MAX_PATH];
	if (argc>2)
	{
		wcscpy(out, argv[2]);
	}
	else
	{
		wcscpy(out, argv[1]);
		wcscat(out, L".raw");
	}

	ilInit();
	FILE *f = _wfopen(argv[1], L"rb");
	if (!f)
	{
		wprintf(L"error opening %s\r\n", argv[1]);
		return -1;
	}
	fseek(f, 0, SEEK_END);
	int size = ftell(f);
	if (!f)
	{
		wprintf(L"error opening %s\r\n", argv[1]);
		return -1;
	}
	char *data = new char[size];
	fseek(f, 0, SEEK_SET);
	fread(data, 1, size, f);
	fclose(f);
	ILboolean result = ilLoadL(IL_TYPE_UNKNOWN, data, size );// = ilLoadImage(argv[1]) ;
	if (!result)
	{
		ILenum err = ilGetError() ;
		printf( "the error %d\n", err );
		printf( "string is %s\n", ilGetString( err ) );
		return -1;
	}

	size = ilGetInteger( IL_IMAGE_SIZE_OF_DATA ) ;
	BYTE *p = (BYTE*)ilGetData();
	f = _wfopen(out, L"wb");
	if (!out)
	{
		wprintf(L"Failed opening %s\n", out);
		return -1;
	}

	char tmp[20];
	for(int i=0; i<size/4; i++)
	{
		DWORD c = p[3] << 24 | p[0] << 16 | p[1] << 8 | p[2];
		sprintf(tmp, "0x%x, ", c);
		OutputDebugStringA(tmp);
		fwrite(&c, 1, 4, f);
		p+=4;
	}
	fclose(f);
	wprintf(L"OK!\n", out);

	return 0;
}

