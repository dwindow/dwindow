#include <Windows.h>
#include <stdio.h>
#include <atlbase.h>
#include "resource.h"
#include "..\dwindow_bar\global_funcs_lite.h"
#include "E3DWriter.h"
#include "Commctrl.h"

HWND hwnd;
bool cancel = false;
HANDLE handle_worker_thread = INVALID_HANDLE_VALUE;
wchar_t in_file[MAX_PATH] = L"";
wchar_t out_file[MAX_PATH] = L"";
unsigned char src_hash[20] = "dhiwuaesrh";
unsigned char key[36] = ".dhwuairhfwa";

HRESULT show_ui(int type);
// 0 = show login ui only
// 1 = show normal ui only
// 2 = show progress bar and cancel button only
// other = hide all
INT_PTR CALLBACK main_proc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
int main();
DWORD WINAPI encoding_thread(LPVOID para);
bool open_file_dlg(wchar_t *pathname, HWND hDlg, bool do_open  = false, wchar_t *filter = NULL);

class gui_callback : public E3D_Writer_Progress
{
	HRESULT CB(int type, __int64 para1, __int64 para2);
} cb;


INT_PTR CALLBACK main_proc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg ) 
	{
	case WM_COMMAND:
		{
			int code = HIWORD(wParam);
			int id = LOWORD(wParam);
			HWND edit_ctrl = (HWND) lParam;

			if (code == EN_CHANGE && id == IDC_IN)
			{
				GetDlgItemTextW(hDlg, IDC_IN, in_file, MAX_PATH);
				unsigned char hash[20];
				if (0 == video_checksum(in_file, (DWORD*)hash))
				{
					wchar_t tmp[60] = L"";
					wchar_t tmp2[20];
					for(int i=0; i<20; i++)
					{
						wsprintfW(tmp2, L"%02x", hash[i]);
						wcscat(tmp, tmp2);
					}
					SetDlgItemTextW(hDlg, IDC_HASH, tmp);
					SetDlgItemTextA(hDlg, IDC_KEY, "");
				}
				else
				{
					SetDlgItemTextW(hDlg, IDC_HASH, L"输入文件打开失败");
				}
			}

			if (id == IDC_LOGIN)
			{
				// gen message
				dwindow_message_uncrypt msg;
				memset(&msg, 0, sizeof(msg));
				msg.client_time = _time64(NULL);
				strcpy((char*)msg.passkey, "login:admin");
				srand(time(NULL));
				for(int i=0; i<sizeof(msg.random_AES_key); i++)
					msg.random_AES_key[i] = rand() % 256;

				char password[20];
				GetDlgItemTextA(hDlg, IDC_PASSWORD, password, 20);
				SHA1Hash(msg.requested_hash, (unsigned char*)password, strlen(password));
				unsigned char encrypted_message[128];
				RSA_dwindow_network_public(&msg, encrypted_message);

				// url and download
				char url[512];
				strcpy(url, g_server_address);
				strcat(url, g_server_E3D_encoder);
				strcat(url, "?");
				char tmp[3];
				for(int i=0; i<128; i++)
				{
					wsprintfA(tmp, "%02X", encrypted_message[i]);
					strcat(url, tmp);
				}

				char downloaded[400] = "连接失败";
				memset(downloaded, 0, 400);
				download_url(url, downloaded, 399);

				printf(downloaded);
				if (strcmp(downloaded, "OK") == 0)
					show_ui(1);
				else
				{
					sprintf(url, "登录失败：%s", downloaded);
					MessageBoxA(hDlg, url, "Error", MB_ICONERROR);
				}
			}

			else if (id == IDC_GETKEY)
			{
				GetDlgItemTextW(hDlg, IDC_IN, in_file, MAX_PATH);

				// gen message
				dwindow_message_uncrypt msg;
				memset(&msg, 0, sizeof(msg));
				if (-1 == video_checksum(in_file, (DWORD*)msg.password_uncrypted))			//password_uncrypted is used as requested_hash due to they have same size
					break;

				msg.client_time = _time64(NULL);
				wchar_t *file = in_file + wcslen(in_file) - 1;
				while (*(file-1) != L'\\' && file > in_file) file --;

				char cmd[256];
				USES_CONVERSION;
				sprintf(cmd, "getkey:admin:%s", W2A(file));
				memcpy(msg.passkey, cmd, 32);
				srand(time(NULL));
				for(int i=0; i<sizeof(msg.random_AES_key); i++)
					msg.random_AES_key[i] = rand() % 256;

				char password[20];
				GetDlgItemTextA(hDlg, IDC_PASSWORD, password, 20);
				SHA1Hash(msg.requested_hash, (unsigned char*)password, strlen(password));
				unsigned char encrypted_message[128];
				RSA_dwindow_network_public(&msg, encrypted_message);

				// url and download
				char url[512];
				strcpy(url, g_server_address);
				strcat(url, g_server_E3D_encoder);
				strcat(url, "?");
				char tmp[3];
				for(int i=0; i<128; i++)
				{
					wsprintfA(tmp, "%02X", encrypted_message[i]);
					strcat(url, tmp);
				}

				char downloaded[400] = "连接失败";
				memset(downloaded, 0, 400);
				download_url(url, downloaded, 399);

				printf(downloaded);
				if (strcmp(downloaded, "OK") == 0)
				{
					char *str_e3d_key = downloaded+3;

					for(int i=0; i<32; i++)
						sscanf(str_e3d_key+i*2, "%02X", key+i);

					AESCryptor aes;
					aes.set_key(msg.random_AES_key, 256);
					aes.decrypt(key, key);
					aes.decrypt(key+16, key+16);

					char tmp[10];
					str_e3d_key[0] = NULL;
					for(int i=0; i<32; i++)
					{
						sprintf(tmp, "%02x", key[i]);
						strcat(str_e3d_key, tmp);
					}
					

					SetDlgItemTextA(hDlg, IDC_KEY, str_e3d_key);

				}
				else
				{
					sprintf(url, "系统错误：\n%s", downloaded);
					MessageBoxA(hDlg, url, "Error", MB_ICONERROR);
				}
			}

			else if (id == IDC_GO)
			{
				char tmp[256];
				GetDlgItemTextA(hDlg, IDC_KEY, tmp, 256);
				if (strlen(tmp) != 64)
				{
					MessageBoxA(hDlg, "请先获取一个密匙", "...", MB_OK);
					break;
				}

				cancel = false;
				GetDlgItemTextW(hDlg, IDC_IN, in_file, MAX_PATH);
				GetDlgItemTextW(hDlg, IDC_OUT, out_file, MAX_PATH);
				SendMessage(GetDlgItem(hwnd, IDC_PROGRESS), PBM_SETPOS, 0, 0);
				handle_worker_thread = CreateThread(NULL, NULL, encoding_thread, NULL, NULL, NULL);
				show_ui(2);
			}

			else if (id == IDC_CANCEL)
			{
				cancel = true;
				if (INVALID_HANDLE_VALUE != handle_worker_thread)
					WaitForSingleObject(handle_worker_thread, INFINITE);
				show_ui(1);
			}
			else if (id == IDC_IN_BROWSE)
			{
				if (open_file_dlg(in_file, hDlg, true))
				{
					wcscpy(out_file, in_file);
					for(int i=wcslen(out_file)-1; i>=0; i--)
						if (out_file[i] == L'.')
						{
							out_file[i] = NULL;
							break;
						}
					wcscat(out_file, L".e3d");
					SetDlgItemTextW(hDlg, IDC_OUT, out_file);
				}
				SetDlgItemTextW(hDlg, IDC_IN, in_file);
			}
			else if (id == IDC_OUT_BROWSE)
			{
				open_file_dlg(out_file, hDlg, false, L"E3D files(*.e3d)\0*.e3d\0\0");
				SetDlgItemTextW(hDlg, IDC_OUT, out_file);
			}
		}
		break;

	case WM_INITDIALOG:
		hwnd = hDlg;
		show_ui(0);
		SendMessage(GetDlgItem(hwnd, IDC_PROGRESS), PBM_SETRANGE32, 0, 1000);
		SetDlgItemTextW(hDlg, IDC_IN, in_file);
		SetDlgItemTextW(hDlg, IDC_OUT, out_file);
		break;

	case WM_CLOSE:
		EndDialog(hDlg, 0);
		break;

	default:
		return FALSE;
	}

	return TRUE; // Handled message
}

