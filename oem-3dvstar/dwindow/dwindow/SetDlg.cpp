// SetDlg.cpp : 定义控制台应用程序的入口点。
//

#include <stdio.h>
#include <tchar.h>
#include <Windows.h>
#include <commctrl.h>
#include "resource3.h"
#include "..\lua\my12doom_lua.h"
#include "global_funcs.h"
#include "dx_player.h"

extern dx_player *g_player;
int g_current_displayid;

enum
{
    ORDINARY_CLICK = 0,
    VOICE_CLICK,
    VIDEO1_CLICK,
    VIDEO2_CLICK,
    SET3D_CLICK,
    SUBTILE_CLICK,
    ABOUT_CLICK,
    RELATION_CLICK,
    VIDEO_CLICK
};

typedef struct tagRelation 
{
	bool mp4;
	bool avi;
	bool rmvb;
	bool _3dv;
	bool ts;
	bool mkv;
	bool wmv;
	bool vob;
}Relationship;

enum slider_hand
{
    LEFT_BRIGHTNESS,
    LEFT_CONTRAST,
    LEFT_SATURATION,
    LEFT_HUE,
    RIGHT_BRIGHTNESS,
    RIGHT_CONTRAST,
    RIGHT_SATURATION,
    RIGHT_HUE
};
enum contrl_type
{
    TYPE_RADIO,
    TYPE_CHECK,
    TYPE_COMBO,
    TYPE_EDIT,
    TYPE_SLIDER
};

struct tagLuaSetting
{
    int nCtrlID;
    char arrSettingName[100];
    int nCtrlType;
    //lua_const& luaVal;
};

struct tagcolor
{
    double left_bri;
    double left_con;
    double left_sat;
    double left_hue;
    double right_bri;
    double right_con;
    double right_sat;
    double right_hue;
};


DWORD color = 16777215;
HBITMAP g_AboutPic;
HBITMAP g_AboutPic2;
HWND Ordinary[11] = {0};
HWND Voice[4] = {0};
HWND VideoPage1[18] = {0};
HWND VideoPage2[23] = {0};
HWND Set3D[20] = {0};
HWND Subtitle[20] = {0};
HWND About[20] = {0};
HWND Relation[20] = {0};
BOOL g_OnShow = TRUE;
Relationship assoc;

tagcolor saveOnShow;

// lua_const& FullMode;

HWND *AllCtrl[10]  = {Ordinary, Voice, VideoPage1, VideoPage2, Set3D, Subtitle, About, Relation};
tagLuaSetting TestSet[] = 
{
    //
//     {IDC_ORD_RADIO_ALWAY, "", TYPE_RADIO},
//     {IDC_ORD_RADIO_WHENPLAY, "", TYPE_RADIO},
//     {IDC_ORD_RADIO_NEVER, "", TYPE_RADIO},
//     {IDC_ORD_COM_LANGUAGE, "", TYPE_COMBO},
    //Video page 1
    {IDC_VIDEO_COM_FULLMODE, "AspectRatioMode", TYPE_COMBO},        //0
    {IDC_VIDEO_COM_IMAGERATE, "Aspect", TYPE_COMBO},                //1
    {IDC_VIDEO_CHECK_LOW_CPU, "GPUIdle", TYPE_CHECK},               //2
    {IDC_VIDEO_CHECK_FULLSCREEN, "ExclusiveMode", TYPE_CHECK},      //3
    //{IDC_VIDEO_CHECK_WIN_2_VIDEO, "", TYPE_CHECK},
    {IDC_VIDEO_CHECK_VIDEO_2_WIN, "OnOpen", TYPE_CHECK},            //4
//     {IDC_VIDEO_CHECK_LOAD_LAST_POS, "", TYPE_CHECK},
    {IDC_VIDEO_CHECK_HARD_SPEED, "CUDA", TYPE_CHECK},               //5
    //{IDC_VIDEO_CHECK_DEINTERLACE, "", TYPE_CHECK},
    //Video page 2
    {IDC_VIDEO_RADIO_SAME_CONTROL, "SyncAdjust", TYPE_CHECK},       //6
    {IDC_VIDEO_SLIDER_LEFT_BRIGHTNESS, "Luminance", TYPE_SLIDER},   //7  GET_CONt(m[7].c) = 
    {IDC_VIDEO_SLIDER_LEFT_CONTRAST, "Contrast", TYPE_SLIDER},      //8
    {IDC_VIDEO_SLIDER_LEFT_SATURATION, "Saturation", TYPE_SLIDER},  //9
    {IDC_VIDEO_SLIDER_LEFT_HUE, "Hue", TYPE_SLIDER},                //10
    {IDC_VIDEO_SLIDER_RIGHT_BRIGHTNESS, "Luminance2", TYPE_SLIDER}, //11
    {IDC_VIDEO_SLIDER_RIGHT_CONTRAST, "Contrast2", TYPE_SLIDER},    //12
    {IDC_VIDEO_SLIDER_RIGHT_SATURATION, "Saturation2", TYPE_SLIDER},//13
    {IDC_VIDEO_SLIDER_RIGHT_HUE, "Hue2", TYPE_SLIDER},              //14
    //3D page
    {IDC_3D_CHECK_SHOW2D, "Force2D", TYPE_CHECK},                   //15
    {IDC_3D_CHECK_EXCHANGE, "SwapEyes", TYPE_CHECK},                //16
    {IDC_3D_COM_INPUT_MODE, "InputLayout", TYPE_COMBO},             //17
    {IDC_3D_COM_OUTPUT_MODE, "OutputMode", TYPE_COMBO},             //18
    {IDC_3D_COM_FALL_MODE_ONE, "Screen1", TYPE_COMBO},              //19
    {IDC_3D_COM_FALL_MODE_ONE, "Screen1", TYPE_COMBO},              //20

    //voice 
    {IDC_VOICE_COM_CHANNEL, "AudioChannel", TYPE_COMBO},            //21
    {IDC_VOICE_CHECK_AUTO_VOICE_DECODE, "InternalAudioDecoder", TYPE_CHECK},//22
    {IDC_VOICE_COM_MAX_VOICE, "NormalizeAudio", TYPE_CHECK}, //16 or 1      //23

    //Subtitle
    //DisplaySubtitle
    {IDC_SUBTITLE_CHECK_SHOWSUB, "DisplaySubtitle", TYPE_CHECK},            //24
    {IDC_SUBTITLE_COM_STYLE, "Font", TYPE_COMBO},                           //25
    {IDC_SUBTITLE_COM_SIZE, "FontSize", TYPE_COMBO},                        //26
    {IDC_SUBTITLE_BTN_SELECT_COLOR, "FontColor", TYPE_COMBO},                      //27
    {IDC_SUBTITLE_EDIT_LATE_SHOW, "SubtitleLatency", TYPE_EDIT},            //28
    {IDC_SUBTITLE_EDIT_LATE_SHOW_SIZE, "SubtitleRatio", TYPE_EDIT},         //29
    //add
    {IDC_VIDEO_CHECK_DEINTERLACE, "ForcedDeinterlace", TYPE_CHECK},          //30
    {IDC_ORD_COM_TOPMOST, "Topmost", TYPE_COMBO},          //30

    //relation
    {IDC_RELA_CHECK_MP4, "mp4", -1},
    {IDC_RELA_CHECK_AVI, "avi", -1},
    {IDC_RELA_CHECK_RMVB, "rmvb", -1},
    {IDC_RELA_CHECK_3DV, "_3dv", -1},
    {IDC_RELA_CHECK_TS, "ts", -1},
    {IDC_RELA_CHECK_MKV, "mkv", -1},
    {IDC_RELA_CHECK_WMV, "wmv", -1},
    {IDC_RELA_CHECK_VOB, "vob", -1}
    //Relationship
//     {IDC_RELA_CHECK_MP4, "", TYPE_CHECK},
//     {IDC_RELA_CHECK_AVI, "", TYPE_CHECK},
//     {IDC_RELA_CHECK_RMVB, "", TYPE_CHECK},
//     {IDC_RELA_CHECK_3DV, "", TYPE_CHECK},
//     {IDC_RELA_CHECK_TS, "", TYPE_CHECK},
//     {IDC_RELA_CHECK_MKV, "", TYPE_CHECK},
//     {IDC_RELA_CHECK_WMV, "", TYPE_CHECK},
//     {IDC_RELA_CHECK_VOB, "", TYPE_CHECK},
};

