#pragma once
// COM & DShow
#include <atlbase.h>
#include <dshow.h>
#include <InitGuid.h>

// renderer
#include "streams.h"
#include "..\renderer_prototype\my12doomRenderer.h"
#include "dwindow.h"
#include "myx264.h"

// other
#include "Iplayer.h"
#include "runnable.h"
#include "global_funcs.h"
#include "CSubtitle.h"
#include "srt\srt_parser.h"
#include "..\lrtb\mySink.h"
#include "PGS\PGSParser.h"
#include "vobsub_renderer.h"
#include "DShowSubtitleRenderer.h"
#include "..\my12doomSource\src\filters\parser\MpegSplitter\IOffsetMetadata.h"
#include "IStereoLayout.h"
#include "color_adjust.h"
#include "IntelWiDiExtensions_i.h"
#include "..\lua\my12doom_lua.h"

#define _AFX
#define __AFX_H__
#include <atlcoll.h>

// some definition
#define max_playlist (50)
#define countof(x) (sizeof(x)/sizeof(x[0]))
#define MAX_WIDI_ADAPTERS (255)
#define PURE2


// dx_player's definition of output mode (combination of output mode and interlace mode)
enum dx_player_outputmode
{
	dx_player_NV3D = 0,
	dx_player_HD3D = 1,
	dx_player_Intel = 2,
	dx_player_IZ3D = 3,
	dx_player_pageflipping = 4,
	dx_player_anaglyph = 5,
	dx_player_2D = 6,
	dx_player_HSBS = 7,
	dx_player_line_interlace = 8,
	dx_player_checkboard = 9,
	dx_player_row_interlace = 10,
	dx_player_tb = 11,
	dx_player_sbs = 12,
	dx_player_dual_projector = 13,
	dx_player_outputmode_max,
};

enum dx_player_topmost
{
	never,
	playing,
	always,
};

class subtitle_file_handler
{
public:
	bool actived/* = false*/;
	wchar_t m_pathname[MAX_PATH];
	wchar_t m_displayname[MAX_PATH];
	CSubtitleRenderer *m_renderer;
	subtitle_file_handler(const wchar_t *pathname);
	~subtitle_file_handler();
};

class lua_drawer;
class dx_player : protected Imy12doomRendererCallback, public dwindow, public Iplayer, public ui_drawer_base
{
public:
	dx_player(HINSTANCE hExe);
	~dx_player();

	// load functions
	HRESULT reset();								// unload all video and subtitle files
	HRESULT start_loading();
	HRESULT reset_and_loadfile(const wchar_t *pathname, const wchar_t*pathname2 = NULL, bool stop = false);
	HRESULT reset_and_loadfile_core(const wchar_t *pathname, const wchar_t*pathname2 = NULL, bool stop = false);
	HRESULT load_audiotrack(const wchar_t *pathname);
	HRESULT load_subtitle(const wchar_t *pathname, bool reset = true);
	HRESULT load_file(const wchar_t *pathname, bool non_mainfile = false, int audio_track = LOADFILE_FIRST_TRACK, int video_track = LOADFILE_ALL_TRACK);			// for multi stream mkv
	HRESULT end_loading();

	// subtitle control functions
	HRESULT set_subtitle_pos(double center_x, double bottom_y);
	HRESULT set_subtitle_parallax(int parallax);
	HRESULT get_subtitle_pos(double *center_x, double *bottom_y);
	HRESULT get_subtitle_parallax(int *parallax);

	// image control functions
	HRESULT toggle_force2d();
	HRESULT set_force_2d(bool force2d);
	HRESULT set_swap_eyes(bool swap_eyes);
	HRESULT set_movie_pos(double x, double y);	// 1.0 = top, -1.0 = bottom, 0 = center
	HRESULT get_force_2d(bool *force2d) PURE2;
	HRESULT get_swap_eyes(bool *swap_eyes) PURE2;
	HRESULT get_movie_pos(int which, double *out) PURE2;	// which: 0 = x, 1=y

