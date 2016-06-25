// Direct3D9 part of my12doom renderer

#include "my12doomRenderer.h"
#include "PixelShaders/YV12.h"
#include "PixelShaders/NV12.h"
#include "PixelShaders/YUY2.h"
#include "PixelShaders/anaglyph.h"
#include "PixelShaders/masking.h"
#include "PixelShaders/stereo_test_sbs.h"
#include "PixelShaders/stereo_test_sbs2.h"
#include "PixelShaders/stereo_test_tb.h"
#include "PixelShaders/stereo_test_tb2.h"
#include "PixelShaders/vs_subtitle.h"
#include "PixelShaders/iz3d_back.h"
#include "PixelShaders/iz3d_front.h"
#include "PixelShaders/color_adjust.h"
#include "PixelShaders/blur.h"
#include "PixelShaders/blur2.h"
#include "PixelShaders/lanczosAll.h"
#include "PixelShaders/multiview4.h"
#include "PixelShaders/multiview6.h"
#include "PixelShaders/Alpha.h"
#include "3dvideo.h"
#include <dvdmedia.h>
#include <math.h>
#include <assert.h>
#include "..\lua\my12doom_lua.h"
#include "PixelShaders\P016.h"
#include "..\dwindow\dwindow_log.h"
#include "..\dwindow\IPinHook.h"
#include "..\dwindow\dx_player.h"

enum helper_sample_format
{
	helper_sample_format_rgb32,
	helper_sample_format_y,
	helper_sample_format_yv12,
	helper_sample_format_nv12,
	helper_sample_format_yuy2,
};

#pragma comment (lib, "igfx_s3dcontrol.lib")
#pragma comment (lib, "comsupp.lib")
#pragma comment (lib, "dxva2.lib")
#pragma comment (lib, "nvapi.lib")

int lockrect_surface = 0;
int lockrect_texture = 0;
__int64 lockrect_texture_cycle = 0;
const int MAX_TEXTURE_SIZE = 8192;



IDirect3DTexture9* helper_get_texture(gpu_sample *sample, helper_sample_format format);
HRESULT myMatrixOrthoRH(D3DMATRIX *matrix, float w, float h, float zn, float zf);
HRESULT myMatrixIdentity(D3DMATRIX *matrix);
RECTF ClipRect(const RECTF border, const RECTF rect2clip);
RECT ClipRect(const RECT border, const RECT rect2clip);

HRESULT mylog(wchar_t *format, ...)
{
	wchar_t tmp[10240];
	wchar_t tmp2[10240];
	va_list valist;
	va_start(valist, format);
	wvsprintf(tmp, format, valist);
	va_end(valist);

	wsprintfW(tmp2, L"(tid=%d)%s", GetCurrentThreadId(), tmp);
	OutputDebugStringW(tmp);
	dwindow_log_line(tmp);
	return S_OK;
}


HRESULT mylog(const char *format, ...)
{
	char tmp[10240];
	char tmp2[10240];
	va_list valist;
	va_start(valist, format);
	vsprintf(tmp, format, valist);
	va_end(valist);

	wsprintfA(tmp2, "(tid=%d)%s", GetCurrentThreadId(), tmp);
	OutputDebugStringA(tmp2);
	dwindow_log_line(tmp2);
	return S_OK;
}

my12doomRenderer::my12doomRenderer(HWND hwnd, HWND hwnd2/* = NULL*/)
:m_left_queue(_T("left queue"))
,m_right_queue(_T("right queue"))
,m_presenter(NULL)
,TEXTURE_SIZE(GET_CONST("TextureSize"))
,SUBTITLE_TEXTURE_SIZE(GET_CONST("SubtitleTextureSize"))
,EVRQueueSize(GET_CONST("EVRQueueSize"))
,SimplePageflipping(GET_CONST("SimplePageflipping"))
,GPUIdle(GET_CONST("GPUIdle"))
,m_saturation(GET_CONST("Saturation"))
,m_luminance(GET_CONST("Luminance"))
,m_hue(GET_CONST("Hue"))
,m_contrast(GET_CONST("Contrast"))
,m_saturation2(GET_CONST("Saturation2"))
,m_luminance2(GET_CONST("Luminance2"))
,m_hue2(GET_CONST("Hue2"))
,m_contrast2(GET_CONST("Contrast2"))
,m_movie_offset_x(GET_CONST("MoviePosX"))//0)
,m_movie_offset_y(GET_CONST("MoviePosY"))//0)
,m_forced_aspect(GET_CONST("Aspect"))//-1)
,m_aspect_mode(GET_CONST("AspectRatioMode"))//aspect_letterbox)
,m_movie_resizing(GET_CONST("MovieResampling"))//bilinear_mipmap_minus_one))//REG_DWORD)
,m_subtitle_resizing(GET_CONST("SubtitleResampling"))//bilinear_mipmap_minus_one))//REG_DWORD)
,m_swap_eyes(GET_CONST("SwapEyes"))//false)
,m_force2d(GET_CONST("Force2D"))//false)
{
	timeBeginPeriod(1);

	typedef HRESULT (WINAPI *LPDIRECT3DCREATE9EX)(UINT, IDirect3D9Ex**);

	// D3D && NV3D && intel 3D
	m_nv3d_enabled = false;
	m_nv3d_actived = false;
	m_nv3d_display = NULL;
	m_nv3d_display = 0;
	m_HD3DStereoModes = NULL;
	m_movie_scissor_rect = NULL;
	NvAPI_Status res = NvAPI_Initialize();
	if (NVAPI_OK == res)
	{
		NvU8 enabled3d;
		res = NvAPI_Stereo_IsEnabled(&enabled3d);
		if (res == NVAPI_OK)
		{
			dwindow_log_line("NV3D enabled.");
			m_nv3d_enabled = (bool)enabled3d;
		}

		//NvAPI_Stereo_SetDriverMode(NVAPI_STEREO_DRIVER_MODE_DIRECT);

		res = NvAPI_EnumNvidiaDisplayHandle(0, &m_nv3d_display);

		m_nv_version.version = NV_DISPLAY_DRIVER_VERSION_VER;
		res = NvAPI_GetDisplayDriverVersion(m_nv3d_display, &m_nv_version);

		if (res == NVAPI_OK && m_nv_version.drvVersion >= 27051)		// only 270.51+ supports windowed 3d vision
			m_nv3d_windowed = true;
		else
			m_nv3d_windowed = false;

		// disable new 3d vision interface for now
		m_nv_version.drvVersion = min(m_nv_version.drvVersion, 28500);
	}
	m_subtitle = NULL;
	m_subtitle_mem = NULL;
	m_nv3d_handle = NULL;
	m_output_mode = mono;
	m_intel_s3d = NULL;
	m_HD3DStereoModesCount = m_HD3Dlineoffset = 0;
	m_render_event = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_pool = NULL;
	m_last_rendered_sample1 = m_last_rendered_sample2 = m_sample2render_1 = m_sample2render_2 = NULL;
	m_enter_counter = 0;
	HMODULE h = LoadLibrary( L"d3d9.dll" );
	if ( h )
	{
		typedef HRESULT (WINAPI *LPDIRECT3DCREATE9EX)( UINT, IDirect3D9Ex**);

		LPDIRECT3DCREATE9EX func = (LPDIRECT3DCREATE9EX)GetProcAddress( h, "Direct3DCreate9Ex" );

		if ( func )
			func( D3D_SDK_VERSION, &m_D3DEx );

		FreeLibrary( h );
	}

	if (m_D3DEx)
		m_D3DEx->QueryInterface(IID_IDirect3D9, (void**)&m_D3D);
	else
		m_D3D = Direct3DCreate9( D3D_SDK_VERSION );


	// UI
	{
		CAutoLock lck(&m_uidrawer_cs);
		m_uidrawer = NULL;
	}

	// AES
	unsigned char ran[32];
	m_AES.set_key(ran, 256);

	// window
	m_hWnd = hwnd;
	m_hWnd2 = hwnd2;

	// thread
	m_device_threadid = GetCurrentThreadId();
	m_device_state = need_create;
	m_render_thread = INVALID_HANDLE_VALUE;
	m_render_thread_exit = false;

	init_variables();

	pump();

	reset();
}

void my12doomRenderer::init_variables()
{
	// movie scissor window
	if (m_movie_scissor_rect)
		delete m_movie_scissor_rect;
	m_movie_scissor_rect = NULL;

	// zoom factor
	m_zoom_factor = 1.0;

	// Vertical Sync
	m_vertical_sync = true;

	// Display Orientation
	m_display_orientation = horizontal;

	// PC level test
	m_PC_level = 0;
	m_last_reset_time = timeGetTime();

	// 2D - 3D Convertion
	m_convert3d = false;

	// parallax
	m_parallax = 0;

	// just for surface creation
	m_lVidWidth = m_lVidHeight = 64;

	// assume already removed from graph
	m_recreating_dshow_renderer = true;
	m_dshow_renderer1 = NULL;
	m_dshow_renderer2 = NULL;

	// dshow creation
	HRESULT hr;
	m_dsr0 = new my12doomRendererDShow(NULL, &hr, this, 0);
	m_dsr1 = new my12doomRendererDShow(NULL, &hr, this, 1);

	m_dsr0->QueryInterface(IID_IBaseFilter, (void**)&m_dshow_renderer1);
	m_dsr1->QueryInterface(IID_IBaseFilter, (void**)&m_dshow_renderer2);

	if(GET_CONST("EVR"))
	{
		//  EVR creation and configuration
		if (!m_evr) m_evr.CoCreateInstance(CLSID_EnhancedVideoRenderer);
		if (!m_evr2) m_evr2.CoCreateInstance(CLSID_EnhancedVideoRenderer);
		if (!m_presenter) m_presenter = new EVRCustomPresenter(hr, 1, this);
		if (!m_presenter2) m_presenter2 = new EVRCustomPresenter(hr, 2, this);

		if (m_evr)
		{
			CComQIPtr<IMFVideoPresenter, &IID_IMFVideoPresenter> presenter(m_presenter);
			CComQIPtr<IMFVideoRenderer, &IID_IMFVideoRenderer> evr_mf(m_evr);
			evr_mf->InitializeRenderer(NULL, presenter);
			CComQIPtr<IMFGetService, &IID_IMFGetService> evr_get(m_evr);
			CComPtr<IMFVideoDisplayControl> display_controll;
			evr_get->GetService(MR_VIDEO_RENDER_SERVICE, IID_IMFVideoDisplayControl, (void**)&display_controll);
			display_controll->SetVideoWindow(m_hWnd);
		}

		if (m_evr2)
		{
			CComQIPtr<IMFVideoPresenter, &IID_IMFVideoPresenter> presenter2(m_presenter);
			CComQIPtr<IMFVideoRenderer, &IID_IMFVideoRenderer> evr_mf(m_evr2);
			evr_mf->InitializeRenderer(NULL, presenter2);
			CComQIPtr<IMFGetService, &IID_IMFGetService> evr_get(m_evr2);
			CComPtr<IMFVideoDisplayControl> display_controll;
			evr_get->GetService(MR_VIDEO_RENDER_SERVICE, IID_IMFVideoDisplayControl, (void**)&display_controll);
			display_controll->SetVideoWindow(m_hWnd);
		}
	}
	m_recreating_dshow_renderer = false;

	// callback
	m_cb = NULL;

	// aspect and offset
	m_movie_offset_x = 0.0;
	m_movie_offset_y = 0.0;
	m_source_aspect = 1.0;

	// input / output
	m_input_layout = input_layout_auto;
	m_mask_mode = row_interlace;
	m_mask_parameter = 0;
	m_color1 = D3DCOLOR_XRGB(255, 0, 0);
	m_color2 = D3DCOLOR_XRGB(0, 255, 255);
	m_normal = m_sbs = m_tb = 0;
	m_forced_deinterlace = false;

	// ui & bitmap
	m_has_subtitle = false;
	m_subtitle_parallax = 0;
	m_subtitle_pixel_width = 0;
	m_subtitle_pixel_height = 0;
}

my12doomRenderer::~my12doomRenderer()
{
	terminate_render_thread();
	invalidate_gpu_objects();
	if (m_pool) delete m_pool;
	m_Device = NULL;
	m_D3D = NULL;
}

HRESULT my12doomRenderer::SetMediaType(const CMediaType *pmt, int id)
{
	if (id == 1)
		return S_OK;


	if (*pmt->FormatType() == FORMAT_VideoInfo)
	{
		dwindow_log_line("SetMediaType() VideoInfo1,  %s deinterlace", m_deinterlace ? "DO" : "NO");
		m_deinterlace = false;
	}
	else if (*pmt->FormatType() == FORMAT_VideoInfo2)
	{
		VIDEOINFOHEADER2 *vihIn = (VIDEOINFOHEADER2*)pmt->Format();
		if (vihIn->dwInterlaceFlags & AMINTERLACE_IsInterlaced)
		{
			m_deinterlace = true;
		}
		dwindow_log_line("SetMediaType() VideoInfo2,  %s deinterlace", m_deinterlace ? "DO" : "NO");
	}

	dwindow_log_line("SetMediaType() %s deinterlace", m_deinterlace ? "DO" : "NO");

	return S_OK;
}

HRESULT my12doomRenderer::CheckMediaType(const CMediaType *pmt, int id)
{
	if (m_sbs + m_tb + m_normal > 5 || (m_dsr0->is_connected() && m_dsr1->is_connected()))
		return E_FAIL;

	HRESULT hr = S_OK;
	bool deinterlace = false;
	int width, height, aspect_x, aspect_y;
	if (*pmt->FormatType() == FORMAT_VideoInfo)
	{
		dwindow_log_line("CheckMediaType(%d) VideoInfo1", id);
		VIDEOINFOHEADER *vihIn = (VIDEOINFOHEADER*)pmt->Format();
		width = vihIn->bmiHeader.biWidth;
		height = vihIn->bmiHeader.biHeight;
		aspect_x = width;
		aspect_y = abs(height);
		m_frame_length = vihIn->AvgTimePerFrame;
		m_first_PTS = 0;
		deinterlace = false;


		//if (m_dsr0->m_formattype == FORMAT_VideoInfo2)
		//	hr = E_FAIL;

	}

	else if (*pmt->FormatType() == FORMAT_VideoInfo2)
	{
		VIDEOINFOHEADER2 *vihIn = (VIDEOINFOHEADER2*)pmt->Format();
		dwindow_log_line("CheckMediaType(%d) VideoInfo2, flag: %d(%02x), %s deinterlace", id, vihIn->dwInterlaceFlags, vihIn->dwInterlaceFlags, deinterlace ? "DO" : "NO");
		if (vihIn->dwInterlaceFlags & AMINTERLACE_IsInterlaced)
		{
			deinterlace = true;
			if ((vihIn->dwInterlaceFlags & AMINTERLACE_FieldPatBothRegular))
			{
				//printf("AMINTERLACE_FieldPatBothRegular not supported, E_FAIL\n");
				//hr = E_FAIL;
			}
			//return E_FAIL;
		}
		else
		{
			deinterlace = false;
		}
		width = vihIn->bmiHeader.biWidth;
		height = vihIn->bmiHeader.biHeight;
		aspect_x = vihIn->dwPictAspectRatioX;
		aspect_y = vihIn->dwPictAspectRatioY;
		m_frame_length = vihIn->AvgTimePerFrame;
		m_first_PTS = 0;

		//AMINTERLACE_IsInterlaced
		// 0x21

	}
	else
	{
		dwindow_log_line("Not Video.");
		hr = E_FAIL;
	}


	if (height > 0)
		m_revert_RGB32 = true;
	else
		m_revert_RGB32 = false;

	height = abs(height);

	if (m_remux_mode && height == 1088)
	{
		height = 1080;
		aspect_x = 16;
		aspect_y = 9;
	}

	if (*pmt->Subtype() != MEDIASUBTYPE_YUY2 && m_remux_mode)
		hr = E_FAIL;

	if (aspect_x == 1920 && aspect_y == 1088)
	{
		aspect_x = 16;
		aspect_y = 9;
	}

	m_last_frame_time = 0;
	m_dsr0->set_queue_size(m_remux_mode ? my12doom_queue_size : my12doom_queue_size/2-1);
	m_dsr0->set_queue_size(my12doom_queue_size/2-1);
	if (m_remux_mode)
		m_frame_length = (m_lVidHeight == 1280) ? 83416 : 208541;

	if (id == 0 && SUCCEEDED(hr))
	{
		m_lVidWidth = width;
		m_lVidHeight = height;
		m_source_aspect = (double)aspect_x / aspect_y;

		hr = S_OK;
	}
	else if (SUCCEEDED(hr))
	{
		if (!m_dsr0->is_connected())
		{
			dwindow_log_line("don't connect 2nd renderer before 1st, E_FAIL.");
			hr = E_FAIL;
		}

		if (width == m_lVidWidth && height == m_lVidHeight  && SUCCEEDED(hr))			// format is not important, but resolution and formattype is
		{
			m_source_aspect = (double)aspect_x / aspect_y;
			hr = S_OK;
		}
		else if (SUCCEEDED(hr))
		{
			dwindow_log_line("resolution mismatch, E_FAIL.");
			hr = E_FAIL;
		}
	}

	if (hr == S_OK)
		m_deinterlace = deinterlace;

	dwindow_log_line("return %s.", hr == S_OK ? "S_OK" : "E_FAIL");

	return hr;
}
HRESULT	my12doomRenderer::BreakConnect(int id)
{
	return S_OK;
}
HRESULT my12doomRenderer::CompleteConnect(IPin *pRecievePin, int id)
{
	AM_MEDIA_TYPE mt;
	pRecievePin->ConnectionMediaType(&mt);
	if (mt.formattype == FORMAT_VideoInfo2)
	{
		VIDEOINFOHEADER2 *vihIn = (VIDEOINFOHEADER2*)mt.pbFormat;

		vihIn = vihIn;

	}

	m_normal = m_sbs = m_tb = 0;
	m_layout_detected = mono2d;
	m_no_more_detect = false;

	if (m_source_aspect > 2.65)
	{
		m_layout_detected = side_by_side;
		m_no_more_detect = true;
	}
	else if (m_source_aspect < 1.325)
	{
		m_layout_detected = top_bottom;
		m_no_more_detect = true;
	}

	return S_OK;
}

HRESULT my12doomRenderer::DoRender(int id, IMediaSample *media_sample)
{
	int l1 = timeGetTime();
	bool should_render = true;
	REFERENCE_TIME start=0, end=0;
	media_sample->GetTime(&start, &end);
	if (m_cb)
		m_cb->SampleCB(start + m_dsr0->m_thisstream, end + m_dsr0->m_thisstream, media_sample, id);
	if (id == 0)
	{
		m_first_PTS = m_first_PTS != 0 ? m_first_PTS : start;
		m_frame_length = m_frame_length != 0 ? m_frame_length :(start - m_first_PTS) ;
	}

	int l2 = timeGetTime();
	if (!m_dsr1->is_connected() && !m_remux_mode && (int)m_input_layout != frame_sequence)
	{
		// single stream
		CAutoLock lck(&m_queue_lock);
		gpu_sample *sample = NULL;
		do
		{
			if (sample) 
			{
				printf("drop left\n");
				m_dsr0->drop_packets(1);
				delete sample;
				sample = NULL;
			}
			if (m_left_queue.GetCount())
				sample = m_left_queue.RemoveHead();
		}while (sample && sample->m_start < start);

		if(sample)
		{
			CAutoLock lck(&m_packet_lock);
			safe_delete(m_sample2render_1);
			m_sample2render_1 = sample;
			should_render = true;
		}
	}
	else
	{
		// pd10
		int fn;
		if (m_remux_mode || ((int)m_input_layout == frame_sequence) && !m_dsr1->is_connected())
		{
			fn = (int)((double)(start+m_dsr0->m_thisstream)/m_frame_length + 0.5);
			start = (REFERENCE_TIME)(fn - (fn&1) +2) *m_frame_length;
		}

find_match:
		bool matched = false;
		REFERENCE_TIME matched_time = -1;
		gpu_sample *sample_left = NULL;
		gpu_sample *sample_right = NULL;

		// media sample matching
		{
			// find match
			CAutoLock lck(&m_queue_lock);
			for(POSITION pos_left = m_left_queue.GetHeadPosition(); pos_left; pos_left = m_left_queue.Next(pos_left))
			{
				gpu_sample *left_sample = m_left_queue.Get(pos_left);
				for(POSITION pos_right = m_right_queue.GetHeadPosition(); pos_right; pos_right = m_right_queue.Next(pos_right))
				{
					gpu_sample *right_sample = m_right_queue.Get(pos_right);
					if (abs((int)(left_sample->m_start - right_sample->m_start)) < m_frame_length/2)
					{
						matched_time = left_sample->m_start;
						matched = true;
						goto match_end;
					}
				}
			}

match_end:
			if (!matched || matched_time > start)
				return S_FALSE;

			if(matched)
			{
				// release any unmatched and retrive matched
				CAutoLock lck(&m_queue_lock);
				while(true)
				{
					sample_left = m_left_queue.RemoveHead();
					if(abs((int)(sample_left->m_start - matched_time)) > m_frame_length/2)
					{
						printf("drop left\n");
						m_dsr0->drop_packets(1);
						delete sample_left;
					}
					else
						break;
				}

				while(true)
				{
					sample_right = m_right_queue.RemoveHead();
					if(abs((int)(sample_right->m_start - matched_time)) > m_frame_length/2)
					{
						printf("drop right\n");
						(m_remux_mode || ((int)m_input_layout == frame_sequence && !m_dsr1->is_connected()) ?m_dsr0:m_dsr1)->drop_packets(1);
						delete sample_right;
					}
					else
						break;
				}
			}
		}

		if (sample_left && sample_right)
		{
			if (matched_time < start && false)
			{
				printf("drop a matched pair\n");
				m_dsr0->drop_packets(1);
				(m_remux_mode || ((int)m_input_layout == frame_sequence && !m_dsr1->is_connected()) ? m_dsr0 : m_dsr1)->drop_packets(1);
				delete sample_left;
				delete sample_right;
				sample_left = NULL;
				sample_right = NULL;
				goto find_match;
			}
			else
			{
				CAutoLock lck(&m_packet_lock);
				safe_delete (m_sample2render_1);
				m_sample2render_1 = sample_left;
				safe_delete(m_sample2render_2);
				m_sample2render_2 = sample_right;
				should_render = true;
			}
		}
	}
	int l3 = timeGetTime();
	if (m_output_mode != pageflipping && should_render)
	{
		render(true);
	}
	int l4 = timeGetTime();
	if (timeGetTime()-l1 > 25)
		mylog("DoRender() cost %dms(%d+%d+%d).\n", l4-l1, l2-l1, l3-l2, l4-l3);
	return S_OK;
}