void GetCtrolItemhWnd(HWND hwndDlg);
void SetShowCtrol(int nItem, int nSubItem = 0);
void LoadSetting(HWND hWnd);
void ApplySetting(HWND hWnd);
int CALLBACK MyEnumFontProc(ENUMLOGFONTEX* lpelf,NEWTEXTMETRICEX* lpntm,DWORD nFontType,long lParam)
{
    HWND hWnd = (HWND)lParam;
    if (SendMessageW(GetDlgItem(hWnd, IDC_SUBTITLE_COM_STYLE), CB_FINDSTRING, 0, WPARAM(lpelf->elfLogFont.lfFaceName)))
        SendMessageW(GetDlgItem(hWnd, IDC_SUBTITLE_COM_STYLE), CB_INSERTSTRING, 0, (WPARAM)lpelf->elfLogFont.lfFaceName);
    return 1;
}

int GetSlider(HWND hwnd, HANDLE hand)
{
    if (hand == GetDlgItem(hwnd, IDC_VIDEO_SLIDER_LEFT_BRIGHTNESS))
        return LEFT_BRIGHTNESS;
    if (hand == GetDlgItem(hwnd, IDC_VIDEO_SLIDER_LEFT_SATURATION))
        return LEFT_SATURATION;
    if (hand == GetDlgItem(hwnd, IDC_VIDEO_SLIDER_LEFT_CONTRAST))
        return LEFT_CONTRAST;
    if (hand == GetDlgItem(hwnd, IDC_VIDEO_SLIDER_LEFT_HUE))
        return LEFT_HUE;
    if (hand == GetDlgItem(hwnd, IDC_VIDEO_SLIDER_RIGHT_BRIGHTNESS))
        return RIGHT_BRIGHTNESS;
    if (hand == GetDlgItem(hwnd, IDC_VIDEO_SLIDER_RIGHT_SATURATION))
        return RIGHT_SATURATION;
    if (hand == GetDlgItem(hwnd, IDC_VIDEO_SLIDER_RIGHT_CONTRAST))
        return RIGHT_CONTRAST;
    if (hand == GetDlgItem(hwnd, IDC_VIDEO_SLIDER_RIGHT_HUE))
        return RIGHT_HUE;
    return -1;
}

BOOL CALLBACK SetProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    RECT rect = {90, 5, 490, 305};
    switch (message)
    {
    case WM_INITDIALOG:
        {
			localize_window(hwndDlg);
            GetCtrolItemhWnd(hwndDlg);
            SetShowCtrol(ORDINARY_CLICK, 0);

            //Language init
			lua_const &lcid =  GET_CONST("LCID");
			luaState L;
			lua_getglobal(L, "core");
			lua_getfield(L, -1, "get_language_list");
			lua_mypcall(L, 0, 1, 0);
			int n = lua_objlen(L, -1);
			int sel = -1;
			for(int i=1; i<=n; i++)
			{
				lua_pushinteger(L, i);
				lua_gettable(L, -2);
				UTF82W w(lua_tostring(L, -1));
				if (wcscmp(lcid, w) == 0)
					sel = i;
				SendMessageW(GetDlgItem(hwndDlg, IDC_ORD_COM_LANGUAGE), CB_INSERTSTRING, -1, (WPARAM)(const wchar_t*)w);
				lua_pop(L, 1);
			}
			SendMessageW(GetDlgItem(hwndDlg, IDC_ORD_COM_LANGUAGE), CB_SETCURSEL, sel-1, sel-1);
			lua_settop(L, 0);

			// fullscreen output
			// list monitors
			for(int i=0; i<get_mixed_monitor_count(true, true); i++)
			{
				RECT rect;// = g_logic_monitor_rects[i];
				wchar_t tmp[256];

				get_mixed_monitor_by_id(i, &rect, tmp, true, true);

				//wsprintfW(tmp, L"%s %d (%dx%d)", (const wchar_t*)C(L"Monitor"), i+1, rect.right - rect.left, rect.bottom - rect.top);
				SendMessageW(GetDlgItem(hwndDlg, IDC_3D_COM_FALL_MODE_ONE), CB_INSERTSTRING, -1, (LPARAM)tmp);
				lua_const &screen = GET_CONST("Screen1");
				if (compare_rect(rect, screen))
				{
					SendMessageW(GetDlgItem(hwndDlg, IDC_3D_COM_FALL_MODE_ONE), CB_SETCURSEL, i, i);
					g_current_displayid = i;
				}

			}

            //image format init 
            SendMessageW(GetDlgItem(hwndDlg, IDC_VIDEO_COM_IMAGERATE), CB_INSERTSTRING, 0, (WPARAM)(const wchar_t*)C(L"2.35:1"));
            SendMessageW(GetDlgItem(hwndDlg, IDC_VIDEO_COM_IMAGERATE), CB_INSERTSTRING, 0, (WPARAM)(const wchar_t*)C(L"16:9"));
            SendMessageW(GetDlgItem(hwndDlg, IDC_VIDEO_COM_IMAGERATE), CB_INSERTSTRING, 0, (WPARAM)(const wchar_t*)C(L"4:3"));
            SendMessageW(GetDlgItem(hwndDlg, IDC_VIDEO_COM_IMAGERATE), CB_INSERTSTRING, 0, (WPARAM)(const wchar_t*)C(L"Source Aspect"));

            //Stretch mode
            SendMessageW(GetDlgItem(hwndDlg, IDC_VIDEO_COM_FULLMODE), CB_INSERTSTRING, 0, (WPARAM)(const wchar_t*)C(L"Vertical Fill"));
            SendMessageW(GetDlgItem(hwndDlg, IDC_VIDEO_COM_FULLMODE), CB_INSERTSTRING, 0, (WPARAM)(const wchar_t*)C(L"Horizontal Fill"));
            SendMessageW(GetDlgItem(hwndDlg, IDC_VIDEO_COM_FULLMODE), CB_INSERTSTRING, 0, (WPARAM)(const wchar_t*)C(L"Stretch"));
            SendMessageW(GetDlgItem(hwndDlg, IDC_VIDEO_COM_FULLMODE), CB_INSERTSTRING, 0, (WPARAM)(const wchar_t*)C(L"Default(Letterbox)"));

            //input mode
            SendMessageW(GetDlgItem(hwndDlg, IDC_3D_COM_INPUT_MODE), CB_INSERTSTRING, 0, (WPARAM)(const wchar_t*)C(L"Auto"));
            SendMessageW(GetDlgItem(hwndDlg, IDC_3D_COM_INPUT_MODE), CB_INSERTSTRING, 0, (WPARAM)(const wchar_t*)C(L"Monoscopic"));
            SendMessageW(GetDlgItem(hwndDlg, IDC_3D_COM_INPUT_MODE), CB_INSERTSTRING, 0, (WPARAM)(const wchar_t*)C(L"Top Bottom"));
            SendMessageW(GetDlgItem(hwndDlg, IDC_3D_COM_INPUT_MODE), CB_INSERTSTRING, 0, (WPARAM)(const wchar_t*)C(L"Side By Side"));

            //output mode
            SendMessageW(GetDlgItem(hwndDlg, IDC_3D_COM_OUTPUT_MODE), CB_INSERTSTRING, 0, (WPARAM)(const wchar_t*)C(L"3DTV - Row Interlace"));
            SendMessageW(GetDlgItem(hwndDlg, IDC_3D_COM_OUTPUT_MODE), CB_INSERTSTRING, 0, (WPARAM)(const wchar_t*)C(L"3DTV - Checkboard Interlace"));
            SendMessageW(GetDlgItem(hwndDlg, IDC_3D_COM_OUTPUT_MODE), CB_INSERTSTRING, 0, (WPARAM)(const wchar_t*)C(L"3DTV - Line Interlace"));
            SendMessageW(GetDlgItem(hwndDlg, IDC_3D_COM_OUTPUT_MODE), CB_INSERTSTRING, 0, (WPARAM)(const wchar_t*)C(L"Intel Stereoscopic"));
            SendMessageW(GetDlgItem(hwndDlg, IDC_3D_COM_OUTPUT_MODE), CB_INSERTSTRING, 0, (WPARAM)(const wchar_t*)C(L"AMD HD 3D"));
            SendMessageW(GetDlgItem(hwndDlg, IDC_3D_COM_OUTPUT_MODE), CB_INSERTSTRING, 0, (WPARAM)(const wchar_t*)C(L"3DTV - Half Top Bottom"));
            SendMessageW(GetDlgItem(hwndDlg, IDC_3D_COM_OUTPUT_MODE), CB_INSERTSTRING, 0, (WPARAM)(const wchar_t*)C(L"3DTV - Half Side By Side"));
            SendMessageW(GetDlgItem(hwndDlg, IDC_3D_COM_OUTPUT_MODE), CB_INSERTSTRING, 0, (WPARAM)(const wchar_t*)C(L"IZ3D Displayer"));
            SendMessageW(GetDlgItem(hwndDlg, IDC_3D_COM_OUTPUT_MODE), CB_INSERTSTRING, 0, (WPARAM)(const wchar_t*)C(L"Gerneral 120Hz Glasses"));
            SendMessageW(GetDlgItem(hwndDlg, IDC_3D_COM_OUTPUT_MODE), CB_INSERTSTRING, 0, (WPARAM)(const wchar_t*)C(L"Monoscopic 2D"));
            SendMessageW(GetDlgItem(hwndDlg, IDC_3D_COM_OUTPUT_MODE), CB_INSERTSTRING, 0, (WPARAM)(const wchar_t*)C(L"Anaglyph"));
            SendMessageW(GetDlgItem(hwndDlg, IDC_3D_COM_OUTPUT_MODE), CB_INSERTSTRING, 0, (WPARAM)(const wchar_t*)C(L"Subpixel Interlace"));
            SendMessageW(GetDlgItem(hwndDlg, IDC_3D_COM_OUTPUT_MODE), CB_INSERTSTRING, 0, (WPARAM)(const wchar_t*)C(L"Nvidia 3D Vision"));

            // audio channel
            SendMessageW(GetDlgItem(hwndDlg, IDC_VOICE_COM_CHANNEL), CB_INSERTSTRING, 0, (WPARAM)(const wchar_t*)C(L"Source"));
            SendMessageW(GetDlgItem(hwndDlg, IDC_VOICE_COM_CHANNEL), CB_INSERTSTRING, 0, (WPARAM)(const wchar_t*)C(L"Dolby Headphone"));
            SendMessageW(GetDlgItem(hwndDlg, IDC_VOICE_COM_CHANNEL), CB_INSERTSTRING, 0, (WPARAM)(const wchar_t*)C(L"7.1 Channel"));
            SendMessageW(GetDlgItem(hwndDlg, IDC_VOICE_COM_CHANNEL), CB_INSERTSTRING, 0, (WPARAM)(const wchar_t*)C(L"5.1 Channel"));
            SendMessageW(GetDlgItem(hwndDlg, IDC_VOICE_COM_CHANNEL), CB_INSERTSTRING, 0, (WPARAM)(const wchar_t*)C(L"Stereo"));
            SendMessageW(GetDlgItem(hwndDlg, IDC_VOICE_COM_CHANNEL), CB_INSERTSTRING, 0, (WPARAM)(const wchar_t*)C(L"Source"));

            SendMessageW(GetDlgItem(hwndDlg, IDC_ORD_COM_TOPMOST), CB_INSERTSTRING, -1, (WPARAM)(const wchar_t*)C(L"Never"));
            SendMessageW(GetDlgItem(hwndDlg, IDC_ORD_COM_TOPMOST), CB_INSERTSTRING, -1, (WPARAM)(const wchar_t*)C(L"When Playing"));
			SendMessageW(GetDlgItem(hwndDlg, IDC_ORD_COM_TOPMOST), CB_INSERTSTRING, -1, (WPARAM)(const wchar_t*)C(L"Always"));

            g_AboutPic = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BMP_ABOUT));

            SendMessageW(GetDlgItem(hwndDlg, IDC_ABOUT_BACK), BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)g_AboutPic);
            //font 
            HDC dc = GetDC(hwndDlg);
            LOGFONTW lf;
            lf.lfCharSet=DEFAULT_CHARSET;
            lf.lfFaceName[0]=NULL;
            lf.lfPitchAndFamily=0;
            EnumFontFamiliesExW(dc, &lf, (FONTENUMPROCW)MyEnumFontProc, (LPARAM)hwndDlg, 0);


            //font size
            wchar_t szSize[10] = {0};
            for (int i = 10; i >=1; i--)
            {
                wsprintf(szSize, L"%d", (i + 1) * 10);
                SendMessageW(GetDlgItem(hwndDlg, IDC_SUBTITLE_COM_SIZE), CB_INSERTSTRING, 0, (WPARAM)szSize);
            }

            SendMessage(GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_LEFT_BRIGHTNESS), TBM_SETRANGE, TRUE, MAKELPARAM(0, 1000));
            SendMessage(GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_LEFT_CONTRAST), TBM_SETRANGE, TRUE, MAKELPARAM(0, 1000));
            SendMessage(GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_LEFT_SATURATION), TBM_SETRANGE, TRUE, MAKELPARAM(0, 1000));
            SendMessage(GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_LEFT_HUE), TBM_SETRANGE, TRUE, MAKELPARAM(0, 1000));
            SendMessage(GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_RIGHT_BRIGHTNESS), TBM_SETRANGE, TRUE, MAKELPARAM(0, 1000));
            SendMessage(GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_RIGHT_CONTRAST), TBM_SETRANGE, TRUE, MAKELPARAM(0, 1000));
            SendMessage(GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_RIGHT_SATURATION), TBM_SETRANGE, TRUE, MAKELPARAM(0, 1000));
            SendMessage(GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_RIGHT_HUE), TBM_SETRANGE, TRUE, MAKELPARAM(0, 1000));



			SendMessageW(GetDlgItem(hwndDlg, IDC_VIDEO_CHECK_ONSHOW), BM_SETCHECK, BST_CHECKED, 0);


            LoadSetting(hwndDlg);

