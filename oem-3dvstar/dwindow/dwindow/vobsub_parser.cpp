#include "vobsub_renderer.h"
#include "global_funcs.h"

typedef struct subpal_struct{
	BYTE pal: 4, tr: 4;
} SubPal;

#define GetNibble(x) DummyGetNibble(x, nOffset, nPlane, fAligned)
BYTE DummyGetNibble(BYTE* lpData, WORD *nOffset, WORD nPlane, char &fAligned)
{
	WORD& off = nOffset[nPlane];
	BYTE ret = (lpData[off] >> (fAligned << 2)) & 0x0f;
	fAligned = !fAligned;
	off += fAligned;
	return(ret);
}

int VobSubParser::handle_data_16(unsigned short *data, bool big, int size)
{
	wchar_t line_w[1024];
	int p = 0;

	if(big)
		for (int i=0; i<size; i++)
		{
			wchar_t c = data[i];
			c = ((c&0xff) << 8) | ((c&0xff00) >> 8);

			if (c != 0xA && c != 0xD && p<1024)
				line_w[p++] = c;
			else
			{
				if(c == 0xD)
				{
					line_w[p] = NULL;
					wcstrim(line_w);

					handle_line(line_w);
				}
				else
					p = 0;
			}
		}
	else//little
		for (int i=0; i<size/2; i++)
		{
			wchar_t c = data[i];
			if (c != 0xA && c != 0xD && p<1024)
				line_w[p++] = c;
			else
			{
				if(c == 0xD)
				{
					line_w[p] = NULL;
					wcstrim(line_w);

					handle_line(line_w);
				}
				else
					p = 0;
			}
		}

		return 0;
}

int VobSubParser::handle_data_8(unsigned char *data, int code_page, int size)
{
	char line[1024];
	wchar_t line_w[1024];
	int p = 0;
	for (int i=0; i<size; i++)
	{
		if (data[i] != 0xA && data[i] != 0xD && p<1024)
			line[p++] = data[i];
		else
		{
			if(data[i] == 0xA || data[i] == 0xD)
			{
				line[p] = NULL;
				MultiByteToWideChar(code_page, 0, line, 1024, line_w, 1024);
				wcstrim(line_w);

				handle_line(line_w);
				p=0;
			}
			else
				p = 0;
		}
	}

	return 0;
}

HRESULT VobSubParser::load_file(void *index_data, int size, int lang_id, const wchar_t *sub_filename)			// set sub_filename = NULL to use in directshow
																					// which means sub is streamed.
{
	reset();
	unsigned char *data = (unsigned char *)index_data;

	if (lang_id>=0)
	{
		m_lang_id = lang_id;
		m_first_line = false;
		m_bad_index = false;
		m_total_width = 720;
		m_total_height = 480;
		m_scale_x = 1.0;
		m_scale_y = 1.0;
		m_global_time_offset = 0;
		m_use_custom_color = false;
	}

	if (data[0] == 0xFE && data[1] == 0xFF && data[3] != 0x00)				//UTF-16 Big Endian
		handle_data_16((unsigned short*)(data+2), true, (int)size-2);
	else if (data[0] == 0xFF && data[1] == 0xFE && data[2] != 0x00)			//UTF-16 Little Endian
		handle_data_16((unsigned short*)(data+2), false, (int)size-2);
	else if (data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF)			//UTF-8
		handle_data_8 (data+3, CP_UTF8, (int)size-3);
	else																	//Unknown BOM
		handle_data_8 (data, CP_ACP, (int)size);

	return 0;
}

HRESULT VobSubParser::load_file(const wchar_t *idx_filename, int lang_id, const wchar_t *sub_filename)
{
	FILE * f = _wfopen(idx_filename, L"rb");
	if (NULL == f) return E_FAIL;

	if (sub_filename)
	{
		m_sub_file = _wfopen(sub_filename, L"rb");
		if (!m_sub_file)
			return E_FAIL;
	}
	else
		m_sub_file = NULL;

	// read data
	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);
	fseek(f, 0, SEEK_SET);
	unsigned char *data = new unsigned char[size];
	fread(data, 1, size, f);
	fclose(f);

	int o = load_file(data, size, lang_id, sub_filename);
	delete [] data;

	if (m_sub_file)
	{
		fclose(m_sub_file);
		m_sub_file = NULL;
	}

	return o;
}

