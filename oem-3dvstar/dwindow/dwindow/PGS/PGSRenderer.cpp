#include <stdio.h>
#include <assert.h>
#include "PGSRenderer.h"


PGSRenderer::PGSRenderer()
{
	m_seg_buffer = (BYTE*) malloc(2048000);
	m_seg_buffer_pos = 0;
	m_last_found = -1;
}

PGSRenderer::~PGSRenderer()
{
	free(m_seg_buffer);
}

HRESULT PGSRenderer::load_file(wchar_t *filename)
{
	FILE *f = _wfopen(filename, L"rb");
	if (NULL == f)
		return E_FAIL;

	BYTE *buf = (BYTE*)malloc(32768);
	int read = 0;
	while (read = fread(buf, 1, 32768, f))
		m_parser.add_data(buf, read);
	free(buf);
	fclose(f);

	return S_OK;
}

HRESULT PGSRenderer::add_data(BYTE *data, int size, int start, int end)
{
	HRESULT hr = S_OK;

	if (size + m_seg_buffer_pos >= 2048000)
	{
		// buffer overrun! reset buffer and drop this packet
		m_seg_buffer_pos = 0;
		return S_OK;
	}


	// pack together
	memcpy(m_seg_buffer+m_seg_buffer_pos, data, size);
	m_seg_buffer_pos += size;

	BYTE *tmp = m_seg_buffer;
	int counter = 0;
	while(counter < 100)
	{
		int type = tmp[0];
		int ele_size = (tmp[1] << 8) + tmp[2];
		if (tmp+ele_size+3 > m_seg_buffer + m_seg_buffer_pos)
			break;

		hr = m_parser.parse_raw_element(tmp+3, type, ele_size, start, end);
		if (FAILED(hr))
		{
			m_seg_buffer_pos = 0;	// drop all data recieved, wait for new packets
			return hr;
		}

		tmp += ele_size + 3;
		counter++;
	}

	memmove(m_seg_buffer, tmp, m_seg_buffer_pos - (tmp-m_seg_buffer));
	m_seg_buffer_pos -= (tmp-m_seg_buffer);

	return hr;
}

// get subtitle on a time point, 
// if last_time != -1, return S_OK = need update, return S_FALSE = same subtitle, and out should be ignored;
HRESULT PGSRenderer::get_subtitle(int time, rendered_subtitle *out, int last_time/* =-1 */)
{
	if (NULL == out)
		return E_POINTER;

	pgs_subtitle tmp;
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

HRESULT PGSRenderer::reset()
{
	m_last_found = -1;
	return m_parser.reset();
}

HRESULT PGSRenderer::seek()
{
	m_seg_buffer_pos = 0;
	m_last_found = -1;
	return m_parser.seek();
}