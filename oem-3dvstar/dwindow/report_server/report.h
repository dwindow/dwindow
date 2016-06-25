#pragma once

typedef struct
{
	DWORD header_size;
	__int64 file_size;
	unsigned char sha1[20];
	int rev;
} report_header_v1;