void wcslower(wchar_t *str)
{
	while(*str)
	{
		*str = towlower(*str);
		str++;
	}
}

int VobSubParser::handle_line(wchar_t *line)
{
	//# VobSub index file, v7 (do not modify this line!)
	//size: 720x576
	//(ignored)org: 0, 0
	//scale: 100%, 100%
	//(ignored)alpha: 100%
	//(ignored)smooth: OFF
	//(ignored)fadein/out: 50, 50
	//(ignored)align: OFF at LEFT TOP
	//time offset: 0
	//delay: 0
	//(ignored)forced subs: OFF
	//palette: 000000, 828282, 828282, 828282, 828282, 828282, 828282, ffffff, 828282, bababa, 828282, 828282, 828282, 828282, 828282, 828282
	//custom colors: OFF, tridx: 1000, colors: 000000, bababa, 828282, 000000
	//langidx: 0
	//id: en, index: 0
	//timestamp: 00:01:25:880, filepos: 000003800
	//...

	// max language id scanning
	if (m_scan_for_lang_id)
	{
		if (wcsstr(line, L"id:") && wcsstr(line, L"index:"))
		{
			wchar_t id[1024];
			wcscpy(id, wcsstr(line, L"index:") + wcslen(L"index:"));
			wcstrim(id);

			int iid = _wtoi(id);
			if (iid > m_max_lang_id && iid<32)
				m_max_lang_id = iid;
		}
	}

	// normal scanning
	else
	{
		if (m_first_line)
		{
			m_first_line = false;
			if (!wcsstr(line, L"VobSub index file"))
				m_bad_index = true;
		}
		else if (line[0] == L'#' || line[0] == NULL)
		{
			// comments
		}
		else
		{
			wchar_t *out[2];
			wcsexplode(line, L":", out, 2);
			wchar_t *entry = out[0];
			wchar_t *str = out[1];
			wcstrim(entry);
			wcstrim(str);
			wcslower(entry);
			wcslower(str);

			//size: 720x576
			if (wcsstr(entry, L"size"))
			{
				if (swscanf(str, L"%dx%d", &m_total_width, &m_total_height) != 2)
					m_bad_index = true;
			}

			//scale: 100%, 100%
			else if (wcsstr(entry, L"scale"))
			{
				if (swscanf(str, L"%d%%,%d%%", &m_scale_x, &m_scale_y) != 2)
					m_bad_index = true;

			}

			//time offset: 0
			//delay: 0
			else if (wcsstr(entry, L"time offset") || wcsstr(entry, L"delay"))
			{

			}

			//palette: 000000, 828282, 828282, 828282, 828282, 828282, 828282, ffffff, 828282, bababa, 828282, 828282, 828282, 828282, 828282, 828282
			else if (wcsstr(entry, L"palette"))
			{
				if(swscanf(str, L"%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x",
					&m_orgpal[0], &m_orgpal[1], &m_orgpal[2], &m_orgpal[3],
					&m_orgpal[4], &m_orgpal[5], &m_orgpal[6], &m_orgpal[7],
					&m_orgpal[8], &m_orgpal[9], &m_orgpal[10], &m_orgpal[11],
					&m_orgpal[12], &m_orgpal[13], &m_orgpal[14], &m_orgpal[15]
				) != 16) 
					m_bad_index = true;
			}

			//custom colors: OFF, tridx: 1000, colors: 000000, bababa, 828282, 000000
			else if (wcsstr(entry, L"custom colors"))
			{
				if(wcsstr(str,L"on") == str || wcsstr(str, L"1") == str)
					m_use_custom_color = true;
				else if(wcsstr(str,L"off") == str || wcsstr(str, L"0") == str)
					m_use_custom_color = false;
				else
					m_bad_index = true;

				wchar_t *s_tridx = wcsstr(str, L"tridx:") + wcslen(L"tridx:");
				int tridx;
				if (swscanf(s_tridx, L"%x", &tridx) != 1)
					m_bad_index = true;

				wchar_t *s_color = wcsstr(str, L"colors:") + wcslen(L"colors:");
				RGBQUAD pal[4];
				if(swscanf(s_color, _T("%x,%x,%x,%x"), &pal[0], &pal[1], &pal[2], &pal[3]) != 4)
					m_bad_index = true;

				memcpy(m_cuspal, pal, sizeof(RGBQUAD)*4);
				tridx = tridx & 0xf;
				for(ptrdiff_t i = 0; i < 4; i++)
					m_cuspal[i].rgbReserved = (tridx&(1<<i)) ? 0 : 0xff;

			}

			//langidx: 0

			//id: en, index: 0
			else if (wcsstr(entry, L"id"))
			{

			}

			//timestamp: 00:01:25:880, filepos: 000003800
			else if (wcsstr(entry, L"timestamp"))
			{
				DWORD pos;
				wchar_t *s_filepos = wcsstr(str, L"filepos:") + wcslen(L"filepos:");
				wcstrim(s_filepos);
				if (swscanf(s_filepos, L"%x", &pos) != 1)
					m_bad_index = true;

				str[12] = NULL;

				memset(&m_current_subtitle, 0, sizeof(m_current_subtitle));
				m_current_subtitle.start = time_to_decimal(str);
				if (m_sub_file)
				{
					if (SUCCEEDED(read_packet_from_subfile(m_sub_file, pos)))
						m_subtitles[m_subtitle_count++] = m_current_subtitle;
				}

			}

			free(entry);
			free(str);
		}

	}


	return 0;
}

