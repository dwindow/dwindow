#pragma once
#include <d3d9.h>
#include <streams.h>
#include <atlbase.h>
#include "..\AESFile\rijndael.h"
#include "..\dwindow\nvapi.h"
#include "AtiDx9Stereo.h"
#include "..\dwindow\igfx_s3dcontrol.h"

#include "my12doomUI.h"
#include "TextureAllocator.h"
#include "my12doomRendererTypes.h"
#include "my12doomAutoShader.h"
#include "gpu_sample.h"
#include "evr\EVRPresenter.h"
#include "..\dwindow\global_funcs.h"

void SetThreadName( DWORD dwThreadID, LPCSTR szThreadName);

// this renderer must have a valid first window, if not, connection will fail.
// setting window to invalid window during 
// you can enter dual projector mode without a second window, but you won't get second image until you set a second window

class Imy12doomRendererCallback
{
public:
	virtual HRESULT SampleCB(REFERENCE_TIME TimeStart, REFERENCE_TIME TimeEnd, IMediaSample *pIn, int stream_id) = 0;
	virtual HRESULT PrerollCB(REFERENCE_TIME TimeStart, REFERENCE_TIME TimeEnd, IMediaSample *pIn, int stream_id) = 0;
};

class my12doomRenderer;
class DRendererInputPin : public CRendererInputPin
{
public:
	DRendererInputPin(__inout CBaseRenderer *pRenderer,	__inout HRESULT *phr, __in_opt LPCWSTR Name) : CRendererInputPin(pRenderer, phr, Name){}
	STDMETHODIMP NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
	STDMETHODIMP BeginFlush(void);
	STDMETHODIMP GetAllocator(__deref_out IMemAllocator ** ppAllocator)
	{
		return __super::GetAllocator(ppAllocator);
	}

	STDMETHODIMP NotifyAllocator(
		IMemAllocator * pAllocator,
		BOOL bReadOnly)
	{
		return __super::NotifyAllocator(pAllocator, bReadOnly);
	}
protected:
	friend class my12doomRenderer;
};



class DBaseVideoRenderer: public CBaseVideoRenderer
{
public:
	DBaseVideoRenderer(REFCLSID clsid,LPCTSTR name , LPUNKNOWN pUnk,HRESULT *phr)
		: CBaseVideoRenderer(clsid, name, pUnk, phr){};
	virtual CBasePin * GetPin(int n);
	virtual HRESULT NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
	virtual HRESULT BeginFlush(void);
	virtual HRESULT OnStartStreaming()
	{
		return __super::OnStartStreaming();
	}

protected:
	friend class DRendererInputPin;
	REFERENCE_TIME m_thisstream;
};




class my12doomRendererDShow : public DBaseVideoRenderer
{
public:
	my12doomRendererDShow(LPUNKNOWN pUnk,HRESULT *phr, my12doomRenderer *owner, int id);
	~my12doomRendererDShow();

	bool is_connected();
	HRESULT set_interlace_mode(interlace_types mode){m_interlace_mode = mode;}
	interlace_types get_interlace_mode(){return m_interlace_mode;}

protected:
	friend class my12doomRenderer;
	HRESULT drop_packets(int num = -1);		//...just for invalidGPUObjects lost...
	HRESULT set_queue_size(int num){m_queue_size = min(num, my12doom_queue_size); return S_OK; }

	// dshow functions
	HRESULT CheckMediaType(const CMediaType *pmt );
	HRESULT SetMediaType(const CMediaType *pmt );
	HRESULT DoRenderSample(IMediaSample *pMediaSample);
	void OnReceiveFirstSample(IMediaSample *pMediaSample);
	HRESULT	BreakConnect();
	HRESULT CompleteConnect(IPin *pRecievePin);
	HRESULT NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
	HRESULT Receive(IMediaSample *sample);
	HRESULT SuperReceive(IMediaSample *sample){return __super::Receive(sample);}
	HRESULT ShouldDrawSampleNow(IMediaSample *pMediaSample,
		__inout REFERENCE_TIME *ptrStart,
		__inout REFERENCE_TIME *ptrEnd);			// override to send Quality Message only when queue empty
	HRESULT ShouldDrawSampleNow(gpu_sample *pMediaSample);		//mine

