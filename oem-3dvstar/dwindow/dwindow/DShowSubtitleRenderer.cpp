#include "DShowSubtitleRenderer.h"
#include "global_funcs.h"

DShowSubtitleRenderer::DShowSubtitleRenderer(HFONT font, DWORD fontcolor)
{
	m_srenderer = NULL;
	m_font = font;
	m_font_color = fontcolor;

	HRESULT hr;
	m_sink = new mySink(NULL, &hr);
	m_sink->QueryInterface(IID_IBaseFilter, (void**)&m_filter);
	m_sink->SetCallback(this);
}

DShowSubtitleRenderer::~DShowSubtitleRenderer()
{
	BreakConnectCB();
}

HRESULT DShowSubtitleRenderer::BreakConnectCB()
{
	CAutoLock lck1(&m_subtitle_sec);
	if (m_srenderer) delete m_srenderer;
	m_srenderer = NULL;

	return S_OK;
}

HRESULT DShowSubtitleRenderer::SampleCB(IMediaSample *sample)
{
	REFERENCE_TIME start, end;
	sample->GetTime(&start, &end);
	BYTE *p = NULL;
	sample->GetPointer(&p);

	AM_MEDIA_TYPE *type = NULL;
	sample->GetMediaType(&type);

	if (type)
	{
		printf("New Type.\n");
	}

	//CAutoLock lck(&m_subtitle_sec);
	if (m_srenderer)
		m_srenderer->add_data(p, sample->GetActualDataLength(), (int)((start+m_subtitle_seg_time)/10000), (int)((end+m_subtitle_seg_time)/10000));

	return S_OK;
}

HRESULT DShowSubtitleRenderer::CheckMediaTypeCB(const CMediaType *inType)
{
	GUID majorType = *inType->Type();
	GUID subType = *inType->Subtype();
	if (majorType != MEDIATYPE_Subtitle)
		return VFW_E_INVALID_MEDIA_TYPE;
	typedef struct SUBTITLEINFO 
	{
		DWORD dwOffset; // size of the structure/pointer to codec init data
		CHAR IsoLang[4]; // three letter lang code + terminating zero
		WCHAR TrackName[256]; // 256 bytes ought to be enough for everyone
	} haali_format;
	haali_format *format = (haali_format*)inType->pbFormat;
	/*
	BYTE * additional_format_data = inType->pbFormat + format->dwOffset;
	int additional_format_size = inType->cbFormat - format->dwOffset;
	FILE * f = fopen("Z:\\sub.idx", "wb");
	fwrite(additional_format_data, 1, additional_format_size, f);
	fclose(f);
	*/
	CAutoLock lck(&m_subtitle_sec);
	if (m_srenderer)
		delete m_srenderer;
	m_srenderer = NULL;
	if (subType == MEDIASUBTYPE_PGS || 
		(subType == GUID_NULL &&inType->FormatLength()>=520 && wcsstr(format->TrackName, L"SUP")))
		m_srenderer = new PGSRenderer();
	else if (subType == MEDIASUBTYPE_UTF8 || (subType == GUID_NULL &&inType->FormatLength()>=520))
	{
		m_srenderer = new CsrtRenderer();
		m_srenderer->load_index(inType->pbFormat + format->dwOffset, inType->cbFormat - format->dwOffset);
	}
	else if (subType == MEDIASUBTYPE_ASS || subType == MEDIASUBTYPE_ASS2)
	{
// 		if (LibassRendererCore::fonts_loaded() != S_OK)
// 			MessageBoxW(NULL, C(L"This is first time to load ass/ssa subtilte, font scanning may take one minute or two, the player may looks like hanged, please wait..."), C(L"Please Wait"), MB_OK);

		m_srenderer = new LibassRenderer();
		m_srenderer->load_index(inType->pbFormat + format->dwOffset, inType->cbFormat - format->dwOffset);
	}
	else if (subType == MEDIASUBTYPE_VOBSUB)
	{
		m_srenderer = new VobSubRenderer();
		m_srenderer->load_index(inType->pbFormat + format->dwOffset, inType->cbFormat - format->dwOffset);
	}
	else
		return VFW_E_INVALID_MEDIA_TYPE;

	m_srenderer->set_font(m_font);
	m_srenderer->set_font_color(m_font_color);

	return S_OK;
}

HRESULT DShowSubtitleRenderer::NewSegmentCB(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	if (m_srenderer)
		m_srenderer->seek();

	m_subtitle_seg_time = tStart;
	return S_OK;
}

IBaseFilter *DShowSubtitleRenderer::GetFilter()
{
	return m_filter;
}

CSubtitleRenderer *DShowSubtitleRenderer::GetSubtitleRenderer()
{
	CAutoLock lck(&m_subtitle_sec);
	return m_srenderer;
}

HRESULT DShowSubtitleRenderer::set_font(HFONT newfont)
{
	m_font = newfont;
	CAutoLock lck(&m_subtitle_sec);
	if (m_srenderer)
		return m_srenderer->set_font(newfont);
	else
		return S_OK;
}

HRESULT DShowSubtitleRenderer::set_font_color(DWORD newcolor)
{
	m_font_color = newcolor;
	CAutoLock lck(&m_subtitle_sec);
	if (m_srenderer)
		return m_srenderer->set_font_color(newcolor);
	else
		return S_OK;
}