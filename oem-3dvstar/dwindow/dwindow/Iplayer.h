#pragma once
#include <Windows.h>
#include "ICommand.h"

// some definition
#define LOADFILE_NO_TRACK -1
#define LOADFILE_ALL_TRACK -2
#define LOADFILE_TRACK_NUMBER(x) (0x01 << x)
#define LOADFILE_FIRST_TRACK LOADFILE_TRACK_NUMBER(0)
#define WM_LOADFILE (WM_USER + 5)
#define WM_DWINDOW_COMMAND (WM_USER + 7)


class Iplayer : public ICommandReciever
{
public:
	// load functions
	virtual HRESULT reset() PURE;								// unload all video and subtitle files
	virtual HRESULT start_loading() PURE;
	virtual HRESULT reset_and_loadfile(const wchar_t *pathname, const wchar_t *pathname2 = NULL, bool stop = false) PURE;
	virtual HRESULT load_audiotrack(const wchar_t *pathname) PURE;
	virtual HRESULT load_subtitle(const wchar_t *pathname, bool reset = true) PURE;
	virtual HRESULT load_file(const wchar_t *pathname, bool non_mainfile = false, 
		int audio_track = LOADFILE_FIRST_TRACK, int video_track = LOADFILE_ALL_TRACK) PURE;			// for multi stream mkv
	virtual HRESULT end_loading() PURE;

	// subtitle control functions
	virtual HRESULT set_subtitle_pos(double center_x, double bottom_y) PURE;
	virtual HRESULT get_subtitle_pos(double *center_x, double *bottom_y) PURE;
	virtual HRESULT set_subtitle_parallax(int offset) PURE;
	virtual HRESULT get_subtitle_parallax(int *parallax) PURE;

	// image control functions
	virtual HRESULT set_swap_eyes(bool revert) PURE;
	virtual HRESULT set_movie_pos(double x, double y) PURE;	// 1.0 = top, -1.0 = bottom, 0 = center

	// play control functions
	virtual HRESULT play() PURE;
	virtual HRESULT pause() PURE;
	virtual HRESULT stop() PURE;
	virtual HRESULT seek(int time) PURE;
	virtual HRESULT tell(int *time) PURE;
	virtual HRESULT total(int *time) PURE;
	virtual HRESULT set_volume(double volume) PURE;			// 0 - 1.0 = 0% - 100%, linear
	virtual HRESULT get_volume(double *volume) PURE;
	virtual bool is_playing() PURE;
	virtual HRESULT show_mouse(bool show) PURE;
	virtual bool is_closed() PURE;
	virtual HRESULT toggle_fullscreen() PURE;
	virtual HRESULT set_output_mode(int mode) PURE;
	virtual HRESULT set_theater(HWND owner) PURE;
	virtual HRESULT popup_menu(HWND owner, int sub = -1) PURE;
	virtual bool is_fullsceen(int window_id) PURE;
	virtual HWND get_window(int window_id) PURE;

	// helper variables
	wchar_t *m_log;
	bool m_file_loaded /*= false*/;
	POINT m_mouse;
};
