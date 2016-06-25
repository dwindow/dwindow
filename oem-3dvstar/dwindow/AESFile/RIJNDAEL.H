#pragma  once

// std_defs.h

/* 1. Standard types for AES cryptography source code               */

typedef unsigned char   u1byte; /* an 8 bit unsigned character type */
typedef unsigned short  u2byte; /* a 16 bit unsigned integer type   */
typedef unsigned long   u4byte; /* a 32 bit unsigned integer type   */

typedef signed char     s1byte; /* an 8 bit signed character type   */
typedef signed short    s2byte; /* a 16 bit signed integer type     */
typedef signed long     s4byte; /* a 32 bit signed integer type     */

class AESCryptor// : public AES
{
private:
	u4byte  k_len;
	u4byte  e_key[64];
	u4byte  d_key[64];

public:
	char *name(void);
	void set_key(const u1byte key[], const u4byte key_len);
	void encrypt(const u1byte in_blk[16], u1byte out_blk[16]);
	void decrypt(const u1byte in_blk[16], u1byte out_blk[16]);
};	 