HRESULT my12doomRenderer::DataPreroll(int id, IMediaSample *media_sample)
{
	SetThreadName(GetCurrentThreadId(), id==0?"Data thread 0":"Data thread 1");

	REFERENCE_TIME start, end;
	media_sample->GetTime(&start, &end);
	if (m_cb)
		m_cb->PrerollCB(start + m_dsr0->m_thisstream, end + m_dsr0->m_thisstream, media_sample, id);


	int l1 = timeGetTime();
	int bit_per_pixel = 12;
	if (m_dsr0->m_format == MEDIASUBTYPE_YUY2)
		bit_per_pixel = 16;
	else if (m_dsr0->m_format == MEDIASUBTYPE_RGB32 || m_dsr0->m_format == MEDIASUBTYPE_ARGB32)
		bit_per_pixel = 32;

	bool should_render = false;

	//mylog("%s : %dms\n", id==0?"left":"right", (int)(start/10000));

	bool dual_stream = (int)m_input_layout == frame_sequence || m_remux_mode || (m_dsr0->is_connected() && m_dsr1->is_connected() );
	input_layout_types input = m_input_layout == input_layout_auto ? m_layout_detected : m_input_layout;
	bool need_detect = !dual_stream && m_input_layout == input_layout_auto && !m_no_more_detect;
	gpu_sample * loaded_sample = NULL;
	int max_retry = 5;
	{
retry:
		CAutoLock lck(&m_pool_lock);
		loaded_sample = new gpu_sample(media_sample, m_pool, m_lVidWidth, m_lVidHeight, m_dsr0->m_format, m_revert_RGB32, m_forced_deinterlace, need_detect, m_remux_mode, D3DPOOL_SYSTEMMEM, m_PC_level);

//		if ( (m_dsr0->m_format == MEDIASUBTYPE_RGB32 && (!loaded_sample->m_tex_RGB32 || loaded_sample->m_tex_RGB32->creator != m_Device)) ||
//			 (m_dsr0->m_format == MEDIASUBTYPE_YUY2 && (!loaded_sample->m_tex_YUY2 || loaded_sample->m_tex_YUY2->creator != m_Device)) ||
//			 ((!loaded_sample->m_tex_Y || loaded_sample->m_tex_Y->creator != m_Device) && (m_dsr0->m_format == MEDIASUBTYPE_YV12 || m_dsr1->m_format == MEDIASUBTYPE_NV12))  )
		if (!loaded_sample->m_ready)
		{
			delete loaded_sample;
			if (max_retry--)
			{
				mylog("video upload failed, retry left: %d", max_retry);
				goto retry;
			}
			else
				return S_FALSE;
		}
	}

	/// GPU RAM protector:
	if (m_left_queue.GetCount() > my12doom_queue_size*2 || m_right_queue.GetCount() > my12doom_queue_size*2)
	{
		CAutoLock lck(&m_queue_lock);
		gpu_sample *sample = NULL;
		while (sample = m_left_queue.RemoveHead())
			delete sample;
		while (sample = m_right_queue.RemoveHead())
			delete sample;
	}


	AM_MEDIA_TYPE *pmt = NULL;
	media_sample->GetMediaType(&pmt);
	if (pmt)
	{
		if (pmt->formattype == FORMAT_VideoInfo2)
		{
			VIDEOINFOHEADER2 *vihIn = (VIDEOINFOHEADER2*)pmt->pbFormat;
			dwindow_log_line("sample video header 2.");

		}

		else if (pmt->formattype == FORMAT_VideoInfo)
		{
			VIDEOINFOHEADER *vihIn = (VIDEOINFOHEADER*)pmt->pbFormat;
			dwindow_log_line("sample video header.");

		}

	}

	int l2 = timeGetTime();
	if (!m_dsr1->is_connected() && !m_remux_mode && (int)m_input_layout != frame_sequence)
	{
		// single stream
		CAutoLock lck(&m_queue_lock);
		m_left_queue.AddTail(loaded_sample);
	}

	else if (m_remux_mode || ((int)m_input_layout == frame_sequence && !m_dsr1->is_connected()))
	{
		// PD10 remux
		// time and frame number
		REFERENCE_TIME TimeStart, TimeEnd;
		media_sample->GetTime(&TimeStart, &TimeEnd);
		int fn;

		fn = (int)((double)(TimeStart+m_dsr0->m_thisstream)/m_frame_length + 0.5);
		loaded_sample->m_start = (REFERENCE_TIME)(fn - (fn&1)) *m_frame_length;


		float frn = (float)(TimeStart+m_dsr0->m_thisstream)/m_frame_length + 0.5;
		loaded_sample->m_end = loaded_sample->m_start + 1;
		loaded_sample->m_fn = fn;

		CAutoLock lck(&m_queue_lock);
		if ( 1- (fn & 0x1))
		{
			m_left_queue.AddTail(loaded_sample);
		}
		else
		{
			m_right_queue.AddTail(loaded_sample);
		}
	}

	else
	{
		// double stream
		// insert
		CAutoLock lck(&m_queue_lock);
		if (id == 0)
			m_left_queue.AddTail(loaded_sample);
		else
			m_right_queue.AddTail(loaded_sample);


	}

	if (l2-l1 > 30)
		mylog("DataPreroll() time it :%d, %d ms\n", l2-l1, timeGetTime()-l2);

	return S_OK;
}

HRESULT my12doomRenderer::fix_nv3d_bug()
{
	HRESULT hr = S_OK;
	CComPtr<IDirect3DSurface9> m_nv3d_bugfix;
	FAIL_RET( m_Device->CreateRenderTarget(m_active_pp.BackBufferWidth, m_active_pp.BackBufferHeight, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, FALSE, &m_nv3d_bugfix, NULL));


	// NV3D acitivation
	NvAPI_Status res;
	if (m_nv3d_handle == NULL)
		res = NvAPI_Stereo_CreateHandleFromIUnknown(m_Device, &m_nv3d_handle);
	res = NvAPI_Stereo_SetNotificationMessage(m_nv3d_handle, (NvU64)m_hWnd, WM_NV_NOTIFY);
	if (m_output_mode == NV3D)
	{
		//mylog("activating NV3D\n");
		res = NvAPI_Stereo_Activate(m_nv3d_handle);
	}

	else
	{
		//mylog("deactivating NV3D\n");
		res = NvAPI_Stereo_Deactivate(m_nv3d_handle);
	}
	NvU8 actived = 0;
	res = NvAPI_Stereo_IsActivated(m_nv3d_handle, &actived);
	if (actived)
	{
		//mylog("init: NV3D actived\n");
		m_nv3d_actived = true;
	}
	else
	{
		//mylog("init: NV3D deactived\n");
		m_nv3d_actived = false;
	}

	if (res == NVAPI_OK && m_nv3d_windowed)
		m_nv3d_actived = m_output_mode == NV3D ? true : false;

	//res = NvAPI_Stereo_DestroyHandle(m_nv3d_handle);
	return hr;
}
HRESULT my12doomRenderer::delete_render_targets()
{
	intel_delete_rendertargets();
	m_swap1ex = NULL;
	m_swap1 = NULL;
	m_swap2 = NULL;
	m_nv3d_surface = NULL;
	m_tex_mask = NULL;
	if(m_Device && m_uidrawer)
		m_uidrawer->invalidate_gpu();
	return S_OK;
}

HRESULT my12doomRenderer::create_render_targets()
{
	HRESULT hr = S_OK;
	if (m_active_pp.Windowed)
	{
		RECT rect;
		GetClientRect(m_hWnd, &rect);
		m_active_pp.BackBufferWidth = rect.right - rect.left;
		m_active_pp.BackBufferHeight = rect.bottom - rect.top;

		D3DPRESENT_PARAMETERS pp = m_active_pp;
// 		pp.SwapEffect = D3DSWAPEFFECT_OVERLAY;
// 		pp.MultiSampleType = D3DMULTISAMPLE_NONE;
// 		pp.BackBufferFormat = D3DFMT_X8R8G8B8;
// 		m_new_pp.BackBufferFormat = D3DFMT_X8R8G8B8;
		if (m_active_pp.BackBufferWidth > 0 && m_active_pp.BackBufferHeight > 0)
			FAIL_RET(m_Device->CreateAdditionalSwapChain(&pp, &m_swap1))

		GetClientRect(m_hWnd2, &rect);
		m_active_pp2 = m_active_pp;
		m_active_pp2.BackBufferWidth = rect.right - rect.left;
		m_active_pp2.BackBufferHeight = rect.bottom - rect.top;
		if (m_active_pp2.BackBufferWidth > 0 && m_active_pp2.BackBufferHeight > 0)
			FAIL_RET(m_Device->CreateAdditionalSwapChain(&m_active_pp2, &m_swap2));
	}
	else
	{
		FAIL_RET(m_Device->GetSwapChain(0, &m_swap1));
	}

	if (m_swap1)
	{
 		m_swap1->QueryInterface(IID_IDirect3DSwapChain9Ex, (void**)&m_swap1ex);

		RECT tar = {0,0, m_active_pp.BackBufferWidth, m_active_pp.BackBufferHeight};
		if (m_output_mode == out_sbs)
			tar.right /= 2;
		else if (m_output_mode == out_tb)
			tar.bottom /= 2;

		if(m_Device && m_uidrawer)
			m_uidrawer->init_gpu(tar.right, tar.bottom, m_Device);
	}


	FAIL_RET(intel_create_rendertargets());
	FAIL_RET( m_Device->CreateTexture(m_active_pp.BackBufferWidth, m_active_pp.BackBufferHeight, 1, NULL, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_tex_mask, NULL));
	FAIL_RET( m_Device->CreateRenderTarget(m_active_pp.BackBufferWidth*2, m_active_pp.BackBufferHeight+1, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, TRUE, &m_nv3d_surface, NULL));
	FAIL_RET(generate_mask());

	// add 3d vision tag at last line on need
	if (m_output_mode == NV3D && m_nv3d_surface)
	{
		mylog("Adding NV3D Tag.\n");
		D3DLOCKED_RECT lr;
		RECT lock_tar={0, m_active_pp.BackBufferHeight, m_active_pp.BackBufferWidth*2, m_active_pp.BackBufferHeight+1};
		FAIL_RET(m_nv3d_surface->LockRect(&lr,&lock_tar,0));
		LPNVSTEREOIMAGEHEADER pSIH = (LPNVSTEREOIMAGEHEADER)(((unsigned char *) lr.pBits) + (lr.Pitch * (0)));	
		pSIH->dwSignature = NVSTEREO_IMAGE_SIGNATURE;
		pSIH->dwBPP = 32;
		pSIH->dwFlags = SIH_SIDE_BY_SIDE;
		pSIH->dwWidth = m_active_pp.BackBufferWidth*2;
		pSIH->dwHeight = m_active_pp.BackBufferHeight;
		FAIL_RET(m_nv3d_surface->UnlockRect());

		mylog("NV3D Tag Added.\n");
	}

	fix_nv3d_bug();
	return hr;
}

HRESULT my12doomRenderer::handle_device_state()							//handle device create/recreate/lost/reset
{
	if (m_device_state < need_reset)
	{
		HRESULT hr = m_Device->TestCooperativeLevel();
		if (FAILED(hr))
			set_device_state(device_lost);
	}

	if (GetCurrentThreadId() != m_device_threadid || m_recreating_dshow_renderer)
	{
		if (m_device_state == fine)
			return S_OK;
		else
			return E_FAIL;
	}

	anti_reenter anti(&m_anti_reenter, &m_enter_counter);
	if (anti.error)
		return S_OK;

	HRESULT hr;
	if (m_device_state != fine)
	{
		char * states[10]=
		{
			"fine",							// device is fine
			"need_resize_back_buffer",		// just resize back buffer and recaculate vertex
			"need_reset_object",				// objects size changed, should recreate objects
			"need_reset",						// reset requested by program, usually to change back buffer size, but program can continue rendering without reset
			"device_lost",					// device lost, can't continue
			"need_create",					// device not created, or need to recreate, can't continue
			"create_failed",					// 
			"device_state_max",				// not used
		};
		mylog("handling device state(%s)\n", states[m_device_state]);
	}

	if (m_device_state == fine)
		return S_OK;

	if (m_device_state == create_failed)
		return E_FAIL;

	else if (m_device_state == need_resize_back_buffer)
	{
		CAutoLock lck(&m_frame_lock);
		MANAGE_DEVICE;
		if (FAILED(hr=(delete_render_targets())))
		{
			m_device_state = device_lost;
			return hr;
		}
		if (FAILED(hr=(create_render_targets())))
		{
			m_device_state = device_lost;
			return hr;
		}
		m_device_state = fine;

		// clear DEFAULT pool
		{
			CAutoLock lck(&m_pool_lock);
			if (m_pool)
				m_pool->DestroyPool(D3DPOOL_DEFAULT);
		}

		render(true);
	}
	else if (m_device_state == need_reset_object)
	{
		terminate_render_thread();
		MANAGE_DEVICE;
		if (FAILED(hr=(invalidate_gpu_objects())))
		{
			m_device_state = device_lost;
			return hr;
		}
		if (FAILED(hr=(restore_gpu_objects())))
		{
			m_device_state = device_lost;
			return hr;
		}
		m_device_state = fine;
	}

	else if (m_device_state == need_reset)
	{
		terminate_render_thread();
		MANAGE_DEVICE;
		mylog("reseting device.\n");
		int l = timeGetTime();
		FAIL_SLEEP_RET(invalidate_gpu_objects());
		mylog("invalidate objects: %dms.\n", timeGetTime() - l);
		HRESULT hr = m_Device->Reset( &m_new_pp );
		hr = m_Device->Reset( &m_new_pp );
		if( FAILED(hr ) )
		{
			m_device_state = device_lost;
			return hr;
		}
		m_active_pp = m_new_pp;
		mylog("Device->Reset: %dms.\n", timeGetTime() - l);
		if (FAILED(hr=(restore_gpu_objects())))
		{
			m_device_state = device_lost;
			return hr;
		}
		mylog("restore objects: %dms.\n", timeGetTime() - l);
		m_device_state = fine;
		if (m_d3d_manager)
		{
			m_d3d_manager->ResetDevice(m_Device, m_resetToken);
			m_d3d_manager->OpenDeviceHandle(&m_device_handle);
		}

		mylog("m_active_pp : %dx%d@%dHz, format %d\n", m_active_pp.BackBufferWidth, 
			m_active_pp.BackBufferHeight, m_active_pp.FullScreen_RefreshRateInHz, m_active_pp.BackBufferFormat);
	}

	else if (m_device_state == device_lost)
	{
		Sleep(100);
		hr = m_Device->TestCooperativeLevel();
		if( hr  == D3DERR_DEVICENOTRESET )
		{
			terminate_render_thread();
			MANAGE_DEVICE;
			FAIL_SLEEP_RET(invalidate_gpu_objects());
			HRESULT hr = m_Device->Reset( &m_new_pp );

			if( FAILED(hr ) )
			{
				m_device_state = device_lost;
				return hr;
			}
			m_active_pp = m_new_pp;
			FAIL_SLEEP_RET(restore_gpu_objects());
			if (m_d3d_manager)
			{
				m_d3d_manager->ResetDevice(m_Device, m_resetToken);
				m_d3d_manager->OpenDeviceHandle(&m_device_handle);
			}
			
			m_device_state = fine;
			return hr;
		}

		else if (hr == D3DERR_DEVICELOST)
		{
			terminate_render_thread();
			MANAGE_DEVICE;
			FAIL_SLEEP_RET(invalidate_gpu_objects());
		}

		else if (hr == S_OK)
			m_device_state = fine;

		else
			return E_FAIL;
	}

	else if (m_device_state == need_create)
	{
		if (!m_Device)
		{
			ZeroMemory( &m_active_pp, sizeof(m_active_pp) );
			m_active_pp.Windowed               = TRUE;
			m_active_pp.SwapEffect             = D3DSWAPEFFECT_DISCARD;
			m_active_pp.PresentationInterval   = m_vertical_sync ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;
			m_active_pp.BackBufferCount = 1;
			m_active_pp.Flags = D3DPRESENTFLAG_VIDEO;
			m_active_pp.BackBufferFormat = D3DFMT_A8R8G8B8;
			m_active_pp.MultiSampleType = D3DMULTISAMPLE_NONE;

			m_style = GetWindowLongPtr(m_hWnd, GWL_STYLE);
			m_exstyle = GetWindowLongPtr(m_hWnd, GWL_EXSTYLE);
			GetWindowRect(m_hWnd, &m_window_pos);

			set_fullscreen(false);

			/*
			set_fullscreen(true);
			m_active_pp.BackBufferWidth = 1680;
			m_active_pp.BackBufferHeight = 1050;
			m_active_pp.Windowed = FALSE;
			*/
		}
		else
		{
			terminate_render_thread();
			MANAGE_DEVICE;
			CAutoLock lck2(&m_pool_lock);
			invalidate_gpu_objects();
			invalidate_cpu_objects();

			// what invalidate_gpu_objects() don't do
			gpu_sample *sample = NULL;
			while (sample = m_left_queue.RemoveHead())
				delete sample;
			while (sample = m_right_queue.RemoveHead())
				delete sample;
			m_dsr0->drop_packets();
			m_dsr1->drop_packets();
			{
				CAutoLock lck(&m_packet_lock);
				safe_delete(m_sample2render_1);
				safe_delete(m_sample2render_2);
			}

			{
				CAutoLock rendered_lock(&m_rendered_packet_lock);
				safe_delete(m_last_rendered_sample1);
				safe_delete(m_last_rendered_sample2);
			}



			{
				CAutoLock lck(&m_uidrawer_cs);
				if(m_uidrawer)
				{
					m_uidrawer->invalidate_gpu();
					m_uidrawer->invalidate_cpu();
				}
			}

			m_Device = NULL;
			m_active_pp = m_new_pp;
		}

		UINT AdapterToUse=D3DADAPTER_DEFAULT;
		char adapter_used_desc[512];	// debug only
		D3DDEVTYPE DeviceType=D3DDEVTYPE_HAL;

		D3DADAPTER_IDENTIFIER9  id1, id2; 
		for (UINT Adapter=0;Adapter<m_D3D->GetAdapterCount();Adapter++)
		{
			HMONITOR windowmonitor1 = MonitorFromWindow(m_hWnd, NULL);
			HMONITOR windowmonitor2 = MonitorFromWindow(m_hWnd2, NULL);

			if (windowmonitor1 == m_D3D->GetAdapterMonitor(Adapter))
				m_D3D->GetAdapterIdentifier(Adapter,0,&id1); 
			if (windowmonitor2 == m_D3D->GetAdapterMonitor(Adapter))
				m_D3D->GetAdapterIdentifier(Adapter,0,&id2); 
		}

		for (UINT Adapter=0;Adapter<m_D3D->GetAdapterCount();Adapter++)
		{ 
			D3DADAPTER_IDENTIFIER9  Identifier; 
			HRESULT       Res; 

			Res = m_D3D->GetAdapterIdentifier(Adapter,0,&Identifier); 
			HMONITOR windowmonitor = MonitorFromWindow(m_hWnd, NULL);
			if (windowmonitor != m_D3D->GetAdapterMonitor(Adapter))
				continue;


			AdapterToUse = Adapter;
			strcpy(adapter_used_desc, Identifier.Description);
#ifdef PERFHUD
			if (strstr(Identifier.Description,"PerfHUD") != 0) 
			{ 
				AdapterToUse=Adapter; 
				strcpy(adapter_used_desc, Identifier.Description);
				DeviceType=D3DDEVTYPE_REF; 

				break; 
			}
#endif
		}
		HRESULT hr;
		MANAGE_DEVICE;

		FAIL_RET(intel_get_caps());

		mylog("using adapter %s\n", adapter_used_desc);

		DWORD creation_flag = D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED;
		if (m_D3DEx)
		{
			creation_flag |= D3DCREATE_ENABLE_PRESENTSTATS;
#ifdef ZHUZHU
			creation_flag |= D3DCREATE_NOWINDOWCHANGES;
#endif
			D3DDISPLAYMODEEX display_mode = {sizeof(D3DDISPLAYMODEEX), m_active_pp.BackBufferWidth, m_active_pp.BackBufferHeight,
				m_active_pp.FullScreen_RefreshRateInHz, m_active_pp.BackBufferFormat, D3DSCANLINEORDERING_PROGRESSIVE};

			m_DeviceEx = NULL;
			FAIL_RET(m_D3DEx->CreateDeviceEx( AdapterToUse, DeviceType, m_hWnd, creation_flag,
				&m_active_pp, m_active_pp.Windowed ? NULL : &display_mode, &m_DeviceEx ));

			m_DeviceEx->QueryInterface(IID_IDirect3DDevice9, (void**)&m_Device);
		}
		else
		{
			hr = m_D3D->CreateDevice( AdapterToUse, DeviceType, m_hWnd, creation_flag, &m_active_pp, &m_Device );
		}

		if (FAILED(hr))
			return hr;

		mylog("new device: 0x%08x\n", m_Device.p);

		m_d3d_manager = NULL;
		hr = myDXVA2CreateDirect3DDeviceManager9(&m_resetToken, &m_d3d_manager);

		if (m_d3d_manager)
		{
			m_d3d_manager->ResetDevice(m_Device, m_resetToken);
			m_d3d_manager->OpenDeviceHandle(&m_device_handle);
		}

		m_new_pp = m_active_pp;
		m_device_state = need_reset_object;

		FAIL_RET(HD3D_one_time_init());


		set_output_mode(m_output_mode);		// just to active nv3d

		{
			CAutoLock lck(&m_pool_lock);
			if (m_pool) delete m_pool;
			m_pool = new CTextureAllocator(m_Device);
			mylog("new pool: 0x%08x\n", m_pool);
		}

		FAIL_SLEEP_RET(restore_cpu_objects());
		FAIL_SLEEP_RET(restore_gpu_objects());
		m_device_state = fine;	
	}

	return S_OK;
}

HRESULT my12doomRenderer::test_device_state()
{
	if (m_device_state == fine)
		return S_OK;
	if (m_device_state < device_lost)
		return S_FALSE;
	return E_FAIL;
}

HRESULT my12doomRenderer::set_device_state(device_state new_state)
{
	m_device_state = max(m_device_state, new_state);

	if (m_device_state >= device_lost)
		Sleep(50);
	return S_OK;
}
HRESULT my12doomRenderer::reset()
{
	terminate_render_thread();
	set_device_state(need_reset_object);
	init_variables();
	MANAGE_DEVICE;

	// what invalidate_gpu_objects() don't do
	gpu_sample *sample = NULL;
	while (sample = m_left_queue.RemoveHead())
		delete sample;
	while (sample = m_right_queue.RemoveHead())
		delete sample;
	m_dsr0->drop_packets();
	m_dsr1->drop_packets();
	{
		CAutoLock lck(&m_packet_lock);
		safe_delete(m_sample2render_1);
		safe_delete(m_sample2render_2);
	}

	{
		CAutoLock rendered_lock(&m_rendered_packet_lock);
		safe_delete(m_last_rendered_sample1);
		safe_delete(m_last_rendered_sample2);
	}


	invalidate_gpu_objects();
	return S_OK;
}
HRESULT my12doomRenderer::create_render_thread()
{
	CAutoLock lck(&m_thread_lock);
	if (INVALID_HANDLE_VALUE != m_render_thread)
		terminate_render_thread();
	m_render_thread_exit = false;
	m_render_thread = CreateThread(0,0,render_thread, this, NULL, NULL);
	return S_OK;
}
HRESULT my12doomRenderer::terminate_render_thread()
{
	CAutoLock lck(&m_thread_lock);
	if (INVALID_HANDLE_VALUE == m_render_thread)
		return S_FALSE;
	m_render_thread_exit = true;
	WaitForSingleObject(m_render_thread, INFINITE);
	m_render_thread = INVALID_HANDLE_VALUE;

	return S_OK;
}

HRESULT my12doomRenderer::invalidate_cpu_objects()
{
	{
		CAutoLock lck(&m_uidrawer_cs);
		if (m_uidrawer) m_uidrawer->invalidate_cpu();
	}
	CAutoLock lck2(&m_pool_lock);
	if (m_pool) m_pool->DestroyPool(D3DPOOL_MANAGED);
	if (m_pool) m_pool->DestroyPool(D3DPOOL_SYSTEMMEM);

	return S_OK;
}

HRESULT my12doomRenderer::invalidate_gpu_objects()
{
	HD3D_invalidate_objects();
	{
		CAutoLock lck(&m_uidrawer_cs);
		if (m_uidrawer) m_uidrawer->invalidate_gpu();
	}
	m_red_blue.invalid();
	m_ps_masking.invalid();
	m_lanczosX.invalid();
	m_lanczosX_NV12.invalid();
	m_lanczosX_YV12.invalid();
	m_lanczosX_P016.invalid();
	m_lanczosY.invalid();
	m_lanczos.invalid();
	m_lanczos_NV12.invalid();
	m_lanczos_YV12.invalid();
	m_lanczos_P016.invalid();
	m_multiview4.invalid();
	m_multiview6.invalid();
	m_alpha_multiply.invalid();
	m_ps_P016.invalid();

	// query
	//m_d3d_query = NULL;

	// pixel shader
	m_ps_yv12 = NULL;
	m_ps_nv12 = NULL;
	m_ps_yuy2 = NULL;
	m_ps_test = NULL;
	m_ps_test_sbs = NULL;
	m_ps_test_sbs2 = NULL;
	m_ps_test_tb = NULL;
	m_ps_test_tb2 = NULL;
	m_ps_anaglyph = NULL;
	m_ps_iz3d_back = NULL;
	m_ps_iz3d_front = NULL;
	m_ps_color_adjust = NULL;
	m_ps_bmp_blur = NULL;

	// textures
	safe_delete(m_subtitle);

	// pending samples
	{
		CAutoLock lck(&m_queue_lock);
		gpu_sample *sample = NULL;

		for(POSITION pos = m_left_queue.GetHeadPosition();
			pos; pos = m_left_queue.Next(pos))
		{
			sample = m_left_queue.Get(pos);
			safe_decommit(sample);
		}

		for(POSITION pos = m_right_queue.GetHeadPosition();
			pos; pos = m_right_queue.Next(pos))
		{
			sample = m_right_queue.Get(pos);
			safe_decommit(sample);
		}

	}
	{
		CAutoLock lck(&m_packet_lock);
		safe_decommit(m_sample2render_1);
		safe_decommit(m_sample2render_2);
	}

	{
		CAutoLock rendered_lock(&m_rendered_packet_lock);
		safe_decommit(m_last_rendered_sample1);
		safe_decommit(m_last_rendered_sample2);
	}

	{
		CAutoLock lck(&m_pool_lock);
		if (m_pool) m_pool->DestroyPool(D3DPOOL_DEFAULT);
	}

	// surfaces
	m_deinterlace_surface = NULL;
	m_PC_level_test = NULL;

	// swap chains
	delete_render_targets();

	return S_OK;
}

