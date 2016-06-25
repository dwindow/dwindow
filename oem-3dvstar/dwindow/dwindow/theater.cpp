#include <Windows.h>
#include <CommCtrl.h>
#include "resource.h"
#include "Iplayer.h"

#define SB_RBUTTON 16

Iplayer * player = NULL;
HWND control = NULL;
HWND hProgress;// = GetDlgItem(hDlg, IDC_PROGRESS);
HWND hVolume;// = GetDlgItem(hDlg, IDC_VOLUME);
LONG_PTR g_OldVolumeProc = 0;
LONG_PTR g_OldProgressProc = 0;
bool is_draging = false;

char the_passkey_and_signature[128+128+128] = "my12doom here!";

// overrided slider proc
LRESULT CALLBACK VolumeProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );
LRESULT CALLBACK ProgressProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );
void format_time(int time, wchar_t *out);
void format_time_noms(int time, wchar_t *out);
void format_time2(int current, int total, wchar_t *out);

INT_PTR CALLBACK threater_countrol_proc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg ) 
	{

	case WM_TIMER:
		{
			// full screen button
			if (player->is_fullsceen(1))
				SetWindowTextW(GetDlgItem(hDlg, IDC_FULL), L"2");
			else
				SetWindowTextW(GetDlgItem(hDlg, IDC_FULL), L"1");	

			int total = 0;
			player->total(&total);
			int current = 0;
			player->tell(&current);

			// play button
			if (player->is_playing())
				SetWindowTextW(GetDlgItem(hDlg, IDC_PLAY), L";");
			else
				SetWindowTextW(GetDlgItem(hDlg, IDC_PLAY), L"4");			

			if (!is_draging && player->m_file_loaded)
			{
				SendMessage(hProgress, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)(1000*((double)current/total)));
				wchar_t tmp[200];
				format_time_noms(total, tmp);
					SetDlgItemTextW(hDlg, IDC_TOTAL, tmp);
				format_time(current, tmp);
					SetDlgItemTextW(hDlg, IDC_CURRENT, tmp);
			}

			else if (!is_draging)
			{
				SetDlgItemTextW(hDlg, IDC_TOTAL, L"");
				SetDlgItemTextW(hDlg, IDC_CURRENT, L"");
			}

			if (player->is_closed())
				EndDialog(hDlg, 0);
		}
		break;

	case WM_HSCROLL:
		if (hProgress == (HWND)lParam)
		{
			is_draging = true;
			int total = 0;
			player->total(&total);
			if ( LOWORD(wParam) & SB_ENDSCROLL)
			{


				int target = (LONGLONG)SendMessage(hProgress, TBM_GETPOS, 0, 0) * total / 1000;

				player->seek(target);

				is_draging = false;
			}
			else if (LOWORD(wParam) & SB_THUMBTRACK)
			{
				int target = (LONGLONG)SendMessage(hProgress, TBM_GETPOS, 0, 0) * total / 1000;
				wchar_t tmp[200];
				format_time(target, tmp);
				SetDlgItemTextW(hDlg, IDC_CURRENT, tmp);
			}
		}
		break;
	case WM_VSCROLL:
		{
			int newVol = SendMessage(hVolume, TBM_GETPOS, 0, 0);
			player->set_volume(1-(double)newVol/1000);
		}
		break;

	case WM_COMMAND:
		{
			int id = LOWORD(wParam);
			if (id == IDC_FULL)
			{
// 				if (IsIconic(player->m_hwnd1)) SendMessageW(player->m_hwnd1, WM_SYSCOMMAND, (WPARAM)SC_RESTORE, 0);
// 				if (IsIconic(player->m_hwnd2)) SendMessageW(player->m_hwnd2, WM_SYSCOMMAND, (WPARAM)SC_RESTORE, 0);

				player->toggle_fullscreen();
			}

			else if (id == IDC_PLAY)
			{
				if (!player->m_file_loaded)
					player->popup_menu(hDlg);
				else if (player->is_playing())
					player->pause();
				else
					player->play();
			}

			else if (id == ID_EXIT)
			{
				EndDialog(hDlg, 0);
			}

			else if (id == IDC_MENU)
			{
				//player->popup_menu(hDlg);
				PostMessage(player->get_window(1), WM_COMMAND, ID_VIDEO_ADJUSTCOLOR, 0);
			}

			else
			{
				PostMessage(player->get_window(1), msg, wParam, lParam);
			}
		}
		break;

	case WM_INITDIALOG:
		{
			// icon
			HICON h_icon = LoadIcon(LoadLibraryW(L"StereoPlayer.exe"), MAKEINTRESOURCE(IDI_ICON1));
			SendMessage(hDlg, WM_SETICON, TRUE, (LPARAM)h_icon);
			SendMessage(hDlg, WM_SETICON, FALSE, (LPARAM)h_icon);

			// values 
			control = hDlg;
			hProgress = GetDlgItem(hDlg, IDC_PROGRESS);
			hVolume = GetDlgItem(hDlg, IDC_VOLUME);

			// set max to 1000
			SendMessage(hProgress, TBM_SETRANGEMAX, (WPARAM)TRUE, (LPARAM)1000);
			SendMessage(hVolume, TBM_SETRANGEMAX, (WPARAM)TRUE, (LPARAM)1000);

			// override sliders' proc
			g_OldVolumeProc = GetWindowLongPtr(hVolume, GWLP_WNDPROC);
			LONG rtn = SetWindowLongPtr(hVolume, GWLP_WNDPROC, (LONG_PTR)VolumeProc);
			g_OldProgressProc = GetWindowLongPtr(hProgress, GWLP_WNDPROC);
			rtn = SetWindowLongPtr(hProgress, GWLP_WNDPROC, (LONG_PTR)ProgressProc);

			// position
			SetWindowPos(hDlg, NULL, 320, 240, 0, 0, SWP_NOSIZE);
			BringWindowToTop(hDlg);

			// font
			HFONT hFont = CreateFont(24,0,0,0,FW_DONTCARE,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_OUTLINE_PRECIS,
				CLIP_DEFAULT_PRECIS, 5, VARIABLE_PITCH,TEXT("Webdings"));
			HFONT hFont2 = CreateFont(48,0,0,0,FW_DONTCARE,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_OUTLINE_PRECIS,
				CLIP_DEFAULT_PRECIS, 5, VARIABLE_PITCH,TEXT("Webdings"));
			HWND hPlay = GetDlgItem(hDlg, IDC_PLAY);
			HWND hFull = GetDlgItem(hDlg, IDC_FULL);
			SendMessage(hPlay, WM_SETFONT, (WPARAM)hFont2, MAKELPARAM(TRUE, 0));
			SendMessage(hFull, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));

			// volume
			double volume;
			player->get_volume(&volume);
			SendMessage(hVolume, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)1000*(1-volume));


			// etc
			player->set_theater(hDlg);
			SetTimer(hDlg, 0, 33, NULL);
		}
		break;

	case WM_DROPFILES:
		{
			HDROP hDropInfo = (HDROP)wParam;
			int count = DragQueryFile(hDropInfo, (UINT)-1, NULL, 0);
			if (count>0)
			{
				wchar_t **filenames = (wchar_t**)malloc(sizeof(wchar_t*)*count);
				for(int i=0; i<count; i++)
				{
					filenames[i] = (wchar_t*)malloc(sizeof(wchar_t) * MAX_PATH);
					DragQueryFileW(hDropInfo, i, filenames[i], MAX_PATH);
				}

				player->reset_and_loadfile(filenames[0], false);

				for(int i=0; i<count; i++) free(filenames[i]);
				free(filenames);
			}
		}
		break;



	case WM_CLOSE:
		EndDialog(hDlg, -1);
		break;

	default:
		return FALSE;
	}

	return TRUE; // Handled message
}