	// interlace variables;
	interlace_types m_interlace_mode;


	// dshow variables
	REFERENCE_TIME m_time;
	GUID m_format;
	GUID m_formattype;

	// queue
	static DWORD WINAPI queue_thread(LPVOID param);
	CCritSec m_queue_lock;
	dummy_packet m_queue[my12doom_queue_size];
	int m_queue_size;
	int m_queue_count;
	bool m_queue_exit;
	CComPtr<IMemAllocator> m_allocator;
	HANDLE m_queue_thread;

	// variables for contact to owner
	my12doomRenderer *m_owner;
	int m_id;
};



#include <map>
typedef struct
{
	IDirect3DDevice9 * device;
	int counter;
} thread_map_entry;
class Direct3DDeviceManagerHelper
{
public:
	Direct3DDeviceManagerHelper(IDirect3DDevice9 *fallback, IDirect3DDeviceManager9 *manager, HANDLE device_handle);
	~Direct3DDeviceManagerHelper();

	operator IDirect3DDevice9* (){return m_device;}
	IDirect3DDevice9* operator->(){return m_device;}
protected:
	CComPtr<IDirect3DDeviceManager9> m_manger;
	HANDLE m_device_handle;
	CComPtr<IDirect3DDevice9> m_device;
};
#define MANAGE_DEVICE Direct3DDeviceManagerHelper device(m_Device, m_d3d_manager, m_device_handle)



class my12doomRenderer : public ID3DPresentEngine
{
public:

	// GetService: Returns the IDirect3DDeviceManager9 interface.
	// (The signature is identical to IMFGetService::GetService but 
	// this object does not derive from IUnknown.)
	virtual HRESULT GetService(REFGUID guidService, REFIID riid, void** ppv);
	virtual HRESULT CheckFormat(D3DFORMAT format);

	// Video window / destination rectangle:
	// This object implements a sub-set of the functions defined by the 
	// IMFVideoDisplayControl interface. However, some of the method signatures 
	// are different. The presenter's implementation of IMFVideoDisplayControl 
	// calls these methods.
	HRESULT SetVideoWindow(HWND hwnd);
	HRESULT SetDestinationRect(const RECT& rcDest);
	void    ReleaseResources();

	HRESULT CreateVideoSamples(IMFMediaType *pFormat, VideoSampleList& videoSampleQueue);
	HRESULT GetSwapChainPresentParameters(IMFMediaType *pType, D3DPRESENT_PARAMETERS* pPP);


	HRESULT CheckDeviceState(ID3DPresentEngine::DeviceState *pState);
	HRESULT PresentSample(IMFSample* pSample, LONGLONG llTarget, int id); 
	HRESULT PrerollSample(IMFSample* pSample, LONGLONG llTarget, int id); 
	UINT    RefreshRate();
	RECT    GetDestinationRect();
	HWND    GetVideoWindow();

protected:
	CritSec                     m_ObjectLock;           // Thread lock for the D3D device.
	HRESULT CreateD3DSample(IDirect3DSurface9 *pSurface, IMFSample **ppVideoSample);

public:
	my12doomRenderer(HWND hwnd, HWND hwnd2 = NULL);
	~my12doomRenderer();
	CComPtr<IBaseFilter> m_dshow_renderer1;
	CComPtr<IBaseFilter> m_dshow_renderer2;
	AESCryptor m_AES;
	unsigned char m_key[32];
	REFERENCE_TIME m_frame_length;
	REFERENCE_TIME m_first_PTS;
	bool m_deinterlace;
	bool m_forced_deinterlace;
	bool m_remux_mode;

