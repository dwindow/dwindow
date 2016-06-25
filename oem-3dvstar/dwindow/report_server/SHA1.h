#pragma  once 

#include "windows.h"
typedef struct SHA1_STATETYPE_TAG
{   
	UINT wbuf[16];
	UINT hash[5];
	UINT count[2];
} SHA1_STATETYPE;
void SHA1Hash(unsigned char *_pOutDigest, const unsigned char *_pData,UINT nSize);
void SHA1_Start(SHA1_STATETYPE* _pcsha1);
void SHA1_Finish(unsigned char* _pShaValue, SHA1_STATETYPE* _pcsha1);
void SHA1_Hash(const unsigned char *_pData, unsigned int _iSize, SHA1_STATETYPE* _pcsha1);
