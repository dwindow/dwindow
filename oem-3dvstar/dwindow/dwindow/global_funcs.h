#pragma  once
#include <Windows.h>
#include <atlbase.h>
#include <d3d9.h>
#include <dshow.h>
#include "..\libchecksum\libchecksum.h"
#include "..\my12doom_revision.h"
#include <dxva.h>
#include <dxva2api.h>
#include <streams.h>
#include <evr.h>

// ffdshow
#include <streams.h>
#include "stdint.h"
#include "IffdshowDecAudio.h"
#include "IffdshowDecVideo.h"
#include "IffdshowBase.h"
#include "ffdshow_constants.h"

// dwindow version
#ifdef nologin
#define dwindow_free
#else
#ifdef no_dual_projector
#define dwindow_jz
#else
#define dwindow_pro
#endif
#endif


// setting class
class AutoSettingString;

// register window proc
INT_PTR CALLBACK register_proc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );


// public variables
extern AutoSettingString g_bar_server;
extern char g_passkey_big[128];
extern char g_passkey[32];
extern DWORD g_last_bar_time;
extern char *g_server_address;
#define HEARTBEAT_TIMEOUT 120000
#define g_server_E3D "w32.php"
#define g_server_gen_key "gen_key.php"
#ifdef VSTAR
#define g_server_update "op=updateCheck"		// VSTAR update
#define g_server_counter "op=launcher"		// normal update
#else
#define g_server_update "update.php"		// normal update
#endif
#define g_server_free "free.php"
#define g_server_reg_check "reg_check.php"
#define g_server_ad "ad.php"
extern int g_logic_monitor_count;
extern RECT g_logic_monitor_rects[16];
extern int g_phy_monitor_count;
extern HMONITOR g_phy_monitors[16];
extern D3DADAPTER_IDENTIFIER9 g_phy_ids[16];
extern D3DADAPTER_IDENTIFIER9 g_logic_ids[16];
extern HMONITOR g_logic_monitors[16];
extern CCritSec g_ILLock;


//definitions
#define AmHresultFromWin32(x) (MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, x))
#if 0
#define TRAIL_TIME_1 17537
#else
#define TRAIL_TIME_1 597537
#endif
#define TRAIL_TIME_2 923579
#define TRAIL_TIME_3 28532