	// dx9 device state test
	HRESULT test_device_state();		// S_OK : all fine, S_FALSE : need reset/resize, but can continue rendering, E_FAIL: can't render at all

	// color adjust controll
	lua_const &m_saturation;
	lua_const &m_luminance;
	lua_const &m_hue;
	lua_const &m_contrast;

	lua_const &m_saturation2;
	lua_const &m_luminance2;
	lua_const &m_hue2;
	lua_const &m_contrast2;
	CCritSec m_frame_lock;

	// public functions
	HRESULT pump();
	HRESULT repaint_video();
	HRESULT NV3D_notify(WPARAM wparam);
	HRESULT reset();
	int hittest(int x, int y, double*outv){CAutoLock lck(&m_uidrawer_cs);int o = -1; if(m_uidrawer) m_uidrawer->hittest(x, y, &o, outv); return o;}
	HRESULT screenshot(const wchar_t*file);	// movie only
	HRESULT screenshot(void *Y, void*U, void*V, int stride, int width, int height);	// movie only
	HRESULT get_movie_desc(int *width, int*height);
	bool is2DMovie();
	bool is2DRendering();


	// settings SET function
	HRESULT set_subtitle_resizing(resampling_method method);
	HRESULT set_ui_drawer(ui_drawer_base * new_ui_drawer);
	HRESULT set_input_layout(int layout);
	HRESULT set_output_mode(int mode);
	HRESULT set_mask_mode(int mode);
	HRESULT set_mask_parameter(int parameter);
	HRESULT set_mask_color(int id, DWORD color);
	HRESULT set_swap_eyes(bool swap);
	HRESULT set_force_2d(bool force2d);
	HRESULT set_fullscreen(bool full);
	HRESULT set_movie_pos(int dimention, double offset);		// dimention1 = x, dimention2 = y, dimention3 = x in pixel, dimention4 = y in pixel
	HRESULT set_aspect(double aspect);
	HRESULT set_window(HWND wnd, HWND wnd2);
	HRESULT set_movie_scissor_rect(RECTF *scissor);
	HRESULT set_subtitle(void* data, int width, int height, float fwidth, float fheight, float fleft, float ftop, bool gpu_shadow = false);
	HRESULT set_subtitle_parallax(double offset);
	HRESULT set_parallax(double parallax);
	HRESULT set_callback(Imy12doomRendererCallback *cb){m_cb = cb; return S_OK;}
	HRESULT set_2dto3d(bool convert){m_convert3d = convert;}
	HRESULT set_aspect_mode(int mode);
	HRESULT set_display_orientation(int orientation){m_display_orientation = (display_orientation)orientation; return S_OK;}
	HRESULT set_vsync(bool on);
	HRESULT set_zoom_factor(float factor, int zoom_center_x = -99999, int zoom_center_y = -99999);	// the zoom_center is in current screen-space, not movie picture space
																									// after zooming, zoom_center point's image should stay unmoved.
																									// m_movie_offset_x and m_movie_offset_y is modified to keep it unmoved.
																									// if you saved these two variables, remember to get it from renderer.

