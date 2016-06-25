#include <Windows.h>
#include <Windowsx.h>
#include <CommCtrl.h>
#include "resource.h"
#include "resource1.h"
#include "Iplayer.h"
#include "audio_device.h"
#include "global_funcs.h"
#include "dx_player.h"
#include "ipconfig.h"

#define MAX_PARALLAX 40
#define MAX_POS_Y 0.5
#define SB_RBUTTON 16

namespace zhuzhu
{
dx_player * player = NULL;
HWND control = NULL;
HWND hVolume;// = GetDlgItem(hDlg, IDC_VOLUME);
LONG_PTR g_OldVolumeProc = 0;
LONG_PTR g_OldProgressProc = 0;
bool is_draging = false;
HINSTANCE inst = NULL;

char the_passkey_and_signature[128+128+128] = "my12doom here!";
AutoSettingString g_IP(L"IP", L"");
AutoSettingString g_subnet_mask(L"SubnetMask", L"");
AutoSettingString g_gateway(L"Gateway", L"");
AutoSettingString g_DNS(L"DNS", L"");
AutoSetting<BOOL> g_dhcp(L"DHCP", true, REG_DWORD);

// overrided slider proc
LRESULT CALLBACK VolumeProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );
LRESULT CALLBACK ProgressProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );
void format_time(int time, wchar_t *out);
void format_time_noms(int time, wchar_t *out);
void format_time2(int current, int total, wchar_t *out);
HRESULT ZHUZHU_resize_window(HWND hwnd);

