#include <stdio.h>
#include "PGSParser.h"

enum HDMV_SEGMENT_TYPE {
    NO_SEGMENT       = 0xFFFF,
    PALETTE          = 0x14,
    OBJECT           = 0x15,
    PRESENTATION_SEG = 0x16,
    WINDOW_DEF       = 0x17,
    INTERACTIVE_SEG  = 0x18,
    DISPLAY          = 0x80,
    HDMV_SUB1        = 0x81,
    HDMV_SUB2        = 0x82
};

// helper
HRESULT PGSParser::remove_head(int size)
{
	memcpy(m_data, m_data+size, m_data_pos-size);
	m_data_pos -= size;
	return S_OK;
}

DWORD PGSParser::readDWORD(BYTE *data)
{
	return (data[0] << 24) | 
		   (data[1] << 16) |
		   (data[2] << 8)  |
		    data[3];
}

DWORD PGSParser::readDWORD(int pos)
{
	return readDWORD(m_data + pos);
}

WORD PGSParser::readWORD(BYTE *data)
{
	return  (data[0] << 8) | data[1];
}

WORD PGSParser::readWORD(int pos)
{
	return readWORD(m_data + pos);
}

DWORD PGSParser::YUV2RGB(BYTE Y, BYTE Cr, BYTE Cb, BYTE A)
{
	const double Rec709_Kr = 0.2125;
	const double Rec709_Kb = 0.0721;
	const double Rec709_Kg = 0.7154;

    double rp = Y + 2*(Cr-128)*(1.0-Rec709_Kr);
    double gp = Y - 2*(Cb-128)*(1.0-Rec709_Kb)*Rec709_Kb/Rec709_Kg - 2*(Cr-128)*(1.0-Rec709_Kr)*Rec709_Kr/Rec709_Kg;
    double bp = Y + 2*(Cb-128)*(1.0-Rec709_Kb);

	
	const double const2 = 255.0/219.0;

	rp = (rp-16)*const2 + 0.5;
	gp = (gp-16)*const2 + 0.5;
	bp = (bp-16)*const2 + 0.5;

	if (rp>255)
		rp = 255;
	if (gp>255)
		gp = 255;
	if (bp>255)
		bp = 255;

	if (rp<0)
		rp = 0;
	if (gp<0)
		gp = 0;
	if (bp<0)
		bp = 0;
	

	//A<<24, R<<16, G<<8, B
    return (A<<24) | ((BYTE)rp << 16) | ((BYTE)gp << 8) | ((BYTE)bp);
}


HRESULT PGSParser::add_data(BYTE *data, int size)
{
	if (m_data_pos + size >= 2048000)
	{
		// buffer overrun, reset buffer and drop this packet
		m_data_pos = 0;
		return S_OK;
	}

	memcpy(m_data + m_data_pos, data, size);
	m_data_pos += size;

	parse();
	return S_OK;
}
HRESULT PGSParser::find_subtitle(int start, int end, pgs_subtitle *out) // in ms
{
	if (NULL == out)
		return E_POINTER;

	out->start = out->end = -1;
	for(int i=0; i<m_subtitle_count; i++)
	{
		pgs_subtitle &sub = m_subtitles[i];
		int delta = sub.window_width - sub.width;
		if (( sub.start <= start && start <= sub.end) ||
			(sub.start <= end && end <= sub.end))
		{
			*out = sub;
			decodeRLE(out);
			out->rle = NULL;

			return i;
		}
	}

	out->rgb = NULL;
	return E_FAIL;
}
HRESULT PGSParser::parse()
{
	while(true)
	{
		// find header
		if (m_data_pos<2)
			return S_OK;

		while(m_data[0] != 80 || m_data[1] != 0x47)
		{
			if (m_data_pos<2)
				return S_OK;
			else
				remove_head(1);
		}

		// test if enough header data
		if (m_data_pos < 2+4+4+1+2) // header(2) + timeStart(4) + timeEnd(4) + type(1) + seg_len(2)
			return S_OK;

		DWORD time_start = readDWORD(2)/90;
		DWORD time_end = readDWORD(6)/90;
		BYTE type = m_data[10];
		WORD size = readWORD(11);

		// enough data?
		if (m_data_pos < 2+4+4+1+2+size)
			return S_OK;

		BYTE *data = m_data + 2+4+4+1+2;
		
		parse_raw_element(data, type, size, time_start, time_end);
		remove_head(2+4+4+1+2+size);
	}
}

