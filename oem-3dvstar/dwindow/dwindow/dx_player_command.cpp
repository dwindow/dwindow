#include "dx_player.h"
#include "..\renderer_prototype\YV12_to_RGB32.h"

// helper functions
int wcscmp_nocase(const wchar_t*in1, const wchar_t *in2)
{
	wchar_t *tmp = new wchar_t[wcslen(in1)+1];
	wchar_t *tmp2 = new wchar_t[wcslen(in2)+1];
	wcscpy(tmp, in1);
	wcscpy(tmp2, in2);
	_wcslwr(tmp);
	_wcslwr(tmp2);

	int out = wcscmp(tmp, tmp2);

	delete [] tmp;
	delete [] tmp2;
	return out;
}

wchar_t * wcscpy2(wchar_t *out, const wchar_t *in)
{
	if (!out || !in)
		return NULL;
	return wcscpy(out, in);
}
wchar_t * wcscat2(wchar_t *out, const wchar_t *in)
{
	if (!out || !in)
		return NULL;
	return wcscat(out, in);
}


// helper classes
static const wchar_t myTRUE[] = L"True";
static const wchar_t myFALSE[] = L"False";
class myBool
{
public:
	myBool(const wchar_t *str, bool _default=false)
	{
		if (str == NULL)
			m_value = _default;
		else
			m_value = wcscmp_nocase(str, myTRUE) == 0;
	}
	myBool(const bool b)
	{
		m_value = b;
	}

	operator bool()
	{
		return m_value;
	}

	operator const wchar_t*()
	{
		return m_value ? myTRUE : myFALSE;
	}
protected:
	bool m_value;
};


class myInt
{
public:
	myInt(const wchar_t *str, int _default = 0)
	{
		if (str == NULL)
			m_value = _default;
		else
			m_value = _wtoi(str);
	}
	myInt(const int b)
	{
		m_value = b;
	}

	operator int()
	{
		return m_value;
	}

	operator const wchar_t*()
	{
		swprintf(m_str, L"%d", m_value);
		return m_str;
	}
protected:
	int m_value;
	wchar_t m_str[200];
};

class myDouble
{
public:
	myDouble(const wchar_t *str, double _default=0)
	{
		if (str == NULL)
			m_value = _default;
		else
			m_value = _wtof(str);
	}
	myDouble(const double b)
	{
		m_value = b;
	}

	operator double()
	{
		return m_value;
	}

	operator const wchar_t*()
	{
		swprintf(m_str, L"%f", m_value);
		return m_str;
	}
protected:
	double m_value;
	wchar_t m_str[200];
};

