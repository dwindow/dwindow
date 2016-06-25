#include "dshow_lua.h"
#include "global_funcs.h"
#include "dx_player.h"
#include "open_double_file.h"
#include "open_url.h"
#include "color_adjust.h"
#include "latency_dialog.h"
#include "MediaInfo.h"
#include "AboudWindow.h"

lua_manager *g_player_lua_manager = NULL;
extern dx_player *g_player;



extern dx_player *g_player;

static int pause(lua_State *L)
{
	g_player->pause();

	return 0;
}
static int play(lua_State *L)
{
	g_player->play();

	return 0;
}

static int stop(lua_State *L)
{
	g_player->stop();

	return 0;
}

static int toggle_fullscreen(lua_State *L)
{
	g_player->toggle_fullscreen();

	return 0;
}
static int toggle_3d(lua_State *L)
{
	g_player->toggle_force2d();

	return 0;
}
static int get_force2d(lua_State *L)
{
	bool b;

	g_player->get_force_2d(&b);

	lua_pushboolean(L, b);

	return 1;
}
static int set_force2d(lua_State *L)
{
	bool b = lua_toboolean(L, -1);

	g_player->set_force_2d(b);

	return 0;
}

static int is_playing(lua_State *L)
{
	lua_pushboolean(L, g_player->is_playing());

	return 1;
}
static int get_swapeyes(lua_State *L)
{
	bool b = false;
	g_player->get_swap_eyes(&b);
	lua_pushboolean(L, b);

	return 1;
}
static int set_swapeyes(lua_State *L)
{
	g_player->set_swap_eyes(lua_toboolean(L, -1));

	lua_pushboolean(L, 1);
	return 1;
}

static int is_fullscreen(lua_State *L)
{
	lua_pushboolean(L, g_player->is_fullsceen(1));

	return 1;
}

static int tell(lua_State *L)
{
	int t = 0;
	g_player->tell(&t);
	lua_pushinteger(L, t);

	return 1;
}

static int total(lua_State *L)
{
	int t = 0;
	g_player->total(&t);
	lua_pushinteger(L, t);

	return 1;
}

static int seek(lua_State *L)
{
	int parameter_count = -lua_gettop(L);
	if (parameter_count >= 0)
		return 0;

	int target = lua_tonumber(L, parameter_count+0);
	g_player->seek(target);

	lua_pushboolean(L, TRUE);
	return 1;
}


static int get_volume(lua_State *L)
{
	double v = 0;
	g_player->get_volume(&v);
	lua_pushnumber(L, v);

	return 1;
}
static int set_volume(lua_State *L)
{
	int parameter_count = -lua_gettop(L);
	if (parameter_count >= 0)
		return 0;

	double v = lua_tonumber(L, parameter_count+0);
	g_player->set_volume(v);

	lua_pushboolean(L, TRUE);
	return 1;
}

static int reset_and_loadfile(lua_State *L)
{
	int n = lua_gettop(L);
	const char *filename1 = lua_tostring(L, -n+0);
	const char *filename2 = lua_tostring(L, -n+1);
	const bool stop = lua_isboolean(L, -n+2) ? lua_toboolean(L, -n+2) : false;

	HRESULT hr = g_player->reset_and_loadfile_core(filename1 ? UTF82W(filename1) : NULL, filename2 ? UTF82W(filename2) : NULL, stop);

	lua_pushboolean(L, SUCCEEDED(hr));
	lua_pushinteger(L, hr);
	return 2;
}

static int load_subtitle(lua_State *L)
{
	const char *filename = lua_tostring(L, -1);
	const bool reset = lua_isboolean(L, -2) ? lua_toboolean(L, -2) : false;

	HRESULT hr = g_player->load_subtitle(filename ? UTF82W(filename) : NULL, reset);

	lua_pushboolean(L, SUCCEEDED(hr));
	lua_pushinteger(L, hr);
	return 2;
}

static int load_audio_track(lua_State *L)
{
	const char *filename = lua_tostring(L, -1);
	if (!filename)
		return 0;

	HRESULT hr = g_player->load_audiotrack(filename ? UTF82W(filename) : NULL);

	lua_pushboolean(L, SUCCEEDED(hr));
	lua_pushinteger(L, hr);
	return 2;
}

