#pragma once
#include <windows.h>

typedef struct _rendered_subtitle
{
	// pos and size
	// range 0.0 - 1.0
	double left;
	double top;
	double width;
	double height;
	double aspect;
	bool gpu_shadow;	// true = shadow should be painted by GPU

	// data size in pixel
	int width_pixel;
	int height_pixel;

	// stereo info...
	bool delta_valid;
	double delta;

	// pixel type and data
	enum
	{
		pixel_type_RGB,		// ARGB32
		pixel_type_YUV,		// AYUV 4:4:4
	} pixel_type;

	BYTE *data;				// if this is not NULL, you should free it after use
} rendered_subtitle;

class CSubtitleRenderer
{
public:
	virtual ~CSubtitleRenderer(){};
	virtual HRESULT load_file(wchar_t *filename)=0;	//maybe you don't need this?
	virtual HRESULT load_index(void *data, int size){return S_OK;}
	virtual HRESULT add_data(BYTE *data, int size, int start, int end)=0;
	virtual HRESULT get_subtitle(int time, rendered_subtitle *out, int last_time=-1)=0;			// get subtitle on a time point, 
																								// if last_time != -1, return S_OK = need update, return S_FALSE = same subtitle, and out should be ignored;
	virtual HRESULT reset()=0;
	virtual HRESULT seek()=0;															// just clear current incompleted data, to support dshow seeking.
	virtual HRESULT set_font_color(DWORD newcolor)=0;									// color format is 8bit BGRA, use RGB() macro, for text based subtitle only
	virtual HRESULT set_font(HFONT newfont)=0;											// for text based subtitles, the main program will try show_dlg=false first, if the SubtitleRenderer is not text based, it should return E_NOT_IMPL.
	virtual HRESULT set_output_aspect(double aspect){return E_NOTIMPL;}					// set the rendered output aspect, to suit movies's aspect, S_FALSE = not changed, S_OK = changed

};