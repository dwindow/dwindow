#include "libass_renderer.h"
#include "..\libass\charset.h"
#include <Windows.h>
#include <streams.h>
#include <emmintrin.h>
#include <assert.h>
#include "command_queue.h"

#define _r(c)  ((c)>>24)
#define _g(c)  (((c)>>16)&0xFF)
#define _b(c)  (((c)>>8)&0xFF)
#define _a(c)  ((c)&0xFF)


#pragma comment(lib, "libass.a")
#pragma comment(lib, "libgcc.a")
#pragma comment(lib, "libmingwex.a")
#pragma comment(lib, "libass.a")
#pragma comment(lib, "libfreetype.a")
#pragma comment(lib, "libfontconfig.a")
#pragma comment(lib, "libexpat.a")

ASS_Library *g_ass_library = NULL;
bool g_fonts_loaded = false;
HANDLE g_fonts_loading_thread = INVALID_HANDLE_VALUE;
static DWORD WINAPI loadFontThread(LPVOID param);
CCritSec g_libass_cs;

LibassRendererCore::LibassRendererCore()
{
	m_ass_renderer = NULL;
	m_track = NULL;

	CAutoLock lck(&g_libass_cs);
	if (NULL == g_ass_library)
		g_ass_library = ass_library_init();

	if (!g_ass_library) 
	{
		printf("ass_library_init failed!\n");
		return;
	}

	reset();
}
LibassRendererCore::~LibassRendererCore()
{
	CAutoLock lck(&g_libass_cs);

	if (m_track)
		ass_free_track(m_track);

	if (m_ass_renderer)
		ass_renderer_done(m_ass_renderer);
}
HRESULT LibassRendererCore::load_file(wchar_t *filename)
{
	CAutoLock lck(&g_libass_cs);
	if (!g_ass_library || !m_ass_renderer)
		return E_FAIL;

	if (m_track)
		ass_free_track(m_track);

	FILE * f = _wfopen(filename, L"rb");
	fseek(f, 0, SEEK_END);
	int file_size = ftell(f);
	fseek(f, 0, SEEK_SET);
	char *src = (char*)malloc(file_size);
	char *utf8 = (char*)malloc(file_size*3);
	fread(src, 1, file_size, f);
	fclose(f);

	int utf8_size = ConvertToUTF8(src, file_size, utf8, file_size*3);

	m_track = ass_read_memory(g_ass_library, utf8, utf8_size, NULL);

	free(src);
	free(utf8);

	return m_track ? S_OK : E_FAIL;
}
HRESULT LibassRendererCore::load_index(void *data, int size)
{
	CAutoLock lck(&g_libass_cs);
	if (!g_ass_library || !m_ass_renderer)
		return E_FAIL;

	if (m_track)
		ass_free_track(m_track);

	m_track = ass_new_track(g_ass_library);

	ass_process_codec_private(m_track, (char*)data, size);

	return S_OK;
}
HRESULT LibassRendererCore::add_data(BYTE *data, int size, int start, int end)
{
	CAutoLock lck(&g_libass_cs);
	if (!g_ass_library || !m_ass_renderer || !m_track)
		return E_FAIL;

	ass_process_chunk(m_track, (char*)data, size, start, end-start);

	return S_OK;
}
HRESULT LibassRendererCore::get_subtitle(int time, rendered_subtitle *out, int last_time/*=-1*/)			// get subtitle on a time point, 
																									// if last_time != -1, return S_OK = need update, return S_FALSE = same subtitle, and out should be ignored;
{
	if (NULL == out)
		return E_POINTER;

	CAutoLock lck(&g_libass_cs);
	if (!g_ass_library || !m_ass_renderer || !m_track)
		return E_FAIL;

	memset(out, 0, sizeof(rendered_subtitle));

	ASS_Image *img = NULL;
	ASS_Image *p = NULL;
	int changed = 0;
	p = img = ass_render_frame(m_ass_renderer, m_track, time, &changed);

	if (!img)
		return S_OK;

	if (changed == 0 && last_time>=0)
		return S_FALSE;

	RECT rect = {0};
	if (p)
	{
		rect.left = p->dst_x;
		rect.right = rect.left + p->w;
		rect.top = p->dst_y;
		rect.bottom = rect.top + p->h;
	}

	while (p)
	{
		rect.left = min(rect.left, p->dst_x);
		rect.top = min(rect.top, p->dst_y);
		rect.right = max(rect.right, p->dst_x + p->w);
		rect.bottom = max(rect.bottom, p->dst_y + p->h);

		p = p->next;
	}

	out->height_pixel = rect.bottom - rect.top;
	out->width_pixel  = rect.right - rect.left;
	out->width = (double)out->width_pixel/1920;
	out->height = (double)out->height_pixel/1080;
	out->aspect = 16.0/9.0;
	out->data = (BYTE *) malloc(out->width_pixel * out->height_pixel * 4);
	memset(out->data, 0, out->width_pixel * out->height_pixel * 4);
	out->left = (double)rect.left/1920;
	out->top = (double)rect.top/1080;

	// blending
	// T(RGBA) = [T(RGBA) * (255-A)]  +  [S(L) * C(RGBA) * A] / 255
	//__m128i k;
// 	__m128i d1, d2;
// 	__m128i s1;
// 	__m128i zero = {0};
	while (img) 
	{
		int x, y;
		unsigned char opacity = 255 - _a(img->color);
		unsigned char r = _r(img->color);
		unsigned char g = _g(img->color);
		unsigned char b = _b(img->color);

		unsigned char *src = img->bitmap;
		unsigned char *dst = out->data + (img->dst_y - rect.top) * out->width_pixel*4 + (img->dst_x-rect.left) * 4;

// 		__m128i bb = _mm_set1_epi8(b);
// 		__m128i gg = _mm_set1_epi8(g);
// 
// 		__m128i bgra = _mm_unpackhi_epi8(bb, gg);
// 		bb = _mm_set1_epi8(r);	// used as tmp
// 		gg = _mm_set1_epi8(255);
// 		bb = _mm_unpackhi_epi8(bb, gg);
// 		bgra = _mm_unpackhi_epi16(bgra, bb);	// C
// 		bgra = _mm_unpackhi_epi8(bgra, zero);
// 		bb = _mm_set1_epi16(opacity);			// A
// 		bgra = _mm_mullo_epi16(bgra, bb);		// C(RGBA) * A
// 		gg = _mm_set1_epi16(255-opacity);		// 255 - A
		// after this, gg = 255-A, rgba = C(RGBA)*A, bb unused (still = A), 16byte, 2 pixel 

		for (y = 0; y < img->h; ++y) 
		{
			// 4 pixel SSE2
// 			for (x = 0; x < img->w; x+=4)
// 			{
// 				s1 = _mm_loadl_epi64((__m128i*)(src+x));		// 8 byte, 8 pixel L8, only  4 pixel used, assume no over reading...
// 				s1 = _mm_unpacklo_epi8(s1,s1);
// 				s1 = _mm_unpacklo_epi8(s1,s1);
// 				bb = _mm_unpackhi_epi8(s1, zero);
// 				s1 = _mm_unpacklo_epi8(s1, zero);
// 
// 				// S(L) * C(RGBA) * A
// 				bb = _mm_mullo_epi16(bb, bgra);
// 				s1 = _mm_mullo_epi16(s1, bgra);
// 
// 				// load
// 				d1 = _mm_loadu_si128((__m128i*)(dst+x*4));		// 16byte, 4 pixel ARGB
// 				d2 = _mm_unpackhi_epi8(d1, zero);
// 				d1 = _mm_unpacklo_epi8(d1, zero);
// 
// 				d2 = _mm_mullo_epi16(d2, gg);
// 				d1 = _mm_mullo_epi16(d1, gg);
// 
// 				bb = _mm_add_epi16(bb, d2);
// 				s1 = _mm_add_epi16(s1, d1);
// 
// 				d2 = _mm_packus_epi16(s1, bb);
// 				_mm_storeu_si128((__m128i*)(dst+x*4), d2);
// 			}
			x=4;

			// remains
			for (x-=4; x < img->w; ++x) 
			{
				unsigned k = ((unsigned) src[x]) * opacity / 256;
				dst[x * 4 + 0] = (k * b   + (255 - k) * dst[x * 4 + 0]) / 256;
				dst[x * 4 + 1] = (k * g   + (255 - k) * dst[x * 4 + 1]) / 256;
				dst[x * 4 + 2] = (k * r   + (255 - k) * dst[x * 4 + 2]) / 256;
				dst[x * 4 + 3] = (k * 255 + (255 - k) * dst[x * 4 + 3]) / 256;
			}
			src += img->stride;
			dst += out->width_pixel*4;
		}

		img = img->next;
	}

	return S_OK;
}