static int show_mouse(lua_State *L)
{
	int parameter_count = -lua_gettop(L);
	if (parameter_count >= 0)
		return 0;

	bool v = lua_toboolean(L, parameter_count+0);
	g_player->show_mouse_core(v);

	lua_pushboolean(L, TRUE);
	return 1;
}
static int popup_menu(lua_State *L)
{
	g_player->popup_menu(g_player->get_window(1));

	return 0;
}
static int reset(lua_State *L)
{
	g_player->reset();

	return 0;
}

static int lua_get_splayer_subtitle(lua_State *L)
{
	int parameter_count = lua_gettop(L);
	if (parameter_count < 1 || lua_tostring(L, -parameter_count) == NULL)
		return 0;

	UTF82W filename (lua_tostring(L, -parameter_count));
	const wchar_t *langs[16] = {L"eng", L"", NULL};
	wchar_t tmp[16][200];
	for(int i=1; i<parameter_count && i<16; i++)
	{
		wcscpy(tmp[i-1], UTF82W(lua_tostring(L, -parameter_count+i)));
		langs[i-1] = tmp[i-1];
	}

	wchar_t out[5000] = {0};
	get_splayer_subtitle(filename, out, langs);

	wchar_t *outs[50] = {0};
	int result_count = wcsexplode(out, L"|", outs, 50);

	for(int i=0; i<result_count; i++)
		lua_pushstring(L, W2UTF8(outs[i]));

	for(int i=0; i<50; i++)
		if (outs[i])
			free(outs[i]);

	return result_count;
}

static int widi_start_scan(lua_State *L)
{
	lua_pushboolean(L, SUCCEEDED(g_player->widi_start_scan()));
	return 1;
}
static int widi_get_adapters(lua_State *L)
{
	int c = g_player->m_widi_num_adapters_found;
	for(int i = 0; i<c; i++)
	{
		wchar_t o[500] = L"";
		g_player->widi_get_adapter_by_id(i, o);
		lua_pushstring(L, W2UTF8(o));
	}

	return c;
}
static int widi_get_adapter_information(lua_State *L)
{
	int n = lua_gettop(L);
	int i = lua_tointeger(L, -n+0);
	const char *p = NULL;
	if (n>1)
		p = lua_tostring(L, -n+1);
	wchar_t o[500] = L"";
	g_player->widi_get_adapter_information(i, o, p ? UTF82W(p) : NULL);

	lua_pushstring(L, W2UTF8(o));
	return 1;
}
static int widi_connect(lua_State *L)
{
	int n = lua_gettop(L);
	int i = lua_tointeger(L, -n+0);
	DWORD mode = GET_CONST("WidiScreenMode");
	if (n>=2)
		mode = lua_tointeger(L, -n+1);
	
	lua_pushboolean(L, SUCCEEDED(g_player->widi_connect(i, mode)));
	return 1;
}
static int widi_set_screen_mode(lua_State *L)
{
	int n = lua_gettop(L);
	DWORD mode = GET_CONST("WidiScreenMode");
	if (n>=1)
		mode = lua_tointeger(L, -n+0);

	lua_pushboolean(L, SUCCEEDED(g_player->widi_set_screen_mode(mode)));
	return 1;
}
static int widi_has_support(lua_State *L)
{
	lua_pushboolean(L, g_player->m_widi_has_support);
	return 1;
}
static int widi_disconnect(lua_State *L)
{
	int id = -1;
	int n = lua_gettop(L);
	if (n>=1)
		id = lua_tointeger(L, -n+0);
	lua_pushboolean(L, SUCCEEDED(g_player->widi_disconnect(id)));
	return 1;
}

