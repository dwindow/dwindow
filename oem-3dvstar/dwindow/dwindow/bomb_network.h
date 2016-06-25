

DWORD WINAPI msg_thread(LPVOID msg)
{
	printf("服务器无响应");
	MessageBoxW(0, L"服务器无响应", L"Error", MB_ICONERROR);
	return 0;
}

DWORD WINAPI bomb_network_thread(LPVOID lpParame)
{
	if (g_bar_server[0])
	{
		// hearbeat for bar mode
		HANDLE msgthread = INVALID_HANDLE_VALUE;
		while (true)
		{
			if (GetTickCount() - g_last_bar_time > HEARTBEAT_TIMEOUT)
			{
				if (msgthread == INVALID_HANDLE_VALUE) msgthread = CreateThread(NULL, NULL, msg_thread, NULL, NULL, NULL);
				CreateThread(NULL, NULL, killer_thread, new DWORD(20000), NULL, NULL);
			}
			else
			{
				msgthread = INVALID_HANDLE_VALUE;
				Sleep(30000);
			}
			load_passkey();
		}
	}
	HWND parent_window = (HWND)lpParame;

 	Sleep(60*1000);


	dwindow_message_uncrypt message;
	memset(&message, 0, sizeof(message));
	message.zero = 0;
	message.client_rev = is_trial_version() ? my12doom_rev | 0x8000 : my12doom_rev;
	message.client_time = _time64(NULL);
	memcpy(message.passkey, g_passkey, 32);
	memset(message.requested_hash, 0, 20);
	srand((unsigned int)time(NULL));
	for(int i=0; i<32; i++)
		message.random_AES_key[i] = rand() & 0xff;

#ifdef VSTAR
	message.client_rev = my12doom_rev | 0x4000;
#endif
	unsigned char encrypted_message[128];
	RSA_dwindow_network_public(&message, encrypted_message);

	char url[512];
	char tmp[3];
	strcpy(url, g_server_address);
	strcat(url, g_server_reg_check);
	strcat(url, "?");
	for(int i=0; i<128; i++)
	{
		sprintf(tmp, "%02X", encrypted_message[i]);
		strcat(url, tmp);
	}

	char result[512];
	int size = 512;
	memset(result, 0, 512);
	while (FAILED(download_url(url, result, &size)))
		Sleep(300*1000);

	HRESULT hr = S_OK;
	if (strstr(result, "S_FALSE"))
		hr = S_FALSE;
	if (strstr(result, "S_OK"))
		hr = S_OK;
	if (strstr(result, "E_FAIL"))
		hr = E_FAIL;

#ifdef DEBUG
	OutputDebugStringA(url);
	OutputDebugStringA("\n");
	OutputDebugStringA(result);
	OutputDebugStringA("\n");
	OutputDebugStringA(result + 5);
#endif


#ifdef VSTAR
	if (strstr(result, "VSTAR") != NULL)
	{
		AutoSetting<BOOL> expired(L"VSTAR", FALSE, REG_DWORD);
		expired = TRUE;
	}
#endif

	if (hr != S_OK)
	{
		memset(g_passkey, 0, 32);
		memset(g_passkey_big, 0, 32);
		del_setting(L"passkey");

#ifndef DS_SHOW_MOUSE
#define DS_SHOW_MOUSE (WM_USER + 3)
#endif

		KillTimer(parent_window, 1);
		KillTimer(parent_window, 2);
		SendMessage(parent_window, DS_SHOW_MOUSE, TRUE, NULL);
#ifdef nologin
		int o = (int)DialogBox( NULL, MAKEINTRESOURCE(IDD_USERID), parent_window, register_proc );
#else
		int o = (int)DialogBox( NULL, MAKEINTRESOURCE(IDD_USERID_PAYED), parent_window, register_proc );
#endif
		TerminateProcess(GetCurrentProcess(), o);
	}
	else
	{

		char *downloaded = result + 5;
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

			if (SUCCEEDED(check_passkey()))
				save_passkey();
		}
	}


	return 1;
}