	// play control functions
	HRESULT play();
	HRESULT pause();
	HRESULT stop();
	HRESULT seek(int time);
	HRESULT tell(int *time);
	HRESULT total(int *time);
	HRESULT set_volume(double volume);			// 0 - 1.0 = 0% - 100%, linear
	HRESULT get_volume(double *volume);
	bool is_playing();
	HRESULT execute_command_adv(wchar_t *command, wchar_t *out, const wchar_t **args, int args_count);
	HRESULT show_mouse(bool show)
	{
#ifdef DEBUG
		return show_mouse_core(show, true);
#else
		return show_mouse_core(show);
#endif
	}
	HRESULT show_mouse_core(bool show, bool test = false)
	{
		if (test)
			return S_OK;
		GetCursorPos(&m_mouse);
		return dwindow::show_mouse(show || m_theater_owner);
	}
	bool is_closed();
	HRESULT toggle_fullscreen_core();
	HRESULT toggle_fullscreen();
	HRESULT set_output_mode(int mode);
	HRESULT set_theater(HWND owner){m_theater_owner = owner; return S_OK;}
	HRESULT popup_menu(HWND owner, int sub = -1);
	bool is_fullsceen(int window_id = 1){return (m_renderer1 && m_renderer1->get_fullscreen()) || ( window_id==1?m_full1:m_full2);}
	HWND get_window(int window_id);
	HRESULT set_output_channel(int channel);
	HRESULT show_hd3d_fullscreen_mode_dialog();
	HRESULT set_topmost(DWORD topmost);
	DWORD get_topmost(){return (int)m_topmost;}


	POINT m_mouse;
	int m_dialog_open;


	int init_done_flag;

	wchar_t m_main_video_filename[MAX_PATH];
// #ifndef ZHUZHU
// protected:
// #endif

	// trial
	lua_const &m_trial_shown;
	lua_const &m_server_port/*(L"DWindowNetworkPort", 8080, REG_DWORD)*/;

	// theater
	HWND m_theater_owner;

	// touch input
	bool m_has_multi_touch;

	// WiDi
	// don't call these functions too frequently
	// or you may corrupt some variables due to WiDi's non-blocking calls design
	bool m_widi_inited;
	bool m_widi_has_support;
	int m_widi_num_adapters_found;
	CComPtr<IWiDiExtensions> m_widi;
	BSTR m_widi_adapters[MAX_WIDI_ADAPTERS][255];
	bool m_widi_scanning;
	bool m_widi_connected;
	lua_const &m_widi_screen_mode;
	lua_const &m_widi_resolution_width;
	lua_const &m_widi_resolution_height;
	HRESULT widi_initialize();
	HRESULT widi_start_scan();
	HRESULT widi_get_adapter_by_id(int id, wchar_t *out);
	HRESULT widi_get_adapter_information(int id, wchar_t *out, const wchar_t *key = NULL);
	HRESULT widi_connect(int id, DWORD screenmode = ExternalOnly, int resolution_width = 0, int resolution_height = 0);
	HRESULT widi_set_screen_mode(DWORD screenmode);
	HRESULT widi_disconnect(int id = -1);	// -1 means disconnect any connection
	HRESULT widi_shutdown();

	// WiDi messages
	LRESULT OnWiDiInitialized(WPARAM wParam, LPARAM lParam);
	LRESULT OnWiDiScanCompleted(WPARAM wParam, LPARAM lParam);
	LRESULT OnWiDiConnected(WPARAM wParam, LPARAM lParam);
	LRESULT OnWiDiDisconnected(WPARAM wParam, LPARAM lParam);
	LRESULT OnWiDiAdapterDiscovered(WPARAM wParam, LPARAM lParam);

	// saved screen settings
	lua_const &m_saved_screen1;
	lua_const &m_saved_screen2;
	lua_const &m_saved_rect1;
	lua_const &m_saved_rect2;
	lua_const &m_topmost;
	bool m_topmost_state;
	HRESULT detect_monitors();
	HRESULT set_output_monitor(int out_id, int monitor_id);
	HRESULT init_window_size_positions();
	bool rect_visible(RECT rect);

	// image control vars
	class subtitle_loader : public Irunnable
	{
	public:
		subtitle_loader(dx_player *owner){m_owner = owner;}
		void run();						// splayer subtitle loader
		dx_player *m_owner;
	};
	thread_pool m_subtitle_loader_pool;
	HINSTANCE m_hexe;
	int m_mirror1;
	int m_mirror2;			// 0x0:no mirror, 0x1 mirror horizontal, 0x2 mirror vertical, 0x3(0x3=0x1|0x2) mirror both
	lua_const &m_resize_window_on_open;	// FALSE
	double m_parallax;
	bool m_is_remux_file;
	CComPtr<IStereoLayout> m_stereo_layout;
	AutoSettingO<D3DDISPLAYMODE> m_hd3d_prefered_mode;
	std::list<x264*> m_x264_encoder;
	CCritSec m_x264_encoder_lock;

	// helper function and vars
	bool m_select_font_active;
	static DWORD WINAPI select_font_thread(LPVOID lpParame);
	DWORD tell_thread();
	static DWORD WINAPI tell_thread_entry(LPVOID lpParame){return ((dx_player*)lpParame)->tell_thread();}
	HRESULT render_audio_pin(IPin *pin = NULL);		// render audio pin, if pin=NULL, add all possible decoder
	HRESULT render_video_pin(IPin *pin = NULL);		// render video pin, if pin=NULL, add all possible decoder
	int hittest(int x, int y, HWND hwnd, double *v);
	int m_dragging_window;
	HANDLE m_tell_thread;
	HRESULT lua_OnMouseEvent(char *event, int x, int y, int key);  // x,y: UIScale applied inside
	int m_renderer_reset_done;
	HRESULT reset_renderer();