bool VobSubParser::wisgidit(wchar_t *str)
{
	size_t len = wcslen(str);
	for (size_t i=0; i<len; i++)
		if (str[i] < L'0' || str[i] > L'9')
			return false;
	return true;
}

int VobSubParser::time_to_decimal(wchar_t *str)
{
	//11:22:33,444

	if (wcslen(str) != 12)
		return -1;

	int h,m,s,ms;
	if (str[8] == L',')
		swscanf(str, L"%d:%d:%d,%d", &h,&m,&s,&ms);
	else if (str[8] == L'.')
		swscanf(str, L"%d:%d:%d.%d", &h,&m,&s,&ms);
	else if (str[8] == L':')
		swscanf(str, L"%d:%d:%d:%d", &h,&m,&s,&ms);

	return h*3600000 + m*60000 +s*1000 +ms;
}

int VobSubParser::max_lang_id(const wchar_t *idx_filename)
{
	m_max_lang_id = -1;
	m_scan_for_lang_id = true;

	// read data
	FILE * f = _wfopen(idx_filename, L"rb");
	if (NULL == f) return -1;
	fseek(f, 0, SEEK_END);
	size_t size = ftell(f);
	fseek(f, 0, SEEK_SET);
	unsigned char *data = new unsigned char[size];
	fread(data, 1, size, f);
	fclose(f);

	int o = load_file(data, size, -1, NULL);
	delete [] data;
	m_scan_for_lang_id = false;

	return m_max_lang_id;
}