// test_PC_leve()'s helper function
HRESULT mark_level_result(IDirect3DSurface9 *result, DWORD *out, DWORD flag)
{
	HRESULT hr;
	D3DLOCKED_RECT lock_rect;
	FAIL_RET(result->LockRect(&lock_rect, NULL, NULL));
	BYTE *p = (BYTE*)lock_rect.pBits;

	for(int i=0; i<stereo_test_texture_size; i++)
	{
		for (int j=0; j<stereo_test_texture_size; j++)
			if (p[j*4] || p[j*4+1] || p[j*4+2])
			{
				*out &= ~flag;
				result->UnlockRect();
				return S_FALSE;
			}

		p += lock_rect.Pitch;
	}

	*out |= flag;
	result->UnlockRect();
	return S_OK;
}

HRESULT my12doomRenderer::test_PC_level()
{
// 	return S_OK;
// 	HRESULT hr;
// 
// 	// YV12
// 	m_PC_level_test = NULL;
// 	FAIL_RET(m_Device->CreateOffscreenPlainSurface(stereo_test_texture_size, stereo_test_texture_size, (D3DFORMAT)MAKEFOURCC('Y','V','1','2'), D3DPOOL_DEFAULT, &m_PC_level_test, NULL));
// 	D3DLOCKED_RECT lock_rect;
// 	m_PC_level_test->LockRect(&lock_rect, NULL, NULL);
// 	if (lock_rect.pBits)
// 	{
// 		memset(lock_rect.pBits, 16, stereo_test_texture_size * lock_rect.Pitch);
// 		memset((BYTE*)lock_rect.pBits +  stereo_test_texture_size * lock_rect.Pitch, 128, stereo_test_texture_size * lock_rect.Pitch / 2);
// 	}
// 	hr = m_PC_level_test->UnlockRect();
// 	hr = clear(m_stereo_test_gpu, D3DCOLOR_XRGB(255,255,255));
// 	hr = m_Device->StretchRect(m_PC_level_test, NULL, m_stereo_test_gpu, NULL, D3DTEXF_NONE);
// 	hr = m_Device->GetRenderTargetData(m_stereo_test_gpu, m_stereo_test_cpu);
// 	mark_level_result(m_stereo_test_cpu, &m_PC_level, PCLEVELTEST_YV12);
// 
// 
// 	// NV12
// 	m_PC_level_test = NULL;
// 	FAIL_RET(m_Device->CreateOffscreenPlainSurface(stereo_test_texture_size, stereo_test_texture_size, (D3DFORMAT)MAKEFOURCC('N','V','1','2'), D3DPOOL_DEFAULT, &m_PC_level_test, NULL));
// 	m_PC_level_test->LockRect(&lock_rect, NULL, NULL);
// 	if (lock_rect.pBits)
// 	{
// 		memset(lock_rect.pBits, 16, stereo_test_texture_size * lock_rect.Pitch);
// 		memset((BYTE*)lock_rect.pBits +  stereo_test_texture_size * lock_rect.Pitch, 128, stereo_test_texture_size * lock_rect.Pitch / 2);
// 	}
// 	hr = m_PC_level_test->UnlockRect();
// 	hr = clear(m_stereo_test_gpu, D3DCOLOR_XRGB(255,255,255));
// 	hr = m_Device->StretchRect(m_PC_level_test, NULL, m_stereo_test_gpu, NULL, D3DTEXF_NONE);
// 	hr = m_Device->GetRenderTargetData(m_stereo_test_gpu, m_stereo_test_cpu);
// 	mark_level_result(m_stereo_test_cpu, &m_PC_level, PCLEVELTEST_NV12);
// 
// 
// 	// YUY2
// 	unsigned char one_line[stereo_test_texture_size*2];
// 	for(int i=0; i<stereo_test_texture_size; i++)
// 	{
// 		one_line[i*2] = 16;
// 		one_line[i*2+1] = 128;
// 	}
// 	m_PC_level_test = NULL;
// 	FAIL_RET(m_Device->CreateOffscreenPlainSurface(stereo_test_texture_size, stereo_test_texture_size, (D3DFORMAT)MAKEFOURCC('Y','U','Y','2'), D3DPOOL_DEFAULT, &m_PC_level_test, NULL));
// 	m_PC_level_test->LockRect(&lock_rect, NULL, NULL);
// 	if (lock_rect.pBits)
// 	{
// 		for(int i=0; i<stereo_test_texture_size; i++)
// 			memcpy((BYTE*)lock_rect.pBits + lock_rect.Pitch * i, one_line, sizeof(one_line));
// 	}
// 	hr = m_PC_level_test->UnlockRect();
// 	hr = clear(m_stereo_test_gpu, D3DCOLOR_XRGB(255,255,255));
// 	hr = m_Device->StretchRect(m_PC_level_test, NULL, m_stereo_test_gpu, NULL, D3DTEXF_NONE);
// 	hr = m_Device->GetRenderTargetData(m_stereo_test_gpu, m_stereo_test_cpu);
// 	mark_level_result(m_stereo_test_cpu, &m_PC_level, PCLEVELTEST_YUY2);
// 
// 
// 	m_PC_level |= PCLEVELTEST_TESTED;
// 	m_PC_level_test = NULL;
// 	m_PC_level = PCLEVELTEST_TESTED;
// 
	return S_OK;
}
HRESULT my12doomRenderer::restore_cpu_objects()
{
	if (m_Device && m_uidrawer)
		m_uidrawer->init_cpu(m_Device);

	return S_OK;
}
HRESULT my12doomRenderer::restore_gpu_objects()
{
	HRESULT hr;
	m_Device->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	m_Device->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

	m_red_blue.set_source(m_Device, g_code_anaglyph, sizeof(g_code_anaglyph), true, (DWORD*)m_key);
	m_ps_masking.set_source(m_Device, g_code_masking, sizeof(g_code_masking), true, (DWORD*)m_key);
	m_lanczosX.set_source(m_Device, g_code_lanczosX, sizeof(g_code_lanczosX), true, (DWORD*)m_key);
	m_lanczosX_NV12.set_source(m_Device, g_code_lanczosX_NV12, sizeof(g_code_lanczosX_NV12), true, (DWORD*)m_key);
	m_lanczosX_P016.set_source(m_Device, g_code_lanczosX_P016, sizeof(g_code_lanczosX_P016), true, (DWORD*)m_key);
	m_lanczosX_YV12.set_source(m_Device, g_code_lanczosX_YV12, sizeof(g_code_lanczosX_YV12), true, (DWORD*)m_key);
	m_lanczosY.set_source(m_Device, g_code_lanczosY, sizeof(g_code_lanczosY), true, (DWORD*)m_key);
	m_lanczos.set_source(m_Device, g_code_lanczos, sizeof(g_code_lanczos), true, (DWORD*)m_key);
	m_lanczos_NV12.set_source(m_Device, g_code_lanczos_NV12, sizeof(g_code_lanczos_NV12), true, (DWORD*)m_key);
	m_lanczos_P016.set_source(m_Device, g_code_lanczos_P016, sizeof(g_code_lanczos_P016), true, (DWORD*)m_key);
	m_lanczos_YV12.set_source(m_Device, g_code_lanczos_YV12, sizeof(g_code_lanczos_YV12), true, (DWORD*)m_key);
	m_multiview4.set_source(m_Device, g_code_multiview4, sizeof(g_code_multiview4), true, (DWORD*)m_key);
	m_multiview6.set_source(m_Device, g_code_multiview6, sizeof(g_code_multiview6), true, (DWORD*)m_key);
	m_alpha_multiply.set_source(m_Device, g_code_alpha_only, sizeof(g_code_alpha_only), true, (DWORD*)m_key);
	m_ps_P016.set_source(m_Device, g_code_P016toRGB, sizeof(g_code_P016toRGB), true, (DWORD*)m_key);

	int l = timeGetTime();
	m_pass1_width = m_lVidWidth;
	m_pass1_height = m_lVidHeight;
	if (!(m_dsr0->is_connected() && m_dsr1->is_connected()) && !m_remux_mode && (int)m_input_layout != frame_sequence)
	{
		input_layout_types input = m_input_layout == input_layout_auto ? m_layout_detected : m_input_layout;
		if (input == side_by_side)
			m_pass1_width /= 2;
		else if (input == top_bottom)
			m_pass1_height /= 2;
	}

	FAIL_RET(HD3D_restore_objects());
	fix_nv3d_bug();

	DWORD use_mipmap = D3DUSAGE_AUTOGENMIPMAP;

	// query
	//m_Device->CreateQuery(D3DQUERYTYPE_TIMESTAMP, &m_d3d_query);

	// confirm max TEXTURE_SIZE
	D3DCAPS9 caps;
	ZeroMemory(&caps, sizeof(caps));
	m_Device->GetDeviceCaps(&caps);
	TEXTURE_SIZE = min((int)TEXTURE_SIZE, min(caps.MaxTextureWidth, caps.MaxTextureHeight));

	// textures
	FAIL_RET(m_Device->CreateRenderTarget(m_pass1_width, m_pass1_height/2, m_active_pp.BackBufferFormat, D3DMULTISAMPLE_NONE, 0, FALSE, &m_deinterlace_surface, NULL));

	FAIL_RET(create_render_targets());

	// pixel shader
	unsigned char *yv12 = (unsigned char *)malloc(sizeof(g_code_YV12toRGB));
	unsigned char *nv12 = (unsigned char *)malloc(sizeof(g_code_NV12toRGB));
	unsigned char *yuy2 = (unsigned char *)malloc(sizeof(g_code_YUY2toRGB));
	//unsigned char *tester = (unsigned char *)malloc(sizeof(g_code_main));
	unsigned char *anaglyph = (unsigned char *)malloc(sizeof(g_code_anaglyph));

	memcpy(yv12, g_code_YV12toRGB, sizeof(g_code_YV12toRGB));
	memcpy(nv12, g_code_NV12toRGB, sizeof(g_code_NV12toRGB));
	memcpy(yuy2, g_code_YUY2toRGB, sizeof(g_code_YUY2toRGB));
	//memcpy(tester, g_code_main, sizeof(g_code_main));
	memcpy(anaglyph, g_code_anaglyph, sizeof(g_code_anaglyph));

	//int size = sizeof(g_code_main);
	for(int i=0; i<16384; i+=16)
	{
		if (i<sizeof(g_code_YV12toRGB)/16*16)
			m_AES.decrypt(yv12+i, yv12+i);
		if (i<sizeof(g_code_NV12toRGB)/16*16)
			m_AES.decrypt(nv12+i, nv12+i);
		if (i<sizeof(g_code_YUY2toRGB)/16*16)
			m_AES.decrypt(yuy2+i, yuy2+i);
		//if (i<sizeof(g_code_main)/16*16)
		//	m_AES.decrypt(tester+i, tester+i);
		if (i<sizeof(g_code_anaglyph)/16*16)
			m_AES.decrypt(anaglyph+i, anaglyph+i);
	}

	int r = memcmp(yv12, g_code_YV12toRGB, sizeof(g_code_YV12toRGB));

	// shaders
	m_Device->CreatePixelShader((DWORD*)yv12, &m_ps_yv12);
	m_Device->CreatePixelShader((DWORD*)nv12, &m_ps_nv12);
	m_Device->CreatePixelShader((DWORD*)yuy2, &m_ps_yuy2);
	//m_Device->CreatePixelShader((DWORD*)tester, &m_ps_test);
	m_Device->CreatePixelShader((DWORD*)g_code_sbs, &m_ps_test_sbs);
	m_Device->CreatePixelShader((DWORD*)g_code_sbs2, &m_ps_test_sbs2);
	m_Device->CreatePixelShader((DWORD*)g_code_tb, &m_ps_test_tb);
	m_Device->CreatePixelShader((DWORD*)g_code_tb2, &m_ps_test_tb2);
	m_Device->CreatePixelShader((DWORD*)anaglyph, &m_ps_anaglyph);
	m_Device->CreatePixelShader((DWORD*)g_code_iz3d_back, &m_ps_iz3d_back);
	m_Device->CreatePixelShader((DWORD*)g_code_iz3d_front, &m_ps_iz3d_front);
	m_Device->CreatePixelShader((DWORD*)g_code_color_adjust, &m_ps_color_adjust);
	//m_Device->CreatePixelShader((DWORD*)g_code_bmp_blur, &m_ps_bmp_blur);
	if (m_ps_bmp_blur == NULL)
		m_Device->CreatePixelShader((DWORD*)g_code_bmp_blur2, &m_ps_bmp_blur);

	// for pixel shader 2.0 cards
	if (m_ps_test_tb2 != NULL && m_ps_test_tb == NULL)
		m_ps_test_tb = m_ps_test_tb2;
	if (m_ps_test_sbs2 != NULL && m_ps_test_sbs == NULL)
		m_ps_test_sbs = m_ps_test_sbs2;


	free(yv12);
	free(nv12);
	free(yuy2);
	//free(tester);
	free(anaglyph);

	// thread
	create_render_thread();

	// restore image if possible
	reload_image();

	return S_OK;
}


HRESULT my12doomRenderer::reload_image()
{
	CAutoLock lck(&m_packet_lock);
	CAutoLock rendered_lock(&m_rendered_packet_lock);

	if (!m_sample2render_1)
	{
		safe_delete(m_sample2render_1);
		safe_delete(m_sample2render_2);
		m_sample2render_1 = m_last_rendered_sample1;
		m_sample2render_2 = m_last_rendered_sample2;

		safe_decommit(m_sample2render_1);
		safe_decommit(m_sample2render_2);

		m_last_rendered_sample1 = NULL;
		m_last_rendered_sample2 = NULL;
	}
	return S_OK;
}

HRESULT my12doomRenderer::render_helper(IDirect3DSurface9 *surfaces[], int nview)
{
	HRESULT hr = S_OK;
	for(int i=0; i<nview; i++)
	{
		int view = (i<2&&m_swap_eyes)?1-i:i;

		IDirect3DSurface9 *p = surfaces[i];
		if (!p)
			continue;

		FAIL_RET(clear(p));
		FAIL_RET(draw_movie(p, view));
		FAIL_RET(draw_subtitle(p, view));
		FAIL_RET(draw_ui(p, i));
		FAIL_RET(adjust_temp_color(p, view));
	}

	return hr;
}

