#include "vobsub_renderer.h"

bool wcs_endwith_nocase(const wchar_t *search_in, const wchar_t *search_for);

HRESULT VobSubRenderer::load_file(wchar_t *filename)
{
	return load_file(filename, 0);
}
HRESULT VobSubRenderer::load_file(wchar_t *filename, int langid)
{
	if (wcslen(filename) <= 3)
		return E_FAIL;

	wchar_t * ext = filename + wcslen(filename) - 3;
	if (wcs_endwith_nocase(ext, L"idx"))
	{
		wchar_t sub[1024];
		wcscpy(sub, filename);
		ext = sub + wcslen(sub) - 3;
		wcscpy(ext, L"sub");

		return m_parser.load_file(filename, langid, sub);

	}

	else if (wcs_endwith_nocase(ext, L"sub"))
	{
		wchar_t idx[1024];
		wcscpy(idx, filename);
		ext = idx + wcslen(idx) - 3;
		wcscpy(ext, L"idx");

		return m_parser.load_file(idx, langid, filename);
	}

	return E_FAIL;
}

HRESULT VobSubRenderer::load_index(void *data, int size)
{
	return m_parser.load_file(data, size, 0, NULL);
}

HRESULT VobSubRenderer::add_data(BYTE *data, int size, int start, int end)
{
	return m_parser.add_subtitle(start, end, data, size);
}

HRESULT VobSubRenderer::get_subtitle(int time, rendered_subtitle *out, int last_time/* =-1 */)
{
	if (NULL == out)
		return E_POINTER;

	vobsub_subtitle tmp;
	int found = m_parser.find_subtitle(time, time, &tmp);

	if (found == m_last_found && last_time != -1)
	{
		if (tmp.rgb) free(tmp.rgb);
		out->data = NULL;

		return S_FALSE;
	}
	else
	{
		if (tmp.start > 0 && tmp.end >0 && tmp.rgb)
		{
			out->aspect = (double)m_parser.m_total_width / m_parser.m_total_height;
			out->delta = 0;
			out->delta_valid = false;
			out->data = (BYTE*)tmp.rgb;
			out->pixel_type = out->pixel_type_RGB;
			out->width_pixel = tmp.width;
			out->height_pixel = tmp.height;
			out->left = (double)tmp.left / m_parser.m_total_width;
			out->width = (double)tmp.width / m_parser.m_total_width;
			out->top = (double)tmp.top / m_parser.m_total_height;
			out->height = (double)tmp.height / m_parser.m_total_height;

			// convert from RGBQUAD to D3DFMT_A8R8G8B8
			for(int i=0; i<out->width*out->height; i++)
			{
				RGBQUAD * tmp = (RGBQUAD*)out->data;
				tmp += i;
				((DWORD*)out->data)[i] = (tmp->rgbReserved << 24) | (tmp->rgbRed << 16) | (tmp->rgbGreen << 8) | tmp->rgbBlue;
			}

			m_last_found = found;
		}
		else
		{
			if (tmp.rgb) free(tmp.rgb);
			out->data = NULL;
			return S_OK;
		}
		return S_OK;
	}
}

HRESULT VobSubRenderer::reset()
{
	m_last_found = -1;
	return m_parser.reset();
}

HRESULT VobSubRenderer::seek()
{
	return S_OK;
}