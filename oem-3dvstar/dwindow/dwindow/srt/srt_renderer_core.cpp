#include "srt_renderer.h"
#include "..\global_funcs.h"

bool wcs_replace(wchar_t *to_replace, const wchar_t *searchfor, const wchar_t *replacer)
{
	const int tmp_size = 2048;
pass:
	wchar_t tmp[tmp_size];
	if (wcslen(to_replace) > tmp_size-1)
		return false;

	wchar_t *left_part = to_replace;
	wchar_t *right_part = wcsstr(to_replace, searchfor);
	if (!right_part)
		return true;

	right_part[0] = NULL;
	wsprintfW(tmp, L"%s%s%s", left_part, replacer, right_part + wcslen(searchfor));
	wcscpy(to_replace, tmp);
	goto pass;


	return false;	// ...
}

CsrtRendererCore::CsrtRendererCore(HFONT font, DWORD fontcolor)
:m_font_color(GET_CONST("FontColor"))
,m_font_size(GET_CONST("FontSize"))
,m_font_name(GET_CONST("Font"))
,m_last_font_size(0)
{
	m_last_font_name[0] = NULL;
	m_font = font;
	m_aspect = 16.0/9.0;
	reset();
}

CsrtRendererCore::~CsrtRendererCore()
{

}
HRESULT CsrtRendererCore::set_font_color(DWORD newcolor)
{
	m_font_color = newcolor;
	return S_OK;
}

HRESULT CsrtRendererCore::set_font(HFONT newfont)
{
	m_font = newfont;
	return S_OK;
}


HRESULT CsrtRendererCore::load_file(wchar_t *filename)
{
	return m_srt.load(filename);
}

HRESULT CsrtRendererCore::reset()
{
	m_srt.init(20480, 2048*1024);
	m_last_found[0] = NULL;
	return S_OK;
}

HRESULT CsrtRendererCore::seek()
{
	m_last_found[0] = NULL;
	return S_OK;
}

HRESULT CsrtRendererCore::add_data(BYTE *data, int size, int start, int end)
{
	// remove some splitter's prefix 2byte datasize
	if (size >=2 && (data[0] << 8) + data[1] == size-2)
		size -= 2, data += 2;

	if (size <= 0)
		return S_OK;

	char *p1 = (char*)malloc(size+1);
	memcpy(p1, data, size);
	p1[size] = NULL;
	wchar_t *p2 = (wchar_t*)malloc(size*2+2);
	MultiByteToWideChar(CP_UTF8, 0, (char*)p1, size+1, p2, size+1);
	HRESULT hr = m_srt.direct_add_subtitle(p2, start, end);

	free(p1);
	free(p2);

	return hr;
}

HRESULT CsrtRendererCore::get_subtitle(int time, rendered_subtitle *out, int last_time/* =-1 */)
{
	if (NULL == out)
		return E_POINTER;

	wchar_t found[1024];
	bool has_offset = false;
	int offset = 0;
	m_srt.get_subtitle(time, time, found, &has_offset, &offset);
	if ((wcscmp(found, m_last_found) == 0) && (last_time != -1))
	{
		out->data = NULL;
		return S_FALSE;
	}

	else
	{
		wcscpy_s(m_last_found, found);

		return render(found, out, has_offset, offset);
	}
}

