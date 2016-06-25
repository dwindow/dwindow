#include "MediaInfo.h"
#include <time.h>
#include <WindowsX.h>
#include <tchar.h>
#include <stdio.h>
#include "resource.h"
#include <commctrl.h>
#include "MediaInfoDLL.h"
#include "global_funcs.h"
#include "dwindow_log.h"
using namespace MediaInfoDLL;


HTREEITEM InsertTreeviewItem(const HWND hTv, const wchar_t *text, HTREEITEM htiParent);
void DoEvents();

class MediaInfoWindow
{
public:
	MediaInfoWindow(const wchar_t *filename, HWND parent);

protected:
	~MediaInfoWindow() {free(m_msg);}			// this class will suicide after window closed, so, don't delete
	static LRESULT CALLBACK DummyMainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	static DWORD WINAPI pump_thread(LPVOID lpParame){return ((MediaInfoWindow*)lpParame)->pump();}
	DWORD pump();
	HRESULT FillTree(HWND root, const wchar_t *filename);

	wchar_t m_filename[MAX_PATH*10];
	wchar_t m_classname[100];		// just a random class name
	wchar_t *m_msg;
	HWND m_parent;

	HWND tree;
	HWND copy_button;
};

MediaInfoWindow::MediaInfoWindow(const wchar_t *filename, HWND parent)
{
	m_parent = parent;

	// generate a random string
	for(int i=0; i<50; i++)
		m_classname[i] = L'a' + rand()%25;
	m_classname[50] = NULL;

	// store filename
	wcscpy(m_filename, filename);

	// alloc message space
	m_msg = (wchar_t*)malloc(1024*1024);		// 1M ought to be enough for everybody
	m_msg[0] = NULL;

	// GOGOGO
	CreateThread(NULL, NULL, pump_thread, this, NULL, NULL);
}

DWORD MediaInfoWindow::pump()
{
	HINSTANCE hinstance = GetModuleHandle(NULL);
	WNDCLASSEX wcx; 
	wcx.cbSize = sizeof(wcx);          // size of structure 
	wcx.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;// redraw if size changes 
	wcx.lpfnWndProc = DummyMainWndProc;     // points to window procedure 
	wcx.cbClsExtra = 0;                // no extra class memory 
	wcx.cbWndExtra = 0;                // no extra window memory 
	wcx.hInstance = hinstance;         // handle to instance 
	wcx.hIcon = LoadIcon(NULL, IDI_APPLICATION);// predefined app. icon 
	wcx.hCursor = LoadCursor(NULL, IDC_ARROW);  // predefined arrow 
	wcx.hbrBackground =  GetStockBrush(WHITE_BRUSH);
	wcx.lpszMenuName =  _T("");    // name of menu resource 
	wcx.lpszClassName = m_classname;  // name of window class 
	wcx.hIconSm = (HICON)LoadImage(hinstance, // small class icon 
		MAKEINTRESOURCE(IDI_ICON1),
		IMAGE_ICON, 
		GetSystemMetrics(SM_CXSMICON), 
		GetSystemMetrics(SM_CYSMICON), 
		LR_DEFAULTCOLOR); 

	// Register the window class. 
	if (!RegisterClassEx(&wcx))
		return -1;

	HWND hwnd; 	// Create the main window. 
	hwnd = CreateWindowExW(
		WS_EX_ACCEPTFILES,
		m_classname,        // name of window class 
		L"",					 // title-bar string 
		WS_OVERLAPPEDWINDOW, // top-level window 
		CW_USEDEFAULT,       // default horizontal position 
		CW_USEDEFAULT,       // default vertical position 
		GET_CONST("MediaInfoWidth"),       // default width 
		GET_CONST("MediaInfoHeight"),       // default height 
		(HWND) m_parent,         // no owner window 
		(HMENU) NULL,        // use class menu 
		hinstance,           // handle to application instance 
		(LPVOID) NULL);      // no window-creation data 

	if (!hwnd) 
		return -2;

	SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)this);

	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);
	SendMessage(hwnd, WM_INITDIALOG, 0, 0);

	MSG msg;
	memset(&msg,0,sizeof(msg));
	while( msg.message != WM_QUIT )
	{
		if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
		else
		{
			Sleep(1);
		}
	}


	// 
	UnregisterClass(m_classname, GetModuleHandle(NULL));

	delete this;

	return (DWORD)msg.wParam; 
}
LRESULT CALLBACK MediaInfoWindow::DummyMainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	MediaInfoWindow *_this = (MediaInfoWindow*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if (!_this)
		return DefWindowProc(hWnd, message, wParam, lParam);
	return _this->MainWndProc(hWnd, message, wParam, lParam);
}