HRESULT VobSubParser::reset()
{
	if (m_subtitles)
	{
		for(int i=0; i<40960; i++)
		{
			if (m_subtitles[i].rgb)
				free(m_subtitles[i].rgb);
			if (m_subtitles[i].packet)
				free(m_subtitles[i].packet);
		}
		free(m_subtitles);
		m_subtitles = NULL;
	}
	if (m_current_subtitle.rgb)
		free(m_current_subtitle.rgb);
	if (m_current_subtitle.packet)
		free(m_current_subtitle.packet);
	memset(&m_current_subtitle, 0, sizeof(m_current_subtitle));

	if (NULL == m_subtitles)
		m_subtitles = (vobsub_subtitle*) malloc(40960*sizeof(vobsub_subtitle));

	m_subtitle_count = 0;
	memset(&m_current_subtitle, 0, sizeof(vobsub_subtitle));
	memset(m_subtitles, 0, 40960*sizeof(vobsub_subtitle));
	m_current_subtitle.start = -1;

	return S_OK;
}
VobSubParser::VobSubParser()
{
	memset(&m_current_subtitle, 0 , sizeof(m_current_subtitle));
	m_subtitles = NULL;
	m_scan_for_lang_id = false;
	reset();
}
VobSubParser::~VobSubParser()
{
	if (m_subtitles)
	{
		for(int i=0; i<40960; i++)
		{
			if (m_subtitles[i].rgb)
				free(m_subtitles[i].rgb);
			if (m_subtitles[i].packet)
				free(m_subtitles[i].packet);
		}
		free(m_subtitles);
	}

	if (m_current_subtitle.rgb)
		free(m_current_subtitle.rgb);
	if (m_current_subtitle.packet)
		free(m_current_subtitle.packet);
}

HRESULT VobSubParser::read_packet_from_subfile(FILE *f, DWORD pos)
{
	fseek(f, pos, SEEK_SET);
	BYTE buff[0x800];
	if(fread(buff,1,sizeof(buff), f) != sizeof(buff))
	{
		printf("read error.\n");
		return E_FAIL;
	}

	if(*(DWORD*)&buff[0x00] != 0xba010000
		|| *(DWORD*)&buff[0x0e] != 0xbd010000
		|| !(buff[0x15] & 0x80)
		|| (buff[0x17] & 0xf0) != 0x20
		|| (buff[buff[0x16] + 0x17] & 0xe0) != 0x20
		|| (buff[buff[0x16] + 0x17] & 0x1f) != m_lang_id) 
	{
		return E_FAIL;
	}

	int packetsize = m_current_subtitle.packetsize = (buff[buff[0x16] + 0x18] << 8) + buff[buff[0x16] + 0x19];
	int datasize = m_current_subtitle.datasize = (buff[buff[0x16] + 0x1a] << 8) + buff[buff[0x16] + 0x1b];

	if (m_current_subtitle.packet)
		free(m_current_subtitle.packet);
	m_current_subtitle.packet = (BYTE*)malloc(packetsize);

	int i = 0, sizeleft = packetsize;
	for(ptrdiff_t size; i < packetsize; i += size, sizeleft -= size) 
	{
		int hsize = 0x18 + buff[0x16];
		size = min(sizeleft, 0x800 - hsize);
		memcpy(&m_current_subtitle.packet[i], &buff[hsize], size);

		if(size != sizeleft) 
		{
			while(fread(buff,1,sizeof(buff),f)) 
			{
				if(/*!(buff[0x15] & 0x80) &&*/ buff[buff[0x16] + 0x17] == (m_lang_id|0x20))
						break;
			}
		}
	}

	// find time length
	int nextctrlblk = datasize;
	m_current_subtitle.end = m_current_subtitle.start;
	BYTE *lpData = m_current_subtitle.packet;

	do 
	{
		i = nextctrlblk;

		int t = (lpData[i] << 8) | lpData[i+1];
		i += 2;
		nextctrlblk = (lpData[i] << 8) | lpData[i+1];
		i += 2;

		if(nextctrlblk > packetsize || nextctrlblk < datasize)
			return E_FAIL;

		bool fBreak = false;

		while(!fBreak) 
		{
			int len = 0;

			switch(lpData[i]) 
			{
			case 0x00:
				len = 0;
				break;
			case 0x01:
				len = 0;
				break;
			case 0x02:
				len = 0;
				break;
			case 0x03:
				len = 2;
				break;
			case 0x04:
				len = 2;
				break;
			case 0x05:
				len = 6;
				break;
			case 0x06:
				len = 4;
				break;
			default:
				len = 0;
				break;
			}

			if(i+len >= packetsize) 
			{
				//TRACE(_T("Warning: Wrong subpicture parameter block ending\n"));
				break;
			}

			switch(lpData[i++]) 
			{
				case 0x02: // stop displaying
					m_current_subtitle.end = 1024 * t / 90;
					m_current_subtitle.end += m_current_subtitle.start;
					break;
				case 0xff: // end of ctrlblk
					fBreak = true;
					continue;
				default: // skip this ctrlblk
					fBreak = true;
					break;
			}
		}
	} while(i <= nextctrlblk && i < packetsize);

	return packetsize;
}

