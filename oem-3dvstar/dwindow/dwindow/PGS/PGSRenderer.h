#pragma once
#include "..\CSubtitle.h"
#include "PGSParser.h"

class PGSRenderer: public CSubtitleRenderer
{
public:
	PGSRenderer();
	~PGSRenderer();
	virtual HRESULT load_file(wchar_t *filename);	//maybe you don't need this?
	virtual HRESULT add_data(BYTE *data, int size, int start, int end);
	virtual HRESULT get_subtitle(int time, rendered_subtitle *out, int last_time=-1);			// get subtitle on a time point, 
																								// if last_time != -1, return S_OK = need update, return S_FALSE = same subtitle, and out should be ignored;
	virtual HRESULT reset();
	virtual HRESULT seek();	// to provide dshow support
	virtual HRESULT set_font_color(DWORD newcolor){return E_NOTIMPL;};
	virtual HRESULT set_font(HFONT newfont){return E_NOTIMPL;};									// for text based subtitles, the main program will try show_dlg=false first, if the SubtitleRenderer is not text based, it should return E_NOT_IMPL.
protected:
	int m_last_found;
	PGSParser m_parser;
	BYTE *m_seg_buffer;		//some splitter may send one segment in serveral packets, pack it together first.
	int m_seg_buffer_pos;
};