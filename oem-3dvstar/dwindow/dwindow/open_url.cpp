#include "open_double_file.h"

#include "global_funcs.h"
#include <wchar.h>

static wchar_t *url = NULL;

static INT_PTR CALLBACK url_dlg_proc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg ) 
	{
	case WM_COMMAND:
		{
			int id = LOWORD(wParam);
			if (id == IDOK)
			{
				GetDlgItemTextW(hDlg, IDC_LEFT, url, MAX_PATH);

				EndDialog(hDlg, 0);
			}
			else if (id == IDCANCEL)
			{
				EndDialog(hDlg, -1);
			}
		}
		break;

	case WM_INITDIALOG:
		// localization
		localize_window(hDlg);
		break;

	case WM_CLOSE:
		EndDialog(hDlg, -1);
		break;

	default:
		return FALSE;
	}

	return TRUE; // Handled message
}

HRESULT open_URL(HINSTANCE inst, HWND parent, wchar_t *url)
{
	if (!url)
		return E_POINTER;

	::url = url;

	int o = DialogBox(inst, MAKEINTRESOURCE(IDD_URL), parent, url_dlg_proc);
	return  o == 0 ? S_OK : E_FAIL;
}