HRESULT my12doomRenderer::render_nolock(bool forced)
{
	HRESULT hr;
	// device state check and restore
// 	if (FAILED(handle_device_state()))
// 		return E_FAIL;

	//if (m_d3d_query) m_d3d_query->Issue(D3DISSUE_BEGIN);

	if (!(m_PC_level&PCLEVELTEST_TESTED))
		test_PC_level();

	// image loading and idle check
	m_Device->BeginScene();
	hr = load_image();
	m_Device->EndScene();

	if (FAILED(hr))
		return hr;

	if (hr != S_OK && !forced)	// no more rendering except pageflipping mode
		return S_FALSE;

	if (!m_Device)
		return E_FAIL;

	static int last_render_time = timeGetTime();
	static int last_nv3d_fix_time = timeGetTime();

//  	m_swapeyes = !m_swapeyes;
	static int lastkey = 0;
	if (GetKeyState(VK_CONTROL) < 0 && GetKeyState(VK_CONTROL) != lastkey)
	{
		lastkey = GetKeyState(VK_CONTROL);
// 		set_device_state(need_create);
	}

	int l = timeGetTime();


	// upload subtitle if updated.
	{
		CAutoLock lck(&m_subtitle_lock);
		if (m_subtitle_mem && m_subtitle)
		{
			int l2 = timeGetTime();
			RECT dirty = {0,0,min(m_subtitle_pixel_width+32, m_subtitle_mem->locked_rect.Pitch/4), min(m_subtitle_pixel_height+32, min((int)TEXTURE_SIZE, (int)SUBTITLE_TEXTURE_SIZE))};
			m_subtitle_mem->Unlock();
			int l3 = timeGetTime();
			//m_pool->UpdateTexture(m_subtitle_mem, m_subtitle, &dirty);
			CComPtr<IDirect3DSurface9> mem_surf;
			CComPtr<IDirect3DSurface9> def_surf;
			m_subtitle_mem->get_first_level(&mem_surf);
			m_subtitle->get_first_level(&def_surf);

			POINT p = {0,0};
			m_Device->UpdateSurface(mem_surf, &dirty, def_surf, &p);


			int l4 = timeGetTime();
			safe_delete(m_subtitle_mem);

			if (l4 - l2 >0)
				dwindow_log_line("upload subtitle cost %d-%d-%d, %dx%d", l4-l2, l3-l2, l4-l3, dirty.right, dirty.bottom);
		}
	}

	CAutoLock lck(&m_frame_lock);
	{
		MANAGE_DEVICE;
		// device state check again
		if (FAILED(handle_device_state()))
			return E_FAIL;

		// pass 2: drawing to back buffer!

		if (timeGetTime() - last_nv3d_fix_time > 1000)
		{
			fix_nv3d_bug();
			last_nv3d_fix_time = timeGetTime();
		}


		hr = m_Device->BeginScene();

		// prepare all samples in queue for rendering
		if(false)
		{
			bool dual_stream = m_dsr0->is_connected() && m_dsr1->is_connected();
			input_layout_types input = m_input_layout == input_layout_auto ? m_layout_detected : m_input_layout;
			bool need_detect = !dual_stream && m_input_layout == input_layout_auto && !m_no_more_detect;

			CAutoLock lck(&m_queue_lock);
			for(POSITION pos_left = m_left_queue.GetHeadPosition(); pos_left; pos_left = m_left_queue.Next(pos_left))
			{
				gpu_sample *left_sample = m_left_queue.Get(pos_left);
	// 			left_sample->convert_to_RGB32(m_Device, m_ps_yv12, m_ps_nv12, m_ps_yuy2, g_VertexBuffer, m_last_reset_time);
				if (need_detect) left_sample->do_stereo_test(m_Device, m_ps_test_sbs, m_ps_test_tb, NULL);
			}
			for(POSITION pos_right = m_right_queue.GetHeadPosition(); pos_right; pos_right = m_right_queue.Next(pos_right))
			{
				gpu_sample *right_sample = m_right_queue.Get(pos_right);
	// 			right_sample->convert_to_RGB32(m_Device, m_ps_yv12, m_ps_nv12, m_ps_yuy2, g_VertexBuffer, m_last_reset_time);
				if (need_detect) right_sample->do_stereo_test(m_Device, m_ps_test_sbs, m_ps_test_tb, NULL);
			}
		}

	#ifndef no_dual_projector
		if (timeGetTime() - m_last_reset_time > TRAIL_TIME_2 && is_trial_version())
			m_ps_yv12 = m_ps_nv12 = m_ps_yuy2 = NULL;
	#endif



		hr = m_Device->SetPixelShader(NULL);

		CComPtr<IDirect3DSurface9> back_buffer;
		if (m_swap1) hr = m_swap1->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &back_buffer);

		// most rendering mode is 2 views, so we create texture and render it (except for mono2d and pageflipping, which render only one view at a time)
		CPooledTexture *view0 = NULL, *view1 = NULL;
		CComPtr<IDirect3DSurface9> surf0;
		CComPtr<IDirect3DSurface9> surf1;
		IDirect3DSurface9 *surfaces[MAX_VIEWS] = {0};
		CComPtr<IDirect3DSurface9> back_buffer2;
		if (m_output_mode == dual_window && m_swap2)
		{
			m_swap2->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &back_buffer2);
			surfaces[0] = back_buffer;
			surfaces[1] = back_buffer2;
		}

		if (is2DMovie() && m_dsr0->is_connected())
		{
			FAIL_RET(m_pool->CreateTexture(m_active_pp.BackBufferWidth, m_active_pp.BackBufferHeight, D3DUSAGE_RENDERTARGET, m_active_pp.BackBufferFormat, D3DPOOL_DEFAULT, &view0));
			view0->get_first_level(&surf0);
			view1 = view0;
			surf1 = surf0;

			surfaces[0] = surf0;
			render_helper(surfaces, 1);
		}

		else if (m_output_mode != mono && m_output_mode != pageflipping)
		{
			FAIL_RET(m_pool->CreateTexture(m_active_pp.BackBufferWidth, m_active_pp.BackBufferHeight, D3DUSAGE_RENDERTARGET, m_active_pp.BackBufferFormat, D3DPOOL_DEFAULT, &view0));
			FAIL_RET(m_pool->CreateTexture(m_active_pp.BackBufferWidth, m_active_pp.BackBufferHeight, D3DUSAGE_RENDERTARGET, m_active_pp.BackBufferFormat, D3DPOOL_DEFAULT, &view1));

			view0->get_first_level(&surf0);
			view1->get_first_level(&surf1);

			surfaces[0] = surf0;
			surfaces[1] = surf1;
			render_helper(surfaces, 2);
		}

		clear(back_buffer, D3DCOLOR_ARGB(255, 0, 0, 0));

		// reset some render state
		hr = m_Device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		hr = m_Device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		hr = m_Device->SetSamplerState( 0, D3DSAMP_MIPFILTER, D3DTEXF_NONE );

		// vertex
		MyVertex whole_backbuffer_vertex[4];
		whole_backbuffer_vertex[0].x = -0.5f; whole_backbuffer_vertex[0].y = -0.5f;
		whole_backbuffer_vertex[1].x = m_active_pp.BackBufferWidth-0.5f; whole_backbuffer_vertex[1].y = -0.5f;
		whole_backbuffer_vertex[2].x = -0.5f; whole_backbuffer_vertex[2].y = m_active_pp.BackBufferHeight-0.5f;
		whole_backbuffer_vertex[3].x = m_active_pp.BackBufferWidth-0.5f; whole_backbuffer_vertex[3].y = m_active_pp.BackBufferHeight-0.5f;
		for(int i=0; i<4; i++)
			whole_backbuffer_vertex[i].z = whole_backbuffer_vertex[i].w = 1;
		whole_backbuffer_vertex[0].tu = 0;
		whole_backbuffer_vertex[0].tv = 0;
		whole_backbuffer_vertex[1].tu = 1;
		whole_backbuffer_vertex[1].tv = 0;
		whole_backbuffer_vertex[2].tu = 0;
		whole_backbuffer_vertex[2].tv = 1;
		whole_backbuffer_vertex[3].tu = 1;
		whole_backbuffer_vertex[3].tv = 1;

		static NvU32 l_counter = 0;
		if (m_output_mode == intel3d)
		{
			intel_render_frame(surf0, surf1);
		}
		else if (m_output_mode == NV3D
	#ifdef explicit_nv3d
			&& m_nv3d_enabled && m_nv3d_actived /*&& !m_active_pp.Windowed*/
	#endif
			)
		{
			// copy left to nv3d surface
			RECT dst = {0,0, m_active_pp.BackBufferWidth, m_active_pp.BackBufferHeight};
			hr = m_Device->StretchRect(surf0, NULL, m_nv3d_surface, &dst, D3DTEXF_NONE);

			dst.left += m_active_pp.BackBufferWidth;
			dst.right += m_active_pp.BackBufferWidth;
			hr = m_Device->StretchRect(surf1, NULL, m_nv3d_surface, &dst, D3DTEXF_NONE);

			// StretchRect to backbuffer!, this is how 3D vision works
			RECT tar = {0,0, m_active_pp.BackBufferWidth*2, m_active_pp.BackBufferHeight};
			hr = m_Device->StretchRect(m_nv3d_surface, &tar, back_buffer, NULL, D3DTEXF_NONE);		//source is as previous, tag line not overwrited
		}

		else if (m_output_mode == multiview)
		{
			CPooledTexture *view2, *view3;
			CComPtr<IDirect3DSurface9> surf2;
			CComPtr<IDirect3DSurface9> surf3;
			
			FAIL_RET(m_pool->CreateTexture(m_active_pp.BackBufferWidth, m_active_pp.BackBufferHeight, D3DUSAGE_RENDERTARGET, m_active_pp.BackBufferFormat, D3DPOOL_DEFAULT, &view2));
			FAIL_RET(m_pool->CreateTexture(m_active_pp.BackBufferWidth, m_active_pp.BackBufferHeight, D3DUSAGE_RENDERTARGET, m_active_pp.BackBufferFormat, D3DPOOL_DEFAULT, &view3));

			view2->get_first_level(&surf2);
			view3->get_first_level(&surf3);

			surfaces[0] = NULL;
			surfaces[1] = NULL;
			surfaces[2] = surf2;
			surfaces[3] = surf3;

			render_helper(surfaces, 4);

			// pass3: multiview interlacing
			m_Device->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
			m_Device->SetRenderTarget(0, back_buffer);
			m_Device->SetTexture( 0, m_tex_mask );
			m_Device->SetTexture( (4+m_mask_parameter)%4+1, view0->texture );
			m_Device->SetTexture( (3+m_mask_parameter)%4+1, view1->texture );
			m_Device->SetTexture( (2+m_mask_parameter)%4+1, view2->texture );
			m_Device->SetTexture( (1+m_mask_parameter)%4+1, view3->texture );
			m_Device->SetPixelShader(m_multiview4);

			hr = m_Device->SetFVF( FVF_Flags );
			hr = m_Device->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, whole_backbuffer_vertex, sizeof(MyVertex) );

			draw_ui(back_buffer, 0);

			safe_delete(view2);
			safe_delete(view3);
		}

		else if (m_output_mode == mono 
	#ifdef explicit_nv3d
			|| (m_output_mode == NV3D && !(m_nv3d_enabled && m_nv3d_actived))
	#endif
			)
		{
			surfaces[0] = back_buffer;
			surfaces[1] = NULL;
			render_helper(surfaces, 1);
		}
		else if (m_output_mode == hd3d)
		{
			HD3DDrawStereo(surf0, surf1, back_buffer);
		}
		else if (m_output_mode == anaglyph)
		{
			m_Device->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
			m_Device->SetRenderTarget(0, back_buffer);
			clear(back_buffer);
			hr = m_Device->SetTexture( 0, view0->texture );
			hr = m_Device->SetTexture( 1, view1->texture );
			m_Device->SetPixelShader(is2DMovie() ? NULL : (IDirect3DPixelShader9*)m_red_blue);

			hr = m_Device->SetFVF( FVF_Flags );
			hr = m_Device->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, whole_backbuffer_vertex, sizeof(MyVertex) );
		}

		else if (m_output_mode == masking)
		{
			m_Device->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
			m_Device->SetRenderTarget(0, back_buffer);
			m_Device->SetPixelShader(m_ps_masking);
			m_Device->SetTexture( 0, m_tex_mask );

			// swap on need
			bool need_swap = false;
			int x = m_window_rect.left&1;
			int y = m_window_rect.top&1;
			if ( ((m_mask_mode == row_interlace || m_mask_mode == subpixel_row_interlace) && x) ||
				(m_mask_mode == line_interlace && y) ||
				(m_mask_mode == checkboard_interlace && ((x+y)%2)))
			{
				need_swap = true;
			}

			m_Device->SetTexture( 1, need_swap ? view1->texture : view0->texture );
			m_Device->SetTexture( 2, need_swap ? view0->texture : view1->texture );
			hr = m_Device->SetFVF( FVF_Flags );
			hr = m_Device->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, whole_backbuffer_vertex, sizeof(MyVertex) );
		}

		else if (m_output_mode == pageflipping)
		{
			int frame_passed = 1;
			m_pageflip_frames = max(0, m_pageflip_frames);

			if (!(BOOL)SimplePageflipping)
			{
				D3DPRESENTSTATS state;

				if (m_swap1ex && SUCCEEDED(hr = m_swap1ex->GetPresentStats(&state)) && state.PresentRefreshCount > 0)
				{
					frame_passed = state.PresentRefreshCount - m_pageflip_frames;
					m_pageflip_frames = state.PresentRefreshCount;

					printf("%d,%d,%d\n", state.PresentCount, state.PresentRefreshCount, state.SyncRefreshCount);
				}
				else
				{
					double delta = (double)timeGetTime()-m_pageflipping_start;
					if (delta/(1500/m_d3ddm.RefreshRate) > 1)
						frame_passed =floor(delta/(1000/m_d3ddm.RefreshRate));

					frame_passed = max(1, frame_passed);
					m_pageflip_frames += frame_passed;
				}
			}
			else
			{
				LARGE_INTEGER cur;
				LARGE_INTEGER fre;
				QueryPerformanceFrequency(&fre);
				QueryPerformanceCounter(&cur);
 				float frame_counter_float = (double)(cur.QuadPart-m_first_pageflip.QuadPart) * m_d3ddm.RefreshRate / fre.QuadPart;
 				printf("counter: %f\n", frame_counter_float);
// 				m_pageflip_frames = frame_counter_float;
				m_pageflip_frames += frame_passed;
			}

			if (frame_passed%2 >= 0)
			{
	// 			if (frame_passed>1 || frame_passed <= 0)
	// 			{
	// 				if (m_nv3d_display && frame_passed > 2)
	// 				{
	// 					DWORD counter;
	// 					NvAPI_GetVBlankCounter(m_nv3d_display, &counter);
	// 					m_pageflip_frames = counter - m_nv_pageflip_counter;
	// 				}
	// 				mylog("delta=%d.\n", (int)delta);
	// 			}

				int swap_offset = m_swap_eyes ? 1 : 0;
				int view = (m_pageflip_frames+swap_offset)%2;

				LARGE_INTEGER l1, l2, l3, l4, l5;
				QueryPerformanceCounter(&l1);
				clear(back_buffer);
				QueryPerformanceCounter(&l2);
				draw_movie(back_buffer, view);
				QueryPerformanceCounter(&l3);
				draw_subtitle(back_buffer, view);
				QueryPerformanceCounter(&l4);
				draw_ui(back_buffer, view);
				QueryPerformanceCounter(&l5);
				adjust_temp_color(back_buffer, view);
			}

			else
			{
				// skip this frame
			}

			//mylog("clear, draw_movie, draw_bmp, draw_ui = %d, %d, %d, %d.\n", (int)(l2.QuadPart-l1.QuadPart), 
			//	(int)(l3.QuadPart-l2.QuadPart), (int)(l4.QuadPart-l3.QuadPart), (int)(l5.QuadPart-l4.QuadPart) );
		}

	#ifndef no_dual_projector	
		else if (m_output_mode == iz3d)
		{
			// pass3: IZ3D
			m_Device->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
			m_Device->SetRenderTarget(0, back_buffer);
			m_Device->SetTexture( 0, view0->texture );
			m_Device->SetTexture( 1, view1->texture );
			m_Device->SetPixelShader(m_ps_iz3d_back);

			hr = m_Device->SetFVF( FVF_Flags );
			hr = m_Device->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, whole_backbuffer_vertex, sizeof(MyVertex) );

			// set render target to swap chain2
			if (m_swap2)
			{
				CComPtr<IDirect3DSurface9> back_buffer2;
				m_swap2->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &back_buffer2);

				clear(back_buffer2);

				hr = m_Device->SetRenderTarget(0, back_buffer2);
				m_Device->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
				m_Device->SetPixelShader(m_ps_iz3d_front);
				hr = m_Device->SetFVF( FVF_Flags );
				hr = m_Device->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, whole_backbuffer_vertex, sizeof(MyVertex) );
			}

			// UI
			m_Device->SetPixelShader(NULL);
			draw_ui(back_buffer, 0);
		}
		else if (m_output_mode == dual_window)
		{
			int view = m_swap_eyes ? 1 : 0;

			clear(back_buffer);
			draw_movie(back_buffer, view);
			draw_subtitle(back_buffer, view);
			draw_ui(back_buffer, view);
			adjust_temp_color(back_buffer, view);

			// set render target to swap chain2
			if (m_swap2)
			{
				view = 1 - view;

				CComPtr<IDirect3DSurface9> back_buffer2;
				m_swap2->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &back_buffer2);
				hr = m_Device->SetRenderTarget(0, back_buffer2);

				clear(back_buffer2);
				draw_movie(back_buffer2, view);
				draw_subtitle(back_buffer2, view);
				draw_ui(back_buffer2, view);
				adjust_temp_color(back_buffer2, view);
			}

		}

	#endif
		else if (m_output_mode == out_sbs || m_output_mode == out_tb || m_output_mode == out_hsbs || m_output_mode == out_htb)
		{
			// pass 3: copy to backbuffer
			if(false)
			{

			}
	#ifndef no_dual_projector
			else if (m_output_mode == out_sbs)
			{
				RECT src = {0, 0, m_active_pp.BackBufferWidth/2, m_active_pp.BackBufferHeight};
				RECT dst = src;

				m_Device->StretchRect(surf0, &src, back_buffer, &dst, D3DTEXF_NONE);

				dst.left += m_active_pp.BackBufferWidth/2;
				dst.right += m_active_pp.BackBufferWidth/2;
				m_Device->StretchRect(surf1, &src, back_buffer, &dst, D3DTEXF_NONE);
			}

			else if (m_output_mode == out_tb)
			{
				RECT src = {0, 0, m_active_pp.BackBufferWidth, m_active_pp.BackBufferHeight/2};
				RECT dst = src;

				m_Device->StretchRect(surf0, &src, back_buffer, &dst, D3DTEXF_NONE);

				dst.top += m_active_pp.BackBufferHeight/2;
				dst.bottom += m_active_pp.BackBufferHeight/2;
				m_Device->StretchRect(surf1, &src, back_buffer, &dst, D3DTEXF_NONE);

			}
	#endif

			else if (m_output_mode == out_hsbs)
			{
				RECT dst = {0, 0, m_active_pp.BackBufferWidth/2, m_active_pp.BackBufferHeight};

				m_Device->StretchRect(surf0, NULL, back_buffer, &dst, D3DTEXF_LINEAR);

				dst.left += m_active_pp.BackBufferWidth/2;
				dst.right += m_active_pp.BackBufferWidth/2;
				m_Device->StretchRect(surf1, NULL, back_buffer, &dst, D3DTEXF_LINEAR);
			}

			else if (m_output_mode == out_htb)
			{
				RECT dst = {0, 0, m_active_pp.BackBufferWidth, m_active_pp.BackBufferHeight/2};

				m_Device->StretchRect(surf0, NULL, back_buffer, &dst, D3DTEXF_LINEAR);

				dst.top += m_active_pp.BackBufferHeight/2;
				dst.bottom += m_active_pp.BackBufferHeight/2;
				m_Device->StretchRect(surf1, NULL, back_buffer, &dst, D3DTEXF_LINEAR);
			}
		}

		m_Device->EndScene();
		if (view0 == view1)
			view1 = NULL;
		safe_delete(view0);
		safe_delete(view1);
	}



// 	if (timeGetTime() - l > 5)
// 		printf("All Draw Calls = %dms\n", timeGetTime() - l);
presant:
	{
		MANAGE_DEVICE;
		static int n = timeGetTime();
		//if (timeGetTime() - n > 43)
		//	printf("(%d):presant delta: %dms\n", timeGetTime(), timeGetTime()-n);
		n = timeGetTime();


		// presenting...
		if (m_output_mode == intel3d && m_overlay_swap_chain)
		{
			RECT rect= {0, 0, min(m_intel_active_3d_mode.ulResWidth,m_active_pp.BackBufferWidth), 
				min(m_active_pp.BackBufferHeight, m_active_pp.BackBufferHeight)};
			hr = m_overlay_swap_chain->Present(&rect, &rect, NULL, NULL, D3DPRESENT_UPDATECOLORKEY);

			//mylog("%08x\n", hr);
		}



		else if (m_output_mode == dual_window || m_output_mode == iz3d)
		{
			if(m_swap1) hr = m_swap1->Present(NULL, NULL, m_hWnd, NULL, D3DPRESENT_DONOTWAIT);
// 			if (FAILED(hr) && hr != DDERR_WASSTILLDRAWING)
// 				set_device_state(device_lost);

			if(m_swap2) if (m_swap2->Present(NULL, NULL, m_hWnd2, NULL, NULL) == D3DERR_DEVICELOST)
				set_device_state(device_lost);
		}

		else
		{
			if (m_output_mode == pageflipping)
			{
				if (m_pageflipping_start == -1 && m_nv3d_display)
					NvAPI_GetVBlankCounter(m_nv3d_display, &m_nv_pageflip_counter);
				m_pageflipping_start = timeGetTime();
				if (m_first_pageflip.QuadPart < 0)
					QueryPerformanceCounter(&m_first_pageflip);
			}

			int l2 = timeGetTime();
			hr = DDERR_WASSTILLDRAWING;
			while (hr == DDERR_WASSTILLDRAWING)
				hr = m_swap1->Present(NULL, NULL, m_hWnd, NULL, NULL);
		// 		if (timeGetTime()-l2 > 9) mylog("Presant() cost %dms.\n", timeGetTime() - l2);
// 			if (FAILED(hr))
// 				set_device_state(device_lost);

			static int n = timeGetTime();
			//if (timeGetTime()-n > 0)printf("delta = %d.\n", timeGetTime()-n);
			n = timeGetTime();
		}

		// debug LockRect times
		//if (lockrect_surface + lockrect_texture)
		//	printf("LockRect: surface, texture, total, cycle = %d, %d, %d, %d.\n", lockrect_surface, lockrect_texture, lockrect_surface+lockrect_texture, (int)lockrect_texture_cycle);
		lockrect_texture_cycle = lockrect_surface = lockrect_texture = 0;
	}

	return S_OK;
}

HRESULT my12doomRenderer::clear(IDirect3DSurface9 *surface, DWORD color)
{
	if (!surface)
		return E_POINTER;

	m_Device->SetRenderTarget(0, surface);
	return m_Device->Clear( 0L, NULL, D3DCLEAR_TARGET, color, 1.0f, 0L );
}

HRESULT my12doomRenderer::draw_movie(IDirect3DSurface9 *surface, int view)
{
	HRESULT hr;
	if (!surface)
		return E_POINTER;

	{
		CAutoLock lck2(&m_uidrawer_cs);
		hr = m_uidrawer == NULL ? S_FALSE : m_uidrawer->pre_render_movie(surface, view);
	}


	view = m_force2d ? 0 : view;

	CComPtr<IDirect3DSurface9> src;

	bool dual_stream = (m_dsr0->is_connected() && m_dsr1->is_connected()) || m_remux_mode || (int)m_input_layout == frame_sequence;
	CAutoLock rendered_lock(&m_rendered_packet_lock);
	gpu_sample *sample = dual_stream ? (view == 0 ? m_last_rendered_sample1 : m_last_rendered_sample2) : m_last_rendered_sample1;

	if (!sample)
		return S_FALSE;

	FAIL_RET(sample->commit());

	if (sample->m_tex_gpu_RGB32)
		sample->m_tex_gpu_RGB32->get_first_level(&src);

	if (!src)
		return S_FALSE;

	// movie picture position
	RECTF target = {0,0, m_active_pp.BackBufferWidth, m_active_pp.BackBufferHeight};
	calculate_movie_position(&target);

	// source rect calculation
	RECTF src_rect = {0,0,m_lVidWidth, m_lVidHeight};
	if (!dual_stream)
	{
		input_layout_types layout = get_active_input_layout();

		switch(layout)
		{
		case side_by_side:
			if (view == 0)
				src_rect.right/=2;
			else
				src_rect.left = m_lVidWidth/2;
			break;
		case top_bottom:
			if (view == 0)
				src_rect.bottom/=2;
			else
				src_rect.top = m_lVidHeight/2;
			break;
		case mono2d:
			break;
		default:
			break;
			assert(0);
		}

	}

	// deinterlace if needed ( Single field )
	if (sample->m_interlace_flags != 0)
		src_rect.bottom -= (src_rect.bottom-src_rect.top)/2;

	if (m_output_mode == multiview)
	{
		// 4 view
		RECT view1 = {0, 0, m_lVidWidth/2, m_lVidHeight/2};
		RECT view2 = {m_lVidWidth/2, 0, m_lVidWidth, m_lVidHeight/2};
		RECT view3 = {0, m_lVidHeight/2, m_lVidWidth/2, m_lVidHeight};
		RECT view4 = {m_lVidWidth/2, m_lVidHeight/2, m_lVidWidth, m_lVidHeight};

		RECT views[4] = {view1, view2, view3, view4};

		if (view <0 || view >= 4)
			return E_NOTIMPL;

		src_rect = views[view];

		// 9view
		if (view <0 || view >= 9)
			return E_NOTIMPL;
		int x = view%3;
		int y = view/3;
		RECT r = {m_lVidWidth*x/3, m_lVidHeight*y/3, m_lVidWidth*(x+1)/3, m_lVidHeight*(y+1)/3};

		src_rect = r;
		
	}


	// parallax adjustments
	if (m_parallax > 0)
	{
		// cut right edge of right eye and left edge of left eye
		if (view == 0)
			src_rect.left += abs(m_parallax) * m_lVidWidth;
		else
			src_rect.right -= abs(m_parallax) * m_lVidWidth;

	}
	else if (m_parallax < 0)
	{
		// cut left edge of right eye and right edge of left eye
		if (view == 0)
			src_rect.right -= abs(m_parallax) * m_lVidWidth;
		else
			src_rect.left += abs(m_parallax) * m_lVidWidth;
	}


	// render
	m_Device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	RECT scissor = get_movie_scissor_rect();
	m_Device->SetScissorRect(&scissor);

	sample->set_interlace(m_forced_deinterlace);

	m_Device->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
	hr = resize_surface(NULL, sample, surface, &src_rect, &target, (resampling_method)(int)m_movie_resizing);
	m_Device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

	return hr;
}

HRESULT my12doomRenderer::draw_subtitle(IDirect3DSurface9 *surface, int view)
{
	if (m_display_orientation != horizontal)
		return E_NOTIMPL;

	if (!surface)
		return E_POINTER;

	if (!m_has_subtitle)
		return S_FALSE;

	// assume draw_movie() handles pointer and not connected issues, so no more check
	HRESULT hr = E_FAIL;

	// movie picture position
	RECTF src_rect = {0,0,m_subtitle_pixel_width, m_subtitle_pixel_height};
	RECTF dst_rect = {0};
	calculate_subtitle_position(&dst_rect, view);

	CAutoLock lck2(&m_subtitle_lock);

	{
		CAutoLock lck(&m_pool_lock);
		if (!m_subtitle && FAILED( hr = m_pool->CreateTexture(min((int)TEXTURE_SIZE, (int)SUBTITLE_TEXTURE_SIZE), min((int)TEXTURE_SIZE, (int)SUBTITLE_TEXTURE_SIZE), D3DUSAGE_RENDERTARGET | D3DUSAGE_AUTOGENMIPMAP, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_subtitle)))
			return hr;
	}

	CComPtr<IDirect3DSurface9> src;
	m_subtitle->get_first_level(&src);

	m_Device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	m_Device->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	m_Device->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	RECT scissor = get_movie_scissor_rect();
	m_Device->SetScissorRect(&scissor);
	m_Device->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
	hr = resize_surface(src, NULL, surface, &src_rect, &dst_rect, (resampling_method)(int)m_subtitle_resizing);
	m_Device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

	return hr;
}
HRESULT my12doomRenderer::draw_ui(IDirect3DSurface9 *surface, int view)
{
	if (!surface)
		return E_POINTER;

	CAutoLock lck2(&m_uidrawer_cs);
	return m_uidrawer == NULL ? S_FALSE : m_uidrawer->draw_ui(surface, view);
}

HRESULT my12doomRenderer::loadBitmap(gpu_sample **out, wchar_t *file)
{
	if (!out)
		return E_POINTER;

	CAutoLock lck(&m_pool_lock);
	gpu_sample *sample = new gpu_sample(file, m_pool);
	if (!sample)
		return E_OUTOFMEMORY;
	if (!sample->m_ready)
	{
		delete sample;
		return E_FAIL;
	}

	*out = sample;

	return S_OK;
}

HRESULT my12doomRenderer::drawFont(gpu_sample **out, HFONT font, wchar_t *text, RGBQUAD color, RECT *dst_rect /* = NULL */, DWORD flag /* = DT_CENTER | DT_WORDBREAK | DT_NOFULLWIDTHCHARBREAK | DT_EDITCONTROL */)
{
	if (!out)
		return E_POINTER;

	CAutoLock lck(&m_pool_lock);
	gpu_sample *sample = new gpu_sample(m_pool, font, text, color, dst_rect, flag);
	if (!sample)
		return E_OUTOFMEMORY;
	if (!sample->m_ready)
	{
		delete sample;
		return E_FAIL;
	}

	*out = sample;

	return S_OK;
}


HRESULT my12doomRenderer::Draw(IDirect3DSurface9 *rt, gpu_sample *resource, RECTF *src, RECTF *dst, float alpha)
{
	m_Device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	return resize_surface(NULL, resource, rt, src, dst, bilinear_no_mipmap, alpha);
}

extern double UIScale;
HRESULT my12doomRenderer::paint(RECTF *dst_rect, resource_userdata *resource, RECTF*src_rect/* = NULL*/, float alpha/* = 1.0f*/, resampling_method method /* =bilinear_no_mipmap */)
{
	CComPtr<IDirect3DSurface9> rt;
	m_Device->GetRenderTarget(0, &rt);
	for(int i=0; i<4; i++)
		((float*)dst_rect)[i] = ((float*)dst_rect)[i] * UIScale;


	if (resource && resource->resource_type == resource_userdata::RESOURCE_TYPE_GPU_SAMPLE)
	{
		gpu_sample * sample = (gpu_sample*)resource->pointer;
		if (sample != NULL )
		{
			sample->commit();
			m_Device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			m_Device->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
			resize_surface(NULL, sample, rt, src_rect, dst_rect, method, alpha );
			m_Device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
		}
	}

	return S_OK;
}

HRESULT my12doomRenderer::set_clip_rect(int left, int top, int right, int bottom)
{
	RECT rect = {left*UIScale, top*UIScale, right*UIScale, bottom*UIScale};
	m_Device->SetScissorRect(&rect);

	return S_OK;
}

HRESULT my12doomRenderer::get_resource(int arg, resource_userdata *resource)
{
	resource->resource_type = resource_userdata::RESOURCE_TYPE_GPU_SAMPLE;
	resource->pointer = m_last_rendered_sample1;
	resource->managed = true;

	return S_OK;
}


HRESULT my12doomRenderer::adjust_temp_color(IDirect3DSurface9 *surface_to_adjust, int view)
{
	if (!surface_to_adjust)
		return E_FAIL;

	// check for size
	D3DSURFACE_DESC desc;
	memset(&desc, 0, sizeof(desc));
	surface_to_adjust->GetDesc(&desc);

	// create a temp texture, copy the surface to it, render to it, and then copy back again
	HRESULT hr = S_OK;
	bool left = view == 0;
	double saturation1 = m_saturation;
	double saturation2 = m_saturation2;

#ifndef no_dual_projector
	if (timeGetTime() - m_last_reset_time > TRAIL_TIME_1 && is_trial_version())
	{
		saturation1 -= (double)(timeGetTime() - m_last_reset_time - TRAIL_TIME_1) / TRAIL_TIME_3;
		saturation2 -= (double)(timeGetTime() - m_last_reset_time - TRAIL_TIME_1) / TRAIL_TIME_3;

		saturation1 = max(saturation1, -1);
		saturation2 = max(saturation2, -2);
	}
#endif
	if ( (left && (abs((double)saturation1-0.5)>0.005 || abs((double)m_luminance-0.5)>0.005 || abs((double)m_hue-0.5)>0.005 || abs((double)m_contrast-0.5)>0.005)) || 
		(!left && (abs((double)saturation2-0.5)>0.005 || abs((double)m_luminance2-0.5)>0.005 || abs((double)m_hue2-0.5)>0.005 || abs((double)m_contrast2-0.5)>0.005)))
	{
		// creating
		CAutoLock lck(&m_pool_lock);
		CPooledTexture *tex_src;
		CPooledTexture *tex_rt;
		hr = m_pool->CreateTexture(desc.Width, desc.Height, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tex_src);
		if (FAILED(hr))
			return hr;
		hr = m_pool->CreateTexture(desc.Width, desc.Height, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tex_rt);
		if (FAILED(hr))
			return hr;
		CComPtr<IDirect3DSurface9> surface_of_tex_src;
		tex_src->texture->GetSurfaceLevel(0, &surface_of_tex_src);
		CComPtr<IDirect3DSurface9> surface_of_tex_rt;
		tex_rt->texture->GetSurfaceLevel(0, &surface_of_tex_rt);

		// copying
		hr = m_Device->StretchRect(surface_to_adjust, NULL, surface_of_tex_src, NULL, D3DTEXF_LINEAR);		//we are using linear filter here, to cover possible coordinate error		

		// vertex
		MyVertex whole_backbuffer_vertex[4];
		for(int i=0;i <4; i++)
		{
			whole_backbuffer_vertex[i].z = 1.0f;
			whole_backbuffer_vertex[i].w = 1.0f;
		}

		whole_backbuffer_vertex[0].x = -0.5f; whole_backbuffer_vertex[0].y = -0.5f;
		whole_backbuffer_vertex[1].x = m_active_pp.BackBufferWidth-0.5f; whole_backbuffer_vertex[1].y = -0.5f;
		whole_backbuffer_vertex[2].x = -0.5f; whole_backbuffer_vertex[2].y = m_active_pp.BackBufferHeight-0.5f;
		whole_backbuffer_vertex[3].x = m_active_pp.BackBufferWidth-0.5f; whole_backbuffer_vertex[3].y = m_active_pp.BackBufferHeight-0.5f;
		whole_backbuffer_vertex[0].tu = 0; whole_backbuffer_vertex[0].tv = 0;
		whole_backbuffer_vertex[1].tu = 1; whole_backbuffer_vertex[1].tv = 0;
		whole_backbuffer_vertex[2].tu = 0; whole_backbuffer_vertex[2].tv = 1;
		whole_backbuffer_vertex[3].tu = 1; whole_backbuffer_vertex[3].tv = 1;

		// rendering
		CComPtr<IDirect3DPixelShader9> ps;
		hr = m_Device->GetPixelShader(&ps);
		hr = m_Device->SetRenderTarget(0, surface_of_tex_rt);
		hr = m_Device->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
		hr = m_Device->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
		hr = m_Device->SetTexture( 0, tex_src->texture );
		hr = m_Device->SetPixelShader(m_ps_color_adjust);
		float ps_parameter[4] = {(double)saturation1, (double)m_luminance, (double)m_hue, (double)m_contrast};
		float ps_parameter2[4] = {(double)saturation2, (double)m_luminance2, (double)m_hue2, (double)m_contrast2};
		hr = m_Device->SetPixelShaderConstantF(0, left?ps_parameter:ps_parameter2, 1);

		hr = m_Device->SetFVF( FVF_Flags );
		hr = m_Device->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, whole_backbuffer_vertex, sizeof(MyVertex) );
		hr = m_Device->SetPixelShader(ps);

		// copying back
		hr = m_Device->StretchRect(surface_of_tex_rt, NULL, surface_to_adjust, NULL, D3DTEXF_LINEAR);

		delete tex_src;
		delete tex_rt;
	}

	return hr;
}

