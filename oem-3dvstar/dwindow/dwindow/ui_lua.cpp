#include "ui_lua.h"
#include "global_funcs.h"
#include "dx_player.h"
#include "open_double_file.h"
#include "open_url.h"

lua_manager *g_lua_ui_manager = NULL;
extern dx_player *g_player;
extern lua_manager *g_player_lua_manager;

class menu_holder_window
{
public:

	HWND m_hwnd;
	HMENU m_menu;	// use this thing to config...
	menu_holder_window(menu_holder_window *root = NULL);
	~menu_holder_window();
	int & uid() {return m_root ? m_root->uid() : m_uid;}
	std::vector<int> & id2tbl() {return m_root ? m_root->id2tbl() : m_id2tbl;}
protected:

	std::vector<int> m_id2tbl;
	std::vector<menu_holder_window*> m_subs;
	int m_uid;
	menu_holder_window *m_root;
	static LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};

static int luaOpenFile(lua_State *L)
{
	int n = lua_gettop(L);
	wchar_t * filter = NULL;
	wchar_t *p = NULL;
	for(int i=0; i<n/2; i++)
	{
		if (!filter)
		{
			filter = new wchar_t[10240];
			memset(filter, 0, sizeof(wchar_t) & 10240);
			p = filter;
		}

		wcscpy(p, UTF82W(lua_tostring(L, -n+i*2)));
		p += wcslen(p)+1;
		wcscpy(p, UTF82W(lua_tostring(L, -n+i*2+1)));
		p += wcslen(p)+1;
	}

	wchar_t out[1024] = L"";
	if (!open_file_dlg(out, g_player->get_window((int)g_player_lua_manager->get_variable("active_view")+1), filter))
		return 0;

	if (filter)
		delete [] filter;

	lua_pushstring(L, W2UTF8(out));
	return 1;
}

static int luaOpenDoubleFile(lua_State *L)
{
	wchar_t left[1024] = L"", right[1024] = L"";
	if (FAILED(open_double_file(g_player->m_hexe, g_player->get_window((int)g_player_lua_manager->get_variable("active_view")+1), left, right)))
		return 0;

	lua_pushstring(L, W2UTF8(left));
	lua_pushstring(L, W2UTF8(right));
	return 2;
}

static int luaOpenFolder(lua_State *L)
{
	wchar_t out[1024] = L"";
	if (!browse_folder(out, g_player->get_window((int)g_player_lua_manager->get_variable("active_view")+1)))
		return 0;

	if (out[wcslen(out)] != L'\\')
		wcscat(out, L"\\");
	lua_pushstring(L, W2UTF8(out));
	return 1;
}

static int luaOpenURL(lua_State *L)
{
	wchar_t url[1024] = L"";
	if (FAILED(open_URL(g_player->m_hexe, g_player->get_window((int)g_player_lua_manager->get_variable("active_view")+1), url)))
		return 0;

	lua_pushstring(L, W2UTF8(url));
	return 1;
}