HRESULT PGSParser::parse_raw_element(BYTE *data, int type, int size, int start, int end)
{
	/*
	static char * un = "Unkown";
	char* tbl[256];
	for(int i=0; i<256; i++)
		tbl[i] = un;
	tbl[PRESENTATION_SEG] = "PRESENTATION_SEG";
	tbl[WINDOW_DEF] = "WINDOW_DEF";
	tbl[PALETTE] = "PALETTE";
	tbl[OBJECT] = "OBJECT";
	tbl[DISPLAY] = "DISPLAY";

	static FILE * f = fopen("Z:\\avt.txt", "wb");
	if(f)fprintf(f, "(#%d)type=%02x(%s), size=%d\r\n", m_subtitle_count, type, tbl[type], size);
	*/

	if (start == 0 || end == 0)
	{
		printf("0:0!.\n");
	}

	HRESULT hr = E_FAIL;		// we use E_FAILT here, to prevent error packets
	if (type == PRESENTATION_SEG)
		hr = parsePresentaionSegment(data, size, start);
	else if (type == WINDOW_DEF)
		hr = parseWindow(data, size);
	else if (type == PALETTE)
		hr = parsePalette(data, size);
	else if (type == OBJECT)
		hr = parseObject(data, size);
	else if (type == DISPLAY)
		hr = parseDisplay(data, size, start);
	else if (type == INTERACTIVE_SEG)
		hr = S_OK;
	else if (type == HDMV_SUB1)
		hr = S_OK;
	else if (type == HDMV_SUB2)
		hr = S_OK;
	//else
	//	printf("type=%02x, size=%d\n", type, size);

	return hr;
}

HRESULT PGSParser::parsePresentaionSegment(BYTE *data, int size, int time)
{
	if (m_current_subtitle.start != -1 && m_current_subtitle.rle &&
		0 < m_current_subtitle.width && m_current_subtitle.width < 4096 &&
		0 < m_current_subtitle.height && m_current_subtitle.height < 4096)
	{
		m_current_subtitle.end = time;

		// find duplicate
		for(int i=0; i<m_subtitle_count; i++)
			if (abs(m_subtitles[i].start - m_current_subtitle.start) < 10
			  &&abs(m_subtitles[i].end - m_current_subtitle.end) < 10)
			{
				if (m_current_subtitle.rle)
				{
					free(m_current_subtitle.rle);
					memset(&m_current_subtitle, 0, sizeof(m_current_subtitle));
				}
				return S_FALSE;
			}

		if (m_current_subtitle.rle_size == 0)
		{
			printf("rle_size = 0\n");
		}

		// add to tail
		m_subtitles[m_subtitle_count++] = m_current_subtitle;

		/*
		static FILE * f = fopen("Z:\\sup.log", "wb");
		fprintf(f, "sup %d, rle size = %d, %dx%d, %d - %d\r\n", m_subtitle_count, m_current_subtitle.rle_size, m_current_subtitle.width, m_current_subtitle.height,
			m_current_subtitle.start, m_current_subtitle.end);
		fflush(f);
		*/
	}
	else
	{
		int n = 0;	//debug
	}
	memset(&m_current_subtitle, 0, sizeof(pgs_subtitle));
	m_current_subtitle.start = -1;
	m_possible_start = time;
	m_has_seg = true;

	if (size >= 4)
	{
		m_total_width = (data[0] << 8) + data[1];
		m_total_height =(data[2] << 8) + data[3];
	}
	return S_OK;
}

HRESULT PGSParser::parseWindow(BYTE *data, int size)
{
	if (!m_has_seg)
		return S_OK;

	if (*data == 0 || size < 10)
		return S_OK;

	m_current_subtitle.window_left = readWORD(data+2);
	m_current_subtitle.window_top = readWORD(data+4);
	m_current_subtitle.window_width = readWORD(data+6);
	m_current_subtitle.window_height = readWORD(data+8);

	return S_OK;
}

HRESULT PGSParser::parsePalette(BYTE *data, int size)
{
	if (!m_has_seg)
		return S_OK;

	BYTE palette_id = data[0];
	BYTE palette_version = data[1];

	int n_colors = (size-2)/5;
	BYTE *tmp = data+2;
	for(int i=0; i<n_colors; i++)
	{
		if (tmp[0] >= 256 || n_colors >= 256)
		{
			printf("bad palette.\n");
			return E_FAIL;
		}

		m_current_subtitle.palette[tmp[0]] = YUV2RGB(tmp[1], tmp[2], tmp[3], tmp[4]);
		tmp += 5;
	}

	return S_OK;
}