LRESULT MediaInfoWindow::MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT lr = S_FALSE;
	const int IDC_TREE = 200;
	const int IDC_COPY = 201;

	switch (message)
	{
	case WM_COMMAND:
		{
		size_t size = sizeof(wchar_t)*(1+wcslen(m_msg));
		OutputDebugStringW(m_msg);
		HGLOBAL hResult = GlobalAlloc(GMEM_MOVEABLE, size); 
		LPTSTR lptstrCopy = (LPTSTR)GlobalLock(hResult); 
		memcpy(lptstrCopy, m_msg, size); 
		GlobalUnlock(hResult);
		OpenClipboard(hWnd);
		EmptyClipboard();
		if (SetClipboardData( CF_UNICODETEXT, hResult ) == NULL )
			GlobalFree(hResult);		//Free buffer if clipboard didn't take it.
		CloseClipboard();
		}
		break;

	case WM_INITDIALOG:
		{
			DWORD style = WS_CHILD | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT;
			tree = CreateWindowEx(0,
				WC_TREEVIEW,
				0,
				style,
				0,
				100,
				GET_CONST("MediaInfoWidth"),
				GET_CONST("MediaInfoHeight"),
				hWnd,
				(HMENU) IDC_TREE,
				GetModuleHandle(NULL),
				NULL);


			style = BS_PUSHBUTTON | WS_CHILD | WS_VISIBLE;
			copy_button = CreateWindowW(L"Button", C(L"Copy To Clipboard"), style, 0, 0, 200, 100, hWnd, NULL, GetModuleHandle(NULL), NULL);

			SendMessage(hWnd, WM_SIZE, 0, 0);

			FillTree(tree, m_filename);
			SetWindowTextW(hWnd, m_filename);
		}
		break;

	case WM_SIZE:
		{
			RECT rect;
			GetClientRect(hWnd, &rect);
			RECT pos;
			GetWindowRect(hWnd, &pos);
			GET_CONST("MediaInfoWidth") = pos.right - pos.left;
			GET_CONST("MediaInfoHeight") = pos.bottom - pos.top;
			MoveWindow(tree, 0, 0, rect.right, rect.bottom-50, TRUE);
			MoveWindow(copy_button, 0, rect.bottom-50, rect.right, 50, TRUE);
			SetWindowTextW(copy_button, C(L"Copy To Clipboard"));
			ShowWindow(tree, SW_SHOW);
			ShowWindow(copy_button, SW_SHOW);
			BringWindowToTop(copy_button);
		}
		break;

	case WM_CLOSE:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}


HTREEITEM InsertTreeviewItem(const HWND hTv, const wchar_t *text,
							 HTREEITEM htiParent)
{
	TVITEM tvi = {0};

	tvi.mask = TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE;


	//copy the text into a temporary array (vector) so it's in a suitable form
	//for the pszText member of the TVITEM struct to use. This avoids using
	//const_cast on 'txt.c_str()' or variations applied directly to the string that
	//break its constant nature.

	tvi.pszText = (LPWSTR)text;
	tvi.cchTextMax =static_cast<int>(wcslen(text));  //length of item label
	tvi.iImage = 0;  //non-selected image index

	TVINSERTSTRUCT tvis={0};

	tvi.iSelectedImage = 1;   //selected image index
	tvis.item = tvi; 
	tvis.hInsertAfter = 0;
	tvis.hParent = htiParent; //parent item of item to be inserted

	return reinterpret_cast<HTREEITEM>(SendMessageW(hTv,TVM_INSERTITEM,0,
		reinterpret_cast<LPARAM>(&tvis)));
}

bool wcs_replace(wchar_t *to_replace, const wchar_t *searchfor, const wchar_t *replacer);


