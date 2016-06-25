#pragma once
#include <Windows.h>
#include <math.h>

#ifndef def_input_layout_types
#define def_input_layout_types
enum input_layout_types
{
	side_by_side, top_bottom, mono2d, input_layout_types_max, input_layout_auto
};
#endif

template<typename pixel_type>
HRESULT get_layout(void *src, int width, int height, int *out, int stride = -1)
// stride is also in pixel, like 1280*720 video in VideoCard usually is 1536*720, stride = 1536
{
	if (!out || !src)
		return E_POINTER;

	if (-1 == stride)
		stride = width;

	pixel_type *data = (pixel_type*) ( ((BYTE*)src) + (sizeof(pixel_type) == 4 ? 2 : 0));

	unsigned int avg1 = 0;
	unsigned int avg2 = 0;
	unsigned int var1 = 0;		// variance
	unsigned int var2 = 0;
	unsigned int lum1 = 0;		// luminance
	unsigned int lum2 = 0;

	int width2 = width / 2;
	int height2 = height / 2;

	int height24 = min(height, int(width / 2.40));		// crop by 2.40:1
	int start24 = (height - height24)/2;
	int height242 = height24 / 2;
	int start242 = start24/2;

	const int test_size = 64;
	const int range = (int)(width*0.06);
	const int range2 = (int)(width2*0.06);
	const int range_step = max(1, range/30);
	const int range_step2 = max(1, range2/30);

	// SBS
	for(int y=0; y<test_size; y++)
	{
		int sy = y*height/test_size;
// 		sy = (y%(test_size/2))*height24/test_size + start242;
// 		if (y >= test_size / 2)
// 			sy += height2;

		for(int x=0; x<test_size; x+=2)
		{
			int sx = x*width2/test_size;
			int min_delta = 255;
			int r1 = *(BYTE*)&data[sy*stride+sx];

			int m = min(width, width2+sx+range2);
			for(int i=max(width2, width2+sx-range2); i<=m; i+=range_step2)
			{
				int r2 = *(BYTE*)&data[sy*stride+i] & 0xff;
				int t = abs(r1-r2);
				min_delta = min(min_delta, t);
			}

			lum1 += r1;
			avg1 += min_delta*2;
			var1 += min_delta * min_delta*2;
		}
	}

	// TB
	pixel_type * data2 = data + stride * height2;
	for(int y=0; y<test_size; y++)
	{
		int sy = y*height2/test_size;
// 		sy = y*height242/test_size + start242;

		for(int x=0; x<test_size; x+=2)
		{
			int sx = x*width/test_size;
			int min_delta = 255;
			int r1 = *(BYTE*)&data[sy*stride+sx] & 0xff;
			int m = min(width, sx+range);
			for(int i=max(0, sx-range); i<=m; i+=range_step)
			{
				int r2 = *(BYTE*)&data2[sy*stride+i] & 0xff;
				int t = abs(r1-r2);
				min_delta = min(min_delta, t);
			}

			lum2 += r1;
			avg2 += min_delta*2;
			var2 += min_delta * min_delta*2;
		}
	}

	double d_avg1 = (double)avg1 / (test_size * test_size);
	double d_var1 = (double)var1 / (test_size * test_size);
	double d_lum1 = (double)lum1 / (test_size * test_size);
	d_var1 -= d_avg1*d_avg1;
	d_var1 = sqrt(d_var1);

	double d_avg2 = (double)avg2 / (test_size * test_size);
	double d_var2 = (double)var2 / (test_size * test_size);
	double d_lum2 = (double)lum2 / (test_size * test_size);
	d_var2 -= d_avg2*d_avg2;
	d_var2 = sqrt(d_var2);

	double cal1 = max(d_avg1 * d_var1, d_avg1 + d_var1);
	double cal2 = max(d_avg2 * d_var2, d_avg2 + d_var2);

	double times = 0;
	if ( (cal1 > 0.01 && cal2 > 0.01) || (((cal1>cal2*10000) || (cal2>cal1*10000)) && cal1 > 0.0001 && cal2 > 0.0001) )
		times = cal1 > cal2 ? cal1 / cal2 : cal2 / cal1;

 	printf("sbs:%f - %f - %f, lum=%f\r\n", d_avg1, d_var1, cal1, d_lum1);
 	printf("tb: %f - %f - %f, lum=%f\r\n", d_avg2, d_var2, cal2, d_lum2);

	d_lum1 = min(1.0, d_lum1 / 25.0);
	printf("times: %f, lum: %f\r\n", times, d_lum1);

	if (
		times > 2000 ||             // titles 
		(times > 31.62 && times * d_lum1 > 31.62)		// normal picture 10^1.5
		)
	{
 		printf("stereo(%s).\r\n", cal1 > cal2 ? "tb" : "sbs");
		*out = cal1 > cal2 ? top_bottom : side_by_side;
	}
	else if ( 1.0 < times && times < 4.68/2  &&
		1.0 < times/d_lum1 && times/d_lum1 < 4.68/2 
		&& d_lum1 > 0.25)		// minimum luminance for 2d detection : 0.25
	{
 		printf("normal.\r\n");
		*out = mono2d;
	}
	else
	{
//  		printf("unkown.\r\n");
		return S_FALSE;
	}

	return S_OK;
}
