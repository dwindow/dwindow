#include "stdafx.h"
#include "resource.h"
#include <windows.h>
#include <DShow.h>
#include <atlbase.h>
#include "full_cache.h"
#include "..\dwindow\runnable.h"

extern disk_manager *g_last_manager;

// drawing
RECT rect;
HBRUSH brush_disk = CreateSolidBrush(RGB(0,0,255));
HBRUSH brush_net = CreateSolidBrush(RGB(0,0,128));
HBRUSH brush_read = CreateSolidBrush(RGB(0,255,0));
HBRUSH brush_preread = CreateSolidBrush(RGB(255,0,0));

HBRUSH brush0 = CreateSolidBrush(RGB(255,255,255));
HDC hdc;
HDC memDC;
HBITMAP bitmap;
HGDIOBJ obj;
HWND g_hwnd;

#define blockSize 10
#define resolution 100

void paint_pos(int i, int j, int type)
{
	HBRUSH color_table[] = {brush_disk, brush_net, brush_read, brush_preread};
	SelectObject(memDC, color_table[type]);

	POINT p[4];


	p[0].x = i*blockSize +1;
	p[0].y = j*blockSize +1;

	p[1].x = (i+1)*blockSize -1;
	p[1].y = (j)*blockSize +1;

	p[2].x = (i+1)*blockSize -1;
	p[2].y = (j+1)*blockSize -1;

	p[3].x = i*blockSize +1;
	p[3].y = (j+1)*blockSize -1;

	Polygon(memDC, p,4);

}

void begin_paint()
{
	GetClientRect(g_hwnd, &rect);
	hdc = GetDC(g_hwnd);
	memDC = CreateCompatibleDC(hdc);
	bitmap = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
	obj = SelectObject(memDC, bitmap);


	FillRect(memDC, &rect, brush0);
}

void end_paint()
{
	BitBlt(hdc, 0, 0, rect.right, rect.bottom, memDC, 0, 0, SRCCOPY);

	DeleteObject(obj);
	DeleteObject(bitmap);
	DeleteDC(memDC);
	ReleaseDC(g_hwnd, hdc);
	return;
}

HRESULT debug_list_filters(IGraphBuilder *gb);
CComPtr<IGraphBuilder> gb;
wchar_t URL[] = L"http://bo3d.net/test/ctm3d_8ma.mkv";
class dshow_threaded_init : public Irunnable
{
	void run()
	{

		CoInitialize(NULL);

		gb.CoCreateInstance(CLSID_FilterGraph);

		wchar_t tmp[MAX_PATH];
		wcscpy(tmp, L"X:\\DWindow\\");
		wcscat(tmp, URL);
		HRESULT hr = gb->RenderFile(tmp, NULL);
		debug_list_filters(gb);
		CComQIPtr<IMediaControl, &IID_IMediaControl> mc(gb);
		mc->Run();

		CoUninitialize();
	}
};

thread_pool pool(2);

static INT_PTR CALLBACK dialog_proc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg ) 
	{
	case WM_COMMAND:
		EndDialog(hDlg, 0);
		break;

	case WM_INITDIALOG:
		g_hwnd = hDlg;
		SetTimer(hDlg, 1, 100, NULL);
		pool.submmit(new dshow_threaded_init);
		break;
	
	case WM_TIMER:
		if (g_last_manager)
		{
			int size = g_last_manager->getsize();
			std::list<debug_info> debug = g_last_manager->debug();

			begin_paint();
			for(std::list<debug_info>::iterator i = debug.begin(); i!= debug.end(); ++i)
			{
				debug_info info = *i;

				int block_start = info.frag.start * resolution * resolution / size;
				int block_end = info.frag.end  * resolution * resolution / size;

				for(int j=block_start; j<=block_end; j++)
				{
					paint_pos(j%resolution, j/resolution, info.type);
				}
			}

			end_paint();
		}
		break;

	case WM_CLOSE:
		EndDialog(hDlg, 0);
		break;

	default:
		return FALSE;
	}

	return TRUE; // Handled message
}

int debug_window()
{
	int o = DialogBoxA(NULL, MAKEINTRESOURCEA(IDD_DIALOG1), NULL, dialog_proc);
	gb = NULL;
	return o;
}


HRESULT debug_list_filters(IGraphBuilder *gb)
{
	// debug: list filters
	wprintf(L"Listing filters.\n");
	CComPtr<IEnumFilters> pEnum;
	CComPtr<IBaseFilter> filter;
	gb->EnumFilters(&pEnum);
	while(pEnum->Next(1, &filter, NULL) == S_OK)
	{

		FILTER_INFO filter_info;
		filter->QueryFilterInfo(&filter_info);
		if (filter_info.pGraph) filter_info.pGraph->Release();
		wchar_t tmp[10240];
		wchar_t tmp2[1024];
		wchar_t friendly_name[200] = L"Unknown";
		//GetFilterFriedlyName(filter, friendly_name, 200);
		wcscpy(tmp, filter_info.achName);
		wcscat(tmp, L"(");
		wcscat(tmp, friendly_name);
		wcscat(tmp, L")");

		CComPtr<IEnumPins> ep;
		CComPtr<IPin> pin;
		filter->EnumPins(&ep);
		while (ep->Next(1, &pin, NULL) == S_OK)
		{
			PIN_DIRECTION dir;
			PIN_INFO pi;
			pin->QueryDirection(&dir);
			pin->QueryPinInfo(&pi);
			if (pi.pFilter) pi.pFilter->Release();

			CComPtr<IPin> connected;
			PIN_INFO pi2;
			FILTER_INFO fi;
			pin->ConnectedTo(&connected);
			pi2.pFilter = NULL;
			if (connected) connected->QueryPinInfo(&pi2);
			if (pi2.pFilter)
			{
				pi2.pFilter->QueryFilterInfo(&fi);
				if (fi.pGraph) fi.pGraph->Release();
				pi2.pFilter->Release();
			}

			wsprintfW(tmp2, L", %s %s", pi.achName, connected?L"Connected to ":L"Unconnected");
			if (connected) wcscat(tmp2, fi.achName);

			wcscat(tmp, tmp2);
			pin = NULL;
		}


		wprintf(L"%s\n", tmp);

		filter = NULL;
	}
	wprintf(L"\n");

	return S_OK;
}

int disable_hookdshow();
int enable_hookdshow();

int _tmain(int argc, _TCHAR* argv[])
{
	enable_hookdshow();

	debug_window();

	disable_hookdshow();


	return 0;
}