//             SendMessage(GetDlgItem(hwndDlg, IDC_ORD_RADIO_ALWAY), BM_SETCHECK, BST_CHECKED, 0); 
//             SendMessage(GetDlgItem(hwndDlg, IDC_ORD_RADIO_WHENPLAY), BM_SETCHECK, BST_CHECKED, 0); 
//             SendMessage(GetDlgItem(hwndDlg, IDC_ORD_RADIO_NEVER), BM_SETCHECK, BST_CHECKED, 0); 
//             SendMessage(GetDlgItem(hwndDlg, IDC_3D_CHECK_EXCHANGE), BM_SETCHECK, BST_UNCHECKED, 0);
            
            //SetCheck()
            //KillbtnFocus(ORDINARY_CLICK, hwndDlg);
			ApplySetting(hwndDlg);
       }
        break;
    case WM_HSCROLL:
        {
            int n = SendMessage(GetDlgItem(hwndDlg, IDC_VIDEO_RADIO_SAME_CONTROL), BM_GETCHECK, 0, 0);
            int nType = GetSlider(hwndDlg, (HANDLE)lParam);
            //if (type != -1 && ((LOWORD(wParam) == SB_THUMBTRACK) || (LOWORD(wParam) == SB_PAGEDOWN || (LOWORD(wParam) == SB_PAGEUP))))
            if ((n == BST_CHECKED) && ((LOWORD(wParam) == SB_THUMBTRACK) || (LOWORD(wParam) == SB_PAGEDOWN || (LOWORD(wParam) == SB_PAGEUP))) )
            {
                if (nType == LEFT_BRIGHTNESS)
                    SendMessage(GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_RIGHT_BRIGHTNESS), TBM_SETPOS, TRUE, HIWORD(wParam));
                if (nType == LEFT_CONTRAST)
                    SendMessage(GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_RIGHT_CONTRAST), TBM_SETPOS, TRUE, HIWORD(wParam));
                if (nType == LEFT_SATURATION)
                    SendMessage(GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_RIGHT_SATURATION), TBM_SETPOS, TRUE, HIWORD(wParam));
                if (nType == LEFT_HUE)
                    SendMessage(GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_RIGHT_HUE), TBM_SETPOS, TRUE, HIWORD(wParam));
                if (nType == RIGHT_BRIGHTNESS)
                    SendMessage(GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_LEFT_BRIGHTNESS), TBM_SETPOS, TRUE, HIWORD(wParam));
                if (nType == RIGHT_CONTRAST)
                    SendMessage(GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_LEFT_CONTRAST), TBM_SETPOS, TRUE, HIWORD(wParam));
                if (nType == RIGHT_SATURATION)
                    SendMessage(GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_LEFT_SATURATION), TBM_SETPOS, TRUE, HIWORD(wParam));
                if (nType == RIGHT_HUE)
                    SendMessage(GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_LEFT_HUE), TBM_SETPOS, TRUE, HIWORD(wParam));
            }

            //预览
            int nn = SendMessage(GetDlgItem(hwndDlg, IDC_VIDEO_CHECK_ONSHOW), BM_GETCHECK, 0, 0);
            if (nn == BST_CHECKED)
			{
				GET_CONST("Luminance")  = SendMessage(GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_LEFT_BRIGHTNESS), TBM_GETPOS, 0, 0) / 1000.0;
				GET_CONST("Contrast")  = SendMessage(GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_LEFT_CONTRAST), TBM_GETPOS, 0, 0) / 1000.0;
				GET_CONST("Saturation")  = SendMessage(GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_LEFT_SATURATION), TBM_GETPOS, 0, 0) / 1000.0;
				GET_CONST("Hue")  = SendMessage(GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_LEFT_HUE), TBM_GETPOS, 0, 0) / 1000.0;
				GET_CONST("Luminance2")  = SendMessage(GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_RIGHT_BRIGHTNESS), TBM_GETPOS, 0, 0) / 1000.0;
				GET_CONST("Contrast2")  = SendMessage(GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_RIGHT_CONTRAST), TBM_GETPOS, 0, 0) / 1000.0;
				GET_CONST("Saturation2")  = SendMessage(GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_RIGHT_SATURATION), TBM_GETPOS, 0, 0) / 1000.0;
				GET_CONST("Hue2")  = SendMessage(GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_RIGHT_HUE), TBM_GETPOS, 0, 0) / 1000.0;
			}
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDOK:
                EndDialog(hwndDlg, wParam);
                break;
            case IDCANCEL:
				{
					EndDialog(hwndDlg, wParam);
				}
                break;
            case IDC_BTN_ORDINARY:
                SetShowCtrol(ORDINARY_CLICK, 0);
                //KillbtnFocus(ORDINARY_CLICK, hwndDlg);
                //                 ShowWindow(BtnHwnd, SW_SHOW);
                break;
            case IDC_BTN_SOUND:
                SetShowCtrol(VOICE_CLICK, 0);
                //KillbtnFocus(VOICE_CLICK, hwndDlg);
                break;
            case IDC_BTN_3D:
                SetShowCtrol(SET3D_CLICK, 0);
                //KillbtnFocus(SET3D_CLICK, hwndDlg);
                break;
            case IDC_BTN_SUBTITLE:
                SetShowCtrol(SUBTILE_CLICK, 0);
                //KillbtnFocus(SUBTILE_CLICK, hwndDlg);
                break;
            case IDC_BTN_VIDEO:
                SetShowCtrol(VIDEO1_CLICK, 0);
                //KillbtnFocus(VIDEO1_CLICK, hwndDlg);
                break;
            case IDC_RELA_BTN_ALL_SELECT:
                SendMessage(GetDlgItem(hwndDlg, IDC_RELA_CHECK_3DV), BM_SETCHECK, BST_CHECKED, 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_RELA_CHECK_MP4), BM_SETCHECK, BST_CHECKED, 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_RELA_CHECK_AVI), BM_SETCHECK, BST_CHECKED, 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_RELA_CHECK_RMVB), BM_SETCHECK, BST_CHECKED, 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_RELA_CHECK_VOB), BM_SETCHECK, BST_CHECKED, 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_RELA_CHECK_TS), BM_SETCHECK, BST_CHECKED, 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_RELA_CHECK_MKV), BM_SETCHECK, BST_CHECKED, 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_RELA_CHECK_WMV), BM_SETCHECK, BST_CHECKED, 0);
                break;
            case IDC_RELA_BTN_ALL_CANCEL:
                SendMessage(GetDlgItem(hwndDlg, IDC_RELA_CHECK_3DV), BM_SETCHECK, BST_UNCHECKED, 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_RELA_CHECK_MP4), BM_SETCHECK, BST_UNCHECKED, 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_RELA_CHECK_AVI), BM_SETCHECK, BST_UNCHECKED, 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_RELA_CHECK_RMVB), BM_SETCHECK, BST_UNCHECKED, 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_RELA_CHECK_VOB), BM_SETCHECK, BST_UNCHECKED, 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_RELA_CHECK_TS), BM_SETCHECK, BST_UNCHECKED, 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_RELA_CHECK_MKV), BM_SETCHECK, BST_UNCHECKED, 0);
                SendMessage(GetDlgItem(hwndDlg, IDC_RELA_CHECK_WMV), BM_SETCHECK, BST_UNCHECKED, 0);
                break;
            case IDC_BTN_ABOUT:
                SetShowCtrol(ABOUT_CLICK, 0);
                //KillbtnFocus(ABOUT_CLICK, hwndDlg);
                break;
            case IDC_BTN_RELATIONSHIP:
                SetShowCtrol(RELATION_CLICK, 0);
                //KillbtnFocus(RELATION_CLICK, hwndDlg);
                break;
            case IDC_BTN_VIDEO_TWO:
                saveOnShow.left_bri = GET_CONST("Luminance");
                saveOnShow.left_con = GET_CONST("Contrast");
                saveOnShow.left_sat = GET_CONST("Saturation");
                saveOnShow.left_hue = GET_CONST("Hue");
                saveOnShow.right_bri = GET_CONST("Luminance2");
                saveOnShow.right_con = GET_CONST("Contrast2");
                saveOnShow.right_sat = GET_CONST("Saturation2");
                saveOnShow.right_hue = GET_CONST("Hue2");
                SetShowCtrol(VIDEO2_CLICK, 0);
                break;
            case IDC_SUBTITLE_BTN_SELECT_COLOR:
                {
                    static COLORREF customColor[16];
                    customColor[0] = color;
                    CHOOSECOLOR cc = {sizeof(CHOOSECOLOR), hwndDlg, NULL, color, customColor, CC_RGBINIT, 0, NULL, NULL};
                    BOOL rtn = ChooseColor(&cc);
                    if (rtn)
                        color = cc.rgbResult;
                }
                break;
            case IDC_BTN_APPLY:
                {
                    ApplySetting(hwndDlg);
                }
                break;
            case IDC_BTN_OK:
                {
                    ApplySetting(hwndDlg);
                    EndDialog(hwndDlg, wParam);
                }
                break;
            case IDC_BTN_CANCEL:
				{
					GET_CONST("Luminance") = saveOnShow.left_bri;
					GET_CONST("Saturation") = saveOnShow.left_sat;
					GET_CONST("Contrast") = saveOnShow.left_con;
					GET_CONST("Hue") = saveOnShow.left_hue;
					GET_CONST("Luminance2") = saveOnShow.right_bri;
					GET_CONST("Saturation2") = saveOnShow.right_sat;
					GET_CONST("Contrast2") = saveOnShow.right_con;
					GET_CONST("Hue2") = saveOnShow.right_hue;
					EndDialog(hwndDlg, wParam);
				}
                break;
            case IDC_SUBTITLE_BTN_RESET:
                SetDlgItemTextW(hwndDlg, IDC_SUBTITLE_EDIT_LATE_SHOW, L"0");
                SetDlgItemTextW(hwndDlg, IDC_SUBTITLE_EDIT_LATE_SHOW_SIZE, L"100");
                break;
            case IDC_VIDEO_BTN_RESET:
                {
                    SendMessage(GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_LEFT_BRIGHTNESS), TBM_SETPOS, TRUE, 500);
                    SendMessage(GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_LEFT_CONTRAST), TBM_SETPOS, TRUE, 500);
                    SendMessage(GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_LEFT_SATURATION), TBM_SETPOS, TRUE, 500);
                    SendMessage(GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_LEFT_HUE), TBM_SETPOS, TRUE, 500);
                    SendMessage(GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_RIGHT_BRIGHTNESS), TBM_SETPOS, TRUE, 500);
                    SendMessage(GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_RIGHT_CONTRAST), TBM_SETPOS, TRUE, 500);
                    SendMessage(GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_RIGHT_SATURATION), TBM_SETPOS, TRUE, 500);
					SendMessage(GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_RIGHT_HUE), TBM_SETPOS, TRUE, 500);
					GET_CONST("Luminance") = 0.5;
					GET_CONST("Saturation") = 0.5;
					GET_CONST("Contrast") = 0.5;
					GET_CONST("Hue") = 0.5;
					GET_CONST("Luminance2") = 0.5;
					GET_CONST("Saturation2") = 0.5;
					GET_CONST("Contrast2") = 0.5;
					GET_CONST("Hue2") = 0.5;
                }
                break;
            case IDC_VIDEO_CHECK_ONSHOW:
                if (SendMessage(GetDlgItem(hwndDlg, IDC_VIDEO_CHECK_ONSHOW), BM_GETCHECK, 0, 0) == BST_UNCHECKED)
                {
                    g_OnShow = FALSE;
                    ApplySetting(hwndDlg);
                }
                else
                {
                    g_OnShow = TRUE;
                }
                break;
            }
        }
            break;
    }
    return FALSE;
}