HRESULT dwindow_dll_go(HINSTANCE inst, HWND owner, Iplayer *p)
{
	player = p;

	int o = DialogBox(inst, MAKEINTRESOURCE(IDD_THEATER), owner, threater_countrol_proc);

	return S_OK;
}

HRESULT dwindow_dll_init(char *passkey_big, int rev)
{
	return S_OK;
}

LRESULT CALLBACK VolumeProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	WPARAM flag = 0;
	bool mouse_event = false;
	static bool dragging = false;
	static int lastflag = 0;
	switch( msg ) 
	{
	case WM_RBUTTONDOWN:
		flag |= SB_RBUTTON;
	case WM_LBUTTONDOWN:
		SetCapture(hDlg);
		SetFocus(hDlg);
		mouse_event = true;
		dragging = true;
		break;
	case WM_RBUTTONUP:
		flag |= SB_RBUTTON;
	case WM_LBUTTONUP:
		ReleaseCapture();
		if (dragging) mouse_event = true;
		dragging = false;
		break;
	case WM_MOUSEMOVE:
		if (dragging) mouse_event = true;
		flag = lastflag;
		break;
	case WM_MBUTTONDOWN:
		SendMessage(hDlg, TBM_SETPOS, (WPARAM)TRUE, 25);
		threater_countrol_proc(control, WM_VSCROLL, SB_RBUTTON, 0);
		break;
	}

	if (mouse_event)
	{
		RECT rc;
		CallWindowProc((WNDPROC)g_OldVolumeProc, hDlg, TBM_GETCHANNELRECT, 0, (LPARAM)&rc);

		POINT point = {LOWORD(lParam), HIWORD(lParam)};


		int nMin = SendMessage(hDlg, TBM_GETRANGEMIN, 0, 0);
		int nMax = SendMessage(hDlg, TBM_GETRANGEMAX, 0, 0);+1;

		if (point.y >= 60000) 
			point.y = rc.left;

		// note: there is a bug in GetChannelRect, it gets the orientation of the rectangle mixed up
		double dPos = (double)(point.y - rc.left)/(rc.right - rc.left);

		int newPos = (int)(nMin + (nMax-nMin)*dPos + 0.5 *(1-dPos) - 0.5 *dPos);

		SendMessage(hDlg, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)newPos);
		SendMessage(control, WM_VSCROLL, flag, (LPARAM)hDlg);

		lastflag = flag;

		return false;
	}

	return 	CallWindowProc((WNDPROC)g_OldVolumeProc,hDlg ,msg, wParam, lParam);   
}