HRESULT VobSubParser::decode(vobsub_subtitle *sub)
{
	BYTE *lpData = sub->packet;
	int packetsize = sub->packetsize;
	int datasize = sub->datasize;

	// GetPacketInfo
	int i, nextctrlblk = datasize;
	WORD pal = 0, tr = 0;
	bool fForced = false;
	int delay = 0;
	RECT rect;
	WORD nOffset[2], nPlane = 0;

	SubPal subpal[4];
	do 
	{
		i = nextctrlblk;

		int t = (lpData[i] << 8) | lpData[i+1];
		i += 2;
		nextctrlblk = (lpData[i] << 8) | lpData[i+1];
		i += 2;

		if(nextctrlblk > packetsize || nextctrlblk < datasize)
			return E_FAIL;

		bool fBreak = false;

		while(!fBreak) 
		{
			int len = 0;

			switch(lpData[i]) 
			{
				case 0x00:
					len = 0;
					break;
				case 0x01:
					len = 0;
					break;
				case 0x02:
					len = 0;
					break;
				case 0x03:
					len = 2;
					break;
				case 0x04:
					len = 2;
					break;
				case 0x05:
					len = 6;
					break;
				case 0x06:
					len = 4;
					break;
				default:
					len = 0;
					break;
			}

			if(i+len >= packetsize) 
			{
				//TRACE(_T("Warning: Wrong subpicture parameter block ending\n"));
				break;
			}

			switch(lpData[i++]) {
				case 0x00: // forced start displaying
					fForced = true;
					break;
				case 0x01: // start displaying
					fForced = false;
					break;
				case 0x02: // stop displaying
					delay = 1024 * t / 90;
					break;
				case 0x03:
					pal = (lpData[i] << 8) | lpData[i+1];
					i += 2;
					break;
				case 0x04:
					tr = (lpData[i] << 8) | lpData[i+1];
					i += 2;
					//tr &= 0x00f0;
					break;
				case 0x05:
					rect.left =  (lpData[i] << 4) + (lpData[i+1] >> 4);
					rect.top = (lpData[i+3] << 4) + (lpData[i+4] >> 4);
					rect.right = ((lpData[i+1] & 0x0f) << 8) + lpData[i+2] + 1,
						rect.bottom = ((lpData[i+4] & 0x0f) << 8) + lpData[i+5] + 1;
					i += 6;
					break;
				case 0x06:
					nOffset[0] = (lpData[i] << 8) + lpData[i+1];
					i += 2;
					nOffset[1] = (lpData[i] << 8) + lpData[i+1];
					i += 2;
					break;
				case 0xff: // end of ctrlblk
					fBreak = true;
					continue;
				default: // skip this ctrlblk
					fBreak = true;
					break;
			}
		}
	} while(i <= nextctrlblk && i < packetsize);

	for(i = 0; i < 4; i++) 
	{
		subpal[i].pal = (pal >> (i << 2)) & 0xf;
		subpal[i].tr = (tr >> (i << 2)) & 0xf;
	}

	// Decode()
	//int tridx;

	char fAligned = 1;
	POINT p = {rect.left, rect.top};
	int pixel_count = (rect.right-rect.left)*(rect.bottom-rect.top);
	RGBQUAD *lpPixels = (RGBQUAD*) (sub->rgb = (DWORD *) malloc(pixel_count * sizeof(RGBQUAD)));
	memset(lpPixels, 0, (rect.right-rect.left)*(rect.bottom-rect.top));


	while((nPlane == 0 && nOffset[0] < nOffset[1]) || (nPlane == 1 && nOffset[1] < datasize)) 
	{
		DWORD code;
		if((code = GetNibble(lpData)) >= 0x4
			|| (code = (code << 4) | GetNibble(lpData)) >= 0x10
			|| (code = (code << 4) | GetNibble(lpData)) >= 0x40
			|| (code = (code << 4) | GetNibble(lpData)) >= 0x100) 
		{
			//DrawPixels(p, code >> 2, code & 3);
			RGBQUAD* ptr = &lpPixels[(rect.right - rect.left) * (p.y - rect.top) + (p.x - rect.left)];
			DWORD colorid = code&3;
			RGBQUAD c;
			if(!m_use_custom_color) 
			{
				c = m_orgpal[subpal[colorid].pal];
				c.rgbReserved = (subpal[colorid].tr<<4)|subpal[colorid].tr;
			} 
			else
				c = m_cuspal[colorid];

			for(DWORD i=0; i < (code >> 2) && (ptr+i<lpPixels+pixel_count); i++)
				ptr[i] = c;


			if((p.x += code >> 2) < rect.right)
				continue;
		}

		//DrawPixels(p, rect.right - p.x, code & 3);
		RGBQUAD* ptr = &lpPixels[(rect.right - rect.left) * (p.y - rect.top) + (p.x - rect.left)];
		DWORD colorid = code&3;
		RGBQUAD c;
		if(!m_use_custom_color) 
		{
			c = m_orgpal[subpal[colorid].pal];
			c.rgbReserved = (subpal[colorid].tr<<4)|subpal[colorid].tr;
		} 
		else
			c = m_cuspal[colorid];

		for(int i=0; i<rect.right - p.x && (ptr+i<lpPixels+pixel_count); i++)
			ptr[i] = c;

		if(!fAligned)
			GetNibble(lpData);    // align to byte

		p.x = rect.left;
		p.y++;
		nPlane = 1 - nPlane;

		if (p.y >= rect.bottom)
			break;
	}

	rect.bottom = min(p.y, rect.bottom);

	sub->left = rect.left;
	sub->top = rect.top;
	sub->width = rect.right - rect.left;
	sub->height = rect.bottom - rect.top;

	return S_OK;
}