int setting_dlg(HWND parent)
{
    DialogBoxA(NULL, MAKEINTRESOURCEA(IDD_DLG_MAIN), parent, SetProc);
	return 0;
}

void GetCtrolItemhWnd(HWND hwndDlg)
{
    Ordinary[0] = GetDlgItem(hwndDlg, IDC_ORD_STATIC_ONTOP);
    Ordinary[1] = GetDlgItem(hwndDlg, IDC_ORD_COM_TOPMOST);
//     Ordinary[1] = GetDlgItem(hwndDlg, IDC_ORD_RADIO_ALWAY);
//     Ordinary[2] = GetDlgItem(hwndDlg, IDC_ORD_RADIO_WHENPLAY);
//     Ordinary[3] = GetDlgItem(hwndDlg, IDC_ORD_RADIO_NEVER);
//     Ordinary[4] = GetDlgItem(hwndDlg, IDC_ORD_STATIC_CLOSE);
//     Ordinary[5] = GetDlgItem(hwndDlg, IDC_ORD_RADIO_ONTRAY);
//     Ordinary[6] = GetDlgItem(hwndDlg, IDC_ORD_RADIO_CLOSE);
//     Ordinary[7] = GetDlgItem(hwndDlg, IDC_ORD_STATIC_BOOT);
//     Ordinary[8] = GetDlgItem(hwndDlg, IDC_ORD_RADIO_BOOT);
    Ordinary[2] = GetDlgItem(hwndDlg, IDC_ORD_STATIC_LANGUAGE);
    Ordinary[3] = GetDlgItem(hwndDlg, IDC_ORD_COM_LANGUAGE);

    //声音页面控件信息
    Voice[0] = GetDlgItem(hwndDlg, IDC_VOICE_STATIC_CHANNEL);
    Voice[1] = GetDlgItem(hwndDlg, IDC_VOICE_COM_CHANNEL);
    Voice[2] = GetDlgItem(hwndDlg, IDC_VOICE_CHECK_AUTO_VOICE_DECODE);
    Voice[3] = GetDlgItem(hwndDlg, IDC_VOICE_COM_MAX_VOICE);



    //视频页面控件1信息
//     VideoPage1[0] = GetDlgItem(hwndDlg, IDC_VIDEO_STATIC_VIDEOLIST);
//     VideoPage1[1] = GetDlgItem(hwndDlg, IDC_VIDEO_RADIO_ALWAY_EXPEND);
//     VideoPage1[2] = GetDlgItem(hwndDlg, IDC_VIDEO_RADIO_AUTO_HIDE);
//     VideoPage1[3] = GetDlgItem(hwndDlg, IDC_VIDEO_RADIO_OPEN_SAME);
//     VideoPage1[4] = GetDlgItem(hwndDlg, IDC_VIDEO_RADIO_LOAD_FLODER);
    VideoPage1[0] = GetDlgItem(hwndDlg, IDC_VIDEO_STATIC_FULLMODE);
    VideoPage1[1] = GetDlgItem(hwndDlg, IDC_VIDEO_COM_FULLMODE);
    VideoPage1[2] = GetDlgItem(hwndDlg, IDC_VIDEO_STATIC_IMAGERATE);
    VideoPage1[3] = GetDlgItem(hwndDlg, IDC_VIDEO_COM_IMAGERATE);
    VideoPage1[4] = GetDlgItem(hwndDlg, IDC_VIDEO_CHECK_LOW_CPU);
    VideoPage1[5] = GetDlgItem(hwndDlg, IDC_VIDEO_CHECK_FULLSCREEN);
    VideoPage1[6] = GetDlgItem(hwndDlg, IDC_VIDEO_CHECK_VIDEO_2_WIN);
    VideoPage1[7] = GetDlgItem(hwndDlg, IDC_VIDEO_CHECK_LOAD_LAST_POS);
    VideoPage1[8] = GetDlgItem(hwndDlg, IDC_VIDEO_CHECK_HARD_SPEED);
    VideoPage1[9] = GetDlgItem(hwndDlg, IDC_VIDEO_CHECK_DEINTERLACE);
    //VideoPage1[10] = GetDlgItem(hwndDlg, IDC_VIDEO_BTN_PAGE_TWO);

    //视频页面控件2信息
    VideoPage2[0] = GetDlgItem(hwndDlg, IDC_VIDEO_GROUP_COLOR_CTROL);
    VideoPage2[1] = GetDlgItem(hwndDlg, IDC_VIDEO_RADIO_SAME_CONTROL);
    VideoPage2[2] = GetDlgItem(hwndDlg, IDC_VIDEO_BTN_RESET);
    VideoPage2[3] = GetDlgItem(hwndDlg, IDC_VIDEO_GROUP_COLOR_CTROL_LEFT);
    VideoPage2[4] = GetDlgItem(hwndDlg, IDC_VIDEO_GROUP_COLOR_CTROL_RIGHT);
    VideoPage2[5] = GetDlgItem(hwndDlg, IDC_VIDEO_STATIC_LEFT);
    VideoPage2[6] = GetDlgItem(hwndDlg, IDC_VIDEO_STATIC_RIGHT);
    VideoPage2[7] = GetDlgItem(hwndDlg, IDC_VIDEO_STATIC_LEFT_BRIGHTNESS);
    VideoPage2[8] = GetDlgItem(hwndDlg, IDC_VIDEO_STATIC_LEFT_CONTRAST);
    VideoPage2[9] = GetDlgItem(hwndDlg, IDC_VIDEO_STATIC_LEFT_SATURATION);
    VideoPage2[10] = GetDlgItem(hwndDlg, IDC_VIDEO_STATIC_LEFT_HUE);
    VideoPage2[11] = GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_LEFT_BRIGHTNESS);
    VideoPage2[12] = GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_LEFT_CONTRAST);
    VideoPage2[13] = GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_LEFT_SATURATION);
    VideoPage2[14] = GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_LEFT_HUE);
    VideoPage2[15] = GetDlgItem(hwndDlg, IDC_VIDEO_STATIC_RIGHT_BRIGHTNESS);
    VideoPage2[16] = GetDlgItem(hwndDlg, IDC_VIDEO_STATIC_RIGHT_CONTRAST);
    VideoPage2[17] = GetDlgItem(hwndDlg, IDC_VIDEO_STATIC_RIGHT_SATURATION);
    VideoPage2[18] = GetDlgItem(hwndDlg, IDC_VIDEO_STATIC_RIGHT_HUE);
    VideoPage2[19] = GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_RIGHT_BRIGHTNESS);
    VideoPage2[20] = GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_RIGHT_CONTRAST);
    VideoPage2[21] = GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_RIGHT_SATURATION);
    VideoPage2[22] = GetDlgItem(hwndDlg, IDC_VIDEO_SLIDER_RIGHT_HUE);
    VideoPage2[23] = GetDlgItem(hwndDlg, IDC_VIDEO_CHECK_ONSHOW);


    //3D页面控件信息
    Set3D[0] = GetDlgItem(hwndDlg, IDC_3D_CHECK_SHOW2D);
    Set3D[1] = GetDlgItem(hwndDlg, IDC_3D_CHECK_EXCHANGE);
    Set3D[2] = GetDlgItem(hwndDlg, IDC_3D_STATIC_INPUT_MODE);
    Set3D[3] = GetDlgItem(hwndDlg, IDC_3D_STATIC_OUTPUT_MODE);
    Set3D[4] = GetDlgItem(hwndDlg, IDC_3D_STATIC_FALL_MODE_ONE);
    Set3D[5] = GetDlgItem(hwndDlg, IDC_3D_COM_INPUT_MODE);
    Set3D[6] = GetDlgItem(hwndDlg, IDC_3D_COM_OUTPUT_MODE);
    Set3D[7] = GetDlgItem(hwndDlg, IDC_3D_COM_FALL_MODE_ONE);
    //Set3D[9] = GetDlgItem(hwndDlg, IDC_3D_COM_FALL_MODE_TWO);
    //Set3D[10] = GetDlgItem(hwndDlg, IDC_3D_SLIDER_INPUT_MODE);

    //字幕页面控件信息
    Subtitle[0] = GetDlgItem(hwndDlg, IDC_SUBTITLE_CHECK_SHOWSUB);
    Subtitle[1] = GetDlgItem(hwndDlg, IDC_SUBTITLE_STATIC_STYLE);
    Subtitle[2] = GetDlgItem(hwndDlg, IDC_SUBTITLE_STATIC_SIZE);
    Subtitle[3] = GetDlgItem(hwndDlg, IDC_SUBTITLE_STATIC_COLOR);
    Subtitle[4] = GetDlgItem(hwndDlg, IDC_SUBTITLE_GROUP_LATE);
    Subtitle[5] = GetDlgItem(hwndDlg, IDC_SUBTITLE_STATIC_LATE_SHOW);
    Subtitle[6] = GetDlgItem(hwndDlg, IDC_SUBTITLE_STATIC_LATE_SHOW_SIZE);
    Subtitle[7] = GetDlgItem(hwndDlg, IDC_SUBTITLE_EDIT_LATE_SHOW);
    Subtitle[8] = GetDlgItem(hwndDlg, IDC_SUBTITLE_EDIT_LATE_SHOW_SIZE);
    Subtitle[9] = GetDlgItem(hwndDlg, IDC_SUBTITLE_COM_STYLE);
    Subtitle[10] = GetDlgItem(hwndDlg, IDC_SUBTITLE_BTN_RESET);
    Subtitle[11] = GetDlgItem(hwndDlg, IDC_SUBTITLE_COM_SIZE);
    Subtitle[12] = GetDlgItem(hwndDlg, IDC_SUBTITLE_BTN_SELECT_COLOR);
    Subtitle[13] = GetDlgItem(hwndDlg, IDC_SUBTITLE_STATIC_LATE_SHOW_MINI_SEC);
    Subtitle[14] = GetDlgItem(hwndDlg, IDC_SUBTITLE_STATIC_LATE_SHOW_CODE);

    //热键页面控件信息
