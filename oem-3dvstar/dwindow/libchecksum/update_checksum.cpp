#include "libchecksum.h"

bool verify_update_signature(dwindow_update_signature *signature)
{
	if (!signature)
		return false;

	unsigned char sha1[20];
	SHA1Hash(sha1, ((unsigned char*)signature) + sizeof(signature->signature), 
		sizeof(dwindow_update_signature) - sizeof(signature->signature));

	return verify_signature((DWORD*)sha1, (DWORD*)signature->signature, dwindow_network_n);
}