static int luaStartDragging(lua_State *L)
{
	int view = (int)g_player_lua_manager->get_variable("active_view")+1;
	HWND hwnd = g_player->get_window(view);
	ReleaseCapture();
	g_player->m_dragging_window = view;
	SendMessage(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
	g_player->m_dragging_window = 0;

	lua_pushboolean(L, 1);
	return 1;
}
static int luaStartResizing(lua_State *L)
{
	int pos = HTBOTTOMRIGHT;
	if (lua_gettop(L)>0)
		pos = lua_tointeger(L, -1);
	int view = (int)g_player_lua_manager->get_variable("active_view")+1;
	HWND hwnd = g_player->get_window(view);
	ReleaseCapture();
	g_player->m_dragging_window = view;
	SendMessage(hwnd, WM_NCLBUTTONDOWN, pos, 0);
	g_player->m_dragging_window = 0;

	lua_pushboolean(L, 1);
	return 1;
}

static int luaCreateMenu(lua_State *L)
{
	menu_holder_window *root = (menu_holder_window*)lua_touserdata(L, -1);
	menu_holder_window *p = new menu_holder_window(root);

	lua_pushlightuserdata(L, p);
	return 1;
}
static int luaAppendMenu(lua_State *L)
{
	int n = lua_gettop(L);
	if (n<4)
		return 0;	// invalid parameter count

	menu_holder_window * handle = (menu_holder_window *)lua_touserdata(L, -n);
	const char* string = lua_tostring(L, -n+1);
	DWORD flags = lua_tointeger(L, -n+2);
	int func = luaL_ref(L, LUA_REGISTRYINDEX);

	AppendMenuW(handle->m_menu, (flags | MF_STRING) & ~MF_POPUP, handle->uid(), UTF82W(string));
	handle->id2tbl().push_back(func);
	handle->uid()++;

	lua_pushboolean(L, 1);
	return 1;
}

static int luaAppendSubmenu(lua_State *L)
{
	int n = lua_gettop(L);
	if (n<4)
		return 0;	// invalid parameter count

	menu_holder_window * handle = (menu_holder_window *)lua_touserdata(L, -n);
	const char* string = lua_tostring(L, -n+1);
	DWORD flags = lua_tointeger(L, -n+2);
	menu_holder_window * sub_handle = (menu_holder_window *)lua_touserdata(L, -n+3);

	AppendMenuW(handle->m_menu, (flags | MF_STRING | MF_POPUP), (UINT_PTR)sub_handle->m_menu, UTF82W(string));

	lua_pushboolean(L, 1);
	return 1;
}

static int luaPopupMenu(lua_State *L)
{
	int n = lua_gettop(L);
	if (n<3)
		return 0;	// invalid parameter count
	menu_holder_window * handle = (menu_holder_window *)lua_touserdata(L, -n+0);
	int dx = lua_tointeger(L, -n+1);
	int dy = lua_tointeger(L, -n+2);
	POINT mouse_pos;
	GetCursorPos(&mouse_pos);
	g_player->m_dialog_open ++;
	BOOL o = TrackPopupMenu(handle->m_menu, TPM_TOPALIGN | TPM_LEFTALIGN, mouse_pos.x+dx, mouse_pos.y+dy, 0, handle->m_hwnd, NULL);
	g_player->m_dialog_open --;

	return 0;
}

static int luaDestroyMenu(lua_State *L)
{
	menu_holder_window * handle = (menu_holder_window *)lua_touserdata(L, -1);

	if (handle)
		PostMessage(handle->m_hwnd, WM_CLOSE, 0, 0);		// send deconstructors message to the end of message queue (after WM_COMMAND sent by TrackPopupMenu())

	return 0;
}

extern double UIScale;
static int get_mouse_pos(lua_State *L)
{
	POINT p;
	GetCursorPos(&p);
	HWND wnd = g_player->get_window((int)g_player_lua_manager->get_variable("active_view")+1);
	ScreenToClient(wnd, &p);

	lua_pushinteger(L, p.x/UIScale);
	lua_pushinteger(L, p.y/UIScale);

	return 2;
}

int ui_lua_init()
{
	g_lua_ui_manager = new lua_manager("ui");
	g_lua_ui_manager->get_variable("OpenFile") = &luaOpenFile;
	g_lua_ui_manager->get_variable("OpenDoubleFile") = &luaOpenDoubleFile;
	g_lua_ui_manager->get_variable("OpenFolder") = &luaOpenFolder;
	g_lua_ui_manager->get_variable("OpenURL") = &luaOpenURL;


	// menu
	g_lua_ui_manager->get_variable("CreateMenu") = &luaCreateMenu;
	g_lua_ui_manager->get_variable("AppendMenu") = &luaAppendMenu;
	g_lua_ui_manager->get_variable("AppendSubmenu") = &luaAppendSubmenu;
	g_lua_ui_manager->get_variable("DestroyMenu") = &luaDestroyMenu;
	g_lua_ui_manager->get_variable("PopupMenu") = &luaPopupMenu;

	// window
	g_lua_ui_manager->get_variable("StartDragging") = &luaStartDragging;
	g_lua_ui_manager->get_variable("StartResizing") = &luaStartResizing;
	g_lua_ui_manager->get_variable("get_mouse_pos") = &get_mouse_pos;

	return 0;
}



menu_holder_window::menu_holder_window(menu_holder_window *root)
:m_root(root)
,m_uid(0)
,m_hwnd(NULL)
,m_menu(CreatePopupMenu())
{
	if (!root)
	{
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
		wcx.lpszClassName = _T("MenuHolderClass");  // name of window class 
		wcx.hIconSm = (HICON)LoadImage(hinstance, // small class icon 
			MAKEINTRESOURCE(IDI_ICON1),
			IMAGE_ICON, 
			GetSystemMetrics(SM_CXSMICON), 
			GetSystemMetrics(SM_CYSMICON), 
			LR_DEFAULTCOLOR);

		// Register the window class. 
		if (!RegisterClassEx(&wcx))
			;//return;

		HWND hwnd; 	// Create the main window. 
		hwnd = CreateWindow( 
			_T("MenuHolderClass"),        // name of window class 
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
			return; 

		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)this);

		m_hwnd = hwnd;

		// Show the window and send a WM_PAINT message to the window 
		// procedure.
		SendMessage(hwnd, WM_INITDIALOG, 0, 0);
		ShowWindow(hwnd, SW_HIDE);
		UpdateWindow(hwnd);
	}
	else
	{
		root->m_subs.push_back(this);
	}
}

menu_holder_window::~menu_holder_window()
{
	DestroyWindow(m_hwnd);

 	if (!m_root)
		UnregisterClass(_T("MenuHolderClass"), GetModuleHandle(NULL));

	luaState L;
	for(std::vector<int>::iterator i = m_id2tbl.begin(); i!= m_id2tbl.end(); ++i)
		luaL_unref(L, LUA_REGISTRYINDEX, *i);
	for(std::vector<menu_holder_window*>::iterator i = m_subs.begin(); i!= m_subs.end(); ++i)
		delete *i;
}

LRESULT CALLBACK menu_holder_window::MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT lr = S_FALSE;
	menu_holder_window *_this = NULL;
	_this = (menu_holder_window*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if (!_this)
		return DefWindowProc(hWnd, message, wParam, lParam);

	int xPos = GET_X_LPARAM(lParam);
	int yPos = GET_Y_LPARAM(lParam);

	switch (message)
	{
	case WM_CLOSE:
		delete _this;
		break;

	case WM_COMMAND:
		{
			int id = LOWORD(wParam);
			luaState L;
			int refid = _this->id2tbl()[id];
			lua_rawgeti(L, LUA_REGISTRYINDEX, refid);
			if (lua_istable(L, -1))
			{
				lua_getfield(L, -1, "on_command");
				if (lua_isfunction(L, -1))
				{
					lua_insert(L, -2);
					lua_mypcall(L, 1, 0, 0);
				}
			}
			lua_settop(L, 0);

			DestroyWindow(hWnd);

			return 0;
		}
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);;
	}

	if (lr == S_FALSE)
		return DefWindowProc(hWnd, message, wParam, lParam);
	return 0;
}