static int enable_audio_track(lua_State *L)
{
	if (lua_gettop(L) != 1)
		return 0;

	int id = lua_tointeger(L, -1);
	lua_pushboolean(L, SUCCEEDED(g_player->enable_audio_track(id)));
	return 1;
}
static int enable_subtitle_track(lua_State *L)
{
	if (lua_gettop(L) != 1)
		return 0;

	int id = lua_tointeger(L, -1);
	lua_pushboolean(L, SUCCEEDED(g_player->enable_subtitle_track(id)));
	return 1;
}
static int list_audio_track(lua_State *L)
{
	wchar_t tmp[32][1024];
	wchar_t *tmp2[32];
	bool connected[32];
	for(int i=0; i<32; i++)
		tmp2[i] = tmp[i];
	int found = 0;
	g_player->list_audio_track(tmp2, connected, &found);

	for(int i=0; i<found; i++)
	{
		lua_pushstring(L, W2UTF8(tmp[i]));
		lua_pushboolean(L, connected[i]);
	}

	return found * 2;
}
static int list_subtitle_track(lua_State *L)
{
	wchar_t tmp[32][1024];
	wchar_t *tmp2[32];
	bool connected[32];
	for(int i=0; i<32; i++)
		tmp2[i] = tmp[i];
	int found = 0;
	g_player->list_subtitle_track(tmp2, connected, &found);

	for(int i=0; i<found; i++)
	{
		lua_pushstring(L, W2UTF8(tmp[i]));
		lua_pushboolean(L, connected[i]);
	}

	return found * 2;
}

static int lua_find_main_movie(lua_State *L)
{
	if (lua_gettop(L) != 1)
		return 0;
	const char *p = lua_tostring(L, -1);

	wchar_t o[MAX_PATH];
	if (FAILED(find_main_movie(UTF82W(p), o)))
		return 0;

	lua_pushstring(L, W2UTF8(o));
	return 1;
}
static int enum_bd(lua_State *L)
{
	int n = 0;
	for(int i=L'B'; i<=L'Z'; i++)
	{
		wchar_t tmp[MAX_PATH] = L"C:\\";
		wchar_t tmp2[MAX_PATH];
		tmp[0] = i;
		tmp[4] = NULL;
		if (GetDriveTypeW(tmp) == DRIVE_CDROM)
		{
			n++;
			lua_pushstring(L, W2UTF8(tmp));
			if (!GetVolumeInformationW(tmp, tmp2, MAX_PATH, NULL, NULL, NULL, NULL, 0))
			{
				// no disc
				lua_pushnil(L);
				lua_pushboolean(L, false);
			}
			else if (FAILED(find_main_movie(tmp, tmp2)))
			{
				// not bluray
				GetVolumeInformationW(tmp, tmp2, MAX_PATH, NULL, NULL, NULL, NULL, 0);
				lua_pushstring(L, W2UTF8(tmp2));
				lua_pushboolean(L, false);
			}
			else
			{
				GetVolumeInformationW(tmp, tmp2, MAX_PATH, NULL, NULL, NULL, NULL, 0);
				lua_pushstring(L, W2UTF8(tmp2));
				lua_pushboolean(L, true);
			}

		}
	}

	return n*3;
}
static int enum_drive(lua_State *L)
{
	int n = 0;

#ifdef ZHUZHU
	for(int i=3; i<26; i++)
#else
	for(int i=0; i<26; i++)
#endif
	{
		wchar_t path[50];
		wchar_t volume_name[MAX_PATH];
		swprintf(path, L"%c:\\", L'A'+i);
		if (GetVolumeInformationW(path, volume_name, MAX_PATH, NULL, NULL, NULL, NULL, 0))
#ifdef ZHUZHU
			if (GetDriveTypeW(path) != DRIVE_CDROM)
#endif
		{
			lua_pushstring(L, W2UTF8(path));
			n++;
		}
	}

	return n;

}
static int enum_folder(lua_State *L)
{
	int n = 0;
	if (lua_gettop(L) != 1)
		return 0;

	const char * p = lua_tostring(L, -1);
	if (!p)
		return 0;

	wchar_t path[1024];
	wcscpy(path, UTF82W(p));
	if (path[wcslen(path)-1] != L'\\')
		wcscat(path, L"\\");
	wcscat(path, L"*.*");

	WIN32_FIND_DATAW find_data;
	HANDLE find_handle = FindFirstFileW(path, &find_data);

	lua_newtable(L);

	if (find_handle != INVALID_HANDLE_VALUE)
	{
		do
		{
			n++;
			if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)!=0
				&& wcscmp(L".",find_data.cFileName ) !=0
				&& wcscmp(L"..", find_data.cFileName) !=0
				)
			{
				wchar_t tmp[MAX_PATH];
				wcscpy(tmp, find_data.cFileName);
				wcscat(tmp, L"\\");
				lua_pushinteger(L, n);
				lua_pushstring(L, W2UTF8(tmp));
				lua_settable(L, -3);
			}
			else if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)==0)

			{
				lua_pushinteger(L, n);
				lua_pushstring(L, W2UTF8(find_data.cFileName));
				lua_settable(L, -3);
			}
		}
		while( FindNextFile(find_handle, &find_data ) );
	}

	return 1;
}

