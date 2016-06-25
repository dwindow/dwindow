#pragma once
#include <streams.h>

// {FF51A55E-6BB4-417f-99A5-13777373187B}
DEFINE_GUID(IID_IStereoLayout, 
			0xff51a55e, 0x6bb4, 0x417f, 0x99, 0xa5, 0x13, 0x77, 0x73, 0x73, 0x18, 0x7b);

class DECLSPEC_UUID("FF51A55E-6BB4-417f-99A5-13777373187B") IStereoLayout : public IUnknown
{
public:
	virtual HRESULT GetLayout(DWORD *out) PURE;
};

enum
{
	IStereoLayout_Unknown				= 0,
	IStereoLayout_2D					= 1,
	IStereoLayout_SideBySide			= 2,				// left on left
	IStereoLayout_TopBottom				= 3,				// left on top
	IStereoLayout_2D_Depth				= 4,				// 2D view on left, Depth view on right


	IStereoLayout_Swap					= 0x10000,			// swap eyes or views
	IStereoLayout_StillImage			= 0x20000,			// this means this is still image, 
															// any layout detector should finalize result immediately after first frame

	IStereoLayout_ForceDWORD			= 0xffffffff,
};