HRESULT show_ui(int type)
// 0 = show login ui only
// 1 = show normal ui only
// 2 = show progress bar and cancel button only
// other = hide all
{
	// hide all
	ShowWindow(GetDlgItem(hwnd, IDC_PASSWORD), SW_HIDE);
	ShowWindow(GetDlgItem(hwnd, IDC_LOGIN), SW_HIDE);

	ShowWindow(GetDlgItem(hwnd, IDC_STATIC1), SW_HIDE);
	ShowWindow(GetDlgItem(hwnd, IDC_STATIC2), SW_HIDE);
	ShowWindow(GetDlgItem(hwnd, IDC_STATIC3), SW_HIDE);
	ShowWindow(GetDlgItem(hwnd, IDC_STATIC4), SW_HIDE);
	ShowWindow(GetDlgItem(hwnd, IDC_STATIC4), SW_HIDE);

	ShowWindow(GetDlgItem(hwnd, IDC_IN), SW_HIDE);
	ShowWindow(GetDlgItem(hwnd, IDC_OUT), SW_HIDE);
	ShowWindow(GetDlgItem(hwnd, IDC_HASH), SW_HIDE);
	ShowWindow(GetDlgItem(hwnd, IDC_KEY), SW_HIDE);

	ShowWindow(GetDlgItem(hwnd, IDC_IN_BROWSE), SW_HIDE);
	ShowWindow(GetDlgItem(hwnd, IDC_OUT_BROWSE), SW_HIDE);
	ShowWindow(GetDlgItem(hwnd, IDC_GETKEY), SW_HIDE);
	ShowWindow(GetDlgItem(hwnd, IDC_GO), SW_HIDE);

	ShowWindow(GetDlgItem(hwnd, IDC_PROGRESS), SW_HIDE);
	ShowWindow(GetDlgItem(hwnd, IDC_CANCEL), SW_HIDE);

	if (type == 0)		// show login ui
	{
		ShowWindow(GetDlgItem(hwnd, IDC_PASSWORD), SW_SHOW);
		ShowWindow(GetDlgItem(hwnd, IDC_LOGIN), SW_SHOW);

	}

	else if (type == 1) // show normal ui
	{
		ShowWindow(GetDlgItem(hwnd, IDC_STATIC1), SW_SHOW);
		ShowWindow(GetDlgItem(hwnd, IDC_STATIC2), SW_SHOW);
		ShowWindow(GetDlgItem(hwnd, IDC_STATIC3), SW_SHOW);
		ShowWindow(GetDlgItem(hwnd, IDC_STATIC4), SW_SHOW);
		ShowWindow(GetDlgItem(hwnd, IDC_STATIC4), SW_SHOW);

		ShowWindow(GetDlgItem(hwnd, IDC_IN), SW_SHOW);
		ShowWindow(GetDlgItem(hwnd, IDC_OUT), SW_SHOW);
		ShowWindow(GetDlgItem(hwnd, IDC_HASH), SW_SHOW);
		ShowWindow(GetDlgItem(hwnd, IDC_KEY), SW_SHOW);

		ShowWindow(GetDlgItem(hwnd, IDC_IN_BROWSE), SW_SHOW);
		ShowWindow(GetDlgItem(hwnd, IDC_OUT_BROWSE), SW_SHOW);
		ShowWindow(GetDlgItem(hwnd, IDC_GETKEY), SW_SHOW);
		ShowWindow(GetDlgItem(hwnd, IDC_GO), SW_SHOW);

		EnableWindow(GetDlgItem(hwnd, IDC_STATIC1), TRUE);
		EnableWindow(GetDlgItem(hwnd, IDC_STATIC2), TRUE);
		EnableWindow(GetDlgItem(hwnd, IDC_STATIC3), TRUE);
		EnableWindow(GetDlgItem(hwnd, IDC_STATIC4), TRUE);
		EnableWindow(GetDlgItem(hwnd, IDC_STATIC4), TRUE);

		EnableWindow(GetDlgItem(hwnd, IDC_IN), TRUE);
		EnableWindow(GetDlgItem(hwnd, IDC_OUT), TRUE);
		EnableWindow(GetDlgItem(hwnd, IDC_HASH), TRUE);
		EnableWindow(GetDlgItem(hwnd, IDC_KEY), TRUE);

		EnableWindow(GetDlgItem(hwnd, IDC_IN_BROWSE), TRUE);
		EnableWindow(GetDlgItem(hwnd, IDC_OUT_BROWSE), TRUE);
		EnableWindow(GetDlgItem(hwnd, IDC_GETKEY), TRUE);
		EnableWindow(GetDlgItem(hwnd, IDC_GO), TRUE);


	}

	else if (type == 2) // show progress and cancel ui
	{
		ShowWindow(GetDlgItem(hwnd, IDC_STATIC1), SW_SHOW);
		ShowWindow(GetDlgItem(hwnd, IDC_STATIC2), SW_SHOW);
		ShowWindow(GetDlgItem(hwnd, IDC_STATIC3), SW_SHOW);
		ShowWindow(GetDlgItem(hwnd, IDC_STATIC4), SW_SHOW);
		ShowWindow(GetDlgItem(hwnd, IDC_STATIC4), SW_SHOW);

		ShowWindow(GetDlgItem(hwnd, IDC_IN), SW_SHOW);
		ShowWindow(GetDlgItem(hwnd, IDC_OUT), SW_SHOW);
		ShowWindow(GetDlgItem(hwnd, IDC_HASH), SW_SHOW);
		ShowWindow(GetDlgItem(hwnd, IDC_KEY), SW_SHOW);

		ShowWindow(GetDlgItem(hwnd, IDC_IN_BROWSE), SW_SHOW);
		ShowWindow(GetDlgItem(hwnd, IDC_OUT_BROWSE), SW_SHOW);
		ShowWindow(GetDlgItem(hwnd, IDC_GETKEY), SW_SHOW);

		EnableWindow(GetDlgItem(hwnd, IDC_STATIC1), FALSE);
		EnableWindow(GetDlgItem(hwnd, IDC_STATIC2), FALSE);
		EnableWindow(GetDlgItem(hwnd, IDC_STATIC3), FALSE);
		EnableWindow(GetDlgItem(hwnd, IDC_STATIC4), FALSE);
		EnableWindow(GetDlgItem(hwnd, IDC_STATIC4), FALSE);

		EnableWindow(GetDlgItem(hwnd, IDC_IN), FALSE);
		EnableWindow(GetDlgItem(hwnd, IDC_OUT), FALSE);
		EnableWindow(GetDlgItem(hwnd, IDC_HASH), FALSE);
		EnableWindow(GetDlgItem(hwnd, IDC_KEY), TRUE);

		EnableWindow(GetDlgItem(hwnd, IDC_IN_BROWSE), FALSE);
		EnableWindow(GetDlgItem(hwnd, IDC_OUT_BROWSE), FALSE);
		EnableWindow(GetDlgItem(hwnd, IDC_GETKEY), FALSE);

		ShowWindow(GetDlgItem(hwnd, IDC_PROGRESS), SW_SHOW);
		ShowWindow(GetDlgItem(hwnd, IDC_CANCEL), SW_SHOW);
	}

	return S_OK;
}