//     Hotkey[0] = GetDlgItem(hwndDlg, IDC_HOTKEY_STATIC_HOTKEY);
//     Hotkey[1] = GetDlgItem(hwndDlg, IDC_HOTKEY_STATIC_BACK_FIVE);
//     Hotkey[2] = GetDlgItem(hwndDlg, IDC_HOTKEY_STATIC_EXCHANGE);
//     Hotkey[3] = GetDlgItem(hwndDlg, IDC_HOTKEY_STATIC_STEP_FIVE);
//     Hotkey[4] = GetDlgItem(hwndDlg, IDC_HOTKEY_STATIC_SUB);
//     Hotkey[5] = GetDlgItem(hwndDlg, IDC_HOTKEY_STATIC_FULL_SCREEN_MODE);
//     Hotkey[6] = GetDlgItem(hwndDlg, IDC_HOTKEY_STATIC_ADD);
//     Hotkey[7] = GetDlgItem(hwndDlg, IDC_HOTKEY_STATIC_PAUSE);
//     Hotkey[8] = GetDlgItem(hwndDlg, IDC_HOTKEY_STATIC_MEANING);
//     Hotkey[9] = GetDlgItem(hwndDlg, IDC_HOTKEY_EDIT_BACK_FIVE);
//     Hotkey[10] = GetDlgItem(hwndDlg, IDC_HOTKEY_EDIT_EXCHANGE);
//     Hotkey[11] = GetDlgItem(hwndDlg, IDC_HOTKEY_EDIT_STEP_FIVE);
//     Hotkey[12] = GetDlgItem(hwndDlg, IDC_HOTKEY_EDIT_SUB);
//     Hotkey[13] = GetDlgItem(hwndDlg, IDC_HOTKEY_EDIT_FULL_SCREEN_MODE);
//     Hotkey[14] = GetDlgItem(hwndDlg, IDC_HOTKEY_EDIT_ADD);
//     Hotkey[15] = GetDlgItem(hwndDlg, IDC_HOTKEY_EDIT_PAUSE);
//     Hotkey[16] = GetDlgItem(hwndDlg, IDC_HOTKEY_GROUP_NULL);
//     Hotkey[17] = GetDlgItem(hwndDlg, IDC_HOTKEY_BTN_RESET);
//     Hotkey[18] = GetDlgItem(hwndDlg, IDC_HOTKEY_BTN_APPLY);

    //截图页面控件信息
