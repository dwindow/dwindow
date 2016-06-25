
#include <windows.h>

#if (SYSTEM_BIG_ENDIAN)

#define _HIBITMASK_ 0x00000008
#define _MAXIMUMNR_ 0xffffffff
#define _MAXHALFNR_ 0x000Lffff 

#else

#define _HIBITMASK_ 0x80000000
#define _MAXIMUMNR_ 0xffffffff 
#define _MAXHALFNR_ 0xffffL 

#endif


#define LOHALF(x) ((DWORD)((x) & _MAXHALFNR_))
#define HIHALF(x) ((DWORD)((x) >> sizeof(DWORD)*4 & _MAXHALFNR_))
#define TOHIGH(x) ((DWORD)((x) << sizeof(DWORD)*4))

#define BigNumberAlloc(n) ((DWORD*)calloc(n, sizeof(DWORD)))
#define BigNumberFree(p) free(*p)
#define BigNumberSetEqual(a,b,size) memcpy(a,b,size*sizeof(DWORD))
#define BigNumberSetZero(a,size) memset(a,0,size*sizeof(DWORD))
#define BigNumberSetEqualdw(a,d,size) memset(a,0,size*sizeof(DWORD));a[0]=d

int RSA(DWORD out[], const DWORD in[], const DWORD private_key_or_e[], const DWORD public_key[], UINT nSize);