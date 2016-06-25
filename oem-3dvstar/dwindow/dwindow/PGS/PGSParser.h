#pragma once
#include <windows.h>

typedef struct pgs_subtitle_struct
{
	// time
	int start;
	int end;

	// "composition window"
	int window_left;
	int window_top;
	int window_width;
	int window_height;

	// object position
	int left;
	int top;
	int width;
	int height;

	// object data
	int rle_size;
	DWORD palette[256];
	BYTE *rle;
	DWORD *rgb;
} pgs_subtitle;

class PGSParser
{
public:
	PGSParser();
	~PGSParser();
	HRESULT add_data(BYTE *data, int size);
	HRESULT find_subtitle(int start, int end, pgs_subtitle *out); // in ms
	HRESULT seek();	// just clear current incompleted data, to support dshow seeking.
	HRESULT reset();
	HRESULT parse_raw_element(BYTE *data, int type, int size, int start, int end);	//for dshow, don't use this directly in file mode
	int m_total_width;
	int m_total_height;

protected:
	BYTE * m_data;
	int m_data_pos;
	pgs_subtitle *m_subtitles;
	int m_subtitle_count;
	int m_possible_start;

	pgs_subtitle m_current_subtitle;

	bool m_has_seg;

	HRESULT parse();
	HRESULT parsePalette(BYTE*data, int size);
	HRESULT parseObject(BYTE*data, int size);
	HRESULT parsePresentaionSegment(BYTE*data, int size, int time);
	HRESULT parseDisplay(BYTE*data, int size, int time);
	HRESULT parseWindow(BYTE*data, int size);
	HRESULT decodeRLE(pgs_subtitle *sub);

	// helper functions
	HRESULT remove_head(int size);
	DWORD readAYUV(BYTE *data);
	DWORD readDWORD(BYTE *data);
	DWORD readDWORD(int pos);
	WORD readWORD(BYTE *data);
	WORD readWORD(int pos);
	DWORD YUV2RGB(BYTE Y, BYTE Cr, BYTE Cb, BYTE A);
};