HRESULT LibassRendererCore::reset()
{
	CAutoLock lck(&g_libass_cs);

	if (m_track)
		ass_free_track(m_track);

	if (m_ass_renderer)
		ass_renderer_done(m_ass_renderer);

	m_ass_renderer = ass_renderer_init(g_ass_library);
	if (!m_ass_renderer)
	{
		printf("ass_renderer_init failed!\n");
		return E_FAIL;
	}

	char conf[MAX_PATH];
	GetModuleFileNameA(NULL, conf, MAX_PATH);
	for(int i=strlen(conf)-1; i>0; i--)
		if (conf[i] == '\\')
		{
			conf[i] = NULL;
			break;
		}

	strcat(conf, "\\fonts.conf");

	ass_set_frame_size(m_ass_renderer, 1920, 1080);
	ass_set_font_scale(m_ass_renderer, 1.0);
	ass_set_fonts(m_ass_renderer, "Arial", "Sans", 1, conf, 1);

	return S_OK;
}


HRESULT LibassRendererCore::load_fonts()
{
	if (g_fonts_loaded)
		return S_OK;

	if (g_fonts_loading_thread != INVALID_HANDLE_VALUE)
		return S_FALSE;

	g_fonts_loading_thread = CreateThread(NULL, NULL, loadFontThread, NULL, NULL, NULL);

	return S_OK;
}