//     PrtSrc[0] = GetDlgItem(hwndDlg, IDC_PRTSCR_STATIC_SAVE_PATH);
//     PrtSrc[1] = GetDlgItem(hwndDlg, IDC_PRTSCR_STATIC_SAVE_FORMAT);
//     PrtSrc[2] = GetDlgItem(hwndDlg, IDC_PRTSCR_EDIT_SAVE_PATH);
//     PrtSrc[3] = GetDlgItem(hwndDlg, IDC_PRTSCR_BTN_OPEN_PATH);
//     PrtSrc[4] = GetDlgItem(hwndDlg, IDC_PRTSCR_COM_SAVE_CONT);
//     PrtSrc[5] = GetDlgItem(hwndDlg, IDC_PRTSCR_COM_SAVE_FORMAT);

    //文件关联页面控件信息
    Relation[0] = GetDlgItem(hwndDlg, IDC_RELA_GROUP_FILE_RELA);
    Relation[1] = GetDlgItem(hwndDlg, IDC_RELA_CHECK_MP4);
    Relation[2] = GetDlgItem(hwndDlg, IDC_RELA_CHECK_AVI);
    Relation[3] = GetDlgItem(hwndDlg, IDC_RELA_CHECK_RMVB);
    Relation[4] = GetDlgItem(hwndDlg, IDC_RELA_CHECK_3DV);
    Relation[5] = GetDlgItem(hwndDlg, IDC_RELA_CHECK_TS);
    Relation[6] = GetDlgItem(hwndDlg, IDC_RELA_CHECK_MKV);
    Relation[7] = GetDlgItem(hwndDlg, IDC_RELA_CHECK_WMV);
    Relation[8] = GetDlgItem(hwndDlg, IDC_RELA_CHECK_VOB);
    Relation[9] = GetDlgItem(hwndDlg, IDC_RELA_BTN_ALL_SELECT);
    Relation[10] = GetDlgItem(hwndDlg, IDC_RELA_BTN_ALL_CANCEL);

    About[0] = GetDlgItem(hwndDlg, IDC_ABOUT_BACK);
}

void SetShowCtrol(int nItem, int nSubItem)
{
    for (int nIndex = 0; nIndex < 8; nIndex++)
    {
        //         printf("%d", sizeof(AllCtrl[nIndex]) / sizeof(HWND));
        for (int i = 0; i < 30; i++)
        {
            if (nItem == nIndex)
            {
                if ( AllCtrl[nIndex][i] != NULL && IsWindow(AllCtrl[nIndex][i]))
                    ShowWindow(AllCtrl[nIndex][i], SW_SHOW);
                else
                    break;
            }
            else
            {
                if ( AllCtrl[nIndex][i] != NULL && IsWindow(AllCtrl[nIndex][i]))
                    ShowWindow(AllCtrl[nIndex][i], SW_HIDE);
                else
                    break;
            }
        }
    }
}