HRESULT VobSubParser::add_subtitle(int start, int end, void *data, int size)
{
	// search for existing subtitle
	for(int i=0; i<m_subtitle_count; i++)
		if (abs(m_subtitles[i].start - start) < 5)
			return S_OK;

	vobsub_subtitle subtitle;
	memset(&subtitle, 0, sizeof(subtitle));
	subtitle.start = start;
	subtitle.end = end;
	subtitle.packetsize = size;
	subtitle.packet = (BYTE*)malloc(size);
	memcpy(subtitle.packet, data, size);

	if(size <= 4 || ((subtitle.packet[0]<<8)|subtitle.packet[1]) != size) 
		return S_OK;

	subtitle.packetsize = (subtitle.packet[0]<<8)|subtitle.packet[1];
	subtitle.datasize = (subtitle.packet[2]<<8)|subtitle.packet[3];

	m_subtitles[m_subtitle_count++] = subtitle;
	return S_OK;
}

HRESULT VobSubParser::find_subtitle(int start, int end, vobsub_subtitle *out) // in ms
{
	if (NULL == out)
		return E_POINTER;

	out->start = out->end = -1;
	for(int i=0; i<m_subtitle_count; i++)
	{
		vobsub_subtitle &sub = m_subtitles[i];
		if (( sub.start <= start && start <= sub.end) ||
			(sub.start <= end && end <= sub.end))
		{
			*out = sub;
			decode(out);
			out->packet = NULL;

			return i;
		}
	}

	out->rgb = NULL;
	return E_FAIL;
}