HRESULT LibassRendererCore::fonts_loaded()
{
	if (g_fonts_loaded)
		return S_OK;

	if (g_fonts_loading_thread != INVALID_HANDLE_VALUE)
		return S_FALSE;

	return E_FAIL;
}

static DWORD WINAPI loadFontThread(LPVOID param)
{
	CAutoLock lck(&g_libass_cs);
	if (NULL == g_ass_library)
		g_ass_library = ass_library_init();

	if (!g_ass_library) 
	{
		printf("ass_library_init() failed!\n");
		return 1;
	}

	ASS_Renderer *ass_renderer = ass_renderer_init(g_ass_library);
	if (!ass_renderer)
	{
		printf("ass_renderer_init() failed!\n");
		return 2;
	}

	char conf[MAX_PATH];
	GetModuleFileNameA(NULL, conf, MAX_PATH);
	for(int i=strlen(conf)-1; i>0; i--)
		if (conf[i] == '\\')
		{
			conf[i] = NULL;
			break;
		}

	strcat(conf, "\\fonts.conf");

	ass_set_frame_size(ass_renderer, 1920, 1080);
	ass_set_font_scale(ass_renderer, 1.0);
	ass_set_fonts(ass_renderer, "Arial", "Sans", 1, conf, 1);
	ass_renderer_done(ass_renderer);


	g_fonts_loaded = true;

	return S_OK;
}


/// WRAP classes


enum command_types
{
	command_loadfile,
	command_loadindex,
	command_adddata,
	command_reset,
	command_nothing,
};