	// settings GET function
	resampling_method get_subtitle_resizing();
	RECTF get_movie_scissor_rect();
	ui_drawer_base *get_ui_drawer();
	DWORD get_mask_color(int id);
	bool get_swap_eyes();
	input_layout_types get_input_layout();
	output_mode_types get_output_mode();
	mask_mode_types get_mask_mode();
	int get_mask_parameter();
	bool get_fullscreen();
	double get_movie_pos(int dimention);
	double get_aspect();
	bool is_connected(int id){return (id?m_dsr1:m_dsr0)->is_connected();}
	double get_subtitle_parallax(){return m_subtitle_parallax;}
	double get_parallax(){return m_parallax;}
	bool get_2dto3d(){return m_convert3d;}
	aspect_mode_types get_aspect_mode(){return (aspect_mode_types)(int)m_aspect_mode;}
	HRESULT intel_get_caps(IGFX_S3DCAPS *caps);
	int get_display_orientation(){return m_display_orientation;}
	bool get_vsync(){return m_vertical_sync;}
	float get_zoom_factor(){return m_zoom_factor;}

protected:
	lua_const & m_movie_resizing;
	lua_const & m_subtitle_resizing;
	display_orientation m_display_orientation;
	double m_parallax;
	bool m_has_subtitle;
	int m_last_ui_draw;
	CPooledTexture *m_subtitle;
	CPooledTexture *m_subtitle_mem;
	CCritSec m_subtitle_lock;
	int m_subtitle_pixel_width, m_subtitle_pixel_height;
	float m_subtitle_fleft, m_subtitle_ftop, m_subtitle_fwidth, m_subtitle_fheight;
	lua_const &m_movie_offset_x /*= -0.0*/;
	lua_const &m_movie_offset_y /*= 0.0*/;
	double m_subtitle_parallax;
	bool m_gpu_shadow;
	double m_source_aspect /*= (double)m_lVidWidth / m_lVidHeight*/;
	lua_const &m_forced_aspect /* = -1 */;
	int m_pass1_width;
	int m_pass1_height;
	Imy12doomRendererCallback *m_cb;
	bool m_revert_RGB32;
	REFERENCE_TIME m_last_frame_time;
	bool m_vertical_sync;
	float m_zoom_factor;
	RECTF *m_movie_scissor_rect;

protected:
	friend class my12doomRendererDShow;

	// settings
	lua_const & TEXTURE_SIZE;//(L"TextureSize", 4096, REG_DWORD);
	lua_const & SUBTITLE_TEXTURE_SIZE;//(L"SubtitleTextureSize", 2048, REG_DWORD);
	lua_const & EVRQueueSize;//(L"EVRQueueSize", 5, REG_DWORD);
	lua_const & SimplePageflipping;//(L"SimplePageflipping", 0, REG_DWORD);
	lua_const & GPUIdle;//(L"GPUIdle", true, REG_DWORD);

	// dshow related things
	bool m_recreating_dshow_renderer;
	my12doomRendererDShow * m_dsr0;
	my12doomRendererDShow * m_dsr1;
	CComPtr<IBaseFilter> m_evr;
	CComPtr<IBaseFilter> m_evr2;
	EVRCustomPresenter *m_presenter;
	EVRCustomPresenter *m_presenter2;



	HRESULT CheckMediaType(const CMediaType *pmt, int id);
	HRESULT SetMediaType(const CMediaType *pmt, int id);
	HRESULT	BreakConnect(int id);
	HRESULT CompleteConnect(IPin *pRecievePin, int id);
	HRESULT DataPreroll(int id, IMediaSample *media_sample);
	HRESULT DoRender(int id, IMediaSample *media_sample);
	LONG m_lVidWidth;   // Video width
	LONG m_lVidHeight;  // Video Height
	CGenericList<gpu_sample> m_left_queue;
	CGenericList<gpu_sample> m_right_queue;
	CCritSec m_queue_lock;
	gpu_sample * m_sample2render_1;
	gpu_sample * m_sample2render_2;
	gpu_sample * m_last_rendered_sample1;
	gpu_sample * m_last_rendered_sample2;
	CCritSec m_packet_lock;
	CCritSec m_rendered_packet_lock;
	CCritSec m_pool_lock;

	// dx9 functions and variables
	enum device_state
	{
		fine,							// device is fine
		need_resize_back_buffer,		// just resize back buffer and recaculate vertex
		need_reset_object,				// objects size changed, should recreate objects
		need_reset,						// reset requested by program, usually to change back buffer size, but program can continue rendering without reset
		device_lost,					// device lost, can't continue
		need_create,					// device not created, or need to recreate, can't continue
		create_failed,					// 
		device_state_max,				// not used
	} m_device_state;					// need_create