void LoadSetting(HWND hWnd)
{
    bool bVal = false;
    int nVal = -1;
    wchar_t szVal[100] = {0};
    char szVal_s[100] = {0};
    double dVal = - 1.00000;

//     lua_const &l = GET_CONST("AudioChannel");
//     lua_const &ggg = GET_CONST("AudioChannel");
    //int n = sizeof(TestSet) / sizeof(tagLuaSetting);
    for (int i = 0; i < sizeof(TestSet) / sizeof(tagLuaSetting); i++)
    {
        if (TestSet[i].nCtrlType == TYPE_CHECK || TestSet[i].nCtrlType == TYPE_CHECK)
        {
            lua_const& test = GET_CONST(TestSet[i].arrSettingName);
            bVal = test;
            if (bVal)
                SendMessage(GetDlgItem(hWnd, TestSet[i].nCtrlID), BM_SETCHECK, BST_CHECKED, 0);
            else
                SendMessage(GetDlgItem(hWnd, TestSet[i].nCtrlID), BM_SETCHECK, BST_UNCHECKED, 0);
        }
        if (TestSet[i].nCtrlType == TYPE_SLIDER)
        {
            lua_const& slider = GET_CONST(TestSet[i].arrSettingName);
            dVal = slider;
            SendMessage(GetDlgItem(hWnd, TestSet[i].nCtrlID), TBM_SETPOS, TRUE, (WPARAM)(dVal * 1000));
        }
        //printf("%d\r\n", test);
    }
    
    lua_const& val0 = GET_CONST(TestSet[0].arrSettingName);
    nVal = val0;
    SendMessage(GetDlgItem(hWnd, TestSet[0].nCtrlID), CB_SETCURSEL, (WPARAM)nVal, 0);

    //imgae rate
    lua_const& val1 = GET_CONST(TestSet[1].arrSettingName);
    dVal = val1;
    int nSel = -1;
    if (abs(dVal - ( -1 )) < 0.000001)
        nSel = 0;
    else if (abs(dVal - 4 / 3) < 0.000001)
        nSel = 1;
    else if (abs (dVal - 16 / 9) < 0.000001)
        nSel = 2;
    else if (abs(dVal - 2.35) < 0.000001)
        nSel = 3;
    SendMessage(GetDlgItem(hWnd, TestSet[1].nCtrlID), CB_SETCURSEL, (WPARAM)nSel, 0);

    //input layout
    lua_const& val17 = GET_CONST(TestSet[17].arrSettingName);
    nVal = val17;
    if (nVal == 0x10000)
        SendMessage(GetDlgItem(hWnd, TestSet[17].nCtrlID), CB_SETCURSEL, (WPARAM)3, 0);
    else
        SendMessage(GetDlgItem(hWnd, TestSet[17].nCtrlID), CB_SETCURSEL, (WPARAM)nVal, 0);

    //output layout
    lua_const& val18 = GET_CONST(TestSet[18].arrSettingName);
    lua_const& val18Mask = GET_CONST("MaskMode");
    nVal = val18;
    switch (nVal)
    {
    case 0:
    case 2:
    case 3:
    case 4:
    case 5:
        nSel = nVal;
    	break;
    case 1:
        nVal = val18Mask;
        if (nVal == 0)
            nSel = 12;
        else if (nVal == 1)
            nSel = 10;
        else if (nVal == 2)
            nSel = 11;
        else if (nVal == 3)
            nSel = 1;
        break;
    case 9:
    case 10:
    case 11:
    case 12:
        nSel = nVal - 3;
        break;
    }
    SendMessage(GetDlgItem(hWnd, TestSet[18].nCtrlID), CB_SETCURSEL, (WPARAM)nSel, 0);

    lua_const& val21 = GET_CONST(TestSet[21].arrSettingName);
    nVal = val21;
    if (nVal == 0)
        nSel = 0;
    else if (nVal == 2)
        nSel = 1;
    else if (nVal == 6)
        nSel = 2;
    else if (nVal == 8)
        nSel = 3;
    else if (nVal == 1)
        nSel = 4;
    else if (nVal == -1)
        nSel = 5;
    SendMessage(GetDlgItem(hWnd, TestSet[21].nCtrlID), CB_SETCURSEL, (WPARAM)nSel, 0);



    lua_const& val25 = GET_CONST(TestSet[25].arrSettingName);
    wcscpy(szVal, val25);
    //WideCharToMultiByte(CP_ACP, 0, szVal, -1, szVal_s, 100, NULL, NULL);
    nSel = SendMessage(GetDlgItem(hWnd, TestSet[25].nCtrlID), CB_FINDSTRING, 0, (LPARAM)szVal);
    SendMessage(GetDlgItem(hWnd, TestSet[25].nCtrlID), CB_SETCURSEL, (WPARAM)nSel, 0);

    lua_const& val26 = GET_CONST(TestSet[26].arrSettingName);
    nVal = val26;
    wsprintf(szVal, L"%d", nVal);
    SendMessageW(GetDlgItem(hWnd, TestSet[26].nCtrlID), CB_SETCURSEL, SendMessageW(GetDlgItem(hWnd, TestSet[26].nCtrlID), CB_FINDSTRING, 0, (LPARAM)szVal), 0);

    lua_const& val27 = GET_CONST(TestSet[27].arrSettingName);
    color = val27;

    lua_const& val28 = GET_CONST(TestSet[28].arrSettingName);
    nVal = val28;
    wsprintf(szVal, L"%d", nVal);
    SetDlgItemText(hWnd, TestSet[28].nCtrlID, szVal);

    lua_const& val29 = GET_CONST(TestSet[29].arrSettingName);
    nVal = val29;
    wsprintf(szVal, L"%d", nVal * 100);
    SetDlgItemText(hWnd, TestSet[29].nCtrlID, szVal);

    lua_const& val24 = GET_CONST(TestSet[23].arrSettingName);
    nVal = val24;
    if (nVal == 16)
        SendMessage(GetDlgItem(hWnd, TestSet[23].nCtrlID), BM_SETCHECK, BST_CHECKED, 0);
    else if (nVal == 1)
        SendMessage(GetDlgItem(hWnd, TestSet[23].nCtrlID), BM_SETCHECK, BST_UNCHECKED, 0);

    lua_const& val31 = GET_CONST(TestSet[31].arrSettingName);
    nSel = val31;
    SendMessage(GetDlgItem(hWnd, TestSet[31].nCtrlID), CB_SETCURSEL, (WPARAM)nSel, 0);

	luaState L;
    lua_getglobal(L, "setting");
    lua_getfield(L, -1, "FileAssociation");
	if (lua_istable(L, -1))
	{
		lua_getfield(L, -1, "mp4");
		bool nbool = lua_toboolean(L, -1);
		assoc.mp4 = nbool;
		SendMessage(GetDlgItem(hWnd, IDC_RELA_CHECK_MP4), BM_SETCHECK, nbool ? BST_CHECKED : BST_UNCHECKED, 0);
		lua_pop(L,1);

		lua_getfield(L, -1, "avi");
		nbool = lua_toboolean(L, -1);
		assoc.avi = nbool;
		SendMessage(GetDlgItem(hWnd, IDC_RELA_CHECK_AVI), BM_SETCHECK, nbool ? BST_CHECKED : BST_UNCHECKED, 0);
		lua_pop(L,1);

		lua_getfield(L, -1, "rmvb");
		nbool = lua_toboolean(L, -1);
		assoc.rmvb = nbool;
		SendMessage(GetDlgItem(hWnd, IDC_RELA_CHECK_RMVB), BM_SETCHECK, nbool ? BST_CHECKED : BST_UNCHECKED, 0);
		lua_pop(L,1);

		lua_getfield(L, -1, "3dv");
		nbool = lua_toboolean(L, -1);
		assoc._3dv = nbool;
		SendMessage(GetDlgItem(hWnd, IDC_RELA_CHECK_3DV), BM_SETCHECK, nbool ? BST_CHECKED : BST_UNCHECKED, 0);
		lua_pop(L,1);

		lua_getfield(L, -1, "ts");
		nbool = lua_toboolean(L, -1);
		assoc.ts = nbool;
		SendMessage(GetDlgItem(hWnd, IDC_RELA_CHECK_TS), BM_SETCHECK, nbool ? BST_CHECKED : BST_UNCHECKED, 0);
		lua_pop(L,1);

		lua_getfield(L, -1, "mkv");
		nbool = lua_toboolean(L, -1);
		assoc.mkv = nbool;
		SendMessage(GetDlgItem(hWnd, IDC_RELA_CHECK_MKV), BM_SETCHECK, nbool ? BST_CHECKED : BST_UNCHECKED, 0);
		lua_pop(L,1);

		lua_getfield(L, -1, "wmv");
		nbool = lua_toboolean(L, -1);
		assoc.wmv = nbool;
		SendMessage(GetDlgItem(hWnd, IDC_RELA_CHECK_WMV), BM_SETCHECK, nbool ? BST_CHECKED : BST_UNCHECKED, 0);
		lua_pop(L,1);

		lua_getfield(L, -1, "vob");
		nbool = lua_toboolean(L, -1);
		assoc.vob = nbool;
		SendMessage(GetDlgItem(hWnd, IDC_RELA_CHECK_VOB), BM_SETCHECK, nbool ? BST_CHECKED : BST_UNCHECKED, 0);
		lua_pop(L,1);
	}
	else
	{
		
	}

	lua_settop(L, 0);
}