HRESULT my12doomRenderer::resize_surface(IDirect3DSurface9 *src, gpu_sample *src2, IDirect3DSurface9 *dst, RECT *src_rect /* = NULL */, RECT *dst_rect /* = NULL */, resampling_method method /* = bilinear_mipmap_minus_one */, float alpha /*= 1.0f*/)
{
	if ((!src && !src2) || !dst)
		return E_POINTER;

	// RECT caculate;
	D3DSURFACE_DESC desc;
	dst->GetDesc(&desc);
	RECTF d = {0,0,(float)desc.Width, (float)desc.Height};
	if (dst_rect)
	{
		d.left = (float)dst_rect->left;
		d.right = (float)dst_rect->right;
		d.top = (float)dst_rect->top;
		d.bottom = (float)dst_rect->bottom;
	}

	RECTF s = {0};
	if (src_rect)
	{
		s.left = (float)src_rect->left;
		s.right = (float)src_rect->right;
		s.top = (float)src_rect->top;
		s.bottom = (float)src_rect->bottom;
	}
	else if (src)
	{
		src->GetDesc(&desc);
		s.right = desc.Width;
		s.bottom = desc.Height;
	}
	else if (src2)
	{
		s.right = src2->m_width;
		s.bottom = src2->m_height;
	}

	return resize_surface(src, src2, dst, &s, &d, method, alpha);
}


HRESULT my12doomRenderer::resize_surface(IDirect3DSurface9 *src, gpu_sample *src2, IDirect3DSurface9 *dst, RECTF *src_rect /* = NULL */, RECTF *dst_rect /* = NULL */, resampling_method method /* = bilinear_mipmap_minus_one */, float alpha /*= 1.0f*/)
{
	if ((!src && !src2) || !dst)
		return E_POINTER;

	CComPtr<IDirect3DTexture9> tex;
	if (src)
	{
		src->GetContainer(IID_IDirect3DTexture9, (void**)&tex);
		if (!tex)
			return E_INVALIDARG;
	}

	// RECT caculate;
	RECT clip;
	m_Device->GetScissorRect(&clip);
	D3DSURFACE_DESC desc;
	dst->GetDesc(&desc);
	RECTF d = {0,0,(float)desc.Width, (float)desc.Height};
	if (dst_rect)
		d = *dst_rect;

	RECTF s = {0};
	if (src_rect)
	{
		s.left = (float)src_rect->left;
		s.right = (float)src_rect->right;
		s.top = (float)src_rect->top;
		s.bottom = (float)src_rect->bottom;
	}
	else if (src)
	{
		src->GetDesc(&desc);
		s.right = desc.Width;
		s.bottom = desc.Height;
	}
	else if (src2)
	{
		s.right = src2->m_width;
		s.bottom = src2->m_height;
	}

	if (src)
	{
		src->GetDesc(&desc);
	}

	else if (src2)
	{
		desc.Width = src2->m_width;
		desc.Height = src2->m_height;
	}

	// clipping
	D3DSURFACE_DESC dst_desc;
	dst->GetDesc(&dst_desc);
	RECTF screen = {0, 0, dst_desc.Width, dst_desc.Height};
	RECTF clipped = ClipRect(screen, d);
	float fx = (s.right - s.left) / (d.right - d.left);
	float fy = (s.bottom - s.top) / (d.bottom - d.top);
	s.left += (clipped.left - d.left) * fx;
	s.right += (clipped.right - d.right) * fx;
	s.top += (clipped.top - d.top) * fy;
	s.bottom += (clipped.bottom - d.bottom) * fy;
	d = clipped;

	// basic vertex calculation
	HRESULT hr = E_FAIL;
	float width_s = s.right - s.left;
	float height_s = s.bottom - s.top;
	float width_d = d.right - d.left;
	float height_d = d.bottom - d.top;

	MyVertex direct_vertex[4];
	direct_vertex[0].x = (float)d.left;
	direct_vertex[0].y = (float)d.top;
	direct_vertex[1].x = (float)d.right;
	direct_vertex[1].y = (float)d.top;
	direct_vertex[2].x = (float)d.left;
	direct_vertex[2].y = (float)d.bottom;
	direct_vertex[3].x = (float)d.right;
	direct_vertex[3].y = (float)d.bottom;
	for(int i=0;i <4; i++)
	{
		direct_vertex[i].x -= 0.5f;
		direct_vertex[i].y -= 0.5f;
		direct_vertex[i].z = 1.0f;
		direct_vertex[i].w = 1.0f;
	}

	direct_vertex[0].tu = (float)s.left / desc.Width;
	direct_vertex[0].tv = (float)s.top / desc.Height;
	direct_vertex[1].tu = (float)s.right / desc.Width;
	direct_vertex[1].tv = (float)s.top / desc.Height;
	direct_vertex[2].tu = (float)s.left / desc.Width;
	direct_vertex[2].tv = (float)s.bottom / desc.Height;
	direct_vertex[3].tu = (float)s.right / desc.Width;
	direct_vertex[3].tv = (float)s.bottom / desc.Height;

	// render state
	BOOL alpha_blend = TRUE;
	m_Device->GetRenderState(D3DRS_ALPHABLENDENABLE, (DWORD*)&alpha_blend);
	m_Device->SetPixelShader(NULL);
	m_Device->SetRenderState( D3DRS_ALPHABLENDENABLE, alpha_blend );

	// texture and shader
	hr = m_Device->SetTexture( 0, tex );
	IDirect3DPixelShader9 *shader_yuv = NULL;
	GUID format = MEDIASUBTYPE_None;
	if (src2)
	{
		format = src2->m_format;
		if (format == MEDIASUBTYPE_NV12)
		{
			m_Device->SetTexture(0, helper_get_texture(src2, helper_sample_format_y));
			m_Device->SetTexture(1, helper_get_texture(src2, helper_sample_format_nv12));
			shader_yuv = m_ps_nv12;
		}

		else if (format == MEDIASUBTYPE_P010 || format == MEDIASUBTYPE_P016)
		{
			m_Device->SetTexture(0, helper_get_texture(src2, helper_sample_format_y));
			m_Device->SetTexture(1, helper_get_texture(src2, helper_sample_format_nv12));
			shader_yuv = m_ps_P016;
		}


		if (format == MEDIASUBTYPE_YUY2)
		{
			m_Device->SetTexture(0, helper_get_texture(src2, helper_sample_format_y));
			m_Device->SetTexture(1, helper_get_texture(src2, helper_sample_format_yuy2));
			shader_yuv = m_ps_nv12;
		}

		if (format == MEDIASUBTYPE_YV12)
		{
			m_Device->SetTexture(0, helper_get_texture(src2, helper_sample_format_y));
			m_Device->SetTexture(1, helper_get_texture(src2, helper_sample_format_yv12));
			shader_yuv = m_ps_yv12;
		}

		if (format == MEDIASUBTYPE_RGB32 || format == MEDIASUBTYPE_ARGB32)
		{
			m_Device->SetTexture(0, helper_get_texture(src2, helper_sample_format_rgb32));
			m_Device->SetTexture(1, NULL);
			shader_yuv = m_alpha_multiply;
		}
	}
	hr = m_Device->SetPixelShader(shader_yuv);

	if (method == lanczos)
	{
		CAutoLock lck(&m_pool_lock);
		CPooledTexture *tmp1 = NULL;
		hr = m_pool->CreateTexture(TEXTURE_SIZE, TEXTURE_SIZE, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tmp1);
		if (FAILED(hr))
		{
			safe_delete(tmp1);
			return hr;
		}

		// pass1, X filter
		CComPtr<IDirect3DSurface9> rt1;
		tmp1->get_first_level(&rt1);
		m_Device->SetRenderTarget(0, rt1);
		clear(rt1, D3DCOLOR_ARGB(0,0,0,0));
		MyVertex vertex[4];	
		vertex[0].x = (float)0;
		vertex[0].y = (float)0;
		vertex[1].x = (float)width_d;
		vertex[1].y = (float)0;
		vertex[2].x = (float)0;
		vertex[2].y = (float)height_s;
		vertex[3].x = (float)width_d;
		vertex[3].y = (float)height_s;
		for(int i=0;i <4; i++)
		{
 			vertex[i].x -= 0.5f;
  			vertex[i].y -= 0.5f;
			vertex[i].z = 1.0f;
			vertex[i].w = 1.0f;
		}

		vertex[0].tu = (float)s.left / desc.Width;
		vertex[0].tv = (float)s.top / desc.Height;
		vertex[1].tu = (float)s.right / desc.Width;
		vertex[1].tv = (float)s.top / desc.Height;
		vertex[2].tu = (float)s.left / desc.Width;
		vertex[2].tv = (float)s.bottom / desc.Height;
		vertex[3].tu = (float)s.right / desc.Width;
		vertex[3].tv = (float)s.bottom / desc.Height;

		float ps[12] = {abs((float)width_d/width_s), abs((float)height_d/height_s), desc.Width, desc.Height,
						desc.Width/2, desc.Height, abs((float)width_d/(width_s/2)), abs((float)height_d/(height_s/2)), alpha};
		ps[0] = ps[0] > 1 ? 1 : ps[0];
		ps[1] = ps[1] > 1 ? 1 : ps[1];
		ps[6] = ps[6] > 1 ? 1 : ps[6];
		ps[7] = ps[7] > 1 ? 1 : ps[7];
		m_Device->SetPixelShaderConstantF(0, ps, 3);


		// shader
		IDirect3DPixelShader9 * lanczos_shader = m_lanczosX;
		if (format == MEDIASUBTYPE_YV12)
			lanczos_shader = m_lanczosX_YV12;
		else if (format == MEDIASUBTYPE_NV12 || format == MEDIASUBTYPE_YUY2)
			lanczos_shader = m_lanczosX_NV12;
		else if (format == MEDIASUBTYPE_P010 || format == MEDIASUBTYPE_P016)
			lanczos_shader = m_lanczosX_P016;
		else if (format == MEDIASUBTYPE_RGB32 || format == MEDIASUBTYPE_ARGB32)
			lanczos_shader = m_lanczosX;
		if (width_s != width_d)
			m_Device->SetPixelShader(lanczos_shader);
		else
		{
			float rect_data[8] = {m_lVidWidth, m_lVidHeight, m_lVidWidth/2, m_lVidHeight, (float)m_last_reset_time/100000, (float)timeGetTime()/100000, alpha};
			hr = m_Device->SetPixelShaderConstantF(0, rect_data, 2);
			m_Device->SetPixelShader(shader_yuv);
		}

		// render state and render
		m_Device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		m_Device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		m_Device->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
		hr = m_Device->SetSamplerState( 0, D3DSAMP_MIPFILTER, D3DTEXF_NONE );
		hr = m_Device->SetFVF( FVF_Flags );
		hr = m_Device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertex, sizeof(MyVertex));

		// pass2, Y filter
		m_Device->SetRenderTarget(0, dst);
		m_Device->SetScissorRect(&clip);
		vertex[0].x = (float)d.left;
		vertex[0].y = (float)d.top;
		vertex[1].x = (float)d.right;
		vertex[1].y = (float)d.top;
		vertex[2].x = (float)d.left;
		vertex[2].y = (float)d.bottom;
		vertex[3].x = (float)d.right;
		vertex[3].y = (float)d.bottom;
		for(int i=0;i <4; i++)
		{
			vertex[i].x -= 0.5f;
 			vertex[i].y -= 0.5f;
			vertex[i].z = 1.0f;
			vertex[i].w = 1.0f;
		}

		vertex[0].tu = (float)0 / (double)TEXTURE_SIZE;
		vertex[0].tv = (float)0 / (double)TEXTURE_SIZE;
		vertex[1].tu = (float)width_d / (double)TEXTURE_SIZE;
		vertex[1].tv = (float)0 / (double)TEXTURE_SIZE;
		vertex[2].tu = (float)0 / (double)TEXTURE_SIZE;
		vertex[2].tv = (float)height_s / (double)TEXTURE_SIZE;
		vertex[3].tu = (float)width_d / (double)TEXTURE_SIZE;
		vertex[3].tv = (float)height_s / (double)TEXTURE_SIZE;

		m_Device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		m_Device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		m_Device->SetRenderState( D3DRS_ALPHABLENDENABLE, alpha_blend );
		ps[1] = (float)height_d/height_s;
		ps[1] = ps[1] > 0 ? ps[1] : -ps[1];
		ps[1] = ps[1] > 1 ? 1 : ps[1];
		ps[2] = ps[3] = (double)TEXTURE_SIZE;
		m_Device->SetPixelShaderConstantF(0, ps, 1);
		if (height_s != height_d)
			m_Device->SetPixelShader(m_lanczosY);
		else
			m_Device->SetPixelShader(NULL);

		hr = m_Device->SetTexture( 0, tmp1->texture );
		hr = m_Device->SetFVF( FVF_Flags );
		hr = m_Device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertex, sizeof(MyVertex));

		safe_delete(tmp1);

	}
	else if (method == lanczos_onepass)
	{
		// pass0, 2D filter, rather slow, a little better quality
		m_Device->SetRenderTarget(0, dst);
		m_Device->SetScissorRect(&clip);

		// shader
		IDirect3DPixelShader9 * lanczos_shader = m_lanczos;
		if (format == MEDIASUBTYPE_YV12)
			lanczos_shader = m_lanczos_YV12;
		else if (format == MEDIASUBTYPE_NV12 || format == MEDIASUBTYPE_YUY2)
			lanczos_shader = m_lanczos_NV12;
		else if (format == MEDIASUBTYPE_P010 || format == MEDIASUBTYPE_P016)
			lanczos_shader = m_lanczos_P016;
		else if (format == MEDIASUBTYPE_RGB32 || format == MEDIASUBTYPE_ARGB32)
			lanczos_shader = m_lanczos;
		if ((height_s != height_d || width_s != width_d)
			&& lanczos_shader != NULL)
		{
			m_Device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
			m_Device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
			float ps[4] = {abs((float)width_d/width_s), abs((float)height_d/height_s), desc.Width, desc.Height};
			ps[0] = ps[0] > 1 ? 1 : ps[0];
			ps[1] = ps[1] > 1 ? 1 : ps[1];
			float ps_alpha[4] = {alpha};
			m_Device->SetPixelShaderConstantF(0, ps, 1);
			m_Device->SetPixelShaderConstantF(2, ps_alpha, 1);
			m_Device->SetPixelShader(lanczos_shader);
		}
		else
			hr = m_Device->SetPixelShader(shader_yuv);

		// render state and go
		m_Device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		m_Device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		m_Device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
		m_Device->SetRenderState( D3DRS_ALPHABLENDENABLE, alpha_blend );
		hr = m_Device->SetFVF( FVF_Flags );
		hr = m_Device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, direct_vertex, sizeof(MyVertex));
		hr = m_Device->SetPixelShader(NULL);
	}
	else if (method == bilinear_mipmap_minus_one || method == bilinear_mipmap || method == bilinear_no_mipmap)
	{
		if (src2 && method != bilinear_no_mipmap)
		{
			// nearly all D3D9 cards doesn't support D3DUSAGE_AUTOGENMIPMAP
			// so if we need to use MIPMAP, then we need convert to RGB32 first;
			FAIL_RET(src2->convert_to_RGB32(m_Device, m_ps_yv12, m_ps_nv12, m_ps_P016, NULL, NULL, m_last_reset_time));
			m_Device->SetTexture(0, src2->m_tex_gpu_RGB32->texture);
			shader_yuv = m_alpha_multiply;
		}

		float shader_alpha_parameter[8] = {desc.Width, desc.Height, desc.Width/2, desc.Height, (float)m_last_reset_time/100000, (float)timeGetTime()/100000, alpha};

		float mip_lod = (method == bilinear_mipmap_minus_one) ?  -1.0f : 0.0f;
		hr = m_Device->SetSamplerState( 0, D3DSAMP_MIPMAPLODBIAS, *(DWORD*)&mip_lod );
		hr = m_Device->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
		hr = m_Device->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
		hr = m_Device->SetSamplerState( 0, D3DSAMP_MIPFILTER, (method == bilinear_no_mipmap) ? D3DTEXF_NONE : D3DTEXF_LINEAR );

		m_Device->SetRenderTarget(0, dst);
		m_Device->SetScissorRect(&clip);
		m_Device->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
		m_Device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		m_Device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
		//m_Device->SetSamplerState(0, D3DSAMP_BORDERCOLOR, 0);					// set to border mode, to remove a single half-line
		hr = m_Device->SetPixelShaderConstantF(0, shader_alpha_parameter, 2);
		hr = m_Device->SetPixelShader(shader_yuv);
		hr = m_Device->SetStreamSource( 0, NULL, 0, sizeof(MyVertex) );
		hr = m_Device->SetFVF( FVF_Flags );
		hr = m_Device->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, direct_vertex, sizeof(MyVertex));
	}

	m_Device->SetPixelShader(NULL);

	return S_OK;
}

HRESULT my12doomRenderer::render(bool forced)
{
	if(m_output_mode == pageflipping)
		return S_FALSE;

	m_last_frame_time = timeGetTime();
	SetEvent(m_render_event);

	return S_OK;
}
DWORD WINAPI my12doomRenderer::test_thread(LPVOID param)
{
	my12doomRenderer *_this = (my12doomRenderer*)param;
	while(_this->m_Device == NULL)
		Sleep(1);

	Sleep(5000);

	while (!_this->m_render_thread_exit)
	{
		D3DLOCKED_RECT locked;
		HRESULT hr;

		int l1 = timeGetTime();
		//hr = _this->m_Device->CreateOffscreenPlainSurface(1920, 1080, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &_this->m_just_a_test_surface, NULL);
		int l2 = timeGetTime();
		//hr = _this->m_just_a_test_surface->LockRect(&locked, NULL, NULL);
		int l3 = timeGetTime();
		//memset(locked.pBits, 0, locked.Pitch * 1080);
		int l4 = timeGetTime();
		//hr = _this->m_just_a_test_surface->UnlockRect();
		int l5 = timeGetTime();
		//_this->m_just_a_test_surface = NULL;
		int l6 = timeGetTime();

		if (l6-l1 > 2)
			mylog("thread: %d,%d,%d,%d,%d\n", l2-l1, l3-l2, l4-l3, l5-l4, l6-l5);

		Sleep(1);
	}

	return 0;
}

extern dx_player *g_player;

DWORD WINAPI my12doomRenderer::render_thread(LPVOID param)
{
	my12doomRenderer *_this = (my12doomRenderer*)param;
	_this->m_render_thread_id = GetCurrentThreadId();

	int l = timeGetTime();
	while (timeGetTime() - l < 0 && !_this->m_render_thread_exit)
		Sleep(1);

	//SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	//SetThreadAffinityMask(GetCurrentThread(), 1);
	while(!_this->m_render_thread_exit)
	{
		if (_this->m_output_mode != pageflipping && _this->GPUIdle)
		{
			if (timeGetTime() - _this->m_last_frame_time < 333 && g_player->is_playing())
			{
				if (WaitForSingleObject(_this->m_render_event, 1) != WAIT_TIMEOUT)
				{

					//_this->m_Device->SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID);
					//_this->render_nolock(true);
					//_this->m_Device->SetRenderState( D3DRS_FILLMODE, D3DFILL_WIREFRAME);
					_this->render_nolock(true);

				}
				continue;
			}
			else
			{
				l = timeGetTime();
				_this->render_nolock(true);
				while (timeGetTime() - l < 33 && !_this->m_render_thread_exit)
					Sleep(1);
			}
		}
		else
		{
			_this->render_nolock(true);
 			Sleep(1);
		}
	}
	
	return S_OK;
}

// dx9 helper functions
HRESULT my12doomRenderer::load_image(int id /*= -1*/, bool forced /* = false */)
{

	CAutoLock lck(&m_packet_lock);
	if (!m_sample2render_1 && !m_sample2render_2)
		return S_FALSE;

	int l = timeGetTime();

	gpu_sample *sample1 = m_sample2render_1;
	gpu_sample *sample2 = m_sample2render_2;

	bool dual_stream = sample1 && sample2;
	input_layout_types input = m_input_layout == input_layout_auto ? m_layout_detected : m_input_layout;
	bool need_detect = !dual_stream && m_input_layout == input_layout_auto && !m_no_more_detect;
	CLSID format = sample1->m_format;
	bool topdown = sample1->m_topdown;

	HRESULT hr = S_OK;
	if (sample1)
	{
// 		FAIL_RET(sample1->convert_to_RGB32(m_Device, m_ps_yv12, m_ps_nv12, m_ps_yuy2, g_VertexBuffer, m_last_reset_time));
		if (need_detect) sample1->do_stereo_test(m_Device, m_ps_test_sbs, m_ps_test_tb, NULL);
	}
	if (sample2)
	{
// 		FAIL_RET(sample2->convert_to_RGB32(m_Device, m_ps_yv12, m_ps_nv12, m_ps_yuy2, g_VertexBuffer, m_last_reset_time));
	}


	CAutoLock rendered_lock(&m_rendered_packet_lock);
	safe_delete(m_last_rendered_sample1);
	safe_delete(m_last_rendered_sample2);
	m_last_rendered_sample1 = m_sample2render_1;
	m_last_rendered_sample2 = m_sample2render_2;
	m_sample2render_1 = NULL;
	m_sample2render_2 = NULL;
	

	// stereo test
	if (need_detect)
	{
		int this_frame_type = 0;
		if (S_OK == m_last_rendered_sample1->get_strereo_test_result(m_Device, &this_frame_type))
		{
			if (this_frame_type == side_by_side)
				m_sbs ++;
			else if (this_frame_type == top_bottom)
				m_tb ++;
			else if (this_frame_type == mono2d)
				m_normal ++;
		}

		input_layout_types next_layout = m_layout_detected;
		if (m_normal - m_sbs > 5)
			next_layout = mono2d;
		if (m_sbs - m_normal > 5)
			next_layout = side_by_side;
		if (m_tb - max(m_sbs,m_normal) > 5)
			next_layout = top_bottom;

		if (m_normal - m_sbs > 500 || m_sbs - m_normal > 500 || m_tb - max(m_sbs,m_normal) > 500)
			m_no_more_detect = true;

		if (next_layout != m_layout_detected)
		{
			m_layout_detected = next_layout;
			if (false)
			set_device_state(need_reset_object);
		}
	}

	static int n = timeGetTime();
	//printf("load and convert time:%dms, presant delta: %dms(%.02f circle)\n", timeGetTime()-l, timeGetTime()-n, (double)(timeGetTime()-n) / (1000/60));
	n = timeGetTime();

	return hr;
}