	CCritSec m_anti_reenter;
	int m_enter_counter;// = 0

	class anti_reenter
	{
	public:
		bool error;
		int *m_counter;
		CCritSec *m_sec;
		anti_reenter(CCritSec *sec, int *counter)
		{
			CAutoLock lck(sec);
			if (*counter>0)
				error = true;
			else
				error = false;

			m_sec = sec;
			m_counter = counter;
			(*counter) ++;
		}
		~anti_reenter()
		{
			CAutoLock lck(m_sec);
			
			(*m_counter) --;
		}
	};

	HRESULT create_render_targets();
	HRESULT delete_render_targets();
	HRESULT fix_nv3d_bug();				// do this after every CreateRenderTarget
	HRESULT handle_device_state();							//handle device create/recreate/lost/reset
	HRESULT create_render_thread();
	HRESULT terminate_render_thread();
	HRESULT invalidate_gpu_objects();
	HRESULT invalidate_cpu_objects();
	HRESULT restore_gpu_objects();
	HRESULT restore_cpu_objects();
	HRESULT render_nolock(bool forced = false);
	HRESULT render_helper(IDirect3DSurface9 *surfaces[], int nview);
	HRESULT draw_movie(IDirect3DSurface9 *surface, int view);		// 0 = left eye, 1 = right eye, others not implemented yet
	HRESULT draw_subtitle(IDirect3DSurface9 *surface, int view);
	HRESULT draw_ui(IDirect3DSurface9 *surface, int view);
	HRESULT adjust_temp_color(IDirect3DSurface9 *surface_to_adjust, int view);
	// assume dst has D3DUSAGE_RENDERTARGET Flag
	// assume src is first level of a texture
	HRESULT resize_surface(IDirect3DSurface9 *src, gpu_sample *src2, IDirect3DSurface9 *dst, RECT *src_rect = NULL, RECT *dst_rect = NULL,
							resampling_method method = bilinear_mipmap_minus_one, float alpha = 1.0f);
	HRESULT resize_surface(IDirect3DSurface9 *src, gpu_sample *src2, IDirect3DSurface9 *dst, RECTF *src_rect = NULL, RECTF *dst_rect = NULL, 
							resampling_method method = bilinear_mipmap_minus_one, float alpha = 1.0f);
#ifdef DEBUG
	HRESULT clear(IDirect3DSurface9 *surface, DWORD color = D3DCOLOR_ARGB(0, 0, 0, 0));
#else
	HRESULT clear(IDirect3DSurface9 *surface, DWORD color = D3DCOLOR_ARGB(0,0,0,0));
#endif

	HRESULT render(bool forced = false);
	static DWORD WINAPI render_thread(LPVOID param);
	static DWORD WINAPI test_thread(LPVOID param);

	// dx9 helper functions
	HRESULT reload_image();
	HRESULT load_image(int id = -1, bool forced = false);		// -1 = both dshow renderer
	HRESULT generate_mask();
	HRESULT set_device_state(device_state new_state);
	HRESULT test_PC_level();		// test hardware YUV-RGB conversion level
	DWORD m_PC_level;				// 0
	HRESULT calculate_movie_position(RECTF *position);
	HRESULT calculate_movie_position_unscaled(RECTF *position);
	HRESULT calculate_subtitle_position(RECTF *postion, int view);


	friend class dx_player;
	int m_last_reset_time;

	// AMD HD3D functions and variables
	CComPtr <IDirect3DSurface9> m_HD3DCommSurface;
	DWORD m_HD3Dlineoffset;					// 0 = HD3D is available but not enabled (HD3D need exclusive mode)
	int m_HD3DStereoModesCount;				// 0 = HD3D unavailable
	D3DDISPLAYMODE *m_HD3DStereoModes;
	D3DDISPLAYMODE m_prefered_mode;			// backbuffer format is ignored, using X8R8G8B8

