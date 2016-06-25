#pragma once
#include "inttypes.h"
#include "stdint.h"
#include <Windows.h>
extern "C"
{
#include <x264.h>
}

class x264
{
public:
	x264();
	~x264();
	int width;
	int height;
	int last_encode_time;

	HRESULT init(int width, int height, int bitrate);
	int encode_a_frame(void *data, void*nal_out, bool *IDR);

protected:
	x264_t *encoder;
	x264_picture_t pic_in;
	x264_picture_t pic_out;
	int64_t i_pts/* = 0*/;
};