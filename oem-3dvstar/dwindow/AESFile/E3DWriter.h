#pragma once
#include <windows.h>
#include "rijndael.h"

class E3D_Writer_Progress
{
public:
	virtual HRESULT CB(int type, __int64 para1, __int64 para2) = 0;
	// type 0: head written, encrypting started.
	// type 1: progressing para1 = byte done, para2 = total byte;
	// type 2: done, para1 = total bytes.
	// return S_OK to continue, others to cancel
};

int encode_file(wchar_t *in, wchar_t *out, unsigned char *key, unsigned char *source_hash, E3D_Writer_Progress *cb = NULL);
