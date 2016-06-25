#include "srt_renderer.h"

HRESULT CsrtRenderer::get_subtitle(int time, rendered_subtitle *out, int last_time/*=-1*/)	// get subtitle on a time point, 
																							// if last_time != -1, return S_OK = need update, return S_FALSE = same subtitle, and out should be ignored;
{
	return m_ass.get_subtitle(time, out, last_time);
}
HRESULT CsrtRenderer::reset()
{
	return m_ass.reset();
}
HRESULT CsrtRenderer::seek()																// just clear current incompleted data, to support dshow seeking.
{
	return m_ass.seek();
}

HRESULT CsrtRenderer::set_font_color(DWORD newcolor)										// color format is 8bit BGRA, use RGB() macro, for text based subtitle only
{
	return m_ass.set_font_color(newcolor);
}
HRESULT CsrtRenderer::set_font(HFONT newfont)											// for text based subtitles, the main program will try show_dlg=false first, if the SubtitleRenderer is not text based, it should return E_NOT_IMPL.
{
	return m_ass.set_font(newfont);
}
HRESULT CsrtRenderer::set_output_aspect(double aspect)									// set the rendered output aspect, to suit movies's aspect
{
	return m_ass.set_output_aspect(aspect);
}


HRESULT CsrtRenderer::load_file(wchar_t *filename)										//maybe you don't need this?
{
	wchar_t tmp[MAX_PATH];
	GetTempPathW(MAX_PATH, tmp);
	wcscat(tmp, L"DWindowSRT2ASS.ass");

	srt_parser p;
	p.init(20480, 2048*1024);
	if (p.load(filename) < 0)
		return E_FAIL;
	if (p.save_as_ass(tmp)<0)
		return E_FAIL;

	HRESULT hr = m_ass.load_file(tmp);
	_wremove(tmp);

	return hr;
}
HRESULT CsrtRenderer::load_index(void *data, int size)
{
	char index_source[18][400] = 
	{
		"[Script Info]",
		"Title:DWindow",
		"Original Script:DWindow",
		"Synch Point:0",
		"ScriptType:v4.00+",
		"Collisions:Norma",
		"PlayResX:640",
		"PlayResY:360",
		"Timer:100.0000",
		"",
		"[V4+ Styles]",
		"Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding",
		"Style: Default,ºÚÌå,25,&H00FFFFFF,&HF0000000,&H00000000,&H80000000,-1,0,0,0,100,100,0,0.00,1,1,1,2,30,30,10,134",
		"",
		"[Events]",
		"Format: Layer, Start, End, Style, Actor, MarginL, MarginR, MarginV, Effect, Text",
		"",
		"",
	};

	char index[2048] = {0};
	for(int i=0; i<18; i++)
	{
		strcat(index, index_source[i]);
		strcat(index, "\r\n");
	}

	HRESULT hr = m_ass.load_index(index, strlen(index));

	return hr;
}

int CsrtRenderer::get_n(int start, int end)
{
	for (std::list<n_table_entry>::iterator it = n_table.begin(); it != n_table.end(); it++)
	{
		if (it->start == start && it->end == end)
			return it->n;
	}

	int o = n++;
	n_table_entry new_n = {o, start, end};
	n_table.push_back(new_n);
	return o;
}

HRESULT CsrtRenderer::add_data(BYTE *data, int size, int start, int end)
{
	if (size >=2 && (data[0] << 8) + data[1] == size-2)
		size -= 2, data += 2;

	if (size <= 0)
		return S_OK;

	char *line = (char*)malloc(size+2);
	memcpy(line, data, size);
	line[size] = NULL;

	// remove <XXX> in the line
	char *l = strstr(line, "<");
	char *r = strstr(line, ">");
	while (l && r && r > l)
	{
		strcpy(l, r+1);
		l = strstr(line, "<");
		r = strstr(line, ">");
	}

	char *p1 = (char*)malloc(size+1024);
	sprintf(p1, "%d,0,Default,,0000,0000,0000,,%s", get_n(start, end), line);

	HRESULT hr = m_ass.add_data((BYTE*)p1, strlen(p1), start, end);

	free(p1);

	return hr;
}