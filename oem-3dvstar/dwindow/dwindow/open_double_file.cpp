#include "open_double_file.h"

#include "global_funcs.h"
#include <wchar.h>

wchar_t *left = NULL;
wchar_t *right = NULL;

INT_PTR CALLBACK double_file_dialog_proc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg ) 
	{
	case WM_COMMAND:
		{
			int id = LOWORD(wParam);
			if (id == IDOK)
			{
				GetDlgItemTextW(hDlg, IDC_LEFT, left, MAX_PATH);
				GetDlgItemTextW(hDlg, IDC_RIGHT, right, MAX_PATH);

				EndDialog(hDlg, 0);
			}
			else if (id == IDCANCEL)
			{
				EndDialog(hDlg, -1);
			}

			else if (id == IDC_BROWSE_LEFT || id == IDC_BROWSE_RIGHT)
			{
				wchar_t file[MAX_PATH] = L"";
				if (open_file_dlg(file, hDlg, 
					L"All Supported files\0"
					L"*.mp4;*.mkv;*.mkv3d;*.mkv2d;*.mk3d;*.avi;*.rmvb;*.wmv;*.avs;*.ts;*.m2ts;*.ssif;*.mpls;*.3dv;*.e3d;*.iso;*.3dp;*.mpo;*.jps;*.pns;*.jpg;*.png;*.gif;*.psd;*.bmp\0"
					L"Video files\0"
					L"*.mp4;*.mkv;*.mkv3d;*.mkv2d;*.mk3d;*.avi;*.rmvb;*.wmv;*.avs;*.ts;*.m2ts;*.ssif;*.mpls;*.3dv;*.e3d;*.iso\0"
					L"Picture files\0"
					L"*.3dp;*.mpo;*.jps;*.pns;*.jpg;*.png;*.gif;*.psd;*.bmp\0"
					L"All Files\0"
					L"*.*\0"
					L"\0\0"))
				{
					SetDlgItemTextW(hDlg, id == IDC_BROWSE_LEFT ? IDC_LEFT : IDC_RIGHT, file);
				}
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

HRESULT open_double_file(HINSTANCE inst, HWND parent, wchar_t *left, wchar_t *right)
{
	if (!left || !right)
		return E_POINTER;

	::left = left;
	::right = right;

	int o = DialogBox(inst, MAKEINTRESOURCE(IDD_DOUBLEFILE), parent, double_file_dialog_proc);
	return  o == 0 ? S_OK : E_FAIL;
}