HRESULT CsrtRendererCore::render(const wchar_t *text, rendered_subtitle *out, bool has_offset, int offset)
{
	if (text[0] == NULL)
	{
		out->width_pixel = out->height_pixel = 0;
		out->width = out->height = 0;
		out->data = NULL;
		return S_OK;
	}

	HDC hdc = GetDC(NULL);
	HDC hdcBmp = CreateCompatibleDC(hdc);

	if (m_last_font_size != (int)m_font_size || wcscmp(m_last_font_name, m_font_name) !=0)
	{
		m_last_font_size = m_font_size;
		wcscpy(m_last_font_name, m_font_name);

		LOGFONTW lf={0};
		lf.lfHeight = -m_last_font_size;
		lf.lfCharSet = GB2312_CHARSET;
		lf.lfOutPrecision =  OUT_STROKE_PRECIS;
		lf.lfClipPrecision = CLIP_STROKE_PRECIS;
		lf.lfQuality = DEFAULT_QUALITY;
		lf.lfPitchAndFamily = VARIABLE_PITCH;
		lf.lfWeight = FW_BOLD*3;
		lstrcpynW(lf.lfFaceName, m_font_name, 32);

		m_font = CreateFontIndirectW(&lf); 
	}

	HFONT hOldFont = (HFONT) SelectObject(hdcBmp, m_font);

	RECT rect = {0,0,1920,int(1920/m_aspect)};
	DrawTextW(hdcBmp, text, (int)wcslen(text), &rect, DT_CENTER | DT_CALCRECT | DT_WORDBREAK | DT_NOFULLWIDTHCHARBREAK | DT_EDITCONTROL);
	out->pixel_type = out->pixel_type_RGB;
	out->height_pixel = rect.bottom - rect.top;
	out->width_pixel  = rect.right - rect.left;
	out->width = (double)out->width_pixel/1920;
	out->height = (double)out->height_pixel/(1920/m_aspect);
	out->aspect = m_aspect;
	out->delta = 0.001 * offset;
	out->delta_valid = has_offset;
	out->gpu_shadow = true;

	out->left = 0.5 - out->width/2;
	out->top = 0.95 - out->height;

	HBITMAP hbm = CreateCompatibleBitmap(hdc, out->width_pixel, out->height_pixel);

	HBITMAP hbmOld = (HBITMAP)SelectObject(hdcBmp, hbm);

	RECT rcText;
	SetRect(&rcText, 0, 0, out->width_pixel, out->height_pixel);
	SetBkColor(hdcBmp, RGB(0, 0, 0));					// Pure black background
	SetTextColor(hdcBmp, RGB(255, 255, 255));			// white text for alpha

	DrawTextW(hdcBmp, text, (int)wcslen(text), &rect, DT_CENTER);

	out->data = (BYTE *) malloc(out->width_pixel * out->height_pixel * 4);
	GetBitmapBits(hbm, out->width_pixel * out->height_pixel * 4, out->data);


	BYTE *data = (BYTE*)out->data;
	DWORD *data_dw = (DWORD*)out->data;
	unsigned char color_r = (BYTE)((DWORD)m_font_color       & 0xff);
	unsigned char color_g = (BYTE)(((DWORD)m_font_color>>8)  & 0xff);
	unsigned char color_b = (BYTE)(((DWORD)m_font_color>>16) & 0xff);
	DWORD color = (color_r<<16) | (color_g <<8) | (color_b);// reverse big endian

	for(int i=0; i<out->width_pixel * out->height_pixel; i++)
	{
		unsigned char alpha = data[i*4+2];
		data_dw[i] = color;
		data[i*4+3] = alpha;
	}

	DeleteObject(SelectObject(hdcBmp, hbmOld));
	SelectObject(hdc, hOldFont);
	DeleteObject(hbm);
	DeleteDC(hdcBmp);
	ReleaseDC(NULL, hdc);

	return S_OK;

}

HRESULT CsrtRendererCore::set_output_aspect(double aspect)
{
	if (m_aspect == aspect)
		return S_FALSE;

	m_aspect = aspect;
	return S_OK;
}

// ASS Subtitle

HRESULT CAssRendererFallback::add_data(BYTE *data, int size, int start, int end)
{
	// remove some splitter's prefix 2byte datasize
	if (size >=2 && (data[0] << 8) + data[1] == size-2)
		size -= 2, data += 2;

	if (size <= 0)
		return S_OK;

	char *p1 = (char*)malloc(size+1);
	memcpy(p1, data, size);
	p1[size] = NULL;
	wchar_t *p2 = (wchar_t*)malloc(size*2+2);
	MultiByteToWideChar(CP_UTF8, 0, (char*)p1, size+1, p2, size+1);

	// remove 8 commas
	int commas_left = 9;
	while (wchar_t *comma = wcsstr(p2, L","))
	{
		if (!commas_left)
			break;

		commas_left--;
		wcscpy(p2, comma+1);
	}

	// remove {XXX} in the line
	wchar_t *l = wcsstr(p2, L"{");
	wchar_t *r = wcsstr(p2, L"}");
	while (l && r && r>l)
	{
		wcscpy(l, r+1);
		l = wcsstr(p2, L"{");
		r = wcsstr(p2, L"}");
	}

	wcs_replace(p2, L"\\N", L"\n");

	wcscat(p2, L"\n");
	wcscat(p2, C(L"(Fonts Loading)"));

	HRESULT hr = m_srt.direct_add_subtitle(p2, start, end);

	free(p1);
	free(p2);

	return hr;
}