HRESULT my12doomRenderer::screenshot(const wchar_t*file)
{
	if (!file)
		return E_POINTER;

	CAutoLock rendered_lock(&m_rendered_packet_lock);
	if (!m_last_rendered_sample1)
		return E_FAIL;

	return m_last_rendered_sample1->convert_to_RGB32_CPU(file);
}

HRESULT my12doomRenderer::screenshot(void *Y, void*U, void*V, int stride, int width, int height)
{
	CAutoLock rendered_lock(&m_rendered_packet_lock);
	if (!m_last_rendered_sample1)
		return E_FAIL;

	if (m_last_rendered_sample1 && m_last_rendered_sample2)
	{
		HRESULT hr;
		FAIL_RET(m_last_rendered_sample1->convert_to_RGB32_CPU(Y, U, V, stride, width/2, height));
		return m_last_rendered_sample2->convert_to_RGB32_CPU((char*)Y+width/2, (char*)U+width/4, (char*)V+width/4, stride, width/2, height);
	}

	return m_last_rendered_sample1->convert_to_RGB32_CPU(Y, U, V, stride, width, height);
}

HRESULT my12doomRenderer::get_movie_desc(int *width, int*height)
{
	if (width)
		*width = m_lVidWidth;

	if (height)
		*height = m_lVidHeight;

	return S_OK;
}


input_layout_types my12doomRenderer::get_active_input_layout()
{
	if (m_input_layout != input_layout_auto)
		return m_input_layout;
	else
		return m_layout_detected;
}

bool my12doomRenderer::is2DMovie()
{
	return !(get_active_input_layout() != mono2d 
		|| m_convert3d
		|| (m_dsr0->is_connected() && m_dsr1->is_connected())
		|| (!m_dsr0->is_connected() && !m_dsr1->is_connected())
		|| m_remux_mode
		|| (int)m_input_layout == frame_sequence);
}

bool my12doomRenderer::is2DRendering()
{
	return !(get_active_input_layout() != mono2d 
		|| m_convert3d
		|| (m_dsr0->is_connected() && m_dsr1->is_connected())
		|| (!m_dsr0->is_connected() && !m_dsr1->is_connected())
		|| m_remux_mode
		|| (int)m_input_layout == frame_sequence) || m_output_mode == mono|| m_force2d;
}

double my12doomRenderer::get_active_aspect()
{
	if ((double)m_forced_aspect > 0)
		return (double)m_forced_aspect;

	bool dual_stream = (m_dsr0->is_connected() && m_dsr1->is_connected()) || m_remux_mode || (int)m_input_layout == frame_sequence;
	if (dual_stream || get_active_input_layout() == mono2d)
		return m_source_aspect;
	else if (get_active_input_layout() == side_by_side)
	{
		if (m_source_aspect > 2.65)
			return m_source_aspect / 2;
		else
			return m_source_aspect;
	}
	else if (get_active_input_layout() == top_bottom)
	{
		if (m_source_aspect < 1.325)
			return m_source_aspect * 2;
		else
			return m_source_aspect;
	}

	return m_source_aspect;
}
HRESULT my12doomRenderer::calculate_movie_position_unscaled(RECTF *position)
{
	if (!position)
		return E_POINTER;

	RECTF &tar = *position;
	tar = get_movie_scissor_rect();

	// swap width and height for vertical orientation
	if (m_display_orientation == vertical)
	{
		float t = tar.right;
		tar.right = tar.bottom;
		tar.bottom = t;
	}

	// half width/height for sbs/tb output mode
	if (m_output_mode == out_sbs)
		tar.right /= 2;
	else if (m_output_mode == out_tb)
		tar.bottom /= 2;

	float width = tar.right - tar.left;
	float height = tar.bottom - tar.top;
	double active_aspect = get_active_aspect();
	float delta_w = width - height * active_aspect;
	float delta_h = height - width  / active_aspect;
	if (delta_w > 0)
	{
		// letterbox left and right (default), or vertical fill(vertical is already full)
		if ((int)m_aspect_mode == aspect_letterbox || (int)m_aspect_mode == aspect_vertical_fill)
		{
			tar.left += delta_w/2;
			tar.right -= delta_w/2;
		}
		else if ((int)m_aspect_mode == aspect_horizontal_fill)
		{
			// extent horizontally, top and bottom cut
			// (delta_h < 0)
			tar.top += delta_h/2;
			tar.bottom -= delta_h/2;
		}
		else	// stretch mode, do nothing
		{
		}
	}
	else if (delta_h > 0)
	{
		// letterbox top and bottome (default)
		if ((int)m_aspect_mode == aspect_letterbox || (int)m_aspect_mode == aspect_horizontal_fill)
		{
			tar.top += delta_h/2;
			tar.bottom -= delta_h/2;
		}
		else if ((int)m_aspect_mode == aspect_vertical_fill)
		{
			// extent vertically, top and bottom cut
			// (delta_w < 0)
			tar.left += delta_w/2;
			tar.right -= delta_w/2;
		}
		else	// stretch mode, do nothing
		{
		}
	}

	return S_OK;
}

HRESULT my12doomRenderer::calculate_movie_position(RECTF *position)
{
	if (!position)
		return E_POINTER;

	// f = factor
	// C = (screen_width/2, screen_height/2)
	// P = point in screen space, before zooming
	// P'= point in zoom center space(
	// P''' = point in screen space, after zooming

	// P' = P - C
	// P'' = f (P')
	// P''' = P'' + C
	// P''''= P'''+ offset * {width(P'''), height(P''')}

	//
	float f = m_zoom_factor;
	RECTF P0;
	calculate_movie_position_unscaled(&P0);
	float movie_width_1x = (P0.right - P0.left)/m_zoom_factor;
	float movie_height_1x = (P0.bottom - P0.top)/m_zoom_factor;
	float zoom_center_x = (P0.left + P0.right)/2;
	float zoom_center_y = (P0.top + P0.bottom)/2;

	// transform to center point space
	RECTF P1 = P0;
	P1.left -= zoom_center_x;
	P1.right -= zoom_center_x;
	P1.bottom -= zoom_center_y;
	P1.top -= zoom_center_y;

	// scale
	RECTF P2 = P1;
	P2.left *= f;
	P2.right *= f;
	P2.bottom *= f;
	P2.top *= f;

	// transform back
	RECTF P3 = P2;
	P3.left += zoom_center_x;
	P3.right += zoom_center_x;
	P3.bottom += zoom_center_y;
	P3.top += zoom_center_y;

	// offsets
	float tar_width = P3.right - P3.left;
	float tar_height = P3.bottom - P3.top;
	P3.left += tar_width * (double)m_movie_offset_x;
	P3.right += tar_width * (double)m_movie_offset_x;
	P3.top += tar_height * (double)m_movie_offset_y;
	P3.bottom += tar_height * (double)m_movie_offset_y;

	*position = P3;
	return S_OK;
}

HRESULT my12doomRenderer::calculate_subtitle_position(RECTF *postion, int view)
{
	if (!postion)
		return E_POINTER;

	RECTF tar;
	calculate_movie_position(&tar);

	float pic_width = tar.right - tar.left;
	float pic_height = tar.bottom - tar.top;

	float left = tar.left + pic_width * (m_subtitle_fleft - (view * m_subtitle_parallax));
	float width = pic_width * m_subtitle_fwidth;
	float top = tar.top + pic_height * m_subtitle_ftop;
	float height = pic_height * m_subtitle_fheight;

	postion->left = left;
	postion->right = left + width;
	postion->top = top;
	postion->bottom = top + height;

	return E_NOTIMPL;
}

AutoSettingString g_mask_shader_file(L"MaskShaderFile", L"");

HRESULT my12doomRenderer::generate_mask()
{
	HRESULT hr;
	if (m_output_mode != masking && m_output_mode != multiview)
		return S_FALSE;

	if (!m_tex_mask)
		return VFW_E_NOT_CONNECTED;

	CComPtr<IDirect3DTexture9> mask_cpu;
	FAIL_RET( m_Device->CreateTexture(m_active_pp.BackBufferWidth, m_active_pp.BackBufferHeight, 1, NULL, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &mask_cpu, NULL));

	D3DLOCKED_RECT locked;
	FAIL_RET(mask_cpu->LockRect(0, &locked, NULL, NULL));

	BYTE *dst = (BYTE*) locked.pBits;

	if (g_mask_shader_file[0] != NULL)
	{
		USES_CONVERSION;
		luaState lua_state;
		luaL_loadfile(lua_state, W2A(g_mask_shader_file));
		int status = lua_pcall(lua_state, 0, 0, 0);
		lua_getglobal(lua_state, "GetCellSize");
		int w = 0;
		int h = 0;
		bool lua_ready = false;
		if (lua_isfunction(lua_state, -1))
		{
			lua_pcall(lua_state, 0, 2, 0);
			w = lua_tointeger(lua_state, -2);
			h = lua_tointeger(lua_state, -1);
			lua_settop(lua_state, 0);

			lua_getglobal(lua_state, "Main");
			if (w>0 && h > 0 && lua_isfunction(lua_state, -1))
			{
				lua_ready = true;
			}
		}
		lua_settop(lua_state, 0);

		if (lua_ready)
		{
			DWORD *atom = new DWORD[w*h];
			int lua_time = timeGetTime();
			for(int i=0; i<1000; i++)
			for(int y = 0; y < h; y++)
			{
				DWORD * d = (DWORD*)(atom+w*y);
				for(int x = 0; x < w; x++)
				{
					lua_getglobal(lua_state, "Main");
					lua_pushinteger(lua_state, x%w);
					lua_pushinteger(lua_state, y%h);
					lua_pcall(lua_state, 2, 4, 0);
					int R = lua_isnil(lua_state, -4) ? 0 : lua_tointeger(lua_state, -4);
					int G = lua_isnil(lua_state, -3) ? 0 : lua_tointeger(lua_state, -3);
					int B = lua_isnil(lua_state, -2) ? 0 : lua_tointeger(lua_state, -2);
					int A = lua_isnil(lua_state, -1) ? 255 : lua_tointeger(lua_state, -1);
 					lua_pop(lua_state, 4);

					d[x] = D3DCOLOR_ARGB(A, R,G,B);
				}
			}
			lua_checkstack(lua_state, 1);
			mylog("lua cost time %d\n", timeGetTime() - lua_time);


			for(int y = 0; y < m_active_pp.BackBufferHeight; y++)
			{
				DWORD * d = (DWORD*)(dst+locked.Pitch*y);
				for(int x = 0; x < m_active_pp.BackBufferWidth; x+=w)
				{
					memcpy(d+x,atom + (y%h)*w + x%w, min(w, m_active_pp.BackBufferWidth - x) * sizeof(DWORD));
				}
			}

			delete [] atom;
		}
		else
		{
			memset(dst, 0, locked.Pitch * m_active_pp.BackBufferHeight);
		}

	}
	else if (m_output_mode == multiview)
	{
		// 412341234123
		// 341234123412
		// 234123412341
		// 123412341234

		DWORD color_table[4][4] = {
			{D3DCOLOR_XRGB(254,63,127), D3DCOLOR_XRGB(191,254,63), D3DCOLOR_XRGB(127,191,254), D3DCOLOR_XRGB(63,127,191)},
			{D3DCOLOR_XRGB(191,254,63), D3DCOLOR_XRGB(127,191,254), D3DCOLOR_XRGB(63,127,191), D3DCOLOR_XRGB(254,63,127)},
			{D3DCOLOR_XRGB(127,191,254), D3DCOLOR_XRGB(63,127,191), D3DCOLOR_XRGB(254,63,127), D3DCOLOR_XRGB(191,254,63)},
			{D3DCOLOR_XRGB(63,127,191), D3DCOLOR_XRGB(254,63,127), D3DCOLOR_XRGB(191,254,63), D3DCOLOR_XRGB(127,191,254)},
		};

		DWORD four_line[4][MAX_TEXTURE_SIZE];
		for(DWORD y=0; y<4; y++)
		for(DWORD x=0; x<MAX_TEXTURE_SIZE; x++)
		{
			four_line[y][x] = color_table[y%4][x%4];
		}
		for(DWORD y=0; y<m_active_pp.BackBufferHeight; y++)
		{
			memcpy(dst, four_line[y%4], m_active_pp.BackBufferWidth*4);
			dst += locked.Pitch;
		}

		/*
		// 412341234123
		// 341234123412
		// 234123412341
		// 123412341234

		DWORD color_table[4][4] = {
			{D3DCOLOR_XRGB(255,63,127), D3DCOLOR_XRGB(191,255,63), D3DCOLOR_XRGB(127,191,255), D3DCOLOR_XRGB(63,127,191)},
			{D3DCOLOR_XRGB(191,255,63), D3DCOLOR_XRGB(127,191,255), D3DCOLOR_XRGB(63,127,191), D3DCOLOR_XRGB(255,63,127)},
			{D3DCOLOR_XRGB(127,191,255), D3DCOLOR_XRGB(63,127,191), D3DCOLOR_XRGB(255,63,127), D3DCOLOR_XRGB(191,255,63)},
			{D3DCOLOR_XRGB(63,127,191), D3DCOLOR_XRGB(255,63,127), D3DCOLOR_XRGB(191,255,63), D3DCOLOR_XRGB(127,191,255)},
		};

		DWORD four_line[4][MAX_TEXTURE_SIZE];
		for(DWORD y=0; y<4; y++)
		for(DWORD x=0; x<TEXTURE_SIZE; x++)
		{
			four_line[y][x] = color_table[y%4][x%4];
		}
		for(DWORD y=0; y<m_active_pp.BackBufferHeight; y++)
		{
			memcpy(dst, four_line[y%4], m_active_pp.BackBufferWidth*4);
			dst += locked.Pitch;
		}
		*/

	}
	else if (m_mask_mode == row_interlace)
	{
		// init row mask texture
		D3DCOLOR one_line[(int)MAX_TEXTURE_SIZE];
		for(DWORD i=0; i<(int)TEXTURE_SIZE; i++)
			one_line[i] = i%2 == 0 ? 0 : D3DCOLOR_ARGB(255,255,255,255);

		for(DWORD i=0; i<m_active_pp.BackBufferHeight; i++)
		{
			memcpy(dst, one_line, m_active_pp.BackBufferWidth*4);
			dst += locked.Pitch;
		}
	}
	else if (m_mask_mode == line_interlace)
	{
		for(DWORD i=0; i<m_active_pp.BackBufferHeight; i++)
		{
			memset(dst, i%2 == 0 ? 255 : 0 ,m_active_pp.BackBufferWidth*4);
			dst += locked.Pitch;
		}
	}
	else if (m_mask_mode == checkboard_interlace)
	{
		// init row mask texture
		D3DCOLOR one_line[(int)MAX_TEXTURE_SIZE];
		for(DWORD i=0; i<(int)TEXTURE_SIZE; i++)
			one_line[i] = i%2 == 0 ? 0 : D3DCOLOR_ARGB(255,255,255,255);

		for(DWORD i=0; i<m_active_pp.BackBufferHeight; i++)
		{
			memcpy(dst, one_line + (i%2), m_active_pp.BackBufferWidth*4);
			dst += locked.Pitch;
		}
	}
	else if (m_mask_mode == subpixel_row_interlace)
	{
		D3DCOLOR one_line[(int)MAX_TEXTURE_SIZE];
		for(DWORD i=0; i<(int)TEXTURE_SIZE; i++)
			one_line[i] = i%2 == 0 ? D3DCOLOR_ARGB(255,255,0,255) : D3DCOLOR_ARGB(255,0,255,0);

		for(DWORD i=0; i<m_active_pp.BackBufferHeight; i++)
		{
			memcpy(dst, one_line, m_active_pp.BackBufferWidth*4);
			dst += locked.Pitch;
		}
	}
	else if (m_mask_mode == subpixel_45_interlace)
	{
		D3DCOLOR one_line[6][(int)MAX_TEXTURE_SIZE];
		for(DWORD i=0; i<(int)TEXTURE_SIZE; i++)
		{
			one_line[0][i] = i%2 == 0 ? D3DCOLOR_ARGB(255,255,255,255) : D3DCOLOR_ARGB(255,0,0,0);
			one_line[1][i] = i%2 == 0 ? D3DCOLOR_ARGB(255,255,255,0) : D3DCOLOR_ARGB(255,0,0,255);
			one_line[2][i] = i%2 == 0 ? D3DCOLOR_ARGB(255,255,0,0) : D3DCOLOR_ARGB(255,0,255,255);
			one_line[3][i] = i%2 == 0 ? D3DCOLOR_ARGB(255,0,0,0) : D3DCOLOR_ARGB(255,255,255,255);
			one_line[4][i] = i%2 == 0 ? D3DCOLOR_ARGB(255,0,0,255) : D3DCOLOR_ARGB(255,255,255,0);
			one_line[5][i] = i%2 == 0 ? D3DCOLOR_ARGB(255,0,255,255) : D3DCOLOR_ARGB(255,255,0,0);
		}

		for(DWORD i=0; i<m_active_pp.BackBufferHeight; i++)
		{
			memcpy(dst, one_line[(i+m_mask_parameter)%6], m_active_pp.BackBufferWidth*4);
			dst += locked.Pitch;
		}
	}
	else if (m_mask_mode == subpixel_x2_interlace || m_mask_mode == subpixel_x2_45_interlace || m_mask_mode == subpixel_x2_minus45_interlace)
	{
		// RGB - RGB - RGB - RGB
		// 112 - 211 - 221 - 122
		D3DCOLOR one_line[MAX_TEXTURE_SIZE+6];
//#define BGR
#ifndef BGR
		D3DCOLOR line_table[4] = {D3DCOLOR_XRGB(255, 255, 0), D3DCOLOR_XRGB(0, 255, 255), D3DCOLOR_XRGB(0, 0, 255), D3DCOLOR_XRGB(255,0,0)};
#else
		D3DCOLOR line_table[4] = {D3DCOLOR_XRGB(0, 255, 255), D3DCOLOR_XRGB(255, 255, 0), D3DCOLOR_XRGB(255, 0, 0), D3DCOLOR_XRGB(0,0,255)};
#endif
		for(DWORD i=0; i<MAX_TEXTURE_SIZE+6; i++)
		{
			one_line[i] = line_table[(i+m_mask_parameter)%4];
		}

		for(DWORD i=0; i<m_active_pp.BackBufferHeight; i++)
		{
			memcpy(dst, one_line + (m_mask_mode == subpixel_x2_45_interlace ? i : (m_mask_mode == subpixel_x2_minus45_interlace ? -i : 0) ) %4, m_active_pp.BackBufferWidth*4);
			dst += locked.Pitch;
		}
	}

	FAIL_RET(mask_cpu->UnlockRect(0));
	FAIL_RET(m_Device->UpdateTexture(mask_cpu, m_tex_mask));

	return hr;
}
HRESULT my12doomRenderer::set_fullscreen(bool full)
{
	m_D3D->GetAdapterDisplayMode( D3DADAPTER_DEFAULT, &m_d3ddm );
	m_new_pp.BackBufferFormat       = m_d3ddm.Format;

	mylog("mode:%dx%d@%dHz, format:%d.\n", m_d3ddm.Width, m_d3ddm.Height, m_d3ddm.RefreshRate, m_d3ddm.Format);

	if(full && m_active_pp.Windowed)
	{
		GetWindowRect(m_hWnd, &m_window_pos);
		m_style = GetWindowLongPtr(m_hWnd, GWL_STYLE);
		m_exstyle = GetWindowLongPtr(m_hWnd, GWL_EXSTYLE);

		LONG f = m_style & ~(WS_BORDER | WS_CAPTION | WS_THICKFRAME);
		LONG exf =  m_exstyle & ~(WS_EX_CLIENTEDGE | WS_EX_STATICEDGE | WS_EX_WINDOWEDGE |WS_EX_DLGMODALFRAME) | WS_EX_TOPMOST;

		SetWindowLongPtr(m_hWnd, GWL_STYLE, f);
		SetWindowLongPtr(m_hWnd, GWL_EXSTYLE, exf);
		if ((DWORD)(LOBYTE(LOWORD(GetVersion()))) < 6)
			SendMessage(m_hWnd, WM_SYSCOMMAND, SC_RESTORE, 0);

		m_new_pp.Windowed = FALSE;
		m_new_pp.BackBufferWidth = m_d3ddm.Width;
		m_new_pp.BackBufferHeight = m_d3ddm.Height;
		m_new_pp.FullScreen_RefreshRateInHz = m_d3ddm.RefreshRate;

		if (/*m_output_mode == hd3d &&*/ m_HD3DStereoModesCount > 0)
			HD3DSetStereoFullscreenPresentParameters();

		set_device_state(need_reset);
		if (m_device_state < need_create)
			handle_device_state();
	}
	else if (!full && !m_active_pp.Windowed)
	{
		m_new_pp.Windowed = TRUE;
		m_new_pp.BackBufferWidth = 0;
		m_new_pp.BackBufferHeight = 0;
		m_new_pp.hDeviceWindow = m_hWnd;
		m_new_pp.FullScreen_RefreshRateInHz = 0;
		SetWindowLongPtr(m_hWnd, GWL_STYLE, m_style);
		SetWindowLongPtr(m_hWnd, GWL_EXSTYLE, m_exstyle);
		SetWindowPos(m_hWnd, m_exstyle & WS_EX_TOPMOST ? HWND_TOPMOST : HWND_NOTOPMOST,
			m_window_pos.left, m_window_pos.top, m_window_pos.right - m_window_pos.left, m_window_pos.bottom - m_window_pos.top, SWP_FRAMECHANGED);

		set_device_state(need_reset);
		if (m_device_state < need_create)
			handle_device_state();
	}

	return S_OK;
}

HRESULT my12doomRenderer::pump()
{
	BOOL success = FALSE;
	RECT rect;
	if (m_hWnd)
	{
		GetWindowRect(m_hWnd, &m_window_rect);
		success = GetClientRect(m_hWnd, &rect);
		if (success && rect.right > 0 && rect.bottom > 0)
		if (m_active_pp.BackBufferWidth != rect.right-rect.left || m_active_pp.BackBufferHeight != rect.bottom - rect.top)
		{
			if (m_active_pp.Windowed)
			set_device_state(need_resize_back_buffer);
		}
	}

	if (m_hWnd2)
	{
		success = GetClientRect(m_hWnd2, &rect);
		if (success && rect.right > 0 && rect.bottom > 0)
		if (m_active_pp2.BackBufferWidth != rect.right-rect.left || m_active_pp2.BackBufferHeight != rect.bottom - rect.top)
			if (m_active_pp.Windowed)
			set_device_state(need_resize_back_buffer);
	}

	return handle_device_state();
}


HRESULT my12doomRenderer::NV3D_notify(WPARAM wparam)
{
	BOOL actived = LOWORD(wparam);
	WORD separation = HIWORD(wparam);
	
	if (actived)
	{
		m_nv3d_actived = true;
		mylog("actived!\n");
	}
	else
	{
		m_nv3d_actived = false;
		if (m_output_mode == NV3D)
			set_device_state(need_resize_back_buffer);
		mylog("deactived!\n");
	}


	return S_OK;
}

HRESULT my12doomRenderer::set_input_layout(int layout)
{
	m_input_layout = (input_layout_types)layout;
	if(false)
	{
		set_device_state(need_reset_object);
		handle_device_state();
	}
	repaint_video();
	return S_OK;
}

HRESULT my12doomRenderer::set_output_mode(int mode)
{
	if (m_output_mode == mode)
		return S_OK;

	HRESULT hr;

	if (mode == intel3d && m_intel_caps.ulNumEntries <= 0)
		return E_NOINTERFACE;

	if (m_output_mode != intel3d && mode == intel3d)
		FAIL_RET(intel_switch_to_3d());

	if (m_output_mode == intel3d && mode != intel3d)
		FAIL_RET(intel_switch_to_2d());

	if (mode == hd3d)
		FAIL_RET(HD3DMatchResolution());			// HD3D now support auto detect / setting fullscreen resolution

	m_output_mode = (output_mode_types)(mode % output_mode_types_max);

	m_pageflipping_start = -1;
	m_first_pageflip.QuadPart = -1;
	set_device_state(need_resize_back_buffer);

	return S_OK;
}