static int set_output_channel(lua_State *L)
{
	int channel = lua_tointeger(L, -1);
	lua_pushboolean(L, SUCCEEDED(g_player->set_output_channel(channel)));
	return 1;
}

static int set_normalize_audio(lua_State *L)
{
	bool normalize = lua_toboolean(L, -1);
	g_player->m_normalize_audio = normalize ? 16.0 : 0;
	
	lua_pushboolean(L, SUCCEEDED(set_ff_audio_normalizing(g_player->m_lav, g_player->m_normalize_audio)));
	return 1;
}
static int get_normalize_audio(lua_State *L)
{
	lua_pushboolean(L, (double)g_player->m_normalize_audio > 1.0);
	return 1;
}

static int set_input_layout(lua_State *L)
{
	int layout = lua_tointeger(L, -1);
	g_player->m_input_layout = layout;
	lua_pushboolean(L, SUCCEEDED(g_player->m_renderer1->set_input_layout(layout)));
	return 1;
}

static int set_output_mode(lua_State *L)
{
	int mode = lua_tointeger(L, -1);
	lua_pushboolean(L, SUCCEEDED(g_player->set_output_mode(mode)));
	return 1;
}

static int set_mask_mode(lua_State *L)
{
	int mode = lua_tointeger(L, -1);
	g_player->m_mask_mode = mode;
	lua_pushboolean(L, SUCCEEDED(g_player->m_renderer1->set_mask_mode(mode)));
	return 1;
}

static int set_topmost(lua_State *L)
{
	g_player->set_topmost(lua_tointeger(L, lua_gettop(L)));
	return 0;
}

static int set_zoom_factor(lua_State *L)
{
	int n = lua_gettop(L);
	double factor = 1.0;
	int x = -99999;
	int y = -99999;
	if (n >= 1)
		factor = lua_tonumber(L, -n+0);
	if (n >= 2)
		x = lua_tonumber(L, -n+1);
	if (n >= 3)
		y = lua_tonumber(L, -n+2);


	lua_pushboolean(L, SUCCEEDED(g_player->m_renderer1->set_zoom_factor(factor, x, y)));
	return 1;
}

static int set_subtitle_pos(lua_State *L)
{
	int n = lua_gettop(L);
	double x = g_player->m_subtitle_center_x;
	double y = g_player->m_subtitle_bottom_y;
	if (n>=1 && lua_isnumber(L, -n+0))
		x = lua_tonumber(L, -n+0);
	if (n>=2 && lua_isnumber(L, -n+1))
		y = lua_tonumber(L, -n+1);

	lua_pushboolean(L, SUCCEEDED(g_player->set_subtitle_pos(x, y)));
	return 1;
}

static int set_subtitle_parallax(lua_State *L)
{
	int n = lua_gettop(L);
	double p = g_player->m_user_subtitle_parallax;
	if (n>=1 && lua_isnumber(L, -n+0))
		p = lua_tonumber(L, -n+0);
	lua_pushboolean(L, SUCCEEDED(g_player->set_subtitle_parallax(p)));
	return 1;
}

