#include "latency_dialog.h"
#include "global_funcs.h"
#include <wchar.h>
#include <Commctrl.h>
#include "resource.h"

bool for_audio;
int t_latency;
double t_ratio;

INT_PTR CALLBACK latency_dialog_proc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg ) 
	{
	case WM_COMMAND:
		{
			int id = LOWORD(wParam);
			if (id == IDOK)
			{
				wchar_t tmp[200];
				GetDlgItemTextW(hDlg, IDC_EDIT2, tmp, 200);
				t_latency = _wtoi(tmp);

				GetDlgItemTextW(hDlg, IDC_EDIT3, tmp, 200);
				t_ratio = _wtof(tmp)/100;

				EndDialog(hDlg, 0);
			}
			else if (id == IDCANCEL)
			{
				EndDialog(hDlg, -1);
			}
			else if (id == IDC_RESET)
			{
				t_latency = 0;
				t_ratio = 1.0;
				latency_dialog_proc(hDlg, WM_INITDIALOG, 0, 0);
			}

		}
		break;
	case WM_NOTIFY:
		{
			NMUPDOWN *nmud = (NMUPDOWN*) lParam;
			if (nmud->hdr.code == UDN_DELTAPOS)
			{
				wchar_t tmp[200];
				GetDlgItemTextW(hDlg, IDC_EDIT2, tmp, 200);
				t_latency = _wtoi(tmp);

				GetDlgItemTextW(hDlg, IDC_EDIT3, tmp, 200);
				t_ratio = _wtof(tmp)/100;

				if (nmud->hdr.idFrom ==  IDC_SPIN1)
					t_latency += nmud->iDelta < 0 ? 100 : -100;
				if (nmud->hdr.idFrom ==  IDC_SPIN2)
					t_ratio += nmud->iDelta < 0 ? 0.001 : -0.001;

				latency_dialog_proc(hDlg, WM_INITDIALOG, 0, 0);
			}
		}
		break;

	case WM_INITDIALOG:
		{
			// localization
			localize_window(hDlg);

			if(for_audio)
			{
				SetDlgItemTextW(hDlg, IDC_HINT, C(L"Audio track stretching is not yet available."));
				EnableWindow(GetDlgItem(hDlg, IDC_EDIT3), FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_SPIN2), FALSE);
			}

			// value
			wchar_t tmp[200];
			swprintf(tmp, L"%d", t_latency);
			SetDlgItemTextW(hDlg, IDC_EDIT2, tmp);

			swprintf(tmp, L"%.2f", t_ratio*100);
			// eliminate tailing '0' and '.'
			wchar_t *p = tmp+wcslen(tmp)-1;
			while (*p == L'0')
			{
				*p = NULL;
				p--;
			}
			if (*p == L'.')
				*p = NULL;

			SetDlgItemTextW(hDlg, IDC_EDIT3, tmp);

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

HRESULT latency_modify_dialog(HINSTANCE inst, HWND parent, int *latency, double *ratio, bool for_audio)
{
	if (!latency || !ratio)
		return E_POINTER;

	t_latency = *latency;
	t_ratio = *ratio;
	::for_audio = for_audio;
	int o = DialogBox(inst, MAKEINTRESOURCE(IDD_LATENCY), parent, latency_dialog_proc);
	if (o == 0)
	{
		*latency = t_latency;
		*ratio = t_ratio;
		return S_OK;
	}
	else
		return E_FAIL;
}