// GUIDs
DEFINE_GUID(CLSID_WiDiExtensions,0x7DC2B7AA,0xBCFD,0x44D2,0xBD,0x58,0xE8,0xBD,0x0D,0x2E,0x3A,0xCC);
DEFINE_GUID(CLSID_HaaliSimple, 0x8F43B7D9, 0x9D6B, 0x4F48, 0xBE, 0x18, 0x4D, 0x78, 0x7C, 0x79, 0x5E, 0xEA);
DEFINE_GUID(CLSID_E3DSource_TS, 0x0A68C3B5, 0x9164, 0x4A54, 0xAF, 0xAF, 0x99, 0x5B, 0x2F, 0xF0, 0xE0, 0xD4);
DEFINE_GUID(CLSID_E3DSource , 0x7de55957, 0xd7d6, 0x4386, 0x90, 0x78, 0x0f, 0xd2, 0xba, 0x96, 0xf4, 0x59);
DEFINE_GUID(CLSID_PD10_DEMUXER, 0xF07E981B, 0x0EC4, 0x4665, 0xA6, 0x71, 0xC2, 0x49, 0x55, 0xD1, 0x1A, 0x38);
DEFINE_GUID(CLSID_PD10_DECODER, 0xD00E73D7, 0x06f5, 0x44F9, 0x8B, 0xE4, 0xB7, 0xDB, 0x19, 0x1E, 0x9E, 0x7E);
DEFINE_GUID(CLSID_3dvSource, 0xfbcfd6af, 0xb25f, 0x4a6d, 0xae, 0x56, 0x5b, 0x53, 0x3, 0xf1, 0xa4, 0xe);
DEFINE_GUID(CLSID_AVSource, 0xD8980E15, 0xE1F6, 0x4916, 0xA1, 0x0F, 0xD7, 0xEB, 0x4E, 0x9E, 0x10, 0xB8);
DEFINE_GUID(CLSID_ffdshowDXVA, 0x0B0EFF97, 0xC750, 0x462C, 0x94, 0x88, 0xb1, 0x0e, 0x7d, 0x87, 0xf1, 0xa6);
static const GUID CLSID_SSIFSource = { 0x916e4c8d, 0xe37f, 0x4fd4, { 0x95, 0xf6, 0xa4, 0x4e, 0x51, 0x46, 0x2e, 0xdf } };
static const GUID CLSID_my12doomSource = { 0x8FD7B1DE, 0x3B84, 0x4817, { 0xA9, 0x6F, 0x4C, 0x94, 0x72, 0x8B, 0x1A, 0xAE } };
DEFINE_GUID(CLSID_CoreAVC, 0x09571A4B, 0xF1FE, 0x4C60, 0x97, 0x60, 0xDE, 0x6D, 0x31, 0x0C, 0x7C, 0x31);
DEFINE_GUID(CLSID_LAVAudio, 0xe8e73b6b, 0x4cb3, 0x44a4, 0xbe, 0x99, 0x4f, 0x7b, 0xcb, 0x96, 0xe4, 0x91);
DEFINE_GUID(CLSID_XvidDecoder, 0x64697678, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);
DEFINE_GUID(CLSID_DivxDecoder, 0x78766964, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);
DEFINE_GUID(CLSID_RMVideoDecoder, 0x238D0F23, 0x5DC9, 0x45A6, 0x9B, 0xE2, 0x66, 0x61, 0x60, 0xC3, 0x24, 0xDD);
DEFINE_GUID(CLSID_RMAudioDecoder, 0x941A4793, 0xA705, 0x4312, 0x8D, 0xFC, 0xC1, 0x1C, 0xA0, 0x5F, 0x39, 0x7E);
DEFINE_GUID(CLSID_MC_J2KDecoder, 0x3BA3A89F, 0x4A48, 0x4E18, 0xA9, 0xA6, 0x01, 0x55, 0xBC, 0xB1, 0xA1, 0xA2);
DEFINE_GUID(CLSID_MXFReader, 0x9622477D, 0xE65F, 0x4A5F, 0xBD, 0x89, 0xAD, 0x75, 0x06, 0x3D, 0xDA, 0xDD);
DEFINE_GUID(MEDIATYPE_Subtitle, 0xe487eb08, 0x6b26, 0x4be9, 0x9d, 0xd3, 0x99, 0x34, 0x34, 0xd3, 0x13, 0xfd);
DEFINE_GUID(MEDIASUBTYPE_UTF8, 0x87c0b230, 0x3a8, 0x4fdf, 0x80, 0x10, 0xb2, 0x7a, 0x58, 0x48, 0x20, 0xd);
DEFINE_GUID(MEDIASUBTYPE_PGS, 0x4eba53e, 0x9330, 0x436c, 0x91, 0x33, 0x55, 0x3e, 0xc8, 0x70, 0x31, 0xdc);
DEFINE_GUID(MEDIASUBTYPE_ASS, 0x326444f7, 0x686f, 0x47ff, 0xa4, 0xb2, 0xc8, 0xc9, 0x63, 0x7, 0xb4, 0xc2);
DEFINE_GUID(MEDIASUBTYPE_ASS2, 0x370689e7, 0xb226, 0x4f67, 0x97, 0x8d, 0xf1, 0xb, 0xc1, 0xa9, 0xc6, 0xae);
DEFINE_GUID(MEDIASUBTYPE_VOBSUB, 0xf7239e31, 0x9599, 0x4e43, 0x8d, 0xd5, 0xfb, 0xaf, 0x75, 0xcf, 0x37, 0xf1);
DEFINE_GUID(MEDIASUBTYPE_HDMVSUB, 0x4eba53e, 0x9330, 0x436c, 0x91, 0x33, 0x55, 0x3e, 0xc8, 0x70, 0x31, 0xdc);

