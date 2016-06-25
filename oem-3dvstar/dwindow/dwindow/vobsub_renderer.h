#pragma once
#include "CSubtitle.h"
#include "vobsub_parser.h"

class VobSubRenderer: public CSubtitleRenderer
{
public:
	VobSubRenderer(){};
	~VobSubRenderer(){};
	virtual HRESULT load_file(wchar_t *filename);												//maybe you don't need this?
	virtual HRESULT load_file(wchar_t *filename, int langid);
	virtual HRESULT load_index(void *data, int size);
	virtual HRESULT add_data(BYTE *data, int size, int start, int end);
	virtual HRESULT get_subtitle(int time, rendered_subtitle *out, int last_time=-1);			// get subtitle on a time point, 
																								// if last_time != -1, return S_OK = need update, return S_FALSE = same subtitle, and out should be ignored;
	virtual HRESULT reset();
	virtual HRESULT seek();																		// to provide dshow support
	virtual HRESULT set_font_color(DWORD newcolor){return E_NOTIMPL;};
	virtual HRESULT set_font(HFONT newfont){return E_NOTIMPL;};									// for text based subtitles, the main program will try show_dlg=false first, if the SubtitleRenderer is not text based, it should return E_NOT_IMPL.
protected:
	int m_last_found;
	VobSubParser m_parser;
};