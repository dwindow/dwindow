#include <tchar.h>
#include <d3dx9.h>
#include <stdio.h>
#include <atlbase.h>

#include "ps_aes_key.h"
#include "../AESFile/rijndael.h"

int _tmain(int argc, _TCHAR* argv[])
{
	AESCryptor codec;
	codec.set_key(ps_aes_key, 256);

	if (argc<4)
	{
		printf("Invalid Arguments!.\n");
		printf("PixelShaderCompiler.exe inputfile outputfile functionname\n");
		return -1;
	}

	USES_CONVERSION;

	LPD3DXBUFFER pCode = NULL;
	LPD3DXBUFFER pErrorMsgs = NULL;

	HRESULT hr ;
	if (argc>=6)
	{
		printf("(%s)", T2A(argv[5]));
		hr = D3DXCompileShaderFromFile(argv[1], NULL, NULL, T2A(argv[3]), T2A(argv[5]), D3DXSHADER_OPTIMIZATION_LEVEL3, &pCode, &pErrorMsgs, NULL);
	}
	else
	{
		printf("ps_3_0");
		hr = D3DXCompileShaderFromFile(argv[1], NULL, NULL, T2A(argv[3]), "ps_3_0", D3DXSHADER_OPTIMIZATION_LEVEL3, &pCode, &pErrorMsgs, NULL);
	}
	if (pErrorMsgs != NULL)
	{
		unsigned char* message = (unsigned char*)pErrorMsgs->GetBufferPointer();
		printf((LPSTR)message);
	}
	if ((FAILED(hr)))
	{
		printf("error : compile failed.\n");
		return E_FAIL;
	}
	else
	{
		FILE*f = _tfopen(argv[2], _T("wb"));
		if (f)
		{
			int size = pCode->GetBufferSize();
			DWORD *codes = (DWORD *)malloc(size);
			memcpy(codes, pCode->GetBufferPointer(), size);
			if (argc<5 || wcscmp(argv[4], L"yes") == 0)
			{
				for(int i=0; i<size/16*16; i+=16)
				codec.encrypt(((unsigned char*)codes)+i, ((unsigned char*)codes)+i);
				printf("(encoded)");
			}

			fprintf(f, "// pixel size = %d.\r\n#include <windows.h>\r\nconst DWORD g_code_%s[%d] = {", size, T2A(argv[3]), size/4);
			for(int i=0; i<size/4; i++)
			{
				fprintf(f, "0x%x, ", codes[i]);
			}
			fprintf(f, "};\r\n");
			fflush(f);
			fclose(f);
			free(codes);

			_tprintf(_T("PixelShaderCompiler: \"%s\" of %s Compiled to %s\n"), argv[3], argv[1], argv[2]);
		}
		else
		{
			_tprintf(_T("Failed Writing %s.\n"), argv[2]);
		}
	}

	return 0;
}