HWND focus = NULL;
BOOL dhcp_enable;
static INT_PTR CALLBACK threater_countrol_proc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg ) 
	{

	case WM_DRAWITEM:
		LPDRAWITEMSTRUCT lpdis;
		//int id;
		//id = (int)wParam;
		lpdis = (LPDRAWITEMSTRUCT) lParam;
		char d[200];
		sprintf(d, "%x, %x\n", lpdis->itemState, lpdis->itemAction);
		OutputDebugStringA(d);
		FillRect(lpdis->hDC, &lpdis->rcItem, (HBRUSH)(lpdis->itemState & ODS_SELECTED ? GetStockObject(BLACK_BRUSH) : GetStockObject(WHITE_BRUSH)));

		break;

	case WM_HSCROLL:
		if (GetDlgCtrlID((HWND)lParam) == IDC_ZHU_PROGRESS)
		{
			HWND hProgress = (HWND)lParam;
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

		if (GetDlgCtrlID((HWND)lParam) == IDC_ZHU_VOLUME)
		{
			double v = ((double)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0) / 1000);
			player->set_volume(v);
		}

		if (GetDlgCtrlID((HWND)lParam) == IDC_ZHU_parallax)
		{
			double v = ((double)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0) / 500 - 1);
			player->set_subtitle_parallax(v*MAX_PARALLAX);
		}
		if (GetDlgCtrlID((HWND)lParam) == IDC_ZHU_POS_X)
		{
			double x, y;
			player->get_subtitle_pos(&x, &y);
			x = ((double)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0) / 1000);
			player->set_subtitle_pos(x, y);
		}
		if (GetDlgCtrlID((HWND)lParam) == IDC_ZHU_POS_Y)
		{
			double x, y;
			player->get_subtitle_pos(&x, &y);
			int l = SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
			y = ((double)SendMessage((HWND)lParam, TBM_GETPOS, 0, 0) / 1000 - 0.5 + 0.95);
			player->set_subtitle_pos(x, y);
		}
		break;

	case WM_TIMER:
		{
			int total = 0;
			player->total(&total);
			int current = 0;
			player->tell(&current);

			// play button
			if (player->is_playing())
				SetWindowTextW(GetDlgItem(hDlg, IDC_ZHU_PLAY), C(L"Pause"));
			else
				SetWindowTextW(GetDlgItem(hDlg, IDC_ZHU_PLAY), C(L"Play"));			

			if (!is_draging && player->m_file_loaded)
			{
				SendMessage(GetDlgItem(hDlg, IDC_ZHU_PROGRESS), TBM_SETPOS, (WPARAM)TRUE, (LPARAM)(1000*((double)current/total)));
				wchar_t tmp[200];
				format_time_noms(total, tmp);
				SetDlgItemTextW(hDlg, IDC_ZHU_TOTAL, tmp);
				format_time(current, tmp);
				SetDlgItemTextW(hDlg, IDC_ZHU_CURRENT, tmp);
			}

			else if (!is_draging)
			{
				SetDlgItemTextW(hDlg, IDC_ZHU_TOTAL, L"");
				SetDlgItemTextW(hDlg, IDC_ZHU_CURRENT, L"");
			}

			if (player->is_closed())
				EndDialog(hDlg, 0);
		}
		break;

	case WM_INITDIALOG:
		SetTimer(hDlg, 0, 33, NULL);
		player->set_theater(hDlg);
		localize_window(hDlg);
		ZHUZHU_resize_window(hDlg);
		detect_monitors();
		RECT pos;
		get_mixed_monitor_by_id(0, &pos, NULL);
		SIZE size;
		size.cx = 720;
		size.cy = 480;
		if (get_special_size_physical_monitor(size).right != 0)
			pos = get_special_size_physical_monitor(size);

		SetWindowPos(hDlg, NULL, pos.left, pos.top, size.cx, size.cy, NULL);
		SendMessage(GetDlgItem(hDlg, IDC_ZHU_PROGRESS), TBM_SETRANGEMAX, (WPARAM)TRUE, (LPARAM)1000);
		SendMessage(GetDlgItem(hDlg, IDC_ZHU_VOLUME), TBM_SETRANGEMAX, (WPARAM)TRUE, (LPARAM)1000);
		SendMessage(GetDlgItem(hDlg, IDC_ZHU_parallax), TBM_SETRANGEMAX, (WPARAM)TRUE, (LPARAM)1000);
		SendMessage(GetDlgItem(hDlg, IDC_ZHU_POS_X), TBM_SETRANGEMAX, (WPARAM)TRUE, (LPARAM)1000);
		SendMessage(GetDlgItem(hDlg, IDC_ZHU_POS_Y), TBM_SETRANGEMAX, (WPARAM)TRUE, (LPARAM)1000);


		{
			int parallax = 0;
			double x = 0.5, y = 0.95;
			double volume = 0;
			player->get_subtitle_parallax(&parallax);
			player->get_subtitle_pos(&x, &y);
			player->get_volume(&volume);
			SendMessage(GetDlgItem(hDlg, IDC_ZHU_parallax), TBM_SETPOS, (WPARAM)TRUE, (LPARAM)(500+parallax*500/MAX_PARALLAX));
			SendMessage(GetDlgItem(hDlg, IDC_ZHU_POS_X), TBM_SETPOS, (WPARAM)TRUE, (LPARAM)(1000*x));
			SendMessage(GetDlgItem(hDlg, IDC_ZHU_POS_Y), TBM_SETPOS, (WPARAM)TRUE, (LPARAM)((y-0.95+0.5)*1000));
			SendMessage(GetDlgItem(hDlg, IDC_ZHU_VOLUME), TBM_SETPOS, (WPARAM)TRUE, (LPARAM)(volume*1000));

		
			// audio devices
			for(int i=0; i<4; i++)
			{
				if (i>= get_audio_device_count())
					ShowWindow(GetDlgItem(hDlg, IDC_DEVICE1+i), SW_HIDE);
				else
				{
					wchar_t tmp[4096];
					get_audio_device_name(i, tmp);
					SetDlgItemTextW(hDlg, IDC_DEVICE1+i, tmp);
				}
			}

			Button_SetCheck (GetDlgItem(hDlg, IDC_DEVICE1+get_default_audio_device()), TRUE);

			// audio modes
			int mode = -1;		// 0: stereo, 1:5.1, 2:7.1, 3:bitstreaming
			switch((int)player->m_channel)
			{
			case -1:
				mode = 3;
				break;
			case 2:
				mode = 0;
				break;
			case 6:
				mode = 1;
				break;
			case 8:
				mode = 2;
				break;				
			}
			Button_SetCheck (GetDlgItem(hDlg, ID_STEREO+mode), TRUE);

			// video device
			if((int)player->m_output_mode == hd3d)
				Button_SetCheck (GetDlgItem(hDlg, IDC_HD3D), TRUE);
			if((int)player->m_output_mode == (int)dual_window)
				Button_SetCheck (GetDlgItem(hDlg, IDC_DUALPROJECTOR), TRUE);

			// monitors
			int select1 = -1;
			int select2 = -1;
			for(int i=0; i<get_mixed_monitor_count(true, true); i++)
			{
				RECT rect;
				wchar_t tmp[256];

				get_mixed_monitor_by_id(i, &rect, tmp, true, true);

				select1 = compare_rect(rect, player->m_screen1) ? i : select1;
				select2 = compare_rect(rect, player->m_screen2) ? i : select2;

				SendMessage(GetDlgItem(hDlg, IDC_OUTPUT1), CB_ADDSTRING, 0, (LPARAM)tmp);
				SendMessage(GetDlgItem(hDlg, IDC_OUTPUT2), CB_ADDSTRING, 0, (LPARAM)tmp);
			}
			SendMessage(GetDlgItem(hDlg, IDC_OUTPUT1), CB_SETCURSEL , select1, 0);
			SendMessage(GetDlgItem(hDlg, IDC_OUTPUT2), CB_SETCURSEL , select2, 0);

			// Network
			Button_SetCheck (GetDlgItem(hDlg, g_dhcp ? IDC_DHCP : IDC_MANUAL), TRUE);
			SendMessage(hDlg, WM_COMMAND, g_dhcp ? IDC_DHCP : IDC_MANUAL, 0);
			AutoSettingString *tbl[] = {&g_IP, &g_subnet_mask, &g_gateway, &g_DNS};
			DWORD ctrlid_tbl[] = {IDC_IP1, IDC_MASK1, IDC_GATE1, IDC_DNS1};
			for(int i=0; i<4; i++)
			{
				wchar_t *tmp[4] = {0};
				wcsexplode(*tbl[i], L".", tmp, 4);

				for(int j=0; j<4; j++)
				{
					if (tmp[j])
					{
						SetDlgItemTextW(hDlg, ctrlid_tbl[i]+j, tmp[j]);
						delete tmp[j];
					}
				}
			}


		}
		break;

	case WM_COMMAND:

		int id;
		id = LOWORD(wParam);

		switch(id)
		{
		case IDC_IP1:
		case IDC_IP2:
		case IDC_IP3:
		case IDC_IP4:
		case IDC_MASK1:
		case IDC_MASK2:
		case IDC_MASK3:
		case IDC_MASK4:
		case IDC_GATE1:
		case IDC_GATE2:
		case IDC_GATE3:
		case IDC_GATE4:
		case IDC_DNS1:
		case IDC_DNS2:
		case IDC_DNS3:
		case IDC_DNS4:
			if (HIWORD(wParam) == EN_SETFOCUS)
				focus = (HWND)lParam;
			break;

		case IDC_0:
		case IDC_1:
		case IDC_2:
		case IDC_3:
		case IDC_4:
		case IDC_5:
		case IDC_6:
		case IDC_7:
		case IDC_8:
		case IDC_9:
			if (focus)
			{
				wchar_t tmp[256] = {0};
				wchar_t tmp2[20];
				swprintf(tmp2, 20, L"%d", id - IDC_0);
				GetWindowTextW(focus, tmp, 255);
				wcscat(tmp, tmp2);
				SetWindowTextW(focus, tmp);
			}
			break;
		case IDC_B:
			if (focus)
			{
				wchar_t tmp[256] = {0};
				GetWindowTextW(focus, tmp, 255);
				if (wcslen(tmp)>0)
					tmp[wcslen(tmp)-1] = NULL;
				SetWindowTextW(focus, tmp);
			}
			break;

		case IDC_DHCP:
		case IDC_MANUAL:
			{
				dhcp_enable = id == IDC_DHCP;
				focus = NULL;
				int ids[] = {IDC_IP1, IDC_MASK1, IDC_GATE1, IDC_DNS1};
				for(int i=0; i<4; i++)
					for(int j=0; j<4; j++)
				{
					EnableWindow(GetDlgItem(hDlg, ids[i]+j), !dhcp_enable);
				}

				for(int i=0; i<10; i++)
					EnableWindow(GetDlgItem(hDlg, IDC_0+i), !dhcp_enable);
				EnableWindow(GetDlgItem(hDlg, IDC_B), !dhcp_enable);
			}
			break;

		case IDC_AUDIO_MODE:
			DialogBox(inst, MAKEINTRESOURCE(IDD_ZHUZHU_AUDIO), hDlg, threater_countrol_proc);
			player->set_theater(hDlg);
			break;
		case IDC_AUDIO_DEVICE:
			DialogBox(inst, MAKEINTRESOURCE(IDD_ZHUZHU_AUDIO_DEVICE), hDlg, threater_countrol_proc);
			player->set_theater(hDlg);
			break;
		case IDC_VIDEO_DEVICE:
			DialogBox(inst, MAKEINTRESOURCE(IDD_ZHUZHU_VIDEO), hDlg, threater_countrol_proc);
			player->set_theater(hDlg);
			break;
		case IDC_NETWORK:
			DialogBox(inst, MAKEINTRESOURCE(IDD_ZHUZHU_NETWORK), hDlg, threater_countrol_proc);
			player->set_theater(hDlg);
			break;
		case IDC_SYSTEM:
			DialogBox(inst, MAKEINTRESOURCE(IDD_ZHUZHU_SYSTEM), hDlg, threater_countrol_proc);
			player->set_theater(hDlg);
			break;

		case IDOK:
		case IDCANCEL:
			if (id == IDOK)
			{
				// save settings here

				if (GetDlgItem(hDlg, IDC_HD3D))
				{
					// Video
					player->set_output_monitor(0, SendMessage(GetDlgItem(hDlg, IDC_OUTPUT1), CB_GETCURSEL, 0, 0));
					player->set_output_monitor(1, SendMessage(GetDlgItem(hDlg, IDC_OUTPUT2), CB_GETCURSEL, 0, 0));
					player->set_output_mode(Button_GetCheck(GetDlgItem(hDlg, IDC_HD3D)) ? hd3d : dual_window);
				}

				if (GetDlgItem(hDlg, IDC_B))
				{
					//Network
					g_dhcp = Button_GetCheck (GetDlgItem(hDlg, IDC_DHCP));
					AutoSettingString *tbl[] = {&g_IP, &g_subnet_mask, &g_gateway, &g_DNS};
					DWORD ctrlid_tbl[] = {IDC_IP1, IDC_MASK1, IDC_GATE1, IDC_DNS1};
					for(int i=0; i<4; i++)
					{
						wchar_t tmp[200] = {0};

						for(int j=0; j<4; j++)
						{
							wchar_t tmp2[50] = {0};
							GetDlgItemTextW(hDlg, ctrlid_tbl[i]+j, tmp2, 49);
							if (j>0&&j<4)
								wcscat(tmp, L".");
							wcscat(tmp, tmp2);
						}
						*tbl[i] = tmp;
						tbl[i]->save();
					}

					USES_CONVERSION;
					if (g_dhcp)
						setip_dhcp(NULL);
					else
						setip(W2A(g_IP), W2A(g_subnet_mask), W2A(g_gateway), W2A(g_DNS));
				}

				if (GetDlgItem(hDlg, IDC_DEVICE1))
				{
					//AUdioDevice
					for(int i=0; i<get_audio_device_count(); i++)
						if(Button_GetCheck (GetDlgItem(hDlg, IDC_DEVICE1+i)))
						{
							active_audio_device(i);
							break;
						}
				}
				if (GetDlgItem(hDlg, ID_STEREO))
				{
					//AudioMode!
					// 0: stereo, 1:5.1, 2:7.1, 3:bitstreaming
					int tbl[] = {2, 6, 8, -1};
					for(int i=0; i<4; i++)
						if(Button_GetCheck (GetDlgItem(hDlg, ID_STEREO+i)))
							player->set_output_channel(tbl[i]);
				}
			}
			EndDialog(hDlg, 0);
			break;



		case IDC_ZHUZHU_OPEN:
			PostMessage(player->get_window(1), WM_COMMAND, ID_OPENFILE, 0);
			break;
		case IDC_ZHU_BLURAY:
			player->popup_menu(hDlg, 2);
			break;
		case IDC_ZHU_SWAP:
			PostMessage(player->get_window(1), WM_COMMAND, ID_SWAPEYES, 0);
			break;
		case IDC_ZHU_STOP:
			player->reset();
			break;
		case IDC_ZHU_PLAY:
			player->pause();
			break;
		case IDC_ZHU_FULL:
			player->toggle_fullscreen();
			break;
		case IDC_ZHU_COLOR:
			PostMessage(player->get_window(1), WM_COMMAND, ID_VIDEO_ADJUSTCOLOR, 0);
			break;
		case IDC_ZHU_AUDIO:
			player->popup_menu(hDlg, 6);
			break;
		case IDC_ZHU_SUBTITLE:
			player->popup_menu(hDlg, 7);
			break;


		default:
			PostMessage(player->get_window(1), msg, wParam, lParam);
			break;
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


HRESULT dwindow_dll_go_zhuzhu(HINSTANCE inst, HWND owner, Iplayer *p)
{
	player = (dx_player*)p;
	zhuzhu::inst = inst;

	int o = DialogBox(inst, MAKEINTRESOURCE(IDD_ZHUZHU_MAIN), owner, threater_countrol_proc);

	return S_OK;
}

HRESULT dwindow_dll_init_zhuzhu(char *passkey_big, int rev)
{
	return S_OK;
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


BOOL CALLBACK ZHUZHU_resize_proc(HWND hwnd, LPARAM lParam)
{
	RECT rect;
	GetWindowRect(hwnd, &rect);
	wchar_t tmp[1024];
	GetWindowTextW(hwnd, tmp, 1024);
	//SetWindowPos(hwnd, NULL, rect.left, rect.top, (rect.right-rect.left)*720/800, rect.bottom-rect.top, SWP_NOMOVE);

	MoveWindow(hwnd, rect.left*720/800, rect.top, (rect.right-rect.left)*720/800, rect.bottom-rect.top, TRUE);

	return TRUE;
}

HRESULT ZHUZHU_resize_window(HWND hwnd)
{
	ZHUZHU_resize_proc(hwnd, 0);
	EnumChildWindows(hwnd, ZHUZHU_resize_proc, NULL);
	return S_OK;
}

}