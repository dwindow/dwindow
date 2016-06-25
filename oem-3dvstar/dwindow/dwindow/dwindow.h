#pragma once
// Win32
#include <Windows.h>
#include <WindowsX.h>
#include <wchar.h>

// a helper function
BOOL SetClientRect(HWND hDlg, RECT rect);

class dwindow
{
public:
	// window controll functions
	dwindow(RECT screen1, RECT screen2);
	~dwindow();
	HRESULT set_fullscreen(int id, bool full, bool nosave = false);
	HRESULT reset_timer(int id, int new_interval);
	HRESULT show_window(int id, bool show);
	HRESULT show_mouse(bool show);
	HRESULT set_window_text(int id, const wchar_t *text);
	HRESULT set_window_client_rect(int id, int width, int height);
	bool m_full1;
	bool m_full2;
	HWND m_hwnd1;
	HWND m_hwnd2;

	void close_and_kill_thread();
// protected:
	typedef struct struct_window_proc_param
	{
		dwindow *that;
		HWND *hwnd1;
		HWND *hwnd2;
	} window_proc_param;

	static LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	static DWORD WINAPI WindowThread(LPVOID lpParame);

	virtual LRESULT on_idle_time(){return S_FALSE;}	// S_FALSE = Sleep(1)
	virtual LRESULT on_unhandled_msg(int id, UINT message, WPARAM wParam, LPARAM lParam)
		{return DefWindowProc(id_to_hwnd(id), message, wParam, lParam);}
	virtual LRESULT on_sys_command(int id, WPARAM wParam, LPARAM lParam){return S_FALSE;}	// don't block WM_SYSCOMMAND
	virtual LRESULT on_command(int id, WPARAM wParam, LPARAM lParam){return S_OK;}
	virtual LRESULT on_getminmaxinfo(int id, MINMAXINFO *info){return S_OK;}
	virtual LRESULT on_move(int id, int x, int y){return S_OK;}
	virtual LRESULT on_mouse_move(int id, int x, int y){return S_OK;}
	virtual LRESULT on_nc_mouse_move(int id, int x, int y){return S_OK;}
	virtual LRESULT on_mouse_down(int id, int button, int x, int y){return S_OK;}
	virtual LRESULT on_mouse_wheel(int id, WORD wheel_delta, WORD button_down, int x, int y){return S_OK;}
	virtual LRESULT on_mouse_up(int id, int button, int x, int y){return S_OK;}
	virtual LRESULT on_double_click(int id, int button, int x, int y){return S_OK;}
	virtual LRESULT on_key_down(int id, int key){return S_OK;}
	virtual LRESULT on_close(int id){return S_OK;}
	virtual LRESULT on_display_change(int id){return S_OK;}
	virtual LRESULT on_paint(int id, HDC hdc){return S_OK;}
	virtual LRESULT on_kill_focus(int id){return S_OK;}
	virtual LRESULT on_timer(int id){return S_OK;}
	virtual LRESULT on_size(int id, int type, int x, int y){return S_OK;}
	virtual LRESULT on_dropfile(int id, int count, wchar_t **filenames){return S_OK;}
	virtual LRESULT on_init_dialog(int id, WPARAM wParam, LPARAM lParam){return S_OK;}

	// helper function
	virtual HWND id_to_hwnd(int id);
	virtual int hwnd_to_id(HWND hwnd);
	bool is_visible(int id);

	// need init:
	HACCEL m_accel;				// should be inited by devired class
	HANDLE m_thread1;	
	HANDLE m_thread2;
	int m_thread_id1;
	LONG m_style1;
	LONG m_style2;
	LONG m_exstyle1;
	LONG m_exstyle2;
	RECT m_normal1;
	RECT m_normal2;
	RECT m_screen1;
	RECT m_screen2;
	bool m_show_mouse;
	int m_border_width;
	int m_caption_height;
	int m_border_height;
};

