#include "fullscreen_modes_select.h"
#include "resource.h"
#include "global_funcs.h"

D3DDISPLAYMODE *g_modes;
int g_modes_count;
int g_selected;
INT_PTR CALLBACK fullscreen_modes_proc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );

wchar_t fmt_table[200][20] = {0};

void init_fmt_table()
{
	wcscpy(fmt_table[D3DFMT_R8G8B8], L"D3DFMT_R8G8B8");
	wcscpy(fmt_table[D3DFMT_A8R8G8B8],L"D3DFMT_A8R8G8B8");
	wcscpy(fmt_table[D3DFMT_X8R8G8B8],L"D3DFMT_X8R8G8B8");
	wcscpy(fmt_table[D3DFMT_R5G6B5], L"D3DFMT_R5G6B5");
	wcscpy(fmt_table[D3DFMT_A2R10G10B10], L"D3DFMT_A2B10G10R10");
}

HRESULT select_fullscreen_mode(HINSTANCE instance, HWND parent, D3DDISPLAYMODE *modes, int modes_count, int *selected)		// return S_FALSE on cancel, selected undefined in this case
{
	g_modes = modes;
	g_modes_count = modes_count;
	g_selected = selected ? *selected : -1;
	init_fmt_table();

	int o = DialogBoxW(instance, MAKEINTRESOURCEW(IDD_FULLSCREEN_MODES), parent, fullscreen_modes_proc);

	g_selected = g_selected>=0 && g_selected<g_modes_count ? g_selected : -1;

	if (selected)
		*selected = g_selected;

	return o == 0 ? S_OK : S_FALSE;
}



INT_PTR CALLBACK fullscreen_modes_proc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg ) 
	{
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			g_selected = SendMessageW(GetDlgItem(hDlg, IDC_COMBO1), CB_GETCURSEL, 0, 0);
			EndDialog(hDlg, 0);
		}
		else if (LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, -1);
		}
		break;

	case WM_INITDIALOG:
		{
			localize_window(hDlg);

			HWND combobox = GetDlgItem(hDlg, IDC_COMBO1);
			wchar_t tmp[1024];
			for(int i=0; i<g_modes_count; i++)
			{
				wchar_t *p = i>=0 && i<200 ? fmt_table[g_modes[i].Format] : NULL;
				wsprintfW(tmp, L"%dx%d @ %dHz (%s)\n", g_modes[i].Width, g_modes[i].Height,g_modes[i].RefreshRate, p&&p[0]?p:L"Unknown Format");
				SendMessageW(combobox, CB_ADDSTRING, NULL, (LPARAM)tmp);
			}
			SendMessageW(combobox, CB_ADDSTRING, NULL, (LPARAM)(const wchar_t*)C(L"Auto Detect"));
			SendMessageW(combobox, CB_SETCURSEL, g_selected>=0 && g_selected<g_modes_count ? g_selected : g_modes_count, 0);
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