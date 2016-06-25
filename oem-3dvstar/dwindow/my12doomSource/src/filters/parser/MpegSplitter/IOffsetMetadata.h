#pragma once
#include <dshow.h>

// {51C07ADF-30AE-4058-9CD7-F76D4B35C2F7}
DEFINE_GUID(IID_IOffsetMetadata, 
			0x51c07adf, 0x30ae, 0x4058, 0x9c, 0xd7, 0xf7, 0x6d, 0x4b, 0x35, 0xc2, 0xf7);

class DECLSPEC_UUID("51C07ADF-30AE-4058-9CD7-F76D4B35C2F7") IOffsetMetadata : public IUnknown
{
public:
	virtual HRESULT GetOffset(REFERENCE_TIME time, REFERENCE_TIME frame_time, int * offset_out)=0;
};

// my12doom's offset metadata format:
typedef struct struct_my12doom_offset_metadata_header
{
	DWORD file_header;		// should be 'offs'
	DWORD version;
	DWORD point_count;
	DWORD fps_numerator;
	DWORD fps_denumerator;
} my12doom_offset_metadata_header;

// point data is stored in 8bit signed integer, upper 1bit is sign bit, lower 7bit is integer bits
// int value = (v&0x80)? -(&0x7f)v:(v&0x7f);