// the code
extern ICommandReciever *command_reciever;
HRESULT dx_player::execute_command_adv(wchar_t *command, wchar_t *out, const wchar_t **args, int args_count)
{
	const wchar_t *p;
	#define SWTICH(x) p=x;if(false);
	#define CASE(x) else if (wcscmp_nocase(p, x) == 0)
	#define DEFAULT else

	HRESULT hr = S_OK;

	SWTICH(command)

	CASE(L"forward")
	{
		int time;
		tell(&time);

		seek(max(0, time + (int)myInt(args[0], 5000)));

		hr = S_OK;
	}

	CASE(L"backward")
	{
		int time;
		tell(&time);

		hr = seek(max(0, time - (int)myInt(args[0], 5000)));
	}

	CASE(L"shot")
	{
		int l = timeGetTime();
		wchar_t tmpPath[MAX_PATH];
		GetTempPathW(MAX_PATH, tmpPath);
		wchar_t tmpFile[MAX_PATH];
		GetTempFileNameW(tmpPath, L"DWindow", 0, tmpFile);
		m_renderer1->screenshot(tmpFile);

		FILE *f = _wfopen(tmpFile, L"rb");
		if (!f)
			return E_FAIL;
		fseek(f, 0, SEEK_END);
		int size = ftell(f);
		fseek(f, 0, SEEK_SET);
		*((int*)out) = size;
		memset(out+2, 0, size);
		int r = fread(out+2, 1, size, f);
		fclose(f);
		DeleteFileW(tmpFile);

		char tmp[200];
		sprintf(tmp, "shot take %dms\n", GetTickCount()-l);
		OutputDebugStringA(tmp);
		return S_RAWDATA;
	}

	CASE(L"rawshot")
	{
		myInt width(args[0], 640);
		myInt height(args[1], 480);
		
		*((int*)out) = width*height*3/2;
		char *Y = ((char*)out)+4;
		char *V = Y+width*height;
		char *U = V+width/2*height/2;
		HRESULT hr = m_renderer1->screenshot(Y, U, V, width, width, height);

		return S_RAWDATA;
	}

	CASE(L"set_mask_parameter")
	{
		hr = m_renderer1->set_mask_parameter(myInt(args[0], m_renderer1->get_mask_parameter()));
	}
	CASE(L"get_mask_parameter")
	{
		int o = m_renderer1->get_mask_parameter();
		wcscpy2(out, myInt(o));
		hr = S_OK;
	}

	CASE(L"reset")
		hr = reset();

	CASE(L"start_loading")
		hr = start_loading();

	CASE(L"reset_and_loadfile")
		hr = reset_and_loadfile(args[0], args[1], myBool(args[2]));

	CASE(L"load_subtitle")
		hr = load_subtitle(args[0], myBool(args[1]));

	CASE(L"load_file")
		hr = load_file(args[0], myBool(args[1]), myInt(args[2]), myInt(args[3]));

	CASE(L"end_loading")
		hr = end_loading();



	CASE(L"set_subtitle_pos")
		hr = set_subtitle_pos(myDouble(args[0]), myDouble(args[1]));
	CASE(L"set_subtitle_pos")
	{
		double center_x = 0;
		double bottom_y = 0;
		get_subtitle_pos(&center_x, &bottom_y);
		wcscpy2(out, myDouble(center_x));
		wcscat2(out, L"|");
		wcscat2(out, myDouble(bottom_y));
		return S_OK;
	}

	CASE(L"set_subtitle_parallax")
		hr = set_subtitle_parallax(myInt(args[0]));
	CASE(L"get_subtitle_parallax")
	{
		int parallax = 0;
		get_subtitle_parallax(&parallax);
		wcscpy2(out, myInt(parallax));
		return S_OK;
	}



	CASE(L"set_swapeyes")
		hr = set_swap_eyes(myBool(args[0]));
	CASE(L"get_swapeyes")
	{
		wcscpy2(out, myBool((bool)m_renderer1->m_swap_eyes));
		return S_OK;
	}

	CASE(L"set_movie_pos")
		hr = set_movie_pos(myDouble(args[0]), myDouble(args[1]));

	CASE(L"play")
		hr = play();

	CASE(L"pause")
		hr = pause();

	CASE(L"stop")
		hr = stop();

	CASE(L"seek")
		hr = seek(myInt(args[0]));

	CASE(L"tell")
	{
		int now;
		hr = tell(&now);
		wcscpy2(out, myInt(now));
	}
#ifdef DEBUG
	CASE(L"error")
	{
		*((BYTE*)NULL) = 8;
		return S_OK;
	}
#endif

	CASE(L"total")
	{
		int t;
		hr = total(&t);
		wcscpy2(out, myInt(t));
	}

	CASE(L"set_volume")
		hr = set_volume(myDouble(args[0]));

	CASE(L"get_volume")
	{
		double v;
		hr = get_volume(&v);
		wcscpy2(out, myDouble(v));
	}

	CASE(L"is_playing")
	{
		wcscpy2(out, myBool(is_playing()));
		hr = S_OK;
	}

	CASE(L"show_mouse")
	{
		hr = show_mouse(myBool(args[0]));
	}

	CASE(L"toggle_fullscreen")
	{
		hr = toggle_fullscreen();
	}

	CASE(L"set_output_mode")
	{
		hr = set_output_mode(myInt(args[0]));
	}

	CASE(L"is_fullscreen")
	{
		wcscpy2(out, myBool(m_full1));
		hr = S_OK;
	}

	CASE(L"list_bd")
	{
		wcscpy2(out, L"");
		for(int i=L'Z'; i>L'B'; i--)
		{
			wchar_t tmp[MAX_PATH] = L"C:\\";
			wchar_t tmp2[MAX_PATH];
			tmp[0] = i;
			tmp[4] = NULL;
			if (GetDriveTypeW(tmp) == DRIVE_CDROM)
			{
				wcscat2(out, tmp);
				if (!GetVolumeInformationW(tmp, tmp2, MAX_PATH, NULL, NULL, NULL, NULL, 0))
				{
					// no disc
					wcscat2(out, L"/|");
				}
				else if (FAILED(find_main_movie(tmp, tmp2)))
				{
					// not bluray
					wcscat2(out, L":|");
				}
				else
				{
					GetVolumeInformationW(tmp, tmp2, MAX_PATH, NULL, NULL, NULL, NULL, 0);
					wcscat2(out, tmp2);
					wcscat2(out, L"|");
				}

			}
		}
		hr = S_OK;
	}

	CASE(L"list_audio_track")
	{
		wchar_t tmp[32][1024];
		wchar_t *tmp2[32];
		bool connected[32];
		for(int i=0; i<32; i++)
			tmp2[i] = tmp[i];
		int found = 0;
		list_audio_track(tmp2, connected, &found);

		out[0] = NULL;
		for(int i=0; i<found; i++)
		{
			wcscat2(out, tmp2[i]);
			wcscat2(out, L"|");
			wcscat2(out, myBool(connected[i]));
			wcscat2(out, L"|");
		}
	}

	CASE(L"list_subtitle_track")
	{
		wchar_t tmp[32][1024];
		wchar_t *tmp2[32];
		bool connected[32];
		for(int i=0; i<32; i++)
			tmp2[i] = tmp[i];
		int found = 0;
		list_subtitle_track(tmp2, connected, &found);

		out[0] = NULL;
		for(int i=0; i<found; i++)
		{
			wcscat2(out, tmp2[i]);
			wcscat2(out, L"|");
			wcscat2(out, myBool(connected[i]));
			wcscat2(out, L"|");
		}
	}

	CASE(L"enable_audio_track")
		hr = enable_audio_track(myInt(args[0], -999));		// -999 should cause no harm

	CASE(L"enable_subtitle_track")
		hr = enable_subtitle_track(myInt(args[0], -999));	// -999 should cause no harm

	CASE(L"list_drive")
	{
		wchar_t *tmp = new wchar_t[102400];
		tmp[0] = NULL;

#ifdef ZHUZHU
		for(int i=3; i<26; i++)
#else
		for(int i=0; i<26; i++)
#endif
		{
			wchar_t path[50];
			wchar_t tmp2[MAX_PATH];
			swprintf(path, L"%c:\\", L'A'+i);
			if (GetVolumeInformationW(path, tmp2, MAX_PATH, NULL, NULL, NULL, NULL, 0))
#ifdef ZHUZHU
			if (GetDriveTypeW(path) != DRIVE_CDROM)
#endif
			{
				wcscat2(tmp, path);
				wcscat2(tmp, L"|");
			}
		}
		wcscpy2(out, tmp);
		delete [] tmp;
		return S_OK;
	}

	CASE(L"shutdown")
	{
		command_reciever = NULL;
#ifdef DEBUG
		TerminateProcess(GetCurrentProcess(), 0);
#else
		show_window(1, false);
		show_window(2, false);
#endif
		return S_OK;
	}

	CASE(L"shutdown_windows")
	{
		HANDLE hToken;
		TOKEN_PRIVILEGES tkp;
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
			return E_FAIL;

		LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);

		tkp.PrivilegeCount = 1;    
		tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);

		if(GetLastError() != ERROR_SUCCESS)
			return E_FAIL;

		if(!ExitWindowsEx(EWX_POWEROFF | EWX_FORCE, 0))
			return E_FAIL;

		return S_OK;
	}

	CASE(L"x264_init")
	{
		CAutoLock lck(&m_x264_encoder_lock);

		myInt width(args[0], 960);
		myInt height(args[1], 540);
		myInt bitrate(args[2], 1500);

		x264 *new_encoder = new x264();
		hr = new_encoder->init(width, height, bitrate);

		if (FAILED(hr))
		{
			delete new_encoder;
			return hr;
		}


		//	swprintf(out, L"%08x", new_encoder);
		wcscpy2(out, myInt((int)new_encoder));
		m_x264_encoder.push_back(new_encoder);
	}

	CASE(L"x264_shot")
	{
		myInt p(args[0]);
		{
			CAutoLock lck(&m_x264_encoder_lock);
			for(std::list<x264*>::iterator v= m_x264_encoder.begin(); v!=m_x264_encoder.end();++v)
			{
				if ((int)(*v) == p)
					goto shot;
			}
		}

		return E_INVALIDARG;

shot:
		x264 *x = ((x264*)(int)p);
		char *Y = (char*)malloc(x->width * x->height * 3 / 2);
		char *V = Y + x->width * x->height;
		char *U = V + x->width * x->height / 4;

		m_renderer1->screenshot(Y, U, V, x->width, x->width, x->height);
		*(int*)out = x->encode_a_frame(Y, out+2, NULL);
		free(Y);
		
		return S_RAWDATA;
	}

	CASE(L"x264_destroy")
	{
		myInt p(args[0]);
		{
			CAutoLock lck(&m_x264_encoder_lock);
			for(std::list<x264*>::iterator v= m_x264_encoder.begin(); v!=m_x264_encoder.end();++v)
			{
				if ((int)(*v) == p)
				{
					x264 *x = ((x264*)(int)p);

					m_x264_encoder.erase(v);

					delete x;

					return S_OK;
				}
			}
		}

		return E_INVALIDARG;
	}

	CASE(L"list_file")
	{
		wchar_t *tmp = new wchar_t[102400];
		wcscpy2(tmp, args[0]);
		wcscat2(tmp, L"*.*");

		WIN32_FIND_DATAW find_data;
		HANDLE find_handle = FindFirstFileW(tmp, &find_data);
		tmp[0] = NULL;

		if (find_handle != INVALID_HANDLE_VALUE)
		{
			do
			{
				if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)!=0
						&& wcscmp(L".",find_data.cFileName ) !=0
						&& wcscmp(L"..", find_data.cFileName) !=0
					)
				{
					wcscat2(tmp, find_data.cFileName);
					wcscat2(tmp, L"\\|");
				}
				else if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)==0)

				{
					wcscat2(tmp, find_data.cFileName);
					wcscat2(tmp, L"|");
				}

			}
			while( FindNextFile(find_handle, &find_data ) );
		}

		wcscpy2(out, tmp);
		delete [] tmp;
		return S_OK;
	}

	CASE(L"widi_refresh")
	{
		hr = widi_start_scan();
	}

	CASE(L"widi_connect")
	{
		hr = widi_connect(myInt(args[0]), myInt(args[1]), myInt(args[2]), myInt(args[3]));
	}

	CASE(L"widi_disconnect")
	{
		hr = widi_disconnect(myInt(args[0], -1));		
	}

	CASE(L"widi_set_screenmode")
	{
		hr = widi_set_screen_mode(myInt(args[0], ExternalOnly));
	}

	CASE(L"widi_num_adapters_found")
	{
		wcscpy2(out, myInt(m_widi_num_adapters_found));
		hr = S_OK;
	}

	CASE(L"widi_get_adapter_by_id")
	{
		hr = S_OK;
		if (out)
			hr = widi_get_adapter_by_id(myInt(args[0]), out);
	}

	CASE(L"widi_get_adapter_information")
	{
		if (args[1] == NULL)
			hr = E_INVALIDARG;
		else
		{
			hr = S_OK;
			if (out)
				hr = widi_get_adapter_information(myInt(args[0]), out, args[1]);
		}
	}

	DEFAULT
		return E_NOTIMPL;

	return hr;
}
