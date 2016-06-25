#pragma once

// constants and macro functions

const HRESULT E_RESOLUTION_MISSMATCH = 0x81000001;
const DWORD FVF_Flags = D3DFVF_XYZRHW | D3DFVF_TEX1;
const DWORD FVF_Flags_subtitle = D3DFVF_XYZW | D3DFVF_TEX1;
struct __declspec(uuid("{71771540-2017-11cf-ae26-0020afd79767}")) CLSID_my12doomRenderer;
#define WM_NV_NOTIFY (WM_USER+10086)
#define PCLEVELTEST_TESTED 1
#define PCLEVELTEST_YV12 2
#define PCLEVELTEST_NV12 4
#define PCLEVELTEST_YUY2 8
const int fade_in_out_time = 500;
#define my12doom_queue_size 16
#define stereo_test_texture_size 64
#define FAIL_RET(x) {hr=x; if(FAILED(hr)){return hr;}}
#define FAIL_FALSE(x) {hr=x; if(FAILED(hr)){return S_FALSE;}}
#define FAIL_SLEEP_RET(x) {hr=x; if(FAILED(hr)){Sleep(1); return hr;}}
#define safe_delete(x) if(x){delete x;x=NULL;}
#define safe_decommit(x) if((x))(x)->decommit()
#define JIF(x) {if(FAILED(hr=x))goto clearup;}
#define MAX_VIEWS 16

// structures

typedef struct _RECTF
{
	float left;
	float top;
	float right;
	float bottom;

	struct _RECTF operator=(const RECT in)
	{
		left = in.left; top = in.top; right = in.right; bottom = in.bottom;

		return *this;
	}
	operator RECT()
	{
		RECT r = {left+0.5,top+0.5,right+0.5,bottom+0.5};
		return r;
	}

} RECTF;

typedef POINTF VECTORF;

typedef struct _dummy_packet
{
	REFERENCE_TIME start;
	REFERENCE_TIME end;
} dummy_packet;

struct MyVertex
{
	float x , y, z;
	float w;
	float tu, tv;
};
struct MyVertex_subtitle
{
	float x , y, z;
	float w;
	float tu, tv;
};
typedef struct _my_quad 
{
	MyVertex vertexes[4];
} my_quad;

// enums

enum output_mode_types
{
	NV3D, masking, anaglyph, mono, pageflipping, iz3d,
	dual_window, out_sbs, out_tb,
	out_hsbs, out_htb, 
	hd3d, 
	intel3d,
	multiview,
	output_mode_types_max
};

enum display_orientation
{
	horizontal,
	vertical,
};

enum resampling_method
{
	bilinear_mipmap_minus_one = 0,
	lanczos = 1,
	bilinear_no_mipmap = 2,
	lanczos_onepass = 3,
	bilinear_mipmap = 4,
};

#ifndef def_input_layout_types
#define def_input_layout_types
enum input_layout_types
{
	side_by_side, 
	top_bottom, mono2d, 
	frame_sequence,
	input_layout_types_max, 
	input_layout_auto = 0x10000,
};
#endif

enum mask_mode_types
{
	row_interlace, 
	line_interlace, 
	checkboard_interlace,
	subpixel_row_interlace,
	subpixel_45_interlace,
	subpixel_x2_interlace,
	subpixel_x2_45_interlace,
	subpixel_x2_minus45_interlace,
	mask_mode_types_max,
};

enum multiview_mode_types
{
	simple_horizontal_4view,
	subpixel_horizontal_4view,
	simple_45_4view,
	subpixel_45_4view,
	subpixel_45_9view,

	multiview_mode_types_max,
};

enum aspect_mode_types
{
	aspect_letterbox,
	aspect_stretch,
	aspect_horizontal_fill,
	aspect_vertical_fill,
	aspect_mode_types_max,
};

enum interlace_types
{
	progressive,
	top_field_first,
	bottom_field_first,
};
enum deinterlace_types
{
	weaving,
	blending,
	bob,
	top_field,
	bottom_field,

	// these inter-frame methods won't be implemented in the near future
	selective_blending,
	inverse_telecine,
	telecide,
};


typedef struct resource_userdata_tag
{
	enum
	{
		RESOURCE_TYPE_GPU_SAMPLE,

		TYPE_FORCE_DWORD = 0xffffffff,
	} resource_type;

	bool managed;
	void *pointer;
} resource_userdata;
