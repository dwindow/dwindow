#include <windows.h>
#include <atlbase.h>
#include "my12doomRenderer.h"
#include "resource.h"
#include "ps_aes_key.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

CComPtr<IGraphBuilder> gb;
my12doomRenderer * renderer;
HWND g_hWnd, g_hWnd2;
#define DS_EVENT (WM_USER + 4)

bool open_file_dlg(wchar_t *pathname, HWND hDlg, wchar_t *filter/* = NULL*/)
{
	static wchar_t *default_filter = L"Video files\0"
		L"*.mp4;*.mkv;*.avi;*.rmvb;*.wmv;*.avs;*.ts;*.m2ts;*.ssif;*.mpls;*.3dv;*.e3d\0"
		L"Audio files\0"
		L"*.wav;*.ac3;*.dts;*.mp3;*.mp2;*.mpa;*.mp4;*.wma;*.flac;*.ape;*.avs\0"
		L"Subtitles\0*.srt;*.sup\0"
		L"All Files\0*.*\0"
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
			OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|OFN_ENABLESIZING, 0, 0,
			L".mp4", 0, NULL, NULL };

		int o = GetOpenFileNameW( &ofn );
		if (o)
		{
			wcsncpy(pathname, strFileName, MAX_PATH);
			return true;
		}

		return false;
}

int WINAPI WinMain( HINSTANCE hInstance,
				   HINSTANCE hPrevInstance,
				   LPSTR     lpCmdLine,
				   int       nCmdShow )
{
	CoInitialize(NULL);

	HANDLE h = GetCurrentProcess();
	//SetProcessAffinityMask(h, 1);
	CloseHandle(h);

	WNDCLASSEX winClass; 
	MSG        uMsg;

	memset(&uMsg,0,sizeof(uMsg));

	winClass.lpszClassName = _T("MY_WINDOWS_CLASS");
	winClass.cbSize        = sizeof(WNDCLASSEX);
	winClass.style         = CS_HREDRAW | CS_VREDRAW;
	winClass.lpfnWndProc   = WindowProc;
	winClass.hInstance     = hInstance;
	winClass.hIcon         = LoadIcon(hInstance, (LPCTSTR)IDI_DIRECTX_ICON);
	winClass.hIconSm       = LoadIcon(hInstance, (LPCTSTR)IDI_DIRECTX_ICON);
	winClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
	winClass.hbrBackground = NULL;//(HBRUSH)GetStockObject(BLACK_BRUSH);
	winClass.lpszMenuName  = NULL;
	winClass.cbClsExtra    = 0;
	winClass.cbWndExtra    = 0;

	if( !RegisterClassEx(&winClass) )
		return E_FAIL;

	g_hWnd = CreateWindowEx( NULL, _T("MY_WINDOWS_CLASS"),
		_T("Direct3D (DX9) - Resize Window"),
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		//WS_EX_TOPMOST | WS_POPUP,    // fullscreen values
		300, 0, 816, 525, NULL, NULL, hInstance, NULL );

	g_hWnd2 = CreateWindowEx( NULL, _T("MY_WINDOWS_CLASS"),
		_T("Direct3D (DX9) - Resize Window2"),
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		//WS_EX_TOPMOST | WS_POPUP,    // fullscreen values
		400, 50, 528-8, 394-4, NULL, NULL, hInstance, NULL );


	if( g_hWnd == NULL  || g_hWnd2 == NULL)
		return E_FAIL;

	ShowWindow( g_hWnd, nCmdShow );
	UpdateWindow( g_hWnd );
	UpdateWindow( g_hWnd2 );

	HRESULT hr;
	renderer = new my12doomRenderer(g_hWnd, g_hWnd2);
	renderer->m_AES.set_key(ps_aes_key, 256);

	// dshow
	wchar_t file[MAX_PATH] = L"test.avi";
	open_file_dlg(file, g_hWnd, NULL);
	gb.CoCreateInstance(CLSID_FilterGraph);
	gb->AddFilter(renderer->m_dshow_renderer1, L"my12doom Renderer #1");
	gb->AddFilter(renderer->m_dshow_renderer2, L"my12doom Renderer #2");
	//gb->RenderFile(L"F:\\TDDOWNLOAD\\00019hsbs.mkv", NULL);
	//gb->RenderFile(L"Z:\\avts.ts", NULL);
	//if (FAILED(gb->RenderFile(L"Z:\\24_double.avi", NULL)))
	//	exit(-1);
	//if (FAILED(gb->RenderFile(L"Z:\\60fps.avi", NULL)))
	//	exit(-1);
	if (FAILED(gb->RenderFile(file, NULL)))
		exit(-1);
	renderer->set_mask_mode(row_interlace);
	renderer->set_input_layout(mono2d);
	renderer->set_output_mode(dual_window);
	ShowWindow(g_hWnd2, renderer->get_output_mode() == dual_window ? SW_SHOW : SW_HIDE);

	//renderer->set_aspect(16.0/9.0);
	renderer->set_input_layout(side_by_side);
	CComQIPtr<IMediaControl, &IID_IMediaControl> mc(gb);
	mc->Run();
	// set event notify
	CComQIPtr<IMediaEventEx, &IID_IMediaEventEx> event_ex(gb);
	event_ex->SetNotifyWindow((OAHWND)g_hWnd, DS_EVENT, 0);

	while( uMsg.message != WM_QUIT )
	{
		if( PeekMessage( &uMsg, NULL, 0, 0, PM_REMOVE ) )
		{ 
			TranslateMessage( &uMsg );
			DispatchMessage( &uMsg );
		}
		else
		{
			if (S_OK == renderer->pump())
				Sleep(1);
		}
	}

	UnregisterClass( _T("MY_WINDOWS_CLASS"), winClass.hInstance );

	CoUninitialize();
	return uMsg.wParam;
}

