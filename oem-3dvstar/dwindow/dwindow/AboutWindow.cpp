#include "AboudWindow.h"
#include "global_funcs.h"
#include "Hyperlink.h"
#include "..\my12doom_revision.h"

CHyperlink hyperlink;
INT_PTR CALLBACK about_window_proc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg ) 
	{
	case WM_COMMAND:
		EndDialog(hDlg, 0);
		break;

	case WM_INITDIALOG:
		{
			localize_window(hDlg);

			hyperlink.create(IDC_BO3D, hDlg);


			// generate build date time
			wchar_t build_time[200];
			const unsigned char MonthStr[12][4] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov","Dec"};
			unsigned char temp_str[4] = {0, 0, 0, 0}, i = 0;

			int year, month = 0, day, hour, minute, second;
			sscanf(__DATE__, "%s %2d %4d", temp_str, &day, &year);
			sscanf(__TIME__, "%2d:%2d:%2d", &hour, &minute, &second);
			for (i = 0; i < 12; i++)
			{
				if (temp_str[0] == MonthStr[i][0] && temp_str[1] == MonthStr[i][1] && temp_str[2] == MonthStr[i][2])
				{
					month = i+1;
					break;
				}
			}
			wsprintfW(build_time, L"%d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, minute, second);


#ifdef dwindow_free
wchar_t version[200] = L"Free";
#endif

#ifdef dwindow_jz
wchar_t version[200] = L"Donate";
#endif

#ifdef dwindow_pro
wchar_t version[200] = L"Personal";
#endif

wcscpy(version, C(version));

#ifdef DEBUG
wcscat(version, C(L"(Debug)"));
#endif

			wchar_t * text = (wchar_t*)malloc(1024*1024);
			wsprintfW(text, C(L"DWindow %s version\r\nrevision %d\r\nBuild Time:%s\r\n"),
				version, my12doom_rev, build_time);

			// get activation expire time
			if (SUCCEEDED(check_passkey()) && !is_trial_version() && false)
			{
				wchar_t tmp[MAX_PATH];
				DWORD e[32];
				dwindow_passkey_big m1;
				BigNumberSetEqualdw(e, 65537, 32);
				RSA((DWORD*)&m1, (DWORD*)&g_passkey_big, e, (DWORD*)dwindow_n, 32);

				struct tm* tm = _localtime64(&m1.time_end);

				wsprintfW(tmp, C(L"Activated until %d-%02d-%02d %02d:%02d:%02d"),
					tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
					tm->tm_hour, tm->tm_min, tm->tm_sec);
				wcscat(text, tmp);
				memset(&m1, 0, sizeof(m1));
			}

			SetDlgItemTextW(hDlg, IDC_ABOUTTEXT, text);
			free(text);
		}
		break;

	case WM_CLOSE:
		EndDialog(hDlg, 0);
		break;

	default:
		return FALSE;
	}

	return TRUE; // Handled message
}

void ShowAbout(HWND parent)
{
	DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ABOUT), parent, about_window_proc);
}