	// window handler
	LRESULT on_display_change(int id);
	LRESULT DecodeGesture(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT on_unhandled_msg(int id, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT on_sys_command(int id, WPARAM wParam, LPARAM lParam);	// don't block WM_SYSCOMMAND
	LRESULT on_command(int id, WPARAM wParam, LPARAM lParam);
	LRESULT on_close(int id);
	LRESULT on_getminmaxinfo(int id, MINMAXINFO *info);
	LRESULT on_move(int id, int x, int y);
	LRESULT on_mouse_move(int id, int x, int y);
	LRESULT on_nc_mouse_move(int id, int x, int y);
	LRESULT on_mouse_down(int id, int button, int x, int y);
	LRESULT on_mouse_wheel(int id, WORD wheel_delta, WORD button_down, int x, int y);
	LRESULT on_mouse_up(int id, int button, int x, int y);
	LRESULT on_double_click(int id, int button, int x, int y);
	LRESULT on_key_down(int id, int key);
	LRESULT on_paint(int id, HDC hdc);
	LRESULT on_timer(int id);
	LRESULT on_kill_focus(int id);
	LRESULT on_size(int id, int type, int x, int y);
	LRESULT on_init_dialog(int id, WPARAM wParam, LPARAM lParam);
	LRESULT on_dropfile(int id, int count, wchar_t **filenames);
	LRESULT on_idle_time();	// S_FALSE = Sleep(1)
	// end window handler

	// helper
	lua_const &m_simple_audio_switching;		// true: call enable_audio_track() to reconnect filter after audio setting changed,  false: no call
	int m_active_audio_track;
	int m_active_subtitle_track;
	HRESULT enable_audio_track(int track);			// special track: -1: disable all, -2: reconnect current,
	HRESULT enable_subtitle_track(int track);		// special track: -1: disable all, -2: reconnect current
	HRESULT handle_downmixer();
	HRESULT list_tracks(int *count, IPin **pins, CLSID majortype, wchar_t *match);
	HRESULT enable_track(int track, CLSID majortype, wchar_t *match);
	HRESULT list_audio_track(wchar_t **out, bool *connected, int *found);		// caller should alloc at least 32 * 1024 * sizeof(wchar_t) byte for out, 32 byte for connected, and free it afterwards
	HRESULT list_subtitle_track(wchar_t **out, bool *connected, int *found);	// caller should alloc at least 32 * 1024 * sizeof(wchar_t) byte for out, 32 byte for connected, and free it afterwards
	HRESULT debug_list_filters();

	// filter callback function
	HRESULT SampleCB(REFERENCE_TIME TimeStart, REFERENCE_TIME TimeEnd, IMediaSample *pIn, int stream_id);
	HRESULT PrerollCB(REFERENCE_TIME TimeStart, REFERENCE_TIME TimeEnd, IMediaSample *pIn, int stream_id);

	// directshow etc. core part
	int m_total_time;			// a buffer 
	int m_current_time;			// from SampleSB()
	DWORD m_lastVideoCBTick;
	CCritSec m_dshow_sec;
	bool m_show_ui;
	bool m_show_volume_bar;
	int m_dragging;	// = 0
	POINT m_mouse_down_point;	// = 0
	int m_mouse_down_time;
	double m_dragging_value;
	OAFilterState m_filter_state;
	lua_const &m_volume;	// = 1.0
	lua_const &m_useInternalAudioDecoder;		// = true
	lua_const &m_normalize_audio;	// = false
	lua_const &m_channel;	// = false
	HRESULT show_ui(bool show);
	HRESULT show_volume_bar(bool show);
	HRESULT on_dshow_event();		//"on move window"
	HRESULT init_direct_show();
	HRESULT exit_direct_show();

	// basic directshow vars
	DWORD rot_id;
	CComPtr<IGraphBuilder>		m_gb;
	CComPtr<IMediaSeeking>		m_ms;
	CComPtr<IMediaControl>		m_mc;
	CComPtr<IBasicAudio>		m_ba;
	CComPtr<IBaseFilter>		m_lav;
	CComPtr<IBaseFilter>		m_downmixer;

	// renderer and input layout and output mode and deinterlacing
	my12doomRenderer *m_renderer1;
	lua_const &m_input_layout; /* = input_layout_auto*/
	lua_const &m_output_mode;  /* = anaglyph*/
	lua_const &m_mask_mode;	   /* = row_interlace */
	lua_const &m_anaglygh_left_color;	   /* = RED */
	lua_const &m_anaglygh_right_color;	   /* = CYAN */
	lua_const &m_forced_deinterlace;
	lua_const &m_display_orientation;

	// subtitle control
	typedef struct rendered_subtitle2_struct : public rendered_subtitle
	{
		bool valid;
		int time;
		HRESULT hr;
	} rendered_subtitle2;
	rendered_subtitle2 m_subtitle_cache[1024];
	CComPtr<IOffsetMetadata> m_offset_metadata;
	CSubtitleRenderer *m_srenderer;
	DShowSubtitleRenderer m_grenderer;
	CAutoPtrList<subtitle_file_handler> m_external_subtitles;
	CCritSec m_subtitle_sec;
	CCritSec m_subtitle_cache_sec;
	int m_lastCBtime;
	HRESULT draw_subtitle();
	lua_const &m_subtitle_center_x;
	lua_const &m_subtitle_bottom_y;
	lua_const &m_user_subtitle_parallax;
	int m_internel_offset;
	int m_last_bitmap_update;
	bool m_subtitle_has_offset;
	lua_const &m_audio_latency;
	lua_const &m_subtitle_latency;
	lua_const &m_subtitle_ratio;

	// font
	HFONT m_font;
	lua_const &m_font_color/*L"FontColor", 0x00ffffff)*/;
	lua_const &m_lFontPointSize/*(L"FontSize", 40)*/;
	lua_const &m_display_subtitle;
	lua_const &m_FontName/*(L"Font", L"Arial")*/;
	lua_const &m_FontStyle/*(L"FontStyle", L"Regular")*/;
	AutoSettingO<LOGFONTW> m_LogFont;
	HRESULT select_font(bool show_dlg);
	// end font


	// UI drawer

	virtual HRESULT init_gpu(int width, int height, IDirect3DDevice9 *device);
	virtual HRESULT init_cpu(IDirect3DDevice9 *device);
	virtual HRESULT invalidate_gpu();
	virtual HRESULT invalidate_cpu();
	virtual HRESULT draw_ui(IDirect3DSurface9 *surface, int view);
	virtual HRESULT draw_nonmovie_bg(IDirect3DSurface9 *surface, int view);
	virtual HRESULT hittest(int x, int y, int *out, double *outv = NULL);

	int m_width;
	int m_height;
	int m_ui_visible_last_change_time;
	int m_volume_visible_last_change_time;
	IDirect3DDevice9 *m_Device;
	CComPtr<IDirect3DVertexBuffer9> m_vertex;
	CComPtr<IDirect3DTexture9> m_ui_logo_cpu;
	CComPtr<IDirect3DTexture9> m_ui_tex_cpu;
	CComPtr<IDirect3DTexture9> m_ui_background_cpu;
	CComPtr<IDirect3DTexture9> m_ui_logo_gpu;
	CComPtr<IDirect3DTexture9> m_ui_tex_gpu;
	CComPtr<IDirect3DTexture9> m_ui_background_gpu;
	CComPtr <IDirect3DPixelShader9> m_ps_UI;
	HRESULT init_ui2(IDirect3DSurface9 * surface);
	HRESULT draw_ui2(IDirect3DSurface9 * surface);


	//elements
	UI_element_fixed
		playbutton,
		pausebutton,
		current_time[5][10],
		colon[4],
		total_time[5][10],
		fullbutton,
		test_button,
		test_button2;

	UI_element_warp
		back_ground,
		volume,
		volume_top,
		volume_back,
		progressbar,
		progress_top,
		progress_bottom;
//#define VSTAR
//#ifdef VSTAR
	gpu_sample *m_toolbar_background;
	gpu_sample *m_UI_logo;
	gpu_sample *m_buttons[9];
	gpu_sample *m_numbers[11];
	gpu_sample *m_progress[6];
	gpu_sample *m_volume_base;
	gpu_sample *m_volume_button;

	lua_drawer *m_lua;
//#endif
};

class lua_drawer : public ui_drawer_base
{
public:
	lua_drawer(dx_player *owner);
	virtual HRESULT init_gpu(int width, int height, IDirect3DDevice9 *device);
	virtual HRESULT init_cpu(IDirect3DDevice9 *device);
	virtual HRESULT invalidate_gpu();
	virtual HRESULT invalidate_cpu();
	virtual HRESULT draw_ui(IDirect3DSurface9 *surface, int view);
	virtual HRESULT pre_render_movie(IDirect3DSurface9 *surface, int view);
	virtual HRESULT draw_nonmovie_bg(IDirect3DSurface9 *surface, int view);
	virtual HRESULT hittest(int x, int y, int *out, double *outv = NULL);

	IDirect3DDevice9 *m_device;
	dx_player *m_owner;
};