#include "private_filter.h"
#include "ImageSource.h"
#include <atlbase.h>
#include "MediaInfo.h"
#include "global_funcs.h"

typedef HRESULT (__stdcall *pDllGetClassObject) (REFCLSID rclsid, REFIID riid, LPVOID*ppv);
HRESULT GetFileSourceMediaInfo(const wchar_t *filename, wchar_t *format);

void GetAppPath(wchar_t *out, int size = MAX_PATH)
{
	GetModuleFileNameW(NULL, out, size);
	for(int i = wcslen(out); i>0; i--)
	{
		if(out[i] == L'\\')
		{
			out[i+1] = NULL;
			break;
		}
	}
}

HRESULT GetFileSource(const wchar_t *filename, CLSID *out, bool use_mediainfo/* = false*/)
{
	wchar_t ini[MAX_PATH];
	GetAppPath(ini);
	wcscat(ini, L"dwindow.ini");

	wchar_t ext[MAX_PATH];
	for(int i=wcslen(filename); i>0; i--)
		if(filename[i] == L'.')
		{
			wcscpy(ext, filename+i+1);
			break;
		}

	if (use_mediainfo)
	{
		wchar_t tmp[20] = {0};
		if (SUCCEEDED(GetFileSourceMediaInfo(filename, tmp)) && tmp[0] != NULL)
			wcscpy(ext, tmp);
	}

	wchar_t tmp[MAX_PATH];
	GetPrivateProfileStringW(L"Extensions", ext, L"", tmp, MAX_PATH, ini);

	if (tmp[0] == NULL)
		return E_FAIL;
	else
		return CLSIDFromString(tmp, out);
}

bool isSlowExtention(const wchar_t *filename)
{
	wchar_t ini[MAX_PATH];
	GetAppPath(ini);
	wcscat(ini, L"dwindow.ini");

	wchar_t ext[MAX_PATH];
	for(int i=wcslen(filename); i>0; i--)
		if(filename[i] == L'.')
		{
			wcscpy(ext, filename+i+1);
			break;
		}
	wchar_t is_slow[MAX_PATH] = {0};
	GetPrivateProfileStringW(L"SlowExtentions", ext, L"", is_slow, MAX_PATH, ini);
	return is_slow[0] != NULL;
}

extern int wcscmp_nocase(const wchar_t*in1, const wchar_t *in2);
DWORD WINAPI GetFileSourceMediaInfoCore(LPVOID t)
{
	const wchar_t *filename = ((wchar_t **)t)[0];
	wchar_t *format = ((wchar_t **)t)[1];
	*format = NULL;

	media_info_entry *o = NULL;
	if (FAILED(get_mediainfo(filename, &o, false)) || NULL == o || o->level_depth != 0)
		return E_FAIL;

	media_info_entry *p = o->next;
	while(p && p->level_depth == 1)
	{
		if (wcscmp_nocase(p->key, L"Format") == 0)
		{
			wchar_t ini[MAX_PATH];
			GetAppPath(ini);
			wcscat(ini, L"dwindow.ini");
			GetPrivateProfileStringW(L"MediaInfoMatch", p->value, L"", format, MAX_PATH, ini);
			break;
		}

		p = p->next;
	}

	// leak 2k each time, this should be ok for a player
	//free(format);


	return 0;
}

HRESULT GetFileSourceMediaInfo(const wchar_t *filename, wchar_t *format)
{
	if (NULL == format)
		return E_POINTER;

	const wchar_t *p[2] = {filename, (wchar_t*)malloc(2048)};

	HANDLE thread = CreateThread(NULL, NULL, GetFileSourceMediaInfoCore, p, NULL, NULL);

	if (WaitForSingleObject(thread, 5000) != WAIT_OBJECT_0)
		return E_FAIL;

	wcscpy(format, p[1]);
	free((void*)p[1]);

	return S_OK;
}

#include "IntelWiDiExtensions_i.h"

HRESULT myCreateInstance(CLSID clsid, IID iid, void**out)
{
	DWORD os = LOBYTE(LOWORD(GetVersion()));
	if (iid == __uuidof(IWiDiExtensions) && os < 6)
		return E_FAIL;

	if (clsid == CLSID_my12doomImageSource )
	{
		HRESULT hr;
		my12doomImageSource *s = (my12doomImageSource *)my12doomImageSource::CreateInstance(NULL, &hr);

		if (FAILED(hr))
			return hr;

		return s->QueryInterface(iid, out);
	}

	if (clsid == CLSID_MXFReader)
		config_MXFReader();

	if (clsid == CLSID_AVSource)
		make_av_splitter_support_my_formats();


	LPOLESTR strCLSID = NULL;
	wchar_t filename[MAX_PATH];
	wchar_t tmp[MAX_PATH];

	StringFromCLSID(clsid, &strCLSID);
	GetAppPath(tmp);
	wcscat(tmp, L"dwindow.ini");
	GetPrivateProfileStringW(L"modules", strCLSID, L"", filename, MAX_PATH, tmp);
	if (strCLSID) CoTaskMemFree(strCLSID);

	// not found in private files
	if (filename[0] == NULL)
		return CoCreateInstance(clsid, NULL, CLSCTX_INPROC, iid, out);

	// load from private filters
	GetAppPath(tmp);
	wcscat(tmp, filename);
	
	HINSTANCE hDll = CoLoadLibrary(tmp, TRUE);
	if (!hDll)
		return E_FAIL;
	pDllGetClassObject pGetClass = (pDllGetClassObject)GetProcAddress(hDll, "DllGetClassObject");
	if (!pGetClass)
		return E_FAIL;

	CComPtr<IClassFactory> factory;
	pGetClass(clsid, IID_IClassFactory, (void**)&factory);
	if (factory)
	{
		return factory->CreateInstance(NULL, iid, (void**) out);
	}
	else
	{
		return E_FAIL;
	}
}