LibassRenderer::LibassRenderer()
{
	m_core = NULL;
	m_fallback = new CAssRendererFallback(NULL, RGB(255,255,255));
	m_exit_flag = (bool*)malloc(sizeof(bool));
	*m_exit_flag = false;
	m_loading_done_flag = false;

	m_thread = CreateThread(NULL, NULL, loading_thread, this, NULL, NULL);
}

LibassRenderer::~LibassRenderer()
{
	delete m_fallback;

	{
		CAutoLock lck(&m_cs);
		if (!m_loading_done_flag)
			*m_exit_flag = true;
	}

	delete m_core;
}

HRESULT LibassRenderer::load_file(wchar_t *filename)												//maybe you don't need this?
{
	void *p = malloc(wcslen(filename)*2+2);
	memcpy(p, filename, wcslen(filename)*2+2);
	m_fallback->load_file(filename);
	return send_command(command_loadfile, p, wcslen(filename)*2+2);
}

HRESULT LibassRenderer::load_index(void *data, int size)
{
	void *p = malloc(size);
	memcpy(p, data, size);
	return send_command(command_loadindex, p, size);
}
HRESULT LibassRenderer::add_data(BYTE *data, int size, int start, int end)
{
	int *p = (int*)malloc(size + sizeof(int) * 2);
	p[0] = start;
	p[1] = end;

	memcpy(p+2, data, size);
	m_fallback->add_data(data, size, start, end);
	return send_command(command_adddata, p, size + sizeof(int) * 2);
}
HRESULT LibassRenderer::get_subtitle(int time, rendered_subtitle *out, int last_time/*=-1*/)			// get subtitle on a time point, 
																									// if last_time != -1, return S_OK = need update, return S_FALSE = same subtitle, and out should be ignored;
{
	CAutoLock lck(&m_cs);
	if (!m_core)
		return m_fallback->get_subtitle(time, out, last_time);

	return m_core->get_subtitle(time, out, last_time);
}
HRESULT LibassRenderer::reset()
{
	m_fallback->reset();

	return send_command(command_reset, NULL, 0);
}


HRESULT LibassRenderer::send_command(int command_type, void *data, int size)
{
	CAutoLock lck(&m_cs);

	if (m_core)
		return execute_command(command_type, data, size);

	command p;

	p.command_type = command_type;
	p.data = data;
	p.size = size;

	m_commands.insert(p);

	return S_OK;
}

HRESULT LibassRenderer::execute_command(int command_type, void *data, int size)
{
	HRESULT hr = E_UNEXPECTED;
	switch(command_type)
	{
	case command_loadfile:
		hr = m_core->load_file((wchar_t*)data);
		break;
	case command_loadindex:
		hr = m_core->load_index(data, size);
		break;
	case command_adddata:
		hr = m_core->add_data(((BYTE*)data)+2*sizeof(int), size-sizeof(int)*2, ((int*)data)[0], ((int*)data)[1]);
		break;
	case command_reset:
		hr = m_core->reset();
		break;
	case command_nothing:
		hr = S_OK;
		break;
	default:
		assert(0);
	}

	return hr;
}

DWORD WINAPI LibassRenderer::loading_thread(LPVOID param)
{
	LibassRenderer *_this = (LibassRenderer *) param;
	bool *exit_flag = _this->m_exit_flag;

	LibassRendererCore *core = new LibassRendererCore();

	if (!*exit_flag)
	{
		CAutoLock (&_this->m_cs);
		_this->m_core = core;
		command *p = _this->m_commands.pop();
		while (p)
		{
			_this->execute_command(p->command_type, p->data, p->size);

			if (p->data)
				free(p->data);
			free(p);
			p = _this->m_commands.pop();
		}
		_this->m_loading_done_flag = true;
	}

	free(exit_flag);

	return 0;
}

HRESULT LibassRenderer::set_font(HFONT newfont)
{
	m_fallback->set_font(newfont);
	return E_NOTIMPL;
}

HRESULT LibassRenderer::set_font_color(DWORD newcolor)
{
	m_fallback->set_font_color(newcolor);
	return E_NOTIMPL;
}