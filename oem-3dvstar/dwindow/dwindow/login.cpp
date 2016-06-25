#include <Windows.h>
#include <time.h>
#include "global_funcs.h"
#include "..\AESFile\rijndael.h"
#include "login.h"
#include "resource.h"
#include "Hyperlink.h"

int on_command(HWND hWnd, int uid);
int init_dialog(HWND hWnd);
DWORD WINAPI spinner_thread(LPVOID p);
HWND g_login_window;
CHyperlink register_hyperlink;
#define REGISTER_FINISH (WM_USER+1)

INT_PTR CALLBACK register_proc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg ) 
	{
	case WM_COMMAND:
		on_command(hDlg, LOWORD(wParam));
		break;

	case WM_INITDIALOG:
		g_login_window = hDlg;
		localize_window(hDlg);
		init_dialog(hDlg);
		break;

	case WM_CLOSE:
		EndDialog(hDlg, 0);
		break;

	default:
		return FALSE;
	}

	return TRUE; // Handled message
}


int check_login()
{
retry:
	load_passkey();
	if (FAILED(check_passkey()))
	{
	if (g_bar_server[0] == NULL)
	{
#ifdef nologin
		int o = (int)DialogBox( NULL, MAKEINTRESOURCE(IDD_USERID), NULL, register_proc );
#else
#ifdef VSTAR
		AutoSetting<BOOL> expired(L"VSTAR", FALSE, REG_DWORD);

		if (expired)
		{
			MessageBoxW(NULL, C(L"This version is marked as expired, please vist http://bo3d.net/download and download the latest version."), C(L"New Update ..."), MB_OK);
			ShellExecuteW(NULL, L"open", L"http://bo3d.net/download", NULL, NULL, SW_SHOWNORMAL);
			ExitProcess(-1);
		}
#else
		int o = (int)DialogBox( NULL, MAKEINTRESOURCE(IDD_USERID_PAYED), NULL, register_proc );
#endif
#endif
		if (!is_trial_version())
			return 0;
	}
	else
	{
		if (MessageBoxW(0, C(L"System initialization failed : server not found, the program will exit now."), L"Error", MB_RETRYCANCEL) == IDRETRY)
			goto retry;
	}
}
#ifndef no_dual_projector
else if (is_trial_version())
	MessageBoxW(NULL, C(L"You are using a trial copy of DWindow, each clip will play normally for 10 minutes, after that the picture will become grayscale.\nYou can reopen it to play normally for another 10 minutes.\nRegister to remove this limitation."), L"....", MB_ICONINFORMATION);
#endif


	save_passkey();

	BasicRsaCheck();

}


int init_dialog(HWND hWnd)
{
	//SetWindowTextW(hWnd, C(L"Enter User ID"));
	register_hyperlink.create(IDC_REGISTERUSER, hWnd, "http://www.bo3d.net/user/");
#ifdef nologin
	on_command(hWnd, ID_TRIAL);
#endif
	return 0;
}

DWORD WINAPI register_download_thread(LPVOID puid)
{
	int uid = (int)puid;

	char username[512];
	GetWindowTextA(GetDlgItem(g_login_window, IDC_EDIT1), username, 512);

	char password[512];
	GetWindowTextA(GetDlgItem(g_login_window, IDC_EDIT2), password, 512);

	// SHA1 it!
	unsigned char sha1[20];
	SHA1Hash(sha1, (unsigned char*)password, strlen(password));

	// generate message
	dwindow_message_uncrypt message;
	memset(&message, 0, sizeof(message));
	message.zero = 0;
	message.client_rev = my12doom_rev;
	memcpy(message.passkey, username, 32);
	memcpy(message.requested_hash, sha1, 20);
	memcpy(message.password_uncrypted, password, min(19, strlen(password)));
	message.password_uncrypted[min(19, strlen(password))] = NULL;
	message.client_time = _time64(NULL);
	for(int i=0; i<32; i++)
		message.random_AES_key[i] = rand() &0xff;
	unsigned char encrypted_message[128];
	RSA_dwindow_network_public(&message, encrypted_message);

	char url[512];
	strcpy(url, g_server_address);
	strcat(url, uid == IDOK ? g_server_gen_key : g_server_free);
	strcat(url, "?");
	char tmp[3];
	for(int i=0; i<128; i++)
	{
		wsprintfA(tmp, "%02X", encrypted_message[i]);
		strcat(url, tmp);
	}

	char downloaded[800] = "";
	memset(downloaded, 0, 400);
	int size = 400;
	HANDLE spinner = CreateThread(NULL, NULL, spinner_thread, (LPVOID)uid, NULL, NULL);
	download_url(url, downloaded, &size);
	TerminateThread(spinner, -1);
	SetWindowTextW(GetDlgItem(g_login_window, uid), uid == IDOK ? C(L"Login") : C(L"Start Activation"));

#ifdef DEBUG
	OutputDebugStringA(url);
	OutputDebugStringA("\n");
	OutputDebugStringA(downloaded);
#endif

	if (strlen(downloaded) == 256)
	{
		unsigned char new_key[256];
		for(int i=0; i<128; i++)
			sscanf(downloaded+i*2, "%02X", new_key+i);
		AESCryptor aes;
		aes.set_key(message.random_AES_key, 256);
		for(int i=0; i<128; i+=16)
			aes.decrypt(new_key+i, new_key+i);

		memcpy(&g_passkey_big, new_key, 128);

		save_passkey();
		mytime(true);

		if (!is_trial_version())
		{
			MessageBoxW(g_login_window, C(L"This program will RESTART NOW to activate new user id."), C(L"Exiting"), MB_ICONINFORMATION);
			restart_this_program();
		}

		EndDialog(g_login_window, IDOK);
	}
	else
	{
		int len = strlen(downloaded);
		USES_CONVERSION;
		wchar_t txt[400];
		wcscpy(txt, C(L"Activation Failed, Server Response:\n."));
		wcscat(txt, A2W(downloaded));
		MessageBoxW(g_login_window, txt, C(L"Exiting"), MB_ICONERROR);

		EnableWindow(GetDlgItem(g_login_window, uid), TRUE);
	}
	return 0;
}

DWORD WINAPI spinner_thread(LPVOID p)
{
	int uid = (int)p;
	int c = 0;
	HWND o = GetDlgItem(g_login_window, uid);

	UTF82W name = uid == IDOK ? C(L"Login") : C(L"Start Activation");
	while(++c)
	{
		wchar_t tmp[200];
		wcscpy(tmp, name);
		for(int i = 0; i<(c%4); i++)
			wcscat(tmp, L".");

		SetWindowTextW(o, tmp);
		Sleep(800);
	}

	return 0;
}

int on_command(HWND hWnd, int uid)
{
	if (uid == IDOK || uid == ID_TRIAL)
	{
		EnableWindow(GetDlgItem(g_login_window, uid), FALSE);

		CreateThread(NULL, NULL, register_download_thread, (LPVOID)uid, NULL, NULL);
	}
	else if (uid == IDCANCEL)
	{
		EndDialog(hWnd, IDCANCEL);
	}

	return 0;
}