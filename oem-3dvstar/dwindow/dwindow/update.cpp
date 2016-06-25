#include "update.h"
#include "global_funcs.h"
#include "..\lua\my12doom_lua.h"
#include "..\libass\charset.h"

DWORD WINAPI check_update_thread(LPVOID);
HANDLE g_thread = INVALID_HANDLE_VALUE;
HWND g_wnd;
DWORD g_message;

HRESULT start_check_for_update(HWND callback_window, DWORD callback_message)			// threaded, S_OK: thread started. S_FALSE: thread already started, this call is ignored, no callback will be made.
{
	if (g_thread != INVALID_HANDLE_VALUE)
		return S_FALSE;

	g_wnd = callback_window;
	g_message = callback_message;

	g_thread = CreateThread(NULL, NULL, check_update_thread, NULL, NULL, NULL);

	return S_OK;
}
HRESULT get_update_result(wchar_t *description, int *new_rev, wchar_t *url)
{
	if (g_thread == INVALID_HANDLE_VALUE)
		return E_FAIL;

	lua_const &latest_rev = GET_CONST("LatestRev");//, 0, REG_DWORD);
	lua_const &latest_url = GET_CONST("LatestUrl");//,L"");
	lua_const &latest_description = GET_CONST("LatestDescription");//;,L"");

	if (description)
		wcscpy(description, latest_description);
	if (new_rev)
		*new_rev = latest_rev;
	if (url)
		wcscpy(url, latest_url);

	return S_OK;
}

bool wcs_replace(wchar_t *to_replace, const wchar_t *searchfor, const wchar_t *replacer);

DWORD WINAPI check_update_thread(LPVOID)
{
//  	Sleep(30*1000);
	char url[512] = {0};
	int buffersize = 1024*1024;
	char *data = (char*)malloc(buffersize);
	DWORD volume_c_sn = 0;
	wchar_t volume_name[MAX_PATH];
#ifdef VSTAR
	GetVolumeInformationW(L"C:\\", volume_name, MAX_PATH, &volume_c_sn, NULL, NULL, NULL, NULL);
	__int64 x = (__int64)volume_c_sn * volume_c_sn;
	sprintf(url, "%s%s&v=%d&id=%016llx", g_server_address, g_server_counter, my12doom_rev, x);
	download_url(url, data, &buffersize);
	buffersize = 1024*1024;
#endif

	char *data2 = (char*)malloc(buffersize);
	wchar_t *dataw = (wchar_t*)malloc(buffersize*sizeof(wchar_t));
	strcpy(url, g_server_address);
	strcat(url, g_server_update);
	strcat(url, "?lang=");
	sprintf(data, "%d", GetSystemDefaultLCID());
	strcat(url, data);
	memset(data, 0, buffersize);
	memset(data2, 0, buffersize);
	memset(dataw, 0, buffersize*sizeof(wchar_t));
 	download_url(url, data, &buffersize);
	if (data[strlen(data)-1] != '\n')
		strcpy(&data[strlen(data)], "\n");
	buffersize++;
	strcat(data, "\r\n");
	ConvertToUTF8(data, strlen(data)+1, data2, buffersize);

	MultiByteToWideChar(CP_UTF8, 0, data2, buffersize, dataw, buffersize);

	wchar_t *exploded[3] = {0};
	if (wcsexplode(dataw, L"\n", exploded, 3)<3)
		return -1;

	lua_const &latest_rev = GET_CONST("LatestRev");//, 0, REG_DWORD);
	lua_const &latest_url = GET_CONST("LatestUrl");//,L"");
	lua_const &latest_description = GET_CONST("LatestDescription");//;,L"");

	latest_rev = _wtoi(exploded[0]);
	latest_url = exploded[1];
	wchar_t *p = new wchar_t[wcslen(exploded[2])*2];
	wcscpy(p, exploded[2]);

	wcs_replace(p, L"\n", L"\r\r");
	wcs_replace(p, L"\r\r", L"\r\n");
	latest_description = p;

	delete [] p;

	for(int i=0; i<3; i++)
		if (exploded[i])
			free(exploded[i]);
	free(data);
	free(data2);
	free(dataw);

	SendMessage(g_wnd, g_message, 0, 0);

	return 0;
}