static int set_subtitle_latency_stretch(lua_State *L)
{
	int n = lua_gettop(L);
	if (n>=1 && lua_isnumber(L, -n+0))
		GET_CONST("SubtitleLatency") = lua_tonumber(L, -n+0);
	if (n>=2 && lua_isnumber(L, -n+1))
		GET_CONST("SubtitleRatio") = lua_tonumber(L, -n+1);

	lua_pushboolean(L, SUCCEEDED(g_player->draw_subtitle()));
	return 1;
}
static int set_parallax(lua_State *L)
{
	int n = lua_gettop(L);
	double p = g_player->m_renderer1->get_parallax();
	if (n>=1 && lua_isnumber(L, -n+0))
		p = lua_tonumber(L, -n+0);
	lua_pushboolean(L, SUCCEEDED(g_player->m_renderer1->set_parallax(p)));
	return 1;
}

static int get_input_layout(lua_State *L)
{
	lua_pushinteger(L, g_player->m_input_layout);
	return 1;
}
static int get_output_mode(lua_State *L)
{
	lua_pushinteger(L, g_player->m_output_mode);
	return 1;
}
static int get_mask_mode(lua_State *L)
{
	lua_pushinteger(L, g_player->m_mask_mode);
	return 1;
}
static int get_zoom_factor(lua_State *L)
{
	lua_pushnumber(L, g_player->m_renderer1->get_zoom_factor());
	return 1;
}
static int get_subtitle_pos(lua_State *L)
{
	lua_pushnumber(L, g_player->m_subtitle_center_x);
	lua_pushnumber(L, g_player->m_subtitle_bottom_y);
	return 2;
}
static int get_subtitle_parallax(lua_State *L)
{
	lua_pushnumber(L, g_player->m_user_subtitle_parallax);
	return 1;
}
static int get_subtitle_latency_stretch(lua_State *L)
{
	lua_pushnumber(L, GET_CONST("SubtitleLatency"));
	lua_pushnumber(L, GET_CONST("SubtitleRatio"));
	return 2;
}
static int get_parallax(lua_State *L)
{
	lua_pushnumber(L, g_player->m_renderer1->get_parallax());
	return 1;
}
static int get_output_channel(lua_State *L)
{
	lua_pushinteger(L, g_player->m_channel);
	return 1;
}

static int enum_monitors(lua_State *L)
{
	bool horizontal = true;
	bool vertical = true;
	int n = lua_gettop(L);
	if (n>=1)
		horizontal = lua_toboolean(L, -n+0);
	if (n>=2)
		vertical = lua_toboolean(L, -n+1);

	for(int i=0; i<get_mixed_monitor_count(horizontal, vertical); i++)
	{
		RECT rect;// = g_logic_monitor_rects[i];
		wchar_t tmp[256];

		get_mixed_monitor_by_id(i, &rect, tmp, horizontal, vertical);

		lua_pushinteger(L, rect.left);
		lua_pushinteger(L, rect.top);
		lua_pushinteger(L, rect.right);
		lua_pushinteger(L, rect.bottom);
		lua_pushstring(L, W2UTF8(tmp));
	}

	return get_mixed_monitor_count(horizontal, vertical) * 5;
}

static int set_monitor(lua_State *L)
{
	int window_id = 0;
	int monitor_id = 0;
	int n = lua_gettop(L);
	if (n<2)
		return 0;

	window_id = lua_tointeger(L, -n+0);
	monitor_id = lua_tointeger(L, -n+1);

	lua_pushboolean(L, SUCCEEDED(g_player->set_output_monitor(window_id, monitor_id)));
	return 1;
}

static int show_color_adjust_dialog(lua_State *L)
{
	g_player->m_dialog_open ++;

	show_color_adjust(GetModuleHandle(NULL), g_player->get_window(-1));

	g_player->m_dialog_open --;

	return 0;
}
static int show_hd3d_fullscreen_mode_dialog(lua_State *L)
{
	g_player->show_hd3d_fullscreen_mode_dialog();

	return 0;
}
static int show_latency_ratio_dialog(lua_State *L)
{
	int n = lua_gettop(L);
	if (n<3)
		return 0;
	bool for_audio = lua_toboolean(L, -n+0);
	int latency = lua_tointeger(L, -n+1);
	double ratio = lua_tonumber(L, -n+2);

	g_player->m_dialog_open ++;
	HRESULT hr = latency_modify_dialog(g_player->m_hexe, g_player->get_window(-1), &latency, &ratio, for_audio);
	g_player->m_dialog_open --;

	lua_pushinteger(L, latency);
	lua_pushnumber(L, ratio);

	return 2;	
}
static int message_box(lua_State *L)
{
	int n = lua_gettop(L);
	if (n<3)
		return 0;
	const char *text = lua_tostring(L, -n+0);
	const char *caption = lua_tostring(L, -n+1);
	int button = lua_tointeger(L, -n+2);

	lua_pushinteger(L, MessageBoxW(g_player->get_window(-1), UTF82W(text), UTF82W(caption), button));
	return 1;
}