HRESULT my12doomRenderer::set_mask_mode(int mode)
{
	m_mask_mode = (mask_mode_types)(mode % mask_mode_types_max);
	generate_mask();
	repaint_video();
	return S_OK;
}

HRESULT my12doomRenderer::set_mask_parameter(int parameter)
{
	m_mask_parameter = parameter;
	generate_mask();
	repaint_video();
	return S_OK;
}

HRESULT my12doomRenderer::set_mask_color(int id, DWORD color)
{
	if (id == 1)
		m_color1 = color;
	else if (id == 2)
		m_color2 = color;
	else
		return E_INVALIDARG;

	repaint_video();
	return S_OK;
}

DWORD my12doomRenderer::get_mask_color(int id)
{
	if (id == 1)
		return m_color1;
	else if (id == 2)
		return m_color2;
	else
		return 0;
}

HRESULT my12doomRenderer::set_swap_eyes(bool swap)
{
	CAutoLock lck(&m_frame_lock);
	m_swap_eyes = swap;

	reload_image();
	repaint_video();
	return S_OK;
}

HRESULT my12doomRenderer::set_force_2d(bool force2d)
{
	m_force2d = force2d;

	reload_image();
	repaint_video();
	return S_OK;
}

input_layout_types my12doomRenderer::get_input_layout()
{
	return m_input_layout;
}

output_mode_types my12doomRenderer::get_output_mode()
{
	return m_output_mode;
}
mask_mode_types my12doomRenderer::get_mask_mode()
{
	return m_mask_mode;
}
int my12doomRenderer::get_mask_parameter()
{
	return m_mask_parameter;
}

bool my12doomRenderer::get_swap_eyes()
{
	return m_swap_eyes;
}

bool my12doomRenderer::get_fullscreen()
{
	return !m_active_pp.Windowed;
}
HRESULT my12doomRenderer::set_movie_pos(int dimention, double offset)		// dimention1 = x, dimention2 = y
{
	if (dimention == 1)
		m_movie_offset_x = offset;
	else if (dimention == 2)
		m_movie_offset_y = offset;
	else if (dimention == 3 || dimention == 4)
	{
		RECTF r;
		calculate_movie_position(&r);

		if (dimention == 3)
			m_movie_offset_x = offset / (r.right - r.left);
		else
			m_movie_offset_y = offset / (r.bottom - r.top);
	}
	else
		return E_INVALIDARG;

	repaint_video();
	return S_OK;
}

HRESULT my12doomRenderer::set_aspect_mode(int mode)
{
	if ((int)m_aspect_mode == mode)
		return S_OK;

	m_aspect_mode = (aspect_mode_types)mode;
	repaint_video();
	return S_OK;

}

HRESULT my12doomRenderer::set_vsync(bool on)
{
	return S_OK;

	on = m_output_mode == pageflipping ? true : on;
	
	if (m_vertical_sync != on)
	{
		m_vertical_sync = on;

		m_new_pp.PresentationInterval   = m_vertical_sync ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;

		set_device_state(need_reset);
	}

	return S_OK;
}

HRESULT my12doomRenderer::set_zoom_factor(float factor, int zoom_center_x /* = -99999 */, int zoom_center_y /* = -99999 */)
{
	// f = factor / m_zoom_factor
	// C = (zoom_center_x, zoom_center_y)
	// P = point in screen space, before zooming
	// P'= point in zoom center space
	// P''' = point in screen space, after zooming

	// P' = P - C
	// P'' = f (P')
	// P''' = P'' + C


	// normal zooming: screen center
	if (zoom_center_x == -99999 && zoom_center_y == -99999)
	{ 	
		m_zoom_factor = factor;
 		return S_OK;
	}

	//
	float f = factor / m_zoom_factor;
	RECTF P0;
	calculate_movie_position(&P0);
	float movie_width_1x = (P0.right - P0.left)/m_zoom_factor;
	float movie_height_1x = (P0.bottom - P0.top)/m_zoom_factor;

	// transform to center point space
	RECTF P1 = {P0.left, P0.top, P0.right, P0.bottom};
	P1.left -= zoom_center_x;
	P1.right -= zoom_center_x;
	P1.bottom -= zoom_center_y;
	P1.top -= zoom_center_y;

	// scale
	RECTF P2 = P1;
	P2.left *= f;
	P2.right *= f;
	P2.bottom *= f;
	P2.top *= f;

	// transform back
	RECTF P3 = P2;
	P3.left += zoom_center_x;
	P3.right += zoom_center_x;
	P3.bottom += zoom_center_y;
	P3.top += zoom_center_y;

	// calculate new offset
	RECTF base;
	calculate_movie_position_unscaled(&base);

	m_zoom_factor = factor;
	m_movie_offset_x = (((double)P3.left - m_active_pp.BackBufferWidth/2) / m_zoom_factor + m_active_pp.BackBufferWidth/2 - base.left) / (base.right - base.left);
	m_movie_offset_y = (((double)P3.top - m_active_pp.BackBufferHeight/2) / m_zoom_factor + m_active_pp.BackBufferHeight/2 - base.top) / (base.bottom - base.top);

	RECTF P4;
	calculate_movie_position(&P4);

	return S_OK;
}

HRESULT my12doomRenderer::set_aspect(double aspect)
{
	m_forced_aspect = aspect;
	repaint_video();
	return S_OK;
}
double my12doomRenderer::get_movie_pos(int dimention)
{
	if (dimention == 1)
		return m_movie_offset_x;
	else if (dimention == 2)
		return m_movie_offset_y;
	else if (dimention == 3 || dimention == 4)
	{
		RECTF r;
		calculate_movie_position(&r);

		if (dimention == 3)
			return (double)m_movie_offset_x * (r.right - r.left);
		else
			return (double)m_movie_offset_y *(r.bottom - r.top);
	}
	else
		return 0.0;
}
double my12doomRenderer::get_aspect()
{
	return get_active_aspect();
}

HRESULT my12doomRenderer::set_window(HWND wnd, HWND wnd2)
{
	return E_NOTIMPL;

	reset();
	m_hWnd = wnd;
	m_hWnd2 = wnd2;
	handle_device_state();
	return S_OK;
}

HRESULT my12doomRenderer::set_movie_scissor_rect(RECTF *scissor)
{
	if (m_movie_scissor_rect)
	{
		delete m_movie_scissor_rect;
		m_movie_scissor_rect = NULL;
	}

	if (scissor)
	{
		m_movie_scissor_rect = new RECTF;
		memcpy(m_movie_scissor_rect, scissor, sizeof(RECTF));
	}
	return S_OK;
}
RECTF my12doomRenderer::get_movie_scissor_rect()
{
	RECTF o = {0,0,m_active_pp.BackBufferWidth, m_active_pp.BackBufferHeight};
	if (m_movie_scissor_rect)
		o = *m_movie_scissor_rect;
	return o;
}

HRESULT my12doomRenderer::repaint_video()
{
	if (m_dsr0->m_State != State_Running && timeGetTime() - m_last_frame_time < 333 && m_output_mode != pageflipping)
		render(true);
	return S_OK;
}

HRESULT my12doomRenderer::set_subtitle(void* data, int width, int height, float fwidth, float fheight, float fleft, float ftop, bool gpu_shadow)
{
	if (m_device_state >= device_lost)
		return S_FALSE;			// TODO : of source it's not SUCCESS

	if (data == NULL)
	{
		m_has_subtitle = false;
	}

	else
	{
		HRESULT hr;
		CPooledTexture *tex_mem = NULL;
		{
			CAutoLock lck(&m_pool_lock);
			if (!m_pool)
				return VFW_E_WRONG_STATE;
			FAIL_RET(m_pool->CreateTexture(min((int)TEXTURE_SIZE, (int)SUBTITLE_TEXTURE_SIZE), min((int)TEXTURE_SIZE, (int)SUBTITLE_TEXTURE_SIZE), NULL, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &tex_mem));
		}

		// copying
		BYTE *src = (BYTE*)data;
		BYTE *dst = (BYTE*) tex_mem->locked_rect.pBits;
		for(int y=0; y<min((int)TEXTURE_SIZE,height); y++)
		{
			memcpy(dst, src, min(width*4, tex_mem->locked_rect.Pitch));
			memset(dst+width*4, 0, 32*4);
			dst += tex_mem->locked_rect.Pitch;
			src += width*4;
		}
		memset(dst, 0, tex_mem->locked_rect.Pitch * min(min((int)TEXTURE_SIZE, (int)SUBTITLE_TEXTURE_SIZE)-height, 32));


		{
			int l = timeGetTime();
 			CAutoLock lck2(&m_subtitle_lock);

			CPooledTexture *p = m_subtitle_mem;
			m_subtitle_mem = NULL;
			m_subtitle_mem = tex_mem;
			m_has_subtitle = true;
			m_gpu_shadow = gpu_shadow;

			m_subtitle_fleft = fleft;
			m_subtitle_ftop = ftop;
			m_subtitle_fwidth = fwidth;
			m_subtitle_fheight = fheight;
			m_subtitle_pixel_width = width;
			m_subtitle_pixel_height = height;

			safe_delete(p);
		}

// 		repaint_video();
	}

	return S_OK;
}

HRESULT my12doomRenderer::set_subtitle_parallax(double offset)
{
	if (m_subtitle_parallax != offset)
	{
		m_subtitle_parallax = offset;
		repaint_video();
	}

	return S_OK;
}

HRESULT my12doomRenderer::set_parallax(double parallax)
{
	if (m_parallax != parallax)
	{
		m_parallax = parallax;
		repaint_video();
	}

	return S_OK;
}

HRESULT my12doomRenderer::intel_get_caps()
{
	HRESULT hr;
	m_intel_s3d = CreateIGFXS3DControl();
	if (!m_intel_s3d)
		return E_UNEXPECTED;
	memset(&m_intel_caps, 0 , sizeof(m_intel_caps));
	FAIL_FALSE(m_intel_s3d->GetS3DCaps(&m_intel_caps));

	if (m_intel_caps.ulNumEntries <= 0)
		return S_FALSE;
	return S_OK;
}

HRESULT my12doomRenderer::intel_get_caps(IGFX_S3DCAPS *caps)
{
	if (!caps)
		return E_POINTER;

	HRESULT hr;
	FAIL_RET(intel_get_caps());

	memcpy(caps, &m_intel_caps, sizeof(IGFX_S3DCAPS));
	return hr;
}

HRESULT my12doomRenderer::intel_switch_to_2d()
{
	if (!m_intel_s3d || m_intel_2d_mode.ulResWidth == 0)
		return E_UNEXPECTED;

	HRESULT hr ;

	FAIL_RET(m_intel_s3d->SwitchTo2D(&m_intel_2d_mode));

	memset(&m_intel_2d_mode, 0, sizeof(m_intel_2d_mode));
	memset(&m_intel_active_3d_mode, 0, sizeof(m_intel_active_3d_mode));

	return S_OK;
}

HRESULT my12doomRenderer::intel_switch_to_3d()
{
	HRESULT hr;
	if (!m_intel_s3d)
		return E_UNEXPECTED;

	// save current mode first
	memset(&m_intel_2d_mode, 0 , sizeof(m_intel_2d_mode));
	D3DDISPLAYMODE current;
	FAIL_RET(m_D3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &current));
	m_intel_2d_mode.ulResWidth = current.Width;
	m_intel_2d_mode.ulResHeight = current.Height;
	m_intel_2d_mode.ulRefreshRate = current.RefreshRate;

	hr = intel_get_caps();
	if (hr != S_OK)
		return E_FAIL;

	// TODO: select modes
	memset(&m_intel_active_3d_mode, 0, sizeof(m_intel_active_3d_mode));

	// resolution match
	for(int i=0; i<m_intel_caps.ulNumEntries; i++)
	{
		if (m_intel_caps.S3DSupportedModes[i].ulResWidth == current.Width && 
			m_intel_caps.S3DSupportedModes[i].ulResHeight == current.Height)
		{
			m_intel_active_3d_mode.ulResWidth = current.Width;
			m_intel_active_3d_mode.ulResHeight = current.Height;
			break;
		}
	}

	// return on match fail
	if (m_intel_active_3d_mode.ulResWidth == 0)
		return E_RESOLUTION_MISSMATCH;

	// select max refresh rate
	for(int i=0; i<m_intel_caps.ulNumEntries; i++)
	{
		if (m_intel_caps.S3DSupportedModes[i].ulResWidth == current.Width && 
			m_intel_caps.S3DSupportedModes[i].ulResHeight == current.Height)
		{
			m_intel_active_3d_mode.ulRefreshRate = max(m_intel_active_3d_mode.ulRefreshRate, m_intel_caps.S3DSupportedModes[i].ulRefreshRate);
		}

		if (m_intel_caps.S3DSupportedModes[i].ulRefreshRate == current.RefreshRate)
		{
			m_intel_active_3d_mode.ulRefreshRate = current.RefreshRate;
			break;
		}
	}

	// return on match fail
	if (m_intel_active_3d_mode.ulRefreshRate == 0)
		return E_RESOLUTION_MISSMATCH;

	hr = m_intel_s3d->SwitchTo2D(&m_intel_active_3d_mode);
	//m_intel_active_3d_mode.ulRefreshRate = 24;
	hr = m_intel_s3d->SwitchTo3D(&m_intel_active_3d_mode);

	return S_OK;
}

HRESULT my12doomRenderer::intel_d3d_init()
{
	HRESULT hr;

	return S_OK;
}
HRESULT my12doomRenderer::intel_delete_rendertargets()
{
	m_overlay_swap_chain = NULL;
	m_intel_VP_right = NULL;
	m_intel_VP_left = NULL;

	return S_OK;
}
HRESULT my12doomRenderer::intel_create_rendertargets()
{
	if (m_output_mode != intel3d)
		return S_FALSE;

	HRESULT hr;

	// swap chain
	D3DPRESENT_PARAMETERS pp2 = m_active_pp;
	pp2.BackBufferWidth = min(pp2.BackBufferWidth, m_intel_active_3d_mode.ulResWidth);
	pp2.BackBufferHeight = min(pp2.BackBufferHeight, m_intel_active_3d_mode.ulResHeight);
	pp2.SwapEffect = D3DSWAPEFFECT_OVERLAY;
	pp2.MultiSampleType = D3DMULTISAMPLE_NONE;
	FAIL_RET(m_Device->CreateAdditionalSwapChain(&pp2, &m_overlay_swap_chain));

	// some present parameter
	DXVA2_VideoDesc g_VideoDesc;
	memset(&g_VideoDesc, 0, sizeof(g_VideoDesc));
	DXVA2_ExtendedFormat format =   {           // DestFormat
		DXVA2_SampleProgressiveFrame,           // SampleFormat
		DXVA2_VideoChromaSubsampling_MPEG2,     // VideoChromaSubsampling
		DXVA_NominalRange_0_255,                // NominalRange
		DXVA2_VideoTransferMatrix_BT709,        // VideoTransferMatrix
		DXVA2_VideoLighting_bright,             // VideoLighting
		DXVA2_VideoPrimaries_BT709,             // VideoPrimaries
		DXVA2_VideoTransFunc_709                // VideoTransferFunction            
	};
	DXVA2_AYUVSample16 color = {         
		0x8000,          // Cr
		0x8000,          // Cb
		0x1000,          // Y
		0xffff           // Alpha
	};

	// video desc
	memcpy(&g_VideoDesc.SampleFormat, &format, sizeof(DXVA2_ExtendedFormat));
	g_VideoDesc.SampleWidth                         = 0;
	g_VideoDesc.SampleHeight                        = 0;    
	g_VideoDesc.InputSampleFreq.Numerator           = 60;
	g_VideoDesc.InputSampleFreq.Denominator         = 1;
	g_VideoDesc.OutputFrameFreq.Numerator           = 60;
	g_VideoDesc.OutputFrameFreq.Denominator         = 1;

	// blt param
	memset(&m_BltParams, 0, sizeof(m_BltParams));
	m_BltParams.TargetFrame = 0;
	memcpy(&m_BltParams.DestFormat, &format, sizeof(DXVA2_ExtendedFormat));
	memcpy(&m_BltParams.BackgroundColor, &color, sizeof(DXVA2_AYUVSample16));  

	// sample
	m_Sample.Start = 0;
	m_Sample.End = 1;
	m_Sample.SampleFormat = format;
	m_Sample.PlanarAlpha.Fraction = 0;
	m_Sample.PlanarAlpha.Value = 1;   


	// CreateVideoProcessor

	CComPtr<IDirectXVideoProcessorService> g_dxva_service;
	FAIL_RET(myDXVA2CreateVideoService(m_Device, IID_IDirectXVideoProcessorService, (void**)&g_dxva_service));
	FAIL_RET(m_intel_s3d->SetDevice(m_d3d_manager));
	FAIL_RET(m_intel_s3d->SelectLeftView());
	FAIL_RET( g_dxva_service->CreateVideoProcessor(DXVA2_VideoProcProgressiveDevice,
		&g_VideoDesc,
		D3DFMT_X8R8G8B8,
		1,
		&m_intel_VP_left));

	FAIL_RET(m_intel_s3d->SelectRightView());
	FAIL_RET(g_dxva_service->CreateVideoProcessor(DXVA2_VideoProcProgressiveDevice,
		&g_VideoDesc,
		D3DFMT_X8R8G8B8,
		1,
		&m_intel_VP_right));

	return S_OK;
}
HRESULT my12doomRenderer::intel_render_frame(IDirect3DSurface9 *left_surface, IDirect3DSurface9 *right_surface)
{
	if (!m_overlay_swap_chain)
		return E_FAIL;

	CComPtr<IDirect3DSurface9> back_buffer;
	m_overlay_swap_chain->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &back_buffer);

	RECT rect = {0,0,min(m_active_pp.BackBufferWidth, m_intel_active_3d_mode.ulResWidth), min(m_active_pp.BackBufferHeight, m_intel_active_3d_mode.ulResHeight)};

	m_BltParams.TargetRect = rect;
	m_Sample.SrcRect = rect;
	m_Sample.DstRect = rect;
	m_Sample.SrcSurface = left_surface;


	HRESULT hr;
	hr = m_intel_VP_left->VideoProcessBlt(back_buffer, &m_BltParams, &m_Sample, 1, NULL);
	if (FAILED(hr))
		set_device_state(need_resize_back_buffer);

	m_Sample.SrcSurface = right_surface;

	hr = m_intel_VP_right->VideoProcessBlt(back_buffer, &m_BltParams, &m_Sample, 1, NULL);

	// 	g_pd3dDevice->EndScene();
	// 	hr = g_overlay_swap_chain->Present(&dest, &dest, NULL, NULL, D3DPRESENT_UPDATECOLORKEY);


	return S_OK;
}

// AMD HD3D functions
HRESULT my12doomRenderer::HD3D_one_time_init()
{
	m_HD3Dlineoffset = 0;
	m_HD3DStereoModesCount = 0;

	HRESULT hr = m_Device->CreateOffscreenPlainSurface(10, 10, (D3DFORMAT)FOURCC_AQBS, D3DPOOL_DEFAULT, &m_HD3DCommSurface, NULL);
	if( FAILED( hr ) )
	{
		mylog("CreateOffscreenPlainSurface(FOURCC_AQBS) FAILED\r\n");
		goto amd_hd3d_fail;
	}
	else
	{
		mylog("CreateOffscreenPlainSurface(FOURCC_AQBS) OK\r\n");
	}

	// Send the command to the driver using the temporary surface
	// TO enable stereo
	hr = HD3DSendStereoCommand(ATI_STEREO_ENABLESTEREO, NULL, 0, 0, 0);
	if( FAILED( hr ) )
	{
		mylog("SendStereoCommand(ATI_STEREO_ENABLESTEREO) FAILED\r\n");
		m_HD3DCommSurface = NULL;
		goto amd_hd3d_fail;
	}
	else
	{
		mylog("SendStereoCommand(ATI_STEREO_ENABLESTEREO) OK\r\n");
	}


	// See what stereo modes are available
	ATIDX9GETDISPLAYMODES displayModeParams;
	displayModeParams.dwNumModes    = 0;
	displayModeParams.pStereoModes = NULL;

	//Send stereo command to get the number of available stereo modes.
	hr = HD3DSendStereoCommand(ATI_STEREO_GETDISPLAYMODES, (BYTE *)(&displayModeParams),
		sizeof(ATIDX9GETDISPLAYMODES), 0, 0);  
	if( FAILED( hr ) )
	{
		mylog("ATI_STEREO_GETDISPLAYMODES count FAILED\r\n");
		m_HD3DCommSurface = NULL;
		goto amd_hd3d_fail;
	}

	//Send stereo command to get the list of stereo modes
	if(displayModeParams.dwNumModes)
	{
		if (m_HD3DStereoModes)
			delete [] m_HD3DStereoModes;
		displayModeParams.pStereoModes = m_HD3DStereoModes = new D3DDISPLAYMODE[displayModeParams.dwNumModes];

		hr = HD3DSendStereoCommand(ATI_STEREO_GETDISPLAYMODES, (BYTE *)(&displayModeParams), 
			sizeof(ATIDX9GETDISPLAYMODES), 0, 0);
		if( FAILED( hr ) )
		{
			mylog("ATI_STEREO_GETDISPLAYMODES modes FAILED\r\n");

			m_HD3DCommSurface = NULL;
			goto amd_hd3d_fail;
		}
		m_HD3DStereoModesCount = displayModeParams.dwNumModes;
	}

	for(int i=0; i<m_HD3DStereoModesCount; i++)
	{
		D3DDISPLAYMODE mode = m_HD3DStereoModes[i];
		mylog("Stereo Mode: %dx%d@%dfps, %dformat\r\n", mode.Width, mode.Height, mode.RefreshRate, mode.Format);
	}

	m_HD3DCommSurface = NULL;
	return S_OK;

amd_hd3d_fail:
	m_HD3DCommSurface = NULL;
	return S_FALSE;		// yes, always success, 
}

HRESULT my12doomRenderer::HD3D_restore_objects()
{
	mylog("1");

	m_HD3Dlineoffset = 0;
	HRESULT hr = m_Device->CreateOffscreenPlainSurface(10, 10, (D3DFORMAT)FOURCC_AQBS, D3DPOOL_DEFAULT, &m_HD3DCommSurface, NULL);
	if( FAILED( hr ) )
	{
		mylog("FOURCC_AQBS FAIL");
		return S_FALSE;
	}

	mylog("2");
	//Retrieve the line offset
	hr = HD3DSendStereoCommand(ATI_STEREO_GETLINEOFFSET, (BYTE *)(&m_HD3Dlineoffset), sizeof(DWORD), 0, 0);
	if( FAILED( hr ) )
	{
		mylog("ATI_STEREO_GETLINEOFFSET FAIL=%d\r\n", m_HD3Dlineoffset);
		return S_FALSE;
	}
	mylog("3");

	// see if lineOffset is valid
	mylog("lineoffset=%d\r\n", m_HD3Dlineoffset);

	return S_OK;
}

HRESULT my12doomRenderer::HD3D_invalidate_objects()
{
	mylog("m_HD3DCommSurface = NULL");
	m_HD3DCommSurface = NULL;
	return S_OK;
}

HRESULT my12doomRenderer::HD3D_set_prefered_mode(D3DDISPLAYMODE mode)
{
	m_prefered_mode = mode;

	return S_OK;
}


