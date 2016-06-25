#pragma  once
#include "atlbase.h"
#include "dshow.h"
#include "..\lrtb\mySink.h"
#include "PGS\PGSRenderer.h"
#include "srt\srt_renderer.h"
#include "vobsub_renderer.h"
#include "libass_renderer.h"

class DShowSubtitleRenderer : protected ImySinkCB
{
public:
	DShowSubtitleRenderer(HFONT font = NULL, DWORD fontcolor = 0xffffff);
	~DShowSubtitleRenderer();
	virtual HRESULT set_font_color(DWORD newcolor);
	virtual HRESULT set_font(HFONT newfont);											// for text based subtitles, the main program will try show_dlg=false first, if the SubtitleRenderer is not text based, it should return E_NOT_IMPL.

	IBaseFilter *GetFilter();
	CSubtitleRenderer *GetSubtitleRenderer();
	CComPtr<IBaseFilter> m_filter;

protected:
	CCritSec m_filter_sec;
	CCritSec m_subtitle_sec;
	CSubtitleRenderer *m_srenderer;
	mySink *m_sink;
	REFERENCE_TIME m_subtitle_seg_time;

	// font
	HFONT m_font;
	DWORD m_font_color;

	virtual HRESULT EndOfStreamCB(){return E_NOTIMPL;}
	virtual HRESULT BreakConnectCB();
	virtual HRESULT SampleCB(IMediaSample *sample);
	virtual HRESULT CheckMediaTypeCB(const CMediaType *inType);
	virtual HRESULT NewSegmentCB(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
};