static int show_media_info_lua(lua_State *L)
{
	int n = lua_gettop(L);
	if (n<1)
		return 0;
	const char *filename = lua_tostring(L, -n+0);


	show_media_info(UTF82W(filename), g_player->m_full1 ? g_player->get_window(-1) : NULL);
	lua_pushboolean(L, true);
	return 1;
}

static int show_about(lua_State *L)
{
	ShowAbout(g_player->get_window(-1));
	return 0;
}

static int lua_exit(lua_State *L)
{
	SendMessage(g_player->get_window(1), WM_CLOSE, 0, 0);
	SendMessage(g_player->get_window(2), WM_CLOSE, 0, 0);
	return 0;
}

static int logout(lua_State *L)
{
	memset(g_passkey_big, 0, 128);
	save_passkey();

	lua_pushboolean(L, true);
	return 1;
}

extern int setting_dlg(HWND parent);
static int show_setting(lua_State *L)
{
	g_player->m_dialog_open++;
	setting_dlg(g_player->get_window(-1));
	g_player->m_dialog_open--;
	return 0;
}

static int set_window_text(lua_State *L)
{
	int n = lua_gettop(L);
	if (n>0)
	{
		if (lua_tostring(L, 1))
			SetWindowTextW(g_player->get_window(1), UTF82W(lua_tostring(L, 1)));
		if (lua_tostring(L, 2))
			SetWindowTextW(g_player->get_window(2), UTF82W(lua_tostring(L, 2)));
	}
	return 0;
}