HRESULT my12doomRenderer::HD3DSetStereoFullscreenPresentParameters()
{
	D3DDISPLAYMODE current_mode;
	m_D3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &current_mode);

	// use prefered mode
	int res_x = 0;
	int res_y = 0;
	int refresh = 0;
	D3DFORMAT fmt = D3DFMT_UNKNOWN;
	for(int i=0; i<m_HD3DStereoModesCount; i++)
	{
		D3DDISPLAYMODE this_mode = m_HD3DStereoModes[i];
		if (this_mode.Width == m_prefered_mode.Width && this_mode.Height == m_prefered_mode.Height && this_mode.RefreshRate == m_prefered_mode.RefreshRate && this_mode.Format == m_prefered_mode.Format)
		{
			res_x = this_mode.Width;
			res_y = this_mode.Height;
			refresh = this_mode.RefreshRate;
			fmt = this_mode.Format;
			break;
		}
	}


	// if not prefered mode
	if (res_x == 0 || res_y == 0)
	{
		// find direct resolution match
		for(int i=0; i<m_HD3DStereoModesCount; i++)
		{
			D3DDISPLAYMODE this_mode = m_HD3DStereoModes[i];
			if (this_mode.Width == current_mode.Width && this_mode.Height == current_mode.Height)
			{
				res_x = this_mode.Width;
				res_y = this_mode.Height;
				break;
			}
		}

		// if no match, then we choose highest mode
		for(int i=0; i<m_HD3DStereoModesCount; i++)
		{
			D3DDISPLAYMODE this_mode = m_HD3DStereoModes[i];
			if (this_mode.Width * this_mode.Height > res_x*res_y)
			{
				res_x = this_mode.Width;
				res_y = this_mode.Height;
			}
		}
	}

	if (refresh == 0)
	{
		// find direct matched refresh rate
		for(int i=0; i<m_HD3DStereoModesCount; i++)
		{
			D3DDISPLAYMODE this_mode = m_HD3DStereoModes[i];
			if (this_mode.Width == res_x && this_mode.Height == res_y && this_mode.RefreshRate == current_mode.RefreshRate)
			{
				refresh = this_mode.RefreshRate;
				break;
			}
		}
		// no match ? use highest refresh rate
		for(int i=0; i<m_HD3DStereoModesCount; i++)
		{
			D3DDISPLAYMODE this_mode = m_HD3DStereoModes[i];
			if (this_mode.Width == res_x && this_mode.Height == res_y)
				refresh = max(refresh, this_mode.RefreshRate);
		}
	}

	if (fmt == D3DFMT_UNKNOWN)
		fmt = D3DFMT_X8R8G8B8;

	m_new_pp.MultiSampleType = D3DMULTISAMPLE_2_SAMPLES;
	m_new_pp.FullScreen_RefreshRateInHz = refresh;
	m_new_pp.BackBufferWidth = res_x;
	m_new_pp.BackBufferHeight = res_y;
	m_new_pp.BackBufferFormat = fmt;

	return S_OK;
}


HRESULT my12doomRenderer::HD3DMatchResolution()
{
	D3DDISPLAYMODE current_mode;
	m_D3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &current_mode);

	if (m_HD3DStereoModesCount <= 0)
		return E_NOINTERFACE;

	for(int i=0; i<m_HD3DStereoModesCount; i++)
	{
		D3DDISPLAYMODE this_mode = m_HD3DStereoModes[i];
		if (this_mode.Width == current_mode.Width && this_mode.Height == current_mode.Height)
			return S_OK;
	}

	return S_FALSE;
}

HRESULT my12doomRenderer::HD3DGetAvailable3DModes(D3DDISPLAYMODE *modes, int *count)
{
	if (NULL == count)
		return E_POINTER;

	if( m_HD3DStereoModesCount <= 0 )
	{
		*count = 0;
		return S_OK;
	}

	for(int i=0; i<m_HD3DStereoModesCount && i<*count; i++)
		modes[i] = m_HD3DStereoModes[i];

	*count = m_HD3DStereoModesCount;
	return S_OK;
}

HRESULT my12doomRenderer::HD3DSendStereoCommand(ATIDX9STEREOCOMMAND stereoCommand, BYTE *pOutBuffer, 
							 DWORD dwOutBufferSize, BYTE *pInBuffer, 
							 DWORD dwInBufferSize)
{
	if (!m_HD3DCommSurface)
		return E_FAIL;

	HRESULT hr;
	ATIDX9STEREOCOMMPACKET *pCommPacket;
	D3DLOCKED_RECT lockedRect;

	hr = m_HD3DCommSurface->LockRect(&lockedRect, 0, 0);
	if(FAILED(hr))
	{
		return hr;
	}
	pCommPacket = (ATIDX9STEREOCOMMPACKET *)(lockedRect.pBits);
	pCommPacket->dwSignature = 'STER';
	pCommPacket->pResult = &hr;
	pCommPacket->stereoCommand = stereoCommand;
	if (pOutBuffer && !dwOutBufferSize)
	{
		return hr;
	}
	pCommPacket->pOutBuffer = pOutBuffer;
	pCommPacket->dwOutBufferSize = dwOutBufferSize;
	if (pInBuffer && !dwInBufferSize)
	{
		return hr;
	}
	pCommPacket->pInBuffer = pInBuffer;
	pCommPacket->dwInBufferSize = dwInBufferSize;
	m_HD3DCommSurface->UnlockRect();
	return hr;
}

HRESULT my12doomRenderer::HD3DDrawStereo(IDirect3DSurface9 *left_surface, IDirect3DSurface9 *right_surface, IDirect3DSurface9 *back_buffer)
{
	m_Device->SetRenderTarget(0, back_buffer);

	// draw left
	HRESULT hr = m_Device->StretchRect(left_surface, NULL, back_buffer, NULL, D3DTEXF_LINEAR);

	// update the quad buffer with the right render target
	D3DVIEWPORT9 viewPort;
	viewPort.X = 0;
	viewPort.Y = m_HD3Dlineoffset;
	viewPort.Width = m_active_pp.BackBufferWidth;
	viewPort.Height = m_active_pp.BackBufferHeight;
	viewPort.MinZ = 0;
	viewPort.MaxZ = 1;
	hr = m_Device->SetViewport(&viewPort);

	mylog("lineoffset = %d, right_surface = %08x\r\n, hr = %08x", m_HD3Dlineoffset, right_surface, hr);


	// set the right quad buffer as the destination for StretchRect
	DWORD dwEye = ATI_STEREO_RIGHTEYE;
	HD3DSendStereoCommand(ATI_STEREO_SETDSTEYE, NULL, 0, (BYTE *)&dwEye, sizeof(dwEye));
	m_Device->StretchRect(right_surface, NULL, back_buffer, NULL, D3DTEXF_LINEAR);

	// restore the destination
	dwEye = ATI_STEREO_LEFTEYE;
	HD3DSendStereoCommand(ATI_STEREO_SETDSTEYE, NULL, 0, (BYTE *)&dwEye, sizeof(dwEye));

	// RenderPixelID();
	struct Vertex
	{
		FLOAT		x, y, z, w;
		D3DCOLOR	diffuse;
	};

	Vertex verts[2];

	verts[0].x = 0;
	verts[0].z = 0.5f;
	verts[0].w = 1;
	verts[1].x = 1;
	verts[1].z = 0.5f;
	verts[1].w = 1;

	viewPort.X = 0;
	viewPort.Width = m_active_pp.BackBufferWidth;
	viewPort.Height = m_active_pp.BackBufferHeight;
	viewPort.MinZ = 0;
	viewPort.MaxZ = 1;

	IDirect3DDevice9 *pd3dDevice = m_Device;

	// save state
	DWORD zEnable;
	pd3dDevice->GetRenderState(D3DRS_ZENABLE, &zEnable);

	pd3dDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
	D3DMATRIX identityMatrix;
	//D3DXMatrixIdentity
	myMatrixIdentity(&identityMatrix);
	pd3dDevice->SetTransform(D3DTS_VIEW, &identityMatrix);
	pd3dDevice->SetTransform(D3DTS_WORLD, &identityMatrix);
	D3DMATRIX orthoMatrix;
	//D3DXMatrixOrthoRH;
	myMatrixOrthoRH(&orthoMatrix, (float)m_active_pp.BackBufferWidth, (float)m_active_pp.BackBufferHeight, 0.0, 1.0f);
	pd3dDevice->SetTransform(D3DTS_PROJECTION, &orthoMatrix);
	pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
	float pointSize = 1.0f;
	//pd3dDevice->SetRenderTarget(0, pBackBufferSurface);
	pd3dDevice->SetRenderState(D3DRS_POINTSIZE, *((DWORD*)&pointSize));
	pd3dDevice->SetRenderState(D3DRS_POINTSCALEENABLE, FALSE);
	pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
	pd3dDevice->SetVertexShader(NULL);
	pd3dDevice->SetPixelShader(NULL);

	// draw right eye
	viewPort.Y = m_HD3Dlineoffset;
	verts[0].y = (float)m_HD3Dlineoffset;
	verts[1].y = (float)m_HD3Dlineoffset;
	pd3dDevice->SetViewport(&viewPort);
	verts[0].diffuse = D3DCOLOR D3DCOLOR_ARGB(255, 0, 0, 0);
	verts[1].diffuse = D3DCOLOR D3DCOLOR_ARGB(255, 255, 255, 255);
	pd3dDevice->DrawPrimitiveUP(D3DPT_POINTLIST, 2, verts, sizeof(Vertex));

	// draw left eye
	viewPort.Y = 0;
	pd3dDevice->SetViewport(&viewPort);
	verts[0].y = 0;
	verts[1].y = 0;
	verts[0].diffuse = D3DCOLOR D3DCOLOR_ARGB(255, 255, 255, 255);
	verts[1].diffuse = D3DCOLOR D3DCOLOR_ARGB(255, 0, 0, 0);
	pd3dDevice->DrawPrimitiveUP(D3DPT_POINTLIST, 2, verts, sizeof(Vertex));

	// restore state
	pd3dDevice->SetRenderState(D3DRS_ZENABLE, zEnable);
	return S_OK;
}

HRESULT my12doomRenderer::set_ui_drawer(ui_drawer_base * new_ui_drawer)
{
	CAutoLock lck2(&m_frame_lock);
	CAutoLock lck(&m_uidrawer_cs);
	if (NULL != m_uidrawer)
	{
		m_uidrawer->invalidate_gpu();
		m_uidrawer->invalidate_cpu();
	}

	RECT tar = {0,0, m_active_pp.BackBufferWidth, m_active_pp.BackBufferHeight};
	if (m_output_mode == out_sbs)
		tar.right /= 2;
	else if (m_output_mode == out_tb)
		tar.bottom /= 2;

	m_uidrawer = new_ui_drawer;

	if(m_Device && m_uidrawer )
	{
		m_uidrawer->init_cpu(m_Device);
		set_device_state(need_reset_object);
	}
	return S_OK;
}

ui_drawer_base *my12doomRenderer::get_ui_drawer()
{
	CAutoLock lck(&m_uidrawer_cs);
	return m_uidrawer;
}


// GetService: Returns the IDirect3DDeviceManager9 interface.
// (The signature is identical to IMFGetService::GetService but 
// this object does not derive from IUnknown.)
HRESULT my12doomRenderer::GetService(REFGUID guidService, REFIID riid, void** ppv)
{

	assert(ppv != NULL);

	HRESULT hr = S_OK;



	if (riid == __uuidof(IDirect3DDeviceManager9))
	{
		if (m_d3d_manager == NULL)
		{
			hr = MF_E_UNSUPPORTED_SERVICE;
		}
		else
		{
			*ppv = m_d3d_manager;
			((IDirect3DDeviceManager9*)m_d3d_manager)->AddRef();
		}
	}
	else if (riid == __uuidof(IDirectXVideoDecoderService) || riid == __uuidof(IDirectXVideoProcessorService) ) 
	{
		return m_d3d_manager->GetVideoService (m_device_handle, riid, ppv);
	} else if (riid == __uuidof(IDirectXVideoAccelerationService)) {
		// TODO : to be tested....
		return myDXVA2CreateVideoService(m_Device, riid, ppv);
	} else if (riid == __uuidof(IDirectXVideoMemoryConfiguration)) {
		GetInterface((IDirectXVideoMemoryConfiguration*)this, ppv);
		return S_OK;
	}
	else
	{
		hr = MF_E_UNSUPPORTED_SERVICE;
	}

	return hr;
}
HRESULT my12doomRenderer::CheckFormat(D3DFORMAT format)
{
	HRESULT hr = S_OK;

	UINT uAdapter = D3DADAPTER_DEFAULT;
	D3DDEVTYPE type = D3DDEVTYPE_HAL;

	D3DDISPLAYMODE mode;
	D3DDEVICE_CREATION_PARAMETERS params;

	if (m_Device)
	{
		CHECK_HR(hr = m_Device->GetCreationParameters(&params));

		uAdapter = params.AdapterOrdinal;
		type = params.DeviceType;

	}

	CHECK_HR(hr = m_D3D->GetAdapterDisplayMode(uAdapter, &mode));

	CHECK_HR(hr = m_D3D->CheckDeviceType(uAdapter, type, mode.Format, format, TRUE)); 

done:
	return hr;
}

// Video window / destination rectangle:
// This object implements a sub-set of the functions defined by the 
// IMFVideoDisplayControl interface. However, some of the method signatures 
// are different. The presenter's implementation of IMFVideoDisplayControl 
// calls these methods.
HRESULT my12doomRenderer::SetVideoWindow(HWND hwnd)
{
	// do nothing!
	return S_OK;
}
HRESULT my12doomRenderer::SetDestinationRect(const RECT& rcDest)
{
	// do nothing!
	return S_OK;
}
void    my12doomRenderer::ReleaseResources()
{

}

HRESULT my12doomRenderer::CreateVideoSamples(IMFMediaType *pFormat, VideoSampleList& videoSampleQueue)
{
	if (m_hWnd == NULL)
	{
		return MF_E_INVALIDREQUEST;
	}

	if (pFormat == NULL)
	{
		return MF_E_UNEXPECTED;
	}

	HRESULT hr = S_OK;
	D3DPRESENT_PARAMETERS pp;

	IDirect3DSurface9 *pSurface = NULL;    // Swap chain
	IMFSample *pVideoSample = NULL;            // Sampl

	AutoLock lock(m_ObjectLock);

	ReleaseResources();

	// Get the swap chain parameters from the media type.
	CHECK_HR(hr = GetSwapChainPresentParameters(pFormat, &pp));

	//UpdateDestRect();

	// Create the video samples.
	for (int i = 0; i < (int)EVRQueueSize; i++)
	{
		// Create a new swap chain.
		CHECK_HR(hr = m_Device->CreateRenderTarget(pp.BackBufferWidth, pp.BackBufferHeight, pp.BackBufferFormat, D3DMULTISAMPLE_NONE, 0, FALSE, &pSurface, NULL));

		// Create the video sample from the swap chain.
		CHECK_HR(hr = CreateD3DSample(pSurface, &pVideoSample));

		// Add it to the list.
		CHECK_HR(hr = videoSampleQueue.InsertBack(pVideoSample));

		// Set the swap chain pointer as a custom attribute on the sample. This keeps
		// a reference count on the swap chain, so that the swap chain is kept alive
		// for the duration of the sample's lifetime.
		CHECK_HR(hr = pVideoSample->SetUnknown(MFSamplePresenter_SampleSwapChain, pSurface));
		pSurface->AddRef();

		SAFE_RELEASE(pVideoSample);
		SAFE_RELEASE(pSurface);
	}

	// Let the derived class create any additional D3D resources that it needs.
// 	CHECK_HR(hr = OnCreateVideoSamples(pp));

done:
	if (FAILED(hr))
	{
		ReleaseResources();
	}

	SAFE_RELEASE(pSurface);
	SAFE_RELEASE(pVideoSample);
	return hr;
}


HRESULT my12doomRenderer::CheckDeviceState(ID3DPresentEngine::DeviceState *pState)
{
	HRESULT hr = S_OK;

	AutoLock lock(m_ObjectLock);

	// Check the device state. Not every failure code is a critical failure.
	hr = m_DeviceEx->CheckDeviceState(m_hWnd);

	*pState = ID3DPresentEngine::DeviceOK;

	switch (hr)
	{
	case S_OK:
	case S_PRESENT_OCCLUDED:
	case S_PRESENT_MODE_CHANGED:
		// state is DeviceOK
		hr = S_OK;
		break;

	case D3DERR_DEVICELOST:
	case D3DERR_DEVICEHUNG:
		// Lost/hung device. Destroy the device and create a new one.
// 		CHECK_HR(hr = CreateD3DDevice());
		*pState = ID3DPresentEngine::DeviceReset;
		hr = S_OK;
		break;

	case D3DERR_DEVICEREMOVED:
		// This is a fatal error.
		*pState = ID3DPresentEngine::DeviceRemoved;
		break;

	case E_INVALIDARG:
		// CheckDeviceState can return E_INVALIDARG if the window is not valid
		// We'll assume that the window was destroyed; we'll recreate the device 
		// if the application sets a new window.
		hr = S_OK;
	}

done:
	return hr;
}
HRESULT my12doomRenderer::PrerollSample(IMFSample* pSample, LONGLONG llTarget, int id)
{
	if (m_cb)
		m_cb->PrerollCB(llTarget + g_tSegmentStart, llTarget + 1 + g_tSegmentStart, NULL, id-1);

	return S_OK;
}

HRESULT my12doomRenderer::PresentSample(IMFSample* pSample, LONGLONG llTarget, int id)
{
	if (m_cb)
		m_cb->SampleCB(llTarget + g_tSegmentStart, llTarget + 1 + g_tSegmentStart, NULL, id-1);

	HRESULT hr = S_OK;

	IMFMediaBuffer* pBuffer = NULL;
	IDirect3DSurface9* pSurface = NULL;
	//     IDirect3DSwapChain9* pSwapChain = NULL;
	IDirect3DSurface9 * back_buffer = NULL;

	if (pSample)
	{
		// Get the buffer from the sample.
		CHECK_HR(hr = pSample->GetBufferByIndex(0, &pBuffer));

		// Get the surface from the buffer.
		CHECK_HR(hr = myMFGetService(pBuffer, MR_BUFFER_SERVICE, __uuidof(IDirect3DSurface9), (void**)&pSurface));
	}

	if (pSurface)
	{
		D3DSURFACE_DESC desc;
		pSurface->GetDesc(&desc);
		m_lVidWidth = desc.Width;
		m_lVidHeight = desc.Height;
		m_source_aspect = (double)m_lVidWidth / m_lVidHeight;

		CAutoLock pool_lock(&m_pool_lock);
		CAutoLock rendered_lock(&m_packet_lock);
		safe_delete(m_sample2render_1);
 		m_sample2render_1 = new gpu_sample(m_Device, pSurface, m_pool);

		render(true);

	}
	else
	{
		// No surface. All we can do is paint a black rectangle.
// 		PaintFrameWithGDI();
	}

done:
	//     SAFE_RELEASE(pSwapChain);
	SAFE_RELEASE(pSurface);
	SAFE_RELEASE(pBuffer);
	SAFE_RELEASE(back_buffer);

	if (FAILED(hr))
	{
		if (hr == D3DERR_DEVICELOST || hr == D3DERR_DEVICENOTRESET || hr == D3DERR_DEVICEHUNG)
		{
			// We failed because the device was lost. Fill the destination rectangle.
// 			PaintFrameWithGDI();

			// Ignore. We need to reset or re-create the device, but this method
			// is probably being called from the scheduler thread, which is not the
			// same thread that created the device. The Reset(Ex) method must be
			// called from the thread that created the device.

			// The presenter will detect the state when it calls CheckDeviceState() 
			// on the next sample.
			hr = S_OK;
		}
	}
	return hr;
	return S_OK;
}
UINT my12doomRenderer::RefreshRate()
{
	return 24;
}
RECT my12doomRenderer::GetDestinationRect()
{
	RECT r = {0,0,1920,1080};
	return r;
}
HWND my12doomRenderer::GetVideoWindow()
{
	return m_hWnd;
}
HRESULT my12doomRenderer::GetSwapChainPresentParameters(IMFMediaType *pType, D3DPRESENT_PARAMETERS* pPP)
{
	// Caller holds the object lock.

	HRESULT hr = S_OK; 

	UINT32 width = 0, height = 0;
	DWORD d3dFormat = 0;

	// Helper object for reading the proposed type.
	VideoType videoType(pType);

	if (m_hWnd == NULL)
	{
		return MF_E_INVALIDREQUEST;
	}

	ZeroMemory(pPP, sizeof(D3DPRESENT_PARAMETERS));

	// Get some information about the video format.
	CHECK_HR(hr = videoType.GetFrameDimensions(&width, &height));
	CHECK_HR(hr = videoType.GetFourCC(&d3dFormat));

	ZeroMemory(pPP, sizeof(D3DPRESENT_PARAMETERS));
	pPP->BackBufferWidth = width;
	pPP->BackBufferHeight = height;
	pPP->Windowed = TRUE;
	pPP->SwapEffect = D3DSWAPEFFECT_COPY;
	pPP->BackBufferFormat = (D3DFORMAT)d3dFormat;
	pPP->hDeviceWindow = m_hWnd;
	pPP->Flags = D3DPRESENTFLAG_VIDEO;
	pPP->PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;

	D3DDEVICE_CREATION_PARAMETERS params;
	CHECK_HR(hr = m_Device->GetCreationParameters(&params));

	if (params.DeviceType != D3DDEVTYPE_HAL)
	{
		pPP->Flags |= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
	}

done:
	return S_OK;
}
HRESULT my12doomRenderer::CreateD3DSample(IDirect3DSurface9 *pSurface, IMFSample **ppVideoSample)
{
	// Caller holds the object lock.

	HRESULT hr = S_OK;
	D3DCOLOR clrBlack = D3DCOLOR_ARGB(0xFF, 0x00, 0x00, 0x00);

	IMFSample* pSample = NULL;

	// Fill it with black.
	CHECK_HR(hr = m_Device->ColorFill(pSurface, NULL, clrBlack));

	// Create the sample.
	CHECK_HR(hr = myMFCreateVideoSampleFromSurface(pSurface, &pSample));

	// Return the pointer to the caller.
	*ppVideoSample = pSample;
	(*ppVideoSample)->AddRef();

done:
	SAFE_RELEASE(pSurface);
	SAFE_RELEASE(pSample);
	return hr;
}



// helper functions

HRESULT myMatrixIdentity(D3DMATRIX *matrix)
{
	if (!matrix)
		return E_POINTER;

	memset(matrix, 0 , sizeof(D3DMATRIX));
	matrix->_11 =
	matrix->_22 =
	matrix->_33 =
	matrix->_44 = 1.0f;

	return S_OK;
}

HRESULT myMatrixOrthoRH(D3DMATRIX *matrix, float w, float h, float zn, float zf)
{
	if (!matrix)
		return E_POINTER;


	memset(matrix, 0 , sizeof(D3DMATRIX));
	matrix->_11 = 2.0f / w;
	matrix->_22 = 2.0f / h;
	matrix->_33 = 1 / (zn-zf);
	matrix->_43 = zn/ (zn-zf);
	matrix->_44 = .0f;

	return S_OK;
}

IDirect3DTexture9* helper_get_texture(gpu_sample *sample, helper_sample_format format)
{
	if (!sample)
		return NULL;

	CPooledTexture *texture = NULL;

	if (format == helper_sample_format_rgb32) texture = sample->m_tex_gpu_RGB32;
	if (format == helper_sample_format_y) texture = sample->m_tex_gpu_Y;
	if (format == helper_sample_format_yv12) texture = sample->m_tex_gpu_YV12_UV;
	if (format == helper_sample_format_nv12) texture = sample->m_tex_gpu_NV12_UV;
	if (format == helper_sample_format_yuy2) texture = sample->m_tex_gpu_YUY2_UV;

	if (!texture)
		return NULL;

	return texture->texture;
}

void SetThreadName( DWORD dwThreadID, LPCSTR szThreadName)
{
	typedef struct tagTHREADNAME_INFO {
		DWORD dwType; // must be 0x1000
		LPCSTR szName; // pointer to name (in user addr space)
		DWORD dwThreadID; // thread ID (-1=caller thread)
		DWORD dwFlags; // reserved for future use, must be zero
	} THREADNAME_INFO;

	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = szThreadName;
	info.dwThreadID = dwThreadID;
	info.dwFlags = 0;

	__try {
		RaiseException( 0x406D1388, 0, sizeof(info)/sizeof(DWORD), (ULONG_PTR*)&info );
	}
	__except(EXCEPTION_CONTINUE_EXECUTION) {
	}
}

float frac(float f)
{
	double tmp;
	return modf(f, &tmp);
}

RECT ClipRect(const RECT border, const RECT rect2clip)
{
	RECT t = rect2clip;

	t.left = max(border.left, t.left);
	t.right = min(border.right, t.right);
	t.top = max(border.top, t.top);
	t.bottom = min(border.bottom, t.bottom);

	return t;
}

RECTF ClipRect(const RECTF border, const RECTF rect2clip)
{
	RECTF t = rect2clip;

	t.left = max(border.left, t.left);
	t.right = min(border.right, t.right);
	t.top = max(border.top, t.top);
	t.bottom = min(border.bottom, t.bottom);

	return t;
}

// helper classes

Direct3DDeviceManagerHelper::Direct3DDeviceManagerHelper(IDirect3DDevice9 *fallback, IDirect3DDeviceManager9 *manager, HANDLE device_handle)
{
	if (!manager)
		m_device = fallback;
	else
	{
		m_manger = manager;
		m_device_handle = device_handle;

		m_manger->LockDevice(device_handle, &m_device, TRUE);
	}
}

Direct3DDeviceManagerHelper::~Direct3DDeviceManagerHelper()
{
	if (m_manger)
		m_manger->UnlockDevice(m_device_handle, FALSE);
}