// funcs
extern wchar_t g_apppath[MAX_PATH];
HRESULT get_monitors_rect(RECT *screen1, RECT *screen2);
bool open_file_dlg(wchar_t *pathname, HWND hDlg, wchar_t *filter = NULL);
bool select_color(DWORD *color, HWND parent);
bool browse_folder(wchar_t *out, HWND owner = NULL);
HRESULT RemoveDownstream(CComPtr<IPin> &input_pin);
HRESULT set_lav_audio_bitstreaming(IBaseFilter *filter, bool active);
HRESULT set_ff_audio_bitstreaming(IBaseFilter *filter, bool active);
HRESULT set_ff_audio_formats(IBaseFilter *filter);
HRESULT set_ff_video_formats(IBaseFilter *filter);
HRESULT set_ff_output_channel(IBaseFilter *filter, int channel);		// channel=-1: no downmixing, channel=9 : headphone, channel=10 : HRTF, // channel=-1: bitstreaming
HRESULT set_ff_audio_normalizing(IBaseFilter *filter, double max_ratio);	// setting a ratio less than 1.0 cause normalizing off, more than 1.0 cause normalizing on
HRESULT find_main_movie(const wchar_t *folder, wchar_t *out);
HRESULT GetUnconnectedPin(IBaseFilter *pFilter,PIN_DIRECTION PinDir, IPin **ppPin);
HRESULT GetConnectedPin(IBaseFilter *pFilter,PIN_DIRECTION PinDir, IPin **ppPin);
HRESULT GetFilterFriedlyName(IBaseFilter *filter, wchar_t *out, int outlen);
HRESULT RemoveUselessFilters(IGraphBuilder *gb);
HRESULT DeterminPin(IPin *pin, wchar_t *name = NULL, CLSID majortype = CLSID_NULL, CLSID subtype = CLSID_NULL);
HRESULT GetPinByName(IBaseFilter *pFilter, PIN_DIRECTION PinDir, const wchar_t *name, IPin **ppPin);
HRESULT load_passkey();
HRESULT save_passkey();
HRESULT save_e3d_key(const unsigned char *file_hash, const unsigned char *file_key);
HRESULT load_e3d_key(const unsigned char *file_hash, unsigned char *file_key);
HRESULT download_e3d_key(const wchar_t *filename);
HRESULT make_xvid_support_mp4v();
HRESULT make_av_splitter_support_my_formats();
HRESULT config_mainconcept_JPEG200_decoder();
HRESULT config_MXFReader();
HRESULT download_url(const char *url_to_download, char *out, int *outlen, int timeout=INFINITE);
HRESULT bar_logout();
DWORD WINAPI killer_thread(LPVOID time);
DWORD WINAPI killer_thread2(LPVOID time);
DWORD WINAPI ad_thread(LPVOID time);
HRESULT detect_monitors();
int wcsexplode(const wchar_t *string_to_explode, const wchar_t *delimeter, wchar_t **out, int max_part = 0xfffffff);			// caller should alloc a wchar_t *out[max_part] and free every element of out;
int wcstrim(wchar_t *str, wchar_t char_ = L' ' );
int get_mixed_monitor_count(bool horizontal = false, bool vertical = false);
int get_mixed_monitor_by_id(int id, RECT *rect, wchar_t *descriptor, bool horizontal = false, bool vertical = false);
HRESULT myDXVA2CreateDirect3DDeviceManager9(UINT* pResetToken, IDirect3DDeviceManager9** ppDeviceManager);
HRESULT myDXVA2CreateVideoService(IDirect3DDevice9* pDD, REFIID riid, void** ppService);
HRESULT myMFGetService(IUnknown* punkObject, REFGUID guidService, REFIID riid, __out LPVOID* ppvObject);
HRESULT myMFCreateVideoSampleFromSurface(IUnknown* pUnkSurface,IMFSample** ppSample);
HRESULT myMFFrameRateToAverageTimePerFrame(UINT32 unNumerator, UINT32 unDenominator, UINT64* punAverageTimePerFrame);
DWORD color_GDI2ARGB(DWORD in);
BOOL myGetGestureInfo(HGESTUREINFO hGestureInfo, PGESTUREINFO pGestureInfo);
BOOL hasGetGustureInfo();
HRESULT CrackPD10(IBaseFilter *filter);
bool jpeg_enc_yv12(unsigned char* Ybuffer, unsigned char *Ubuffer, unsigned char*Vbuffer, int width, int height, int Ystride, int UVstride, int quality, const wchar_t* filename);
DWORD shellexecute_and_wait(const wchar_t *file, const wchar_t *parameter);
HRESULT restart_this_program();
HRESULT report_file(const wchar_t *filepath);
HRESULT get_splayer_subtitle(const wchar_t *filepath, wchar_t *out, const wchar_t **langs = NULL);
RECT get_special_size_physical_monitor(SIZE size);
HRESULT update_file_association(bool uac = false);
int  RegisterFileAssociation(const wchar_t *strExt, const wchar_t *strAppName, const wchar_t *strAppKey, const wchar_t *strDefaultIcon, const wchar_t *strDescribe);
int UnregisterFileAssociation(const wchar_t *strExt);