int player_lua_init()
{
	g_player_lua_manager = new lua_manager("player");
	g_player_lua_manager->get_variable("play") = &play;
	g_player_lua_manager->get_variable("pause") = &pause;
	g_player_lua_manager->get_variable("stop") = &stop;
	g_player_lua_manager->get_variable("tell") = &tell;
	g_player_lua_manager->get_variable("total") = &total;
	g_player_lua_manager->get_variable("seek") = &seek;
	g_player_lua_manager->get_variable("is_playing") = &is_playing;
	g_player_lua_manager->get_variable("reset") = &reset;
	g_player_lua_manager->get_variable("reset_and_loadfile") = &reset_and_loadfile;
	g_player_lua_manager->get_variable("load_subtitle") = &load_subtitle;
	g_player_lua_manager->get_variable("load_audio_track") = &load_audio_track;
	g_player_lua_manager->get_variable("is_fullscreen") = &is_fullscreen;
	g_player_lua_manager->get_variable("toggle_fullscreen") = &toggle_fullscreen;
	g_player_lua_manager->get_variable("toggle_3d") = &toggle_3d;
	g_player_lua_manager->get_variable("get_swapeyes") = &get_swapeyes;
	g_player_lua_manager->get_variable("set_swapeyes") = &set_swapeyes;
	g_player_lua_manager->get_variable("get_volume") = &get_volume;
	g_player_lua_manager->get_variable("set_volume") = &set_volume;
	g_player_lua_manager->get_variable("show_mouse") = &show_mouse;
	g_player_lua_manager->get_variable("popup_menu") = &popup_menu;
	g_player_lua_manager->get_variable("enum_monitors") = &enum_monitors;
	g_player_lua_manager->get_variable("set_monitor") = &set_monitor;
	g_player_lua_manager->get_variable("show_color_adjust_dialog") = &show_color_adjust_dialog;
	g_player_lua_manager->get_variable("show_hd3d_fullscreen_mode_dialog") = &show_hd3d_fullscreen_mode_dialog;
	g_player_lua_manager->get_variable("show_latency_ratio_dialog") = &show_latency_ratio_dialog;
	g_player_lua_manager->get_variable("message_box") = &message_box;
	g_player_lua_manager->get_variable("show_media_info") = &show_media_info_lua;
	g_player_lua_manager->get_variable("show_about") = &show_about;
	g_player_lua_manager->get_variable("show_setting") = &show_setting;
	g_player_lua_manager->get_variable("set_window_text") = &set_window_text;
	g_player_lua_manager->get_variable("set_topmost") = &set_topmost;
	g_player_lua_manager->get_variable("logout") = &logout;
	g_player_lua_manager->get_variable("exit") = &lua_exit;

	g_player_lua_manager->get_variable("get_splayer_subtitle") = &lua_get_splayer_subtitle;

	g_player_lua_manager->get_variable("enable_audio_track") = &enable_audio_track;
	g_player_lua_manager->get_variable("enable_subtitle_track") = &enable_subtitle_track;
	g_player_lua_manager->get_variable("list_audio_track") = &list_audio_track;
	g_player_lua_manager->get_variable("list_subtitle_track") = &list_subtitle_track;

	g_player_lua_manager->get_variable("find_main_movie") = &lua_find_main_movie;
	g_player_lua_manager->get_variable("enum_bd") = &enum_bd;
	g_player_lua_manager->get_variable("enum_drive") = &enum_drive;
	g_player_lua_manager->get_variable("enum_folder") = &enum_folder;

	g_player_lua_manager->get_variable("set_output_channel") = &set_output_channel;
	g_player_lua_manager->get_variable("set_normalize_audio") = &set_normalize_audio;
	g_player_lua_manager->get_variable("set_input_layout") = &set_input_layout;
	g_player_lua_manager->get_variable("set_output_mode") = &set_output_mode;
	g_player_lua_manager->get_variable("set_mask_mode") = &set_mask_mode;
	g_player_lua_manager->get_variable("set_zoom_factor") = &set_zoom_factor;
	g_player_lua_manager->get_variable("set_subtitle_pos") = &set_subtitle_pos;
	g_player_lua_manager->get_variable("set_subtitle_parallax") = &set_subtitle_parallax;
	g_player_lua_manager->get_variable("set_subtitle_latency_stretch") = &set_subtitle_latency_stretch;
	g_player_lua_manager->get_variable("set_parallax") = &set_parallax;

	

	g_player_lua_manager->get_variable("get_normalize_audio") = &get_normalize_audio;
	g_player_lua_manager->get_variable("get_output_channel") = &get_output_channel;
	g_player_lua_manager->get_variable("get_input_layout") = &get_input_layout;
	g_player_lua_manager->get_variable("get_output_mode") = &get_output_mode;
	g_player_lua_manager->get_variable("get_mask_mode") = &get_mask_mode;
	g_player_lua_manager->get_variable("get_zoom_factor") = &get_zoom_factor;
	g_player_lua_manager->get_variable("get_subtitle_pos") = &get_subtitle_pos;
	g_player_lua_manager->get_variable("get_subtitle_parallax") = &get_subtitle_parallax;
	g_player_lua_manager->get_variable("get_subtitle_latency_stretch") = &get_subtitle_latency_stretch;
	g_player_lua_manager->get_variable("get_parallax") = &get_parallax;
	

	// widi
	g_player_lua_manager->get_variable("widi_has_support") = &widi_has_support;
	g_player_lua_manager->get_variable("widi_start_scan") = &widi_start_scan;
	g_player_lua_manager->get_variable("widi_get_adapters") = &widi_get_adapters;
	g_player_lua_manager->get_variable("widi_get_adapter_information") = &widi_get_adapter_information;
	g_player_lua_manager->get_variable("widi_connect") = &widi_connect;
	g_player_lua_manager->get_variable("widi_set_screen_mode") = &widi_set_screen_mode;
	g_player_lua_manager->get_variable("widi_disconnect") = &widi_disconnect;

	return 0;
}