void ApplySetting(HWND hWnd)
{
    BOOL bRet = FALSE;
    double dVal;
    int nVal = -1;
    wchar_t szVal[100] = {0};
    wchar_t szwVal[100] = {0};

    for (int i = 0; i < sizeof(TestSet) / sizeof(tagLuaSetting); i++)
    {
        if (TestSet[i].nCtrlType == TYPE_CHECK || TestSet[i].nCtrlType == TYPE_RADIO)
        {
            int nRet = SendMessage(GetDlgItem(hWnd, TestSet[i].nCtrlID), BM_GETCHECK, 0, 0);
            if (nRet == BST_UNCHECKED)
                GET_CONST(TestSet[i].arrSettingName) = false;
            else if (nRet == BST_CHECKED)
                GET_CONST(TestSet[i].arrSettingName) = true;
        }
        if (TestSet[i].nCtrlType == TYPE_SLIDER)
        {
            dVal = (double)SendMessage(GetDlgItem(hWnd, TestSet[i].nCtrlID), TBM_GETPOS, 0, 0) / 1000.0;
            GET_CONST(TestSet[i].arrSettingName) = dVal;
        }
        if (TestSet[i].nCtrlType == TYPE_EDIT)
            if (TestSet[i].nCtrlID == IDC_SUBTITLE_EDIT_LATE_SHOW_SIZE)
                GET_CONST(TestSet[i].arrSettingName) = (double)GetDlgItemInt(hWnd, TestSet[i].nCtrlID, &bRet, FALSE) / 100;
            else
                GET_CONST(TestSet[i].arrSettingName) = (double)GetDlgItemInt(hWnd, TestSet[i].nCtrlID, &bRet, FALSE);
    }

	saveOnShow.left_bri = GET_CONST("Luminance");
	saveOnShow.left_con = GET_CONST("Contrast");
	saveOnShow.left_sat = GET_CONST("Saturation");
	saveOnShow.left_hue = GET_CONST("Hue");
	saveOnShow.right_bri = GET_CONST("Luminance2");
	saveOnShow.right_con = GET_CONST("Contrast2");
	saveOnShow.right_sat = GET_CONST("Saturation2");
	saveOnShow.right_hue = GET_CONST("Hue2");



    GET_CONST(TestSet[0].arrSettingName) = SendMessage(GetDlgItem(hWnd, TestSet[0].nCtrlID), CB_GETCURSEL, 0, 0);

    int nRet = SendMessage(GetDlgItem(hWnd, TestSet[1].nCtrlID), CB_GETCURSEL, 0, 0);
    if (nRet == 0 )
        dVal = -1;
    else if(nRet == 1)
        dVal = (double)4 / 3; 
    else if(nRet == 2)
        dVal = (double)16 / 9; 
    else if(nRet == 3)
        dVal = (double)2.35 / 1; 
    GET_CONST(TestSet[1].arrSettingName) = dVal;

    nRet = SendMessage(GetDlgItem(hWnd, TestSet[17].nCtrlID), CB_GETCURSEL, 0, 0);
    if (nRet == 3)
        GET_CONST(TestSet[17].arrSettingName) = 0x10000;
    else
        GET_CONST(TestSet[17].arrSettingName) = nRet;

    nRet = SendMessage(GetDlgItem(hWnd, TestSet[18].nCtrlID), CB_GETCURSEL, 0, 0);
    switch (nRet)
    {
    case 0:
    case 2:
    case 3:
    case 4:
    case 5:
        nVal = nRet;
    	break;
    case 6:
    case 7:
    case 8:
    case 9:
        nVal = nRet + 3;
        break;
    case 1:
        nVal = 1;
        GET_CONST("MaskMode") = 3;
        break;
    case 10:
        nVal = 1;
        GET_CONST("MaskMode") = 1;
        break;
    case 11:
        nVal = 1;
        GET_CONST("MaskMode") = 2;
        break;
    case 12:
        nVal = 1;
        GET_CONST("MaskMode") = 0;
        break;
    }
    GET_CONST(TestSet[18].arrSettingName) = nVal;

    nRet = SendMessage(GetDlgItem(hWnd, TestSet[21].nCtrlID), CB_GETCURSEL, 0, 0);
	int tbl[] = {0, 2, 6, 8, 1, -1};
	nVal = tbl[nRet];
    //GET_CONST(TestSet[21].arrSettingName) = nVal;
	g_player->set_output_channel(nVal);

    GetDlgItemTextW(hWnd, TestSet[25].nCtrlID, szVal, 100);
    //MultiByteToWideChar(CP_ACP, 0, szVal, -1, (LPWSTR)szwVal, 100);
    GET_CONST(TestSet[25].arrSettingName) = szVal;

    nVal = GetDlgItemInt(hWnd, TestSet[26].nCtrlID, &bRet, FALSE);
    GET_CONST(TestSet[26].arrSettingName) = nVal;

    GET_CONST(TestSet[27].arrSettingName) = color;

    nRet = SendMessage(GetDlgItem(hWnd, TestSet[23].nCtrlID), BM_GETCHECK, 0, 0);
    if (nRet == BST_UNCHECKED)
        nVal = 1;
    else if (nRet == BST_CHECKED)
        nVal = 16;
    GET_CONST(TestSet[23].arrSettingName) = nVal;

    nRet = SendMessage(GetDlgItem(hWnd, TestSet[31].nCtrlID), CB_GETCURSEL, 0, 0);
    GET_CONST(TestSet[31].arrSettingName) = nRet;

    tagRelation rela;
    nRet = SendMessage(GetDlgItem(hWnd, IDC_RELA_CHECK_MP4), BM_GETCHECK, 0, 0);
    if (nRet == BST_CHECKED)
        rela.mp4 = true;
    else if (nRet == BST_UNCHECKED)
        rela.mp4 = false;
    nRet = SendMessage(GetDlgItem(hWnd, IDC_RELA_CHECK_AVI), BM_GETCHECK, 0, 0);
    if (nRet == BST_CHECKED)
        rela.avi = true;
    else if (nRet == BST_UNCHECKED)
        rela.avi = false;
    nRet = SendMessage(GetDlgItem(hWnd, IDC_RELA_CHECK_RMVB), BM_GETCHECK, 0, 0);
    if (nRet == BST_CHECKED)
        rela.rmvb = true;
    else if (nRet == BST_UNCHECKED)
        rela.rmvb = false;
    nRet = SendMessage(GetDlgItem(hWnd, IDC_RELA_CHECK_3DV), BM_GETCHECK, 0, 0);
    if (nRet == BST_CHECKED)
        rela._3dv = true;
    else if (nRet == BST_UNCHECKED)
        rela._3dv = false;
    nRet = SendMessage(GetDlgItem(hWnd, IDC_RELA_CHECK_MKV), BM_GETCHECK, 0, 0);
    if (nRet == BST_CHECKED)
        rela.mkv = true;
    else if (nRet == BST_UNCHECKED)
        rela.mkv = false;
    nRet = SendMessage(GetDlgItem(hWnd, IDC_RELA_CHECK_TS), BM_GETCHECK, 0, 0);
    if (nRet == BST_CHECKED)
        rela.ts = true;
    else if (nRet == BST_UNCHECKED)
        rela.ts = false;
    nRet = SendMessage(GetDlgItem(hWnd, IDC_RELA_CHECK_WMV), BM_GETCHECK, 0, 0);
    if (nRet == BST_CHECKED)
        rela.wmv = true;
    else if (nRet == BST_UNCHECKED)
        rela.wmv = false;
    nRet = SendMessage(GetDlgItem(hWnd, IDC_RELA_CHECK_VOB), BM_GETCHECK, 0, 0);
    if (nRet == BST_CHECKED)
        rela.vob = true;
    else if (nRet == BST_UNCHECKED)
        rela.vob = false;


	luaState L;
	if (memcmp(&rela, &assoc, sizeof(rela)) != 0)
	{
		lua_getglobal(L, "setting");
		lua_newtable(L);
		lua_pushboolean(L, rela.mp4 ? 1 : 0);
		lua_setfield(L, -2, "mp4");
		lua_pushboolean(L, rela.avi ? 1 : 0);
		lua_setfield(L, -2, "avi");
		lua_pushboolean(L, rela.rmvb ? 1 : 0);
		lua_setfield(L, -2, "rmvb");
		lua_pushboolean(L, rela._3dv ? 1 : 0);
		lua_setfield(L, -2, "3dv");
		lua_pushboolean(L, rela.ts ? 1 : 0);
		lua_setfield(L, -2, "ts");
		lua_pushboolean(L, rela.mkv ? 1 : 0);
		lua_setfield(L, -2, "mkv");
		lua_pushboolean(L, rela.wmv ? 1 : 0);
		lua_setfield(L, -2, "wmv");
		lua_pushboolean(L, rela.vob ? 1 : 0);
		lua_setfield(L, -2, "vob");
		lua_setfield(L, -2, "FileAssociation");
		lua_pop(L, 1);

		update_file_association(true);
		assoc = rela;
	}

	// language
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "get_language_list");
	lua_mypcall(L, 0, 1, 0);
	int n = lua_objlen(L, -1);
	int lid = SendMessageW(GetDlgItem(hWnd, IDC_ORD_COM_LANGUAGE), CB_GETCURSEL, 0, 0);
	if (lid < n )
	{
		lua_pushinteger(L, lid+1);
		lua_gettable(L, -2);
		const char *language = lua_tostring(L, -1);
		lua_getglobal(L, "core");
		lua_getfield(L, -1, "set_language");
		lua_pushstring(L, language);
		lua_mypcall(L, 1, 0, 0);
	}

	lua_settop(L, 0);

	// monitor
	int displayid = SendMessageW(GetDlgItem(hWnd, IDC_3D_COM_FALL_MODE_ONE), CB_GETCURSEL, 0, 0);
	if (displayid != g_current_displayid)
		g_player->set_output_monitor(0, displayid);
	g_current_displayid = displayid;

	lua_getglobal(L, "apply_adv_settings");
	lua_mypcall(L, 0, 0, 0);
	lua_settop(L, 0);
}