inline bool compare_rect(const RECT in1, const RECT in2)
{
	return memcmp(&in1, &in2, sizeof(RECT)) == 0;
}
inline void NormalizeRect(RECT *rect)
{
	RECT normal = {min(rect->left, rect->right), min(rect->top, rect->bottom), max(rect->left, rect->right), max(rect->top, rect->bottom)};
	*rect = normal;
}

// CoreMVC
HRESULT ActiveCoreMVC(IBaseFilter *decoder);
HRESULT beforeCreateCoreMVC();
HRESULT afterCreateCoreMVC();
class coremvc_hooker
{
public:
	coremvc_hooker();
	~coremvc_hooker();
};

// Time
__int64 mytime(bool reset = false);


// Settings loader & saver
bool save_setting(const WCHAR *key, const void *data, int len, DWORD REG_TYPE=REG_BINARY);
bool save_D3D_setting(const WCHAR *key, const void *data, int len, DWORD REG_TYPE=REG_BINARY);
int load_setting(const WCHAR *key, void *data, int len);
int load_D3D_setting(const WCHAR *key, void *data, int len);
bool del_setting(const WCHAR *key);


class W2UTF8
{
public:
	W2UTF8(const wchar_t *in);
	~W2UTF8();
	operator char*();
	char *p;
};

class UTF82W
{
public:
	UTF82W(UTF82W &o);
	UTF82W(const char *in);
	~UTF82W();
	operator wchar_t*();
	wchar_t *p;
};

#include "..\lua\my12doom_lua.h"

template<class ValueType>
class AutoSetting
{
public:
	AutoSetting(const wchar_t *key, const ValueType default_value, DWORD reg_type = REG_BINARY)
	{
		wcscpy(m_key, key);
		m_value = default_value;
		m_reg_type = reg_type;
		load_setting(key, &m_value, sizeof(ValueType));
		save_setting(m_key, &m_value, sizeof(ValueType), m_reg_type);

		if (g_lua_setting_manager)
		{
			g_lua_setting_manager->get_const(W2UTF8(key)) = default_value;
		}
		else
		{
			wprintf(L"lua failed for %s\n", key);
		}
	}
	~AutoSetting()
	{
		save_setting(m_key, &m_value, sizeof(ValueType), m_reg_type);
	}
	operator ValueType()
	{
		return m_value;
	}
	ValueType* operator& ()
	{
		return &m_value;
	}
	ValueType& operator= (ValueType in)
	{
		if (memcmp(&in, &m_value, sizeof(in)))
		{
			m_value = in;
			save_setting(m_key, &m_value, sizeof(ValueType), m_reg_type);
		}
		return m_value;
	}

protected:
	DWORD m_reg_type;
	wchar_t m_key[256];
	ValueType m_value;
};

template<class ValueType>
class AutoSettingO
{
public:
	AutoSettingO(const wchar_t *key, const ValueType default_value, DWORD reg_type = REG_BINARY)
	{
		wcscpy(m_key, key);
		m_value = default_value;
		m_reg_type = reg_type;
		load_setting(key, &m_value, sizeof(ValueType));
		save_setting(m_key, &m_value, sizeof(ValueType), m_reg_type);
	}
	~AutoSettingO()
	{
		save_setting(m_key, &m_value, sizeof(ValueType), m_reg_type);
	}
	operator ValueType()
	{
		return m_value;
	}
	ValueType* operator& ()
	{
		return &m_value;
	}
	ValueType& operator= (ValueType in)
	{
		if (memcmp(&in, &m_value, sizeof(in)))
		{
			m_value = in;
			save_setting(m_key, &m_value, sizeof(ValueType), m_reg_type);
		}
		return m_value;
	}

protected:
	DWORD m_reg_type;
	wchar_t m_key[256];
	ValueType m_value;
};