HRESULT MediaInfoWindow::FillTree(HWND root, const wchar_t *filename)
{
	HTREEITEM file = InsertTreeviewItem(root, filename, TVI_ROOT);
	InsertTreeviewItem(root, C(L"Reading Infomation ...."), file);
	SendMessage(root, TVM_EXPAND, TVE_EXPAND, (LPARAM)file);
	DoEvents();

	MediaInfo MI;

	// language

	wchar_t path[MAX_PATH];
	wcscpy(path, g_apppath);
	wcscat(path, C(L"MediaInfoLanguageFile"));

	FILE *f = _wfopen(path, L"rb");
	if (f)
	{
		wchar_t lang[102400] = L"";
		char tmp[1024];
		wchar_t tmp2[1024];
		USES_CONVERSION;
		while (fscanf(f, "%s", tmp, 1024, f) != EOF)
		{
			MultiByteToWideChar(CP_UTF8, 0, tmp, 1024, tmp2, 1024);

			if (wcsstr(tmp2, L";"))
			{
				wcscat(lang, tmp2);
				wcscat(lang, L"\n");
			}
		}
		fclose(f);
		MI.Option(_T("Language"), W2T(lang));
	}
	else
	{
		MI.Option(_T("Language"));
	}

	HANDLE h_file = CreateFileW (filename, GENERIC_READ, FILE_SHARE_DELETE|FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	String str = L"";
	if (h_file != INVALID_HANDLE_VALUE)
	{
		__int64 filesize = 0;
		GetFileSizeEx(h_file, (LARGE_INTEGER*)&filesize);

		DWORD From_Buffer_Size = 0;
		unsigned char From_Buffer[1316];
		MI.Open_Buffer_Init(filesize);

		__int64 last_seek_target, seek_target = -5;

		do
		{
			if (seek_target >= 0)
				last_seek_target = seek_target;

			if (!ReadFile(h_file, From_Buffer, 1316, &From_Buffer_Size, NULL) || From_Buffer_Size <= 0)
				break;

			size_t result = MI.Open_Buffer_Continue(From_Buffer, From_Buffer_Size);
			if ((result&0x08)==0x08) // 8 = all done
				break;

			seek_target = MI.Open_Buffer_Continue_GoTo_Get();
			if (seek_target>=0)
				SetFilePointerEx(h_file, *(LARGE_INTEGER*)&seek_target, NULL, SEEK_SET);
			else if (seek_target >= filesize)
				break;
		}
		while (From_Buffer_Size>0 && last_seek_target != seek_target);
		MI.Open_Buffer_Finalize();

		MI.Option(_T("Complete"));
		MI.Option(_T("Inform"));
		str = MI.Inform().c_str();
		MI.Close();

		CloseHandle(h_file);
	}
	wchar_t *p = (wchar_t*)str.c_str();
	wchar_t *p2 = wcsstr(p, L"\n");
	wchar_t tmp[1024];
	bool next_is_a_header = true;
	
	//TreeView_DeleteAllItems (root);
	TreeView_DeleteItem(root, file);
	file = InsertTreeviewItem(root, filename, TVI_ROOT);
	wcscat(m_msg, filename);
	wcscat(m_msg, L"\r\n");
	HTREEITEM insert_position = file;
	HTREEITEM headers[4096] = {0};
	int headers_count = 0;

	wchar_t tbl[3][20]={L"\t", L"\t\t", L"\t\t\t"};

	while (true)
	{
		if (p2)
		{
			p2[0] = NULL;
			p2 ++;
		}
		
		wcscpy(tmp, p);
		wcstrim(tmp);
		wcstrim(tmp, L'\n');
		wcstrim(tmp, L'\r');
		wcs_replace(tmp, L"  ", L" ");

		if (tmp[0] == NULL || tmp[0] == L'\n' || tmp[0] == L'\r')
		{
			next_is_a_header = true;
		}		
		else if (next_is_a_header)
		{
			next_is_a_header = false;
			
			headers[headers_count++] = insert_position = InsertTreeviewItem(root, tmp, file);

			wcscat(m_msg, tbl[0]);
			wcscat(m_msg, tmp);
			wcscat(m_msg, L"\r\n");
		}
		else
		{
			InsertTreeviewItem(root, tmp, insert_position);
			wcscat(m_msg, tbl[1]);
			wcscat(m_msg, tmp);
			wcscat(m_msg, L"\r\n");
		}


		if (!p2)
			break;

		p = p2;
		p2 = wcsstr(p2, L"\n");
	}

	//add some items to the the tree view common control
	SendMessage(root, TVM_EXPAND, TVE_EXPAND, (LPARAM)file);
	for (int i=0; i<headers_count; i++)
		SendMessage(root, TVM_EXPAND, TVE_EXPAND, (LPARAM)headers[i]);

	TreeView_SelectItem (root, file);

	return S_OK;
}

HRESULT show_media_info(const wchar_t *filename, HWND parent)
{
	new MediaInfoWindow(filename, parent);

	return S_OK;
}


void DoEvents()
{
	MSG msg;
	BOOL result;

	while ( ::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE ) )
	{
		result = ::GetMessage(&msg, NULL, 0, 0);
		if (result == 0) // WM_QUIT
		{                
			::PostQuitMessage(msg.wParam);
			break;
		}
		else if (result == -1)
		{
			// Handle errors/exit application, etc.
		}
		else 
		{
			::TranslateMessage(&msg);
			:: DispatchMessage(&msg);
		}
	}
}

