#include "update_confirm_window.h"

#include "AboudWindow.h"
#include "global_funcs.h"
#include "..\my12doom_revision.h"
#include "update.h"
#include "resource.h"

INT_PTR CALLBACK update_window_proc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg ) 
	{
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_UPDATE:
			EndDialog(hDlg, update_update);
			break;
		case IDC_CANCEL:
			EndDialog(hDlg, update_cancel);
			break;
		case IDC_DONTASK:
			EndDialog(hDlg, update_dontask);
			break;
		}
		break;

	case WM_INITDIALOG:
		SetForegroundWindow(hDlg);
		SetWindowPos(hDlg, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE );
		localize_window(hDlg);
		wchar_t description[20480];
		get_update_result(description, NULL, NULL);
		SetDlgItemTextW(hDlg, IDC_DESCRIPTION, description);
		break;

	case WM_CLOSE:
		EndDialog(hDlg, update_cancel);
		break;

	default:
		return FALSE;
	}

	return TRUE; // Handled message
}

int show_update_confirm(HWND parent)
{
	return DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_UPDATE), parent, update_window_proc);
}