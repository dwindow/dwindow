#include "TrayMenu.h"
#include "resource.h"

#define UM_ICONNOTIFY (WM_USER + 101)
#define IDI_ICON1 0

TrayMenu::TrayMenu(ITrayMenuCallback *cb, LPCTSTR tooltip)
{
	m_cb = cb;
	_tcscpy(m_tooltip, tooltip);

	HINSTANCE hinstance = GetModuleHandle(NULL);
	WNDCLASSEX wcx;
	wcx.cbSize = sizeof(wcx);
	wcx.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;// redraw if size changes 
	wcx.lpfnWndProc = MainWndProc;     // points to window procedure 
	wcx.cbClsExtra = 0;                // no extra class memory 
	wcx.cbWndExtra = 0;                // no extra window memory 
	wcx.hInstance = hinstance;         // handle to instance 
	wcx.hIcon = LoadIcon(NULL, IDI_APPLICATION);// predefined app. icon 
	wcx.hCursor = LoadCursor(NULL, IDC_ARROW);  // predefined arrow 
	wcx.hbrBackground = NULL;
	wcx.lpszMenuName =  _T("NoMenu");    // name of menu resource 
	wcx.lpszClassName = _T("TrayMenuClass");  // name of window class 
	wcx.hIconSm = (HICON)LoadImage(hinstance, // small class icon 
		MAKEINTRESOURCE(IDI_ICON1),
		IMAGE_ICON, 
		GetSystemMetrics(SM_CXSMICON), 
		GetSystemMetrics(SM_CYSMICON), 
		LR_DEFAULTCOLOR); 

	// Register the window class. 
	if (!RegisterClassEx(&wcx))
		return;

	m_window_thread = CreateThread(0,0,WindowThread, this, NULL, NULL);
}

TrayMenu::~TrayMenu()
{
	NOTIFYICONDATA nid;
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = m_hwnd;
	nid.uID = IDI_ICON1;
	Shell_NotifyIcon(NIM_DELETE, &nid);

	SendMessage(m_hwnd, WM_DESTROY, 0, 0);

	WaitForSingleObject(m_window_thread, INFINITE);

	UnregisterClass(_T("TrayMenuClass"), GetModuleHandle(NULL));
}

DWORD WINAPI TrayMenu::WindowThread(LPVOID lpParame)
{
	TrayMenu *_this = 	(TrayMenu *)lpParame;
	HINSTANCE hinstance = GetModuleHandle(NULL);
	MSG msg;

	HWND hwnd; 	// Create the main window. 
	hwnd = CreateWindow( 
		_T("TrayMenuClass"),        // name of window class 
		_T(""),					 // title-bar string 
		WS_OVERLAPPEDWINDOW, // top-level window 
		CW_USEDEFAULT,       // default horizontal position 
		CW_USEDEFAULT,       // default vertical position 
		CW_USEDEFAULT,       // default width 
		CW_USEDEFAULT,       // default height 
		(HWND) NULL,         // no owner window 
		(HMENU) NULL,        // use class menu 
		hinstance,           // handle to application instance 
		(LPVOID) NULL);      // no window-creation data 

	if (!hwnd) 
		return FALSE; 
	SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)(TrayMenu*)lpParame);

	_this->m_hwnd = hwnd;

	// Show the window and send a WM_PAINT message to the window 
	// procedure.
	SendMessage(hwnd, WM_INITDIALOG, 0, 0);
	ShowWindow(hwnd, SW_HIDE);
	UpdateWindow(hwnd);

	// Create TrayIcon
	NOTIFYICONDATA nid;
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = hwnd;
	nid.uID = IDI_ICON1;
	nid.uFlags = NIF_ICON|NIF_MESSAGE|NIF_TIP;
	nid.uCallbackMessage = UM_ICONNOTIFY;
	nid.hIcon = (HICON)LoadImage(GetModuleHandle(_T("my12doomSource.ax")), MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 16, 16, 0);
	_tcscpy(nid.szTip, _this->m_tooltip);
	Shell_NotifyIcon(NIM_ADD, &nid);


	BOOL fGotMessage;
	while ((fGotMessage = GetMessage(&msg, (HWND) NULL, 0, 0)) != 0 && fGotMessage != -1) 
	{ 
		TranslateMessage(&msg); 
		DispatchMessage(&msg); 
	}
	return (DWORD)msg.wParam; 
}

LRESULT CALLBACK TrayMenu::MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT lr = S_FALSE;
	TrayMenu *_this = NULL;
	_this = (TrayMenu*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if (!_this)
		return DefWindowProc(hWnd, message, wParam, lParam);

	int xPos = GET_X_LPARAM(lParam);
	int yPos = GET_Y_LPARAM(lParam);

	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case UM_ICONNOTIFY:
		POINT mouse_pos;
		GetCursorPos(&mouse_pos);
		if (WM_LBUTTONDOWN == lParam || WM_RBUTTONDOWN == lParam)
		{
			if (_this->m_Menu) _this->m_Menu.DestroyMenu();
			_this->m_Menu.CreatePopupMenu();

			if (S_OK == _this->m_cb->BeforeShow())
			{
				SetForegroundWindow(hWnd);
				TrackPopupMenu(_this->m_Menu.m_hMenu,TPM_BOTTOMALIGN | TPM_RIGHTALIGN, mouse_pos.x+5, mouse_pos.y+5, 0, hWnd, NULL);
			}
		}
		break;

	case WM_COMMAND:
		if(HIWORD(wParam) == 0 && lParam == 0)
		{
			int id = LOWORD(wParam);
			lr = _this->m_cb->Click(id);
		}
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);;
	}

	if (lr == S_FALSE)
		return DefWindowProc(hWnd, message, wParam, lParam);
	return 0;
}