// localization
enum{ENGLISH, CHINESE};
typedef DWORD localization_language;
extern AutoSetting<int> g_active_language;
UTF82W C(const wchar_t *English);
HRESULT localize_menu(HMENU menu);
HRESULT localize_window(HWND hwnd);
localization_language get_system_default_lang();

// forceinline functions

const DWORD trial_m1[8] = {0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff};
const DWORD trial_m2[8] = {0};

bool __forceinline is_theeater_version()
{
	DWORD e[32];
	dwindow_passkey_big m1;
	BigNumberSetEqualdw(e, 65537, 32);
	RSA((DWORD*)&m1, (DWORD*)&g_passkey_big, e, (DWORD*)dwindow_n, 32);
	if (memcmp(m1.passkey, trial_m1, 32) == 0 || memcmp(m1.passkey2, trial_m2, 32) == 0)
		return false;
	for(int i=0; i<32; i++)
		if (m1.passkey[i] != m1.passkey2[i])
			return false;

	__time64_t t = mytime();

	tm * t2 = _localtime64(&m1.time_end);

	if (m1.time_start > mytime() || mytime() > m1.time_end)
	{
		memset(&m1, 0, 128);
		return false;
	}

	memcpy(g_passkey, &m1, 32);
	bool is = (USERTYPE_THEATER == m1.usertype);
	memset(&m1, 0, 128);
	return is;
}

bool __forceinline is_bar_version()
{
	DWORD e[32];
	dwindow_passkey_big m1;
	BigNumberSetEqualdw(e, 65537, 32);
	RSA((DWORD*)&m1, (DWORD*)&g_passkey_big, e, (DWORD*)dwindow_n, 32);
	if (memcmp(m1.passkey, trial_m1, 32) != 0 || memcmp(m1.passkey2, trial_m2, 32) != 0)
		return false;
	for(int i=0; i<32; i++)
		if (m1.passkey[i] != m1.passkey2[i])
			return false;

	__time64_t t = mytime();

	tm * t2 = _localtime64(&m1.time_end);

	if (m1.time_start > mytime() || mytime() > m1.time_end)
	{
		memset(&m1, 0, 128);
		return false;
	}

	memcpy(g_passkey, &m1, 32);
	bool is = (USERTYPE_BAR == m1.usertype);
	memset(&m1, 0, 128);
	return is;
}

__forceinline bool is_payed_version()
{
	DWORD e[32];
	dwindow_passkey_big m1;
	BigNumberSetEqualdw(e, 65537, 32);
	RSA((DWORD*)&m1, (DWORD*)&g_passkey_big, e, (DWORD*)dwindow_n, 32);

	if (memcmp(m1.passkey, trial_m1, 32) == 0 && memcmp(m1.passkey2, trial_m2, 32) == 0)
		return false;

	for(int i=0; i<32; i++)
		if (m1.passkey[i] != m1.passkey2[i])
			return false;

	__time64_t t = mytime();

	tm * t2 = _localtime64(&m1.time_end);

	if (m1.time_start > mytime() || mytime() > m1.time_end)
	{
		memset(&m1, 0, 128);
		return false;
	}

	memcpy(g_passkey, &m1, 32);
	memset(&m1, 0, 128);
	return true;
}

__forceinline bool is_pro_version()
{
	DWORD e[32];
	dwindow_passkey_big m1;
	BigNumberSetEqualdw(e, 65537, 32);
	RSA((DWORD*)&m1, (DWORD*)&g_passkey_big, e, (DWORD*)dwindow_n, 32);

	if (memcmp(m1.passkey, trial_m1, 32) == 0 && memcmp(m1.passkey2, trial_m2, 32) == 0)
		return false;

	for(int i=0; i<32; i++)
		if (m1.passkey[i] != m1.passkey2[i])
			return false;

	__time64_t t = mytime();

	tm * t2 = _localtime64(&m1.time_end);

	if (m1.time_start > mytime() || mytime() > m1.time_end)
	{
		memset(&m1, 0, 128);
		return false;
	}

	memcpy(g_passkey, &m1, 32);
	bool rtn = m1.usertype == USERTYPE_NORMAL || m1.usertype == USERTYPE_VSTAR;
	memset(&m1, 0, 128);
	return rtn;
}