LRESULT CALLBACK ProgressProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	WPARAM flag = 0;
	bool mouse_event = false;
	static bool dragging = false;
	switch( msg ) 
	{
	case WM_LBUTTONDOWN:
		flag = SB_THUMBTRACK;
		SetCapture(hDlg);
		SetFocus(hDlg);
		mouse_event = true;
		dragging = true;
		break;
	case WM_LBUTTONUP:
		flag = SB_ENDSCROLL;
		ReleaseCapture();
		if (dragging) mouse_event = true;
		dragging = false;
		break;
	case WM_MOUSEMOVE:
		flag = SB_THUMBTRACK;
		if (dragging) mouse_event = true;
		break;
	}

	if (mouse_event)
	{
		RECT rc;
		SendMessage(hDlg, TBM_GETCHANNELRECT, 0, (LPARAM)&rc);

		POINT point = {LOWORD(lParam), HIWORD(lParam)};


		int nMin = SendMessage(hDlg, TBM_GETRANGEMIN, 0, 0);
		int nMax = SendMessage(hDlg, TBM_GETRANGEMAX, 0, 0);+1;

		if (point.x >= 60000) 
			point.x = rc.left;
		double dPos = (double)(point.x - rc.left)/(rc.right - rc.left);

		int newPos = (int)(nMin + (nMax-nMin)*dPos + 0.5 *(1-dPos) - 0.5 *dPos);

		SendMessage(hDlg, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)newPos);
		SendMessage(control, WM_HSCROLL, flag, (LPARAM)hDlg);

		return false;
	}

	return 	CallWindowProc((WNDPROC)g_OldProgressProc, hDlg ,msg, wParam, lParam);
}

void format_time(int time, wchar_t *out)
{
	int ms = time % 1000;
	int s = (time / 1000) % 60;
	int m = (time / 1000 / 60) % 60;
	int h = (time / 1000 / 3600);

	wsprintfW(out, L"%02d:%02d:%02d.%03d", h, m, s, ms);
}

void format_time_noms(int time, wchar_t *out)
{
	int s = (time / 1000) % 60;
	int m = (time / 1000 / 60) % 60;
	int h = (time / 1000 / 3600);

	wsprintfW(out, L"%02d:%02d:%02d", h, m, s);
}

void format_time2(int current, int total, wchar_t *out)
{
	wchar_t tmp[256];

	format_time(current, tmp);
	wcscpy(out, tmp);
	
	wcscat(out, L" / ");

	format_time(total, tmp);
	wcscat(out, tmp);
}