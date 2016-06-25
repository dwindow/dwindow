#include "SHA1.h"

#define SHA1_BLOCK_SIZE  64
#define SHA1_DIGEST_SIZE 20


#define rotate32(x,n) _lrotl((x), (n))
#if (SYSTEM_BIG_ENDIAN)
#define SHA_BLOCK32(x) (x)
const UINT _SHA_MASK_[4]={0x00000000, 0xff000000, 0xffff0000, 0xffffff00};
const UINT _SHA_BITS_[4]={0x80000000, 0x00800000, 0x00008000, 0x00000080};
#else
#define SHA_BLOCK32(x) ((rotate32((x), 8) & 0x00ff00ff) | (rotate32((x), 24) & 0xff00ff00))
const UINT _SHA_MASK_[4]={0x00000000, 0x000000ff, 0x0000ffff, 0x00ffffff};
const UINT _SHA_BITS_[4]={0x00000080, 0x00008000, 0x00800000, 0x80000000};
#endif

#define sha_round(func,k)  t = a; a = rotate32(a,5) + func(b,c,d) + e + k + w[i];\
	e = d;d = c; c = rotate32(b, 30); b = t;

#define F0to19(x,y,z)       (((x) & (y)) ^ (~(x) & (z)))
#define F20to39(x,y,z)		((x) ^ (y) ^ (z))
#define F40to59(x,y,z)      (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define F60to79(x,y,z)		 F20to39(x,y,z)


void SHA1Hash(unsigned char *_pOutDigest, const unsigned char *_pData,UINT nSize)
{

	// Be safe
	if ( !_pOutDigest || !_pData )
		return;

	SHA1_STATETYPE csha1;
	memset(&csha1,0,sizeof(csha1));
	SHA1_Start(&csha1);
	SHA1_Hash(_pData,nSize,&csha1);
	SHA1_Finish(_pOutDigest,&csha1);
}


void SHA1_Transform(SHA1_STATETYPE* _pcsha1)
{

	UINT   w[80], i, a, b, c, d, e, t;

	for (i = 0; i < SHA1_BLOCK_SIZE / 4; ++i)
		w[i] = SHA_BLOCK32(_pcsha1->wbuf[i]);

	for (i = SHA1_BLOCK_SIZE / 4; i < 80; ++i)
		w[i] = rotate32(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);

	a = _pcsha1->hash[0];
	b = _pcsha1->hash[1];
	c = _pcsha1->hash[2];
	d = _pcsha1->hash[3];
	e = _pcsha1->hash[4];

	for(i = 0; i < 20; ++i)
	{
		sha_round(F0to19, 0x5a827999);    
	}

	for(i = 20; i < 40; ++i)
	{
		sha_round(F20to39, 0x6ed9eba1);
	}

	for(i = 40; i < 60; ++i)
	{
		sha_round(F40to59, 0x8f1bbcdc);
	}

	for(i = 60; i < 80; ++i)
	{
		sha_round(F60to79, 0xca62c1d6);
	}

	_pcsha1->hash[0] += a; 
	_pcsha1->hash[1] += b; 
	_pcsha1->hash[2] += c; 
	_pcsha1->hash[3] += d; 
	_pcsha1->hash[4] += e;

}
void SHA1_Start(SHA1_STATETYPE *_pcsha1)
{

	_pcsha1->hash[0] = 0x67452301;
	_pcsha1->hash[1] = 0xefcdab89;
	_pcsha1->hash[2] = 0x98badcfe;
	_pcsha1->hash[3] = 0x10325476;
	_pcsha1->hash[4] = 0xc3d2e1f0;
	_pcsha1->count[0] = 0;
	_pcsha1->count[1] = 0;
}

void SHA1_Finish(unsigned char* _pShaValue, SHA1_STATETYPE* _pcsha1)
{
	UINT    i = (UINT)(_pcsha1->count[0] & (SHA1_BLOCK_SIZE - 1));
	_pcsha1->wbuf[i >> 2] = (_pcsha1->wbuf[i >> 2] & _SHA_MASK_[i & 3]) | _SHA_BITS_[i & 3];

	if(i > SHA1_BLOCK_SIZE - 9)
	{
		if(i < 60) _pcsha1->wbuf[15] = 0;
		SHA1_Transform(_pcsha1);
		i = 0;
	}
	else   
		i = (i >> 2) + 1;

	while(i < 14) 
		_pcsha1->wbuf[i++] = 0;


	_pcsha1->wbuf[14] = SHA_BLOCK32((_pcsha1->count[1] << 3) | (_pcsha1->count[0] >> 29));
	_pcsha1->wbuf[15] = SHA_BLOCK32(_pcsha1->count[0] << 3);

	SHA1_Transform(_pcsha1);


	for(i = 0; i < SHA1_DIGEST_SIZE; ++i)
		_pShaValue[i] = (unsigned char)(_pcsha1->hash[i >> 2] >> 8 * (~i & 3));
}


void SHA1_Hash(const unsigned char *_pData, unsigned int _iSize, SHA1_STATETYPE* _pcsha1)
{
	UINT ipos = (UINT)(_pcsha1->count[0] & (SHA1_BLOCK_SIZE - 1));
	UINT ispace = SHA1_BLOCK_SIZE - ipos;
	unsigned char *pData=(unsigned char *)_pData;
	if((_pcsha1->count[0] += _iSize) < _iSize)
		++(_pcsha1->count[1]);
	while(_iSize >= ispace)     
	{
		memcpy(((unsigned char*)_pcsha1->wbuf) + ipos, pData, ispace);
		ipos = 0; 
		_iSize -= ispace; 
		pData += ispace; 
		ispace = SHA1_BLOCK_SIZE; 
		SHA1_Transform(_pcsha1);
	}	
	memcpy(((unsigned char*)_pcsha1->wbuf) + ipos, pData, _iSize);
}