	HRESULT HD3D_one_time_init();
	HRESULT HD3D_restore_objects();
	HRESULT HD3D_invalidate_objects();
	HRESULT HD3DSetStereoFullscreenPresentParameters();
	HRESULT HD3DMatchResolution();
	HRESULT HD3DGetAvailable3DModes(D3DDISPLAYMODE *modes, IN OUT int *count);			// count: caller:buffer count, return:modes count
	HRESULT HD3DDrawStereo(IDirect3DSurface9 *left_surface, IDirect3DSurface9 *right_surface, IDirect3DSurface9 *back_buffer);
	HRESULT HD3DSendStereoCommand(ATIDX9STEREOCOMMAND stereoCommand, BYTE *pOutBuffer, DWORD dwOutBufferSize, BYTE *pInBuffer, DWORD dwInBufferSize);
	HRESULT HD3D_set_prefered_mode(D3DDISPLAYMODE mode);

	// Intel S3D support
	IGFXS3DControl *m_intel_s3d/* = NULL*/;
	CComPtr<IDirect3DDeviceManager9> m_d3d_manager;
	HANDLE m_device_handle;
	CComPtr<IDirect3DSwapChain9> m_overlay_swap_chain;
	UINT m_resetToken/* = 0*/;
	IGFX_S3DCAPS m_intel_caps;  // = {0}
	CComPtr<IDirectXVideoProcessor> m_intel_VP_left;
	CComPtr<IDirectXVideoProcessor> m_intel_VP_right;
	DXVA2_VideoProcessBltParams m_BltParams; 
	DXVA2_VideoSample m_Sample;
	IGFX_DISPLAY_MODE m_intel_2d_mode;
	IGFX_DISPLAY_MODE m_intel_active_3d_mode;


	HRESULT intel_get_caps();
	HRESULT intel_d3d_init();
	HRESULT intel_switch_to_3d();
	HRESULT intel_switch_to_2d();
	HRESULT intel_delete_rendertargets();
	HRESULT intel_create_rendertargets();
	HRESULT intel_render_frame(IDirect3DSurface9 *left_surface, IDirect3DSurface9 *right_surface);


	// variables
	void init_variables();
	bool m_nv3d_enabled;			// false if ATI card
	bool m_nv3d_actived;
	bool m_nv3d_windowed;			// false if driver does not support windowed 3d vision
	NvDisplayHandle m_nv3d_display;
	NV_DISPLAY_DRIVER_VERSION m_nv_version;
	DWORD m_nv_pageflip_counter;
	StereoHandle m_nv3d_handle;
	int m_pageflip_frames;
	int m_pageflipping_start;
	LARGE_INTEGER m_first_pageflip;
	lua_const &m_swap_eyes;
	lua_const &m_force2d;
	output_mode_types m_output_mode;
	input_layout_types m_input_layout;
	bool m_convert3d;			// = false
	lua_const &m_aspect_mode;
	mask_mode_types m_mask_mode;
	int m_mask_parameter;
	HWND m_hWnd;
	HWND m_hWnd2;
	CComPtr<IDirect3D9>		m_D3D;
	CComPtr<IDirect3D9Ex>	m_D3DEx;
	CComPtr<IDirect3DDevice9> m_Device;
	CComPtr<IDirect3DDevice9Ex> m_DeviceEx;
	CComPtr<IDirect3DSwapChain9> m_swap1;
	CComPtr<IDirect3DSwapChain9Ex> m_swap1ex;
	CComPtr<IDirect3DSwapChain9> m_swap2;
	//CComPtr<IDirect3DQuery9> m_d3d_query;
	D3DPRESENT_PARAMETERS   m_new_pp;
	D3DPRESENT_PARAMETERS   m_active_pp;
	D3DPRESENT_PARAMETERS   m_active_pp2;
	D3DDISPLAYMODE m_d3ddm;
	HANDLE m_device_not_reseting;
	CCritSec m_device_lock;
	HANDLE m_render_event;
	int m_device_threadid;
	CTextureAllocator *m_pool;