int main()
{
	return WinMain(GetModuleHandle(NULL), NULL, NULL, SW_SHOW);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// speed test
	/*
	AESCryptor aes;
	aes.set_key(key, 256);


	unsigned char source[16];
	int l = GetTickCount();
	for(int i=0; i<10000000; i++)
		aes.encrypt(source, source);

	int k = GetTickCount() - l;
	printf("%d KByte/s.\n", 160000000/k);
	*/

	return DialogBox(NULL, MAKEINTRESOURCE(IDD_DIALOG1), NULL, main_proc);
}

HRESULT gui_callback::CB(int type, __int64 para1, __int64 para2)
{
	if (type == 1)
		PostMessage(GetDlgItem(hwnd, IDC_PROGRESS), PBM_SETPOS, (DWORD)(para1*1000/para2), 0);

	return cancel ? S_FALSE : S_OK;
}

DWORD WINAPI encoding_thread(LPVOID para)
{
	video_checksum(in_file, (DWORD*)src_hash);
	int o = encode_file(in_file, out_file, (unsigned char*)key, src_hash, &cb);

	if (!cancel)
	{
		show_ui(1);
		MessageBoxA(hwnd, "操作完成", "...", MB_OK );
	}

	return 0;
}

bool open_file_dlg(wchar_t *pathname, HWND hDlg, bool do_open /* = false*/, wchar_t *filter/* = NULL*/)
{
	static wchar_t *default_filter = L"MKV files (*.mkv)\0"
		L"*.mkv\0"
		L"\0";

	if (filter == NULL) filter = default_filter;
	wchar_t strFileName[MAX_PATH] = L"";
	wchar_t strPath[MAX_PATH] = L"";

	wcsncpy(strFileName, pathname, MAX_PATH);
	wcsncpy(strPath, pathname, MAX_PATH);
	for(int i=(int)wcslen(strPath)-2; i>=0; i--)
		if (strPath[i] == L'\\')
		{
			strPath[i+1] = NULL;
			break;
		}

		OPENFILENAMEW ofn = { sizeof(OPENFILENAMEW), hDlg , NULL,
			filter, NULL,
			0, 1, strFileName, MAX_PATH, NULL, 0, strPath,
			(L"Open File"),
			do_open ? OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|OFN_ENABLESIZING : OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY, 0, 0,
			L".mp4", 0, NULL, NULL };

		int o = do_open ? GetOpenFileNameW( &ofn ) : GetSaveFileName( &ofn );
		if (o)
		{
			wcsncpy(pathname, strFileName, MAX_PATH);
			return true;
		}

		return false;
}