HRESULT get_mediainfo(const wchar_t *filename, media_info_entry **out, bool use_localization /* = false */)
{
	if (!out)
		return E_POINTER;

	MediaInfo MI;

	dwindow_log_line(L"Gettting MediaInfo for %s", filename);

	// localization
	if (use_localization)
	{
		wchar_t path[MAX_PATH];
		wcscpy(path, g_apppath);
		wcscat(path, C(L"MediaInfoLanguageFile"));

		FILE *f = _wfopen(path, L"rb");
		if (f)
		{
			wchar_t lang[102400] = L"";
			char tmp[1024];
			wchar_t tmp2[1024];
			USES_CONVERSION;
			while (fscanf(f, "%s", tmp, 1024, f) != EOF)
			{
				MultiByteToWideChar(CP_UTF8, 0, tmp, 1024, tmp2, 1024);

				if (wcsstr(tmp2, L";"))
				{
					wcscat(lang, tmp2);
					wcscat(lang, L"\n");
				}
			}
			fclose(f);
			MI.Option(_T("Language"), W2T(lang));
		}
		else
		{
			MI.Option(_T("Language"));
		}
	}
	else
	{
		MI.Option(_T("Language"));
	}

	MI.Open(filename);
	MI.Option(_T("Complete"));
	MI.Option(_T("Inform"));
	String str = MI.Inform().c_str();
	MI.Close();
	wchar_t *p = (wchar_t*)str.c_str();
	wchar_t *p2 = wcsstr(p, L"\n");
	wchar_t tmp[20480];
	bool next_is_a_header = true;

	media_info_entry *pm = *out = NULL;

	while (true)
	{
		if (p2)
		{
			p2[0] = NULL;
			p2 ++;
		}

		wcscpy(tmp, p);
		wcstrim(tmp);
		wcstrim(tmp, L'\n');
		wcstrim(tmp, L'\r');
		wcs_replace(tmp, L"  ", L" ");

		if (tmp[0] == NULL || tmp[0] == L'\n' || tmp[0] == L'\r')
		{
			next_is_a_header = true;
		}		
		else if (next_is_a_header)
		{
			next_is_a_header = false;

			if (NULL == pm)
				pm = *out = (media_info_entry*)calloc(1, sizeof(media_info_entry));
			else
				pm = pm->next = (media_info_entry*)calloc(1, sizeof(media_info_entry));



			wcscpy(pm->key, tmp);
			if (wcschr(pm->key, L':'))
			{
				*((wchar_t*)wcsrchr(pm->key, L':')) = NULL;

				wcscpy(pm->value, wcsrchr(tmp, L':')+1);
			}
			wcstrim(pm->key);
			wcstrim(pm->value);
			pm->level_depth = 0;
		}
		else
		{
			pm = pm->next = (media_info_entry*)calloc(1, sizeof(media_info_entry));

			wcscpy(pm->key, tmp);
			if (wcschr(pm->key, L':'))
			{
				*((wchar_t*)wcsrchr(pm->key, L':')) = NULL;

				wcscpy(pm->value, wcsrchr(tmp, L':')+1);
			}
			wcstrim(pm->key);
			wcstrim(pm->value);
			pm->level_depth = 1;
		}


		if (!p2)
			break;

		p = p2;
		p2 = wcsstr(p2, L"\n");
	}

	if(out)
	{
		dwindow_log_line(L"MediaInfo for %s", filename);
		for(media_info_entry *p = *out;p; p=p->next)
		{
			wchar_t space[] = L"          ";
			space[p->level_depth*2] = NULL;
			dwindow_log_line(L"%s %s - %s", space, p->key, p->value);
		}
	}

	return S_OK;
}

HRESULT free_mediainfo(media_info_entry *p)
{
	if (NULL == p)
		return E_POINTER;

	if (p->next)
		free_mediainfo(p->next);

	free(p);

	return S_OK;
}