HRESULT PGSParser::parseObject(BYTE *data, int size)
{
	if (!m_has_seg)
		return S_OK;

	BYTE seq_desc = data[3];
	DWORD n_data_size;
	n_data_size = data[4] + readWORD(data+5);
	if (m_current_subtitle.rle_size == 0)
	{
		WORD object_id = readWORD(data);
		BYTE version_number = data[2];
		BYTE seq_desc = data[3];
		BYTE n_data_size[3];
		memcpy(n_data_size, data+4, 3);
		DWORD data_size = (n_data_size[0] << 16) + (n_data_size[1] << 8) + n_data_size[2];
		m_current_subtitle.width = readWORD(data+7);
		m_current_subtitle.height = readWORD(data+9);
		m_current_subtitle.left = m_current_subtitle.window_left + (m_current_subtitle.window_width - m_current_subtitle.width)/2;
		m_current_subtitle.top = m_current_subtitle.window_top + (m_current_subtitle.window_height - m_current_subtitle.height)/2;

		// copy to rle buffer;
		m_current_subtitle.rle_size = size-11;
		m_current_subtitle.rle = (BYTE*) malloc(size-11);
		memcpy(m_current_subtitle.rle, data+11, size-11);
	}
	else
	{
		// append to rle buffer;
		int last_size = m_current_subtitle.rle_size;
		m_current_subtitle.rle_size += size-4;
		m_current_subtitle.rle = (BYTE*) realloc(m_current_subtitle.rle, m_current_subtitle.rle_size);
		memcpy(m_current_subtitle.rle + last_size, data+4, size-4);
	}
	return S_OK;
}


HRESULT PGSParser::parseDisplay(BYTE *data, int size, int time)
{
	if (!m_has_seg)
		return S_OK;

	m_current_subtitle.start = m_possible_start;

	return S_OK;
}

HRESULT PGSParser::decodeRLE(pgs_subtitle *sub)
{
	// assume palette is ready;

	BYTE *rle = sub->rle;
	if (!rle)
		return E_FAIL;

	DWORD *decoded = sub->rgb = (DWORD*) malloc(sub->width * sub->height*4);
	if (decoded == NULL)
		return E_FAIL;	// may happen when width and height is incorrect(maybe file corrupted.)

	// decode data;
	int pos = 0;
	int idx = 0;
	int xpos = 0;
	do
	{
		int b = rle[idx++];
		int n_pixel = 0;
		int color = 0;
		if (b ==0)
		{
			b = rle[idx++];
			if (b == 0)
			{
				// 00 00 = new line
				n_pixel = 0;
				if (xpos < sub->width)
					pos += sub->width - (pos% sub->width);
				else
					pos -= pos % sub->width;
				xpos = 0;
			}
			else if ((b & 0xC0) == 0x40)
			{
				// 00 4x xx -> x xx pixel of zero
				n_pixel = ((b-0x40)<<8) + rle[idx++];
				color = 0;
			}
			else if ((b & 0xC0) == 0x80)
			{
				// 00 8x yy -> x pixels of yy
				n_pixel = b - 0x80;
				color = rle[idx++];
			}
			else if ((b & 0xC0) !=0)
			{
				// 00 cx yy zz -> xyy pixels of zz
				n_pixel = ((b-0xC0) <<8) + rle[idx++];
				color = rle[idx++];
			}
			else
			{
				n_pixel = b;
				// 00 xx -> xx pixels of 0
			}

		}
		else
		{
			// xx -> 1 pixel of xx
			n_pixel = 1;
			color = b;
		}

		xpos += n_pixel;
		for(int i=0; i<n_pixel; i++)
		{
			if (pos>=sub->width * sub->height)
			{
				printf("possible bad PGS object data or additional bytes, ignored.\n");
				return S_OK;
			}

			decoded[pos++] = sub->palette[color];
		}
	} while (pos < sub->width * sub->height && idx < sub->rle_size);

	return S_OK;
}


PGSParser::PGSParser()
{
	m_data = NULL;
	m_subtitles = NULL;
	reset();
}
PGSParser::~PGSParser()
{
	if (m_data)
		free(m_data);
	if (m_subtitles)
	{
		for(int i=0; i<40960; i++)
		{
			if (m_subtitles[i].rgb)
				free(m_subtitles[i].rgb);
			if (m_subtitles[i].rle)
				free(m_subtitles[i].rle);
		}
		free(m_subtitles);
	}
}

HRESULT PGSParser::seek()	// just clear current incompleted data, to support dshow seeking.
{
	m_data_pos = 0;
	memset(&m_current_subtitle, 0, sizeof(pgs_subtitle));
	m_current_subtitle.start = -1;
	m_has_seg = false;
	return S_OK;
}

HRESULT PGSParser::reset()
{
	// free first
	if (m_data)
	{
		free(m_data);
		m_data = NULL;
	}

	if (m_subtitles)
	{
		for(int i=0; i<40960; i++)
			if (m_subtitles[i].rgb)
				free(m_subtitles[i].rgb);
		free(m_subtitles);
		m_subtitles = NULL;
	}

	if (NULL == m_data)
		m_data = (BYTE*) malloc(2048000);
	if (NULL == m_subtitles)
		m_subtitles = (pgs_subtitle*) malloc(40960*sizeof(pgs_subtitle));
	m_data_pos = 0;
	m_subtitle_count = 0;
	m_has_seg = false;
	memset(&m_current_subtitle, 0, sizeof(pgs_subtitle));
	memset(m_subtitles, 0, 40960*sizeof(pgs_subtitle));
	m_current_subtitle.start = -1;

	return S_OK;
}