LRESULT CALLBACK WindowProc( HWND   hWnd,
							UINT   msg,
							WPARAM wParam,
							LPARAM lParam )
{
	static POINT ptLastMousePosit;
	static POINT ptCurrentMousePosit;
	static bool bMousing;

	switch( msg )
	{
	case DS_EVENT:
		{
			CComQIPtr<IMediaEvent, &IID_IMediaEvent> event(gb);

			long event_code;
			LONG_PTR param1;
			LONG_PTR param2;
			if (FAILED(event->GetEvent(&event_code, &param1, &param2, 0)))
				return S_OK;

			if (event_code == EC_COMPLETE)
			{
				CComQIPtr<IMediaSeeking, &IID_IMediaSeeking> ms(gb);
				REFERENCE_TIME target = 0;
				ms->SetPositions(&target, AM_SEEKING_AbsolutePositioning, NULL, NULL);
			}
		}
		break;

	case WM_KEYDOWN:
		{
			switch( wParam )
			{
			case VK_ESCAPE:
				PostQuitMessage(0);
				break;

			case '1':
				renderer->set_input_layout(renderer->get_input_layout()+1);
				break;

			case '2':
				renderer->set_swap_eyes(!renderer->get_swap_eyes());
				break;

			case '3':
				renderer->set_output_mode(renderer->get_output_mode()+1);
				ShowWindow(g_hWnd2, renderer->get_output_mode() == dual_window ? SW_SHOW : SW_HIDE);
				break;

			case '4':
				renderer->set_mask_mode(renderer->get_mask_mode()+1);
				break;
			
			case '5':
				renderer->set_mask_color(1, D3DCOLOR_XRGB(255, 255, 0));
				renderer->set_mask_color(2, D3DCOLOR_XRGB(0,0,255));
				break;

			case VK_F11:
				renderer->set_fullscreen(!renderer->get_fullscreen());
				break;

			case VK_F1:
				{
					CComQIPtr<IMediaControl, &IID_IMediaControl> mc(gb);
					mc->Pause();
				}
				break;

			case VK_F2:
				{
					CComQIPtr<IMediaControl, &IID_IMediaControl> mc(gb);
					mc->Run();
				}
				break;
			case VK_F3:
				{
					/*
					CComQIPtr<IQualProp, &IID_IQualProp> qp(renderer);
					int avg_fps = 0, avg_sync = 0, dev_sync = 0, n_drop = 0, jitter = 0;
					qp->get_AvgFrameRate(&avg_fps);
					qp->get_AvgSyncOffset(&avg_sync);
					qp->get_DevSyncOffset(&dev_sync);
					qp->get_FramesDroppedInRenderer(&n_drop);
					qp->get_Jitter(&jitter);

					wchar_t tmp[256];
					wsprintfW(tmp, L"avg_fps = %d, avg_sync = %d, dev_sync = %d, n_drop = %d, jitter = %d\n", avg_fps, avg_sync, dev_sync, n_drop, jitter);
					SetWindowText(hWnd, tmp);
					*/
				}
				break;

			case VK_F5:
				{
					HWND wnd = g_hWnd;
					g_hWnd = g_hWnd2;
					g_hWnd2 = wnd;
					renderer->set_window(g_hWnd, g_hWnd2);
				}
				break;

			case VK_NUMPAD8:
				renderer->set_movie_pos(2, renderer->get_movie_pos(2)-0.05);
				break;
			case VK_NUMPAD2:
				renderer->set_movie_pos(2, renderer->get_movie_pos(2)+0.05);
				break;
			case VK_NUMPAD4:
				renderer->set_movie_pos(1, renderer->get_movie_pos(1)-0.05);
				break;
			case VK_NUMPAD6:
				renderer->set_movie_pos(1, renderer->get_movie_pos(1)+0.05);
				break;
			case VK_NUMPAD5:
				renderer->set_movie_pos(1, 0.0);
				renderer->set_movie_pos(2, 0.0);
				break;
			}
		}
		break;

	case WM_RBUTTONDOWN:
		{
			HMENU menu = CreatePopupMenu();
			BOOL rtn = InsertMenu(menu, 0, MF_STRING | MF_BYPOSITION, 0, _T("Test Menu Item"));
			POINT mouse_pos;
			GetCursorPos(&mouse_pos);
			TrackPopupMenu(menu, TPM_TOPALIGN | TPM_LEFTALIGN, mouse_pos.x-5, mouse_pos.y-5, 0, hWnd, NULL);
		}
		break;

	case WM_SIZE:
		{
			if (renderer) renderer->pump();
		}
		break;

	case WM_CLOSE:
		{
			PostQuitMessage(0); 
		}


	case WM_MOVE:
		break;

	case WM_DESTROY:
		{
			PostQuitMessage(0);
		}
		break;

	default:
		{
			return DefWindowProc( hWnd, msg, wParam, lParam );
		}
		break;
	}

	return 0;
}

