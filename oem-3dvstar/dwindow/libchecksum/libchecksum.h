#pragma  once

#include "basicRSA.h"
#include "SHA1.h"
#include <time.h>

typedef struct dwindow_license_struct
{
	DWORD id[32];			// license id,
							// first dword is number, second dword is reserved for later number, now is zero.
							// last(the 31th) dword is CRC32 checksum of other 31 bytes.
							// other bytes can be used to store special rights

	DWORD signature[32];	// the signature, RSA(signature) must == id
							// load & save only use this signature, id is ignored.
} dwindow_license;

typedef struct dwindow_license_detail_struct
{
	DWORD id;
	DWORD id_high;
	DWORD user_rights;
	DWORD ext[28];

	DWORD CRC32;

	DWORD signature[32];
} dwindow_license_detail;

typedef struct _dwindow_message_uncrypt
{
	unsigned char passkey[32];
	unsigned char requested_hash[20];
	unsigned char random_AES_key[32];
	unsigned char password_uncrypted[20];
	__time64_t client_time;
	unsigned short client_rev;
	unsigned char reserved[13];
	unsigned char zero;
}dwindow_message_uncrypt;


typedef struct _dwindow_passkey_big
{
	unsigned char passkey[32];
	unsigned char passkey2[32];		// should be same
	unsigned char ps_aes_key[32];
	__time64_t time_start;			// 8 byte
	__time64_t time_end;			// 8 byte
	unsigned short max_bar_user;
	unsigned char usertype;
	unsigned int user_rights;
	unsigned char reserved[8];
	unsigned char zero;				// = 0
} dwindow_passkey_big;
const unsigned char USERTYPE_NORMAL = 0;
const unsigned char USERTYPE_THEATER = 1;
const unsigned char USERTYPE_JZ = 2;
const unsigned char USERTYPE_BAR = 3;		// unused
const unsigned char USERTYPE_VSTAR = 204;	// I forget why 204...

const unsigned int USER_RIGHTS_NO_AD = 1;										// donation user
const unsigned int USER_RIGHTS_DUAL_PROJECTOR_AND_EXTERNAL_SOUNDTRACK = 2;		// professional user
const unsigned int USER_RIGHTS_ACTIVE_SERVER = 4;								// net bar user (unused)
const unsigned int USER_RIGHTS_TELNET_SERVER = 8;
const unsigned int USER_RIGHTS_THEATER_BOX = 16;


typedef struct _dwindow_update_signature
{
	DWORD signature[32];		// signature of sha1
	BYTE sha1_of_file[20];		// expected sha1 of downloaded file of url.
	int revision;				// revision of new version
	wchar_t url[1024];			// NULL-terminated string, url
	wchar_t description[2048];	// 
}dwindow_update_signature;

#ifdef VSTAR
#define dwindow_n dwindow_vstar_n
#else
extern unsigned int dwindow_n[32];
#endif
extern DWORD dwindow_network_n[32];
extern DWORD dwindow_vstar_n[32];
bool verify_signature(const DWORD *checksum, const DWORD *signature, DWORD *public_key = NULL); // checksum is 20byte, signature is 128byte, true = match
int verify_file(wchar_t *file); //return value:
int video_checksum(wchar_t *file, DWORD *checksum);	// checksum is 20byte
int find_startcode(wchar_t *file);
bool veryfy_file_sha1(const wchar_t *file, DWORD *checksum);		// checksum is 20byte SHA1
void sha1_file(const wchar_t *file, DWORD *checksum);	// calculate sha1 of a file
HRESULT RSA_dwindow_public(const void *input, void *output);
HRESULT RSA_dwindow_network_public(const void *input, void *output);

// license funcs
bool is_valid_license(dwindow_license license);			// true = valid
bool load_license(wchar_t *file, dwindow_license *out);	// false = fail
bool save_license(wchar_t *file, dwindow_license in);	// false = fail
bool encrypt_license(dwindow_license license, DWORD *encrypt_out);	// out is 128byte, encrypt id for sending to server
bool decrypt_license(dwindow_license *license_out, DWORD *msg);		// msg is 128byte, decrypt license from message recieved from server
																	// warning: these two "en/de crypt" is not a pair
																	// encrypt <-> server_decrypt 
																	// server_encrypt <-> decrypt
																	// this is how it works.