	CComPtr <IDirect3DPixelShader9> m_ps_yv12;
	CComPtr <IDirect3DPixelShader9> m_ps_nv12;
	CComPtr <IDirect3DPixelShader9> m_ps_yuy2;
	CComPtr <IDirect3DPixelShader9> m_ps_anaglyph;
	CComPtr <IDirect3DPixelShader9> m_ps_iz3d_back;
	CComPtr <IDirect3DPixelShader9> m_ps_iz3d_front;
	CComPtr <IDirect3DPixelShader9> m_ps_test;
	CComPtr <IDirect3DPixelShader9> m_ps_test_tb;
	CComPtr <IDirect3DPixelShader9> m_ps_test_tb2;
	CComPtr <IDirect3DPixelShader9> m_ps_test_sbs;
	CComPtr <IDirect3DPixelShader9> m_ps_test_sbs2;
	CComPtr <IDirect3DPixelShader9> m_ps_color_adjust;
	CComPtr <IDirect3DPixelShader9> m_ps_bmp_blur;



	my12doom_auto_shader m_red_blue;
	my12doom_auto_shader m_ps_masking;
	my12doom_auto_shader m_lanczosX;
	my12doom_auto_shader m_lanczosX_YV12;
	my12doom_auto_shader m_lanczosX_NV12;
	my12doom_auto_shader m_lanczosX_P016;
	my12doom_auto_shader m_lanczosY;
	my12doom_auto_shader m_lanczos;
	my12doom_auto_shader m_lanczos_YV12;
	my12doom_auto_shader m_lanczos_NV12;
	my12doom_auto_shader m_lanczos_P016;
	my12doom_auto_shader m_multiview4;
	my12doom_auto_shader m_multiview6;
	my12doom_auto_shader m_alpha_multiply;
	my12doom_auto_shader m_ps_P016;

	// TV - PC level test surfaces
	CComPtr<IDirect3DSurface9> m_PC_level_test;

	CComPtr<IDirect3DSurface9> m_deinterlace_surface;		// surface for deinterlace
	RECT m_window_rect;
	CComPtr<IDirect3DTexture9> m_tex_mask;					// mask txture


	CComPtr<IDirect3DSurface9> m_nv3d_surface;				// nv3d temp surface

	// input layout detector
	input_layout_types get_active_input_layout();
	double get_active_aspect();
	int m_sbs;
	int m_normal;
	int m_tb;
	input_layout_types m_layout_detected;
	bool m_no_more_detect;

	// render thread variables
	CCritSec m_thread_lock;
	HANDLE m_render_thread;
	DWORD m_render_thread_id;
	bool m_render_thread_exit;
	DWORD m_color1;
	DWORD m_color2;
	LONG m_style, m_exstyle;
	RECT m_window_pos;


	// test draw ui
	CCritSec m_uidrawer_cs;
	ui_drawer_base *m_uidrawer;
public:

	// for ui_drawer class
	HRESULT loadBitmap(gpu_sample **out, wchar_t *file);
	HRESULT Draw(IDirect3DSurface9 *rt, gpu_sample *resource, RECTF *src, RECTF *dst, float alpha);
	HRESULT drawFont(gpu_sample **out, HFONT font, wchar_t *text, RGBQUAD color, RECT *dst_rect = NULL, DWORD flag = DT_CENTER | DT_WORDBREAK | DT_NOFULLWIDTHCHARBREAK | DT_EDITCONTROL);

	
	// for lua useage
	HRESULT get_resource(int arg, resource_userdata *resource);
	HRESULT paint(RECTF *dst, resource_userdata *resource, RECTF*src = NULL, float alpha = 1.0f, resampling_method method = bilinear_no_mipmap);
	HRESULT set_clip_rect(int left, int top, int right, int bottom);
};