__forceinline bool is_actived()
{
	DWORD e[32];
	dwindow_passkey_big m1;
	BigNumberSetEqualdw(e, 65537, 32);
	RSA((DWORD*)&m1, (DWORD*)&g_passkey_big, e, (DWORD*)dwindow_n, 32);

	if (memcmp(m1.passkey, trial_m1, 32) != 0 && memcmp(m1.passkey2, trial_m2, 32) != 0)
	for(int i=0; i<32; i++)
		if (m1.passkey[i] != m1.passkey2[i])
			return false;

	__time64_t t = mytime();

	tm * t2 = _localtime64(&m1.time_end);

	if (m1.time_start > mytime() || mytime() > m1.time_end)
	{
		memset(&m1, 0, 128);
		return false;
	}

	memcpy(g_passkey, &m1, 32);
	memset(&m1, 0, 128);
	return true;
}

__forceinline DWORD get_passkey_rights()
{
	DWORD e[32];
	dwindow_passkey_big m1;
	BigNumberSetEqualdw(e, 65537, 32);
	RSA((DWORD*)&m1, (DWORD*)&g_passkey_big, e, (DWORD*)dwindow_n, 32);

	if (memcmp(m1.passkey, trial_m1, 32) != 0 && memcmp(m1.passkey2, trial_m2, 32) != 0)
		for(int i=0; i<32; i++)
			if (m1.passkey[i] != m1.passkey2[i])
				return false;

	__time64_t t = mytime();

	tm * t2 = _localtime64(&m1.time_end);

	if (m1.time_start > mytime() || mytime() > m1.time_end)
	{
		memset(&m1, 0, 128);
		return false;
	}

	memcpy(g_passkey, &m1, 32);
	DWORD rtn = m1.user_rights;
	memset(&m1, 0, 128);
	return rtn;
}


__forceinline bool  is_trial_version()
{
#ifdef nologin
	return true;
#else
	return false;
#endif
}

//const LARGE_INTEGER freq;
//const BOOL helper = QueryPerformanceFrequency((LARGE_INTEGER*)&freq);

__forceinline HRESULT check_passkey()
{
	if (!is_actived())
		return E_FAIL;

	if (is_bar_version())
		return S_OK;

	if (is_theeater_version())
		return S_OK;

	if (is_trial_version())
		return S_OK;

#ifdef dwindow_free
	return S_OK;
#endif

#ifdef dwindow_jz
	if (is_payed_version())
		return S_OK;
#endif

#ifdef dwindow_pro
	if (is_pro_version())
		return S_OK;
#endif


	return E_FAIL;

}
__forceinline void BasicRsaCheck()
{	
	mytime();
	static __int64 delta = timeGetTime()/1000 - _time64(NULL);
	static int delta2 = timeGetTime() - GetTickCount();
	if (abs((int)(timeGetTime()/1000- _time64(NULL)-delta)) > 16
		|| abs((int)(timeGetTime() - GetTickCount() - delta2)) > 16000)
	{
		memset(g_passkey, 0, 32);
		memset(g_passkey_big, 0, 32);
		del_setting(L"passkey");
	}

	if (FAILED(check_passkey()))
	{
		memset(g_passkey, 0, 32);
		memset(g_passkey_big, 0, 32);
		del_setting(L"passkey");
		TerminateProcess(GetCurrentProcess(), 0);
	}
}


class AutoSettingString
{
public:
	void save()
	{
		save_setting(m_key, m_value, 20480, REG_SZ);
	}
	AutoSettingString(const wchar_t*key, const wchar_t *default_value)
	{
		if (g_lua_setting_manager)
		{
			g_lua_setting_manager->get_const(W2UTF8(key)) = default_value;
		}

		wcscpy(m_key, key);
		m_value = new wchar_t[20480];
		wcscpy(m_value, default_value);
		load_setting(m_key, m_value, 20480);
		save_setting(m_key, m_value, 20480, REG_SZ);
	}
	~AutoSettingString()
	{
		save();
		delete m_value;
	}
	operator wchar_t*()
	{
		return m_value;
	}
	wchar_t* operator=(const wchar_t*in)
	{
		if (wcscmp(in, m_value))
		{
			wcscpy(m_value, in);
			save_setting(m_key, m_value, 20480, REG_SZ);
		}
		return m_value;
	}
protected:
	wchar_t m_key[256];
	wchar_t *m_value;
};