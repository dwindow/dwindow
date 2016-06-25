#pragma once
#include <stdio.h>

typedef struct vobsub_subtitle_struct
{
	// time
	int start;
	int end;

	// object position
	int left;
	int top;
	int width;
	int height;

	// object data
	int datasize;
	int packetsize;
	BYTE *packet;
	DWORD *rgb;
} vobsub_subtitle;

class VobSubParser
{
public:
	VobSubParser();
	~VobSubParser();

	HRESULT reset();
	int max_lang_id(const wchar_t *idx_filename);
	HRESULT load_file(const wchar_t *idx_filename, int lang_id, const wchar_t *sub_filename);
	HRESULT load_file(void *index_data, int size, int lang_id, const wchar_t *sub_filename);		// set sub_filename = NULL to use in directshow
																									// which means sub is streamed.
	HRESULT find_subtitle(int start, int end, vobsub_subtitle *out);								// time is in ms
	HRESULT add_subtitle(int start, int end, void *data, int size);

	int m_total_width;	// = 720
	int m_total_height; // = 480
	double m_scale_x;	// = 1.0
	double m_scale_y;	// = 1.0
	int m_global_time_offset;	// = 0;
	FILE *m_sub_file;		// = NULL;

	vobsub_subtitle *m_subtitles;
	HRESULT decode(vobsub_subtitle *sub);
protected:
	int handle_data_16(unsigned short *data, bool big, int size);
	int handle_data_8(unsigned char *data, int code_page, int size);
	int handle_line(wchar_t *line);

	HRESULT read_packet_from_subfile(FILE *f, DWORD pos);

	// helper functions
	int time_to_decimal(wchar_t *str);
	bool wisgidit(wchar_t *str);

	// parsing variables
	bool m_first_line;				// = false
	bool m_bad_index;
	int m_lang_id;

	// index global settings
	RGBQUAD m_orgpal [16];
	RGBQUAD m_cuspal [4];
	bool m_use_custom_color;				//= false;


	bool m_scan_for_lang_id;		// = false
	int m_max_lang_id;

	// subtitles
	int m_subtitle_count;
	vobsub_subtitle m_current_subtitle;
};