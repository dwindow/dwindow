#include "unpack_YUY2.h"
#include <emmintrin.h>
#include <windows.h>
#include <stdio.h>

void unpack_YUY2(int width, int height, void *src, int stride_src, void*Y, int stride_Y, void *UV, int stride_UV, bool deinterlace/*=false*/)
{
	const __m128i mask_Y = _mm_set1_epi16(0x00ff);
	const __m128i mask_UV = _mm_set1_epi16(0xff00);
	__m128i m1, m2, m3, m4;

	for(int y=0; y<height; y++)
	{
		int line = !deinterlace ? y : ( ((y<<1)%height)+y/height );
		__m128i * srcI = (__m128i*)((char*)src + stride_src * line);
		__m128i * dstY = (__m128i*)((char*)Y + stride_Y * y);
		__m128i * dstUV = (__m128i*)((char*)UV + stride_UV * y);

		for(int x=0; x<width / 16; x++)
		{
			// load
			m3 = m1 = _mm_loadu_si128(srcI);
			srcI ++;
			m4 = m2 = _mm_loadu_si128(srcI);
			srcI ++;

			// mask Y
			m1 = _mm_and_si128(m1, mask_Y);
			m2 = _mm_and_si128(m2, mask_Y);

			// mask UV
			m3 = _mm_and_si128(m3, mask_UV);
			m4 = _mm_and_si128(m4, mask_UV);

			// pack Y
			m1 = _mm_packus_epi16(m1, m2);

			// shift and pack UV
			m3 = _mm_srli_epi16(m3, 8);
			m4 = _mm_srli_epi16(m4, 8);
			m3 = _mm_packus_epi16(m3, m4);

			// store Y
			_mm_storeu_si128(dstY, m1);

			// store UV
			_mm_storeu_si128(dstUV, m3);

			dstY ++;
			dstUV++;
		}
	}
}

void copy_nv12(int width, int height, void *src, int stride_src, void*Y, int stride_Y, void *UV, int stride_UV, bool deinterlace/*=false*/)
{
	BYTE *dst = (BYTE*)Y;
	for(int i=0; i<height; i++)
	{
		int line = !deinterlace ? i : ( ((i<<1)%height)+i/height );
		memcpy(dst, (BYTE*)src+line*stride_src, width);
		dst += stride_Y;
	}

	dst = (BYTE*)UV;
	for(int i=0; i<height/2; i++)
	{
		int line = !deinterlace ? i : ( ((i<<1)%(height/2))+i/(height/2) );
		memcpy(dst, (BYTE*)src +height*stride_src +line*stride_src, width);
		dst += stride_UV;
	}
}
void copy_yv12(int width, int height, void *src, int stride_src, void*Y, int stride_Y, void *UV, int stride_UV, bool deinterlace/*=false*/)
{
	BYTE *dst = (BYTE*)Y;
	for(int i=0; i<height; i++)
	{
		int line = !deinterlace ? i : ( ((i<<1)%height)+i/height );
		memcpy(dst, (BYTE*)src+line*stride_src, width);
		dst += stride_Y;
	}

	// load V
	dst = (BYTE*)UV;
	for(int i=0; i<height/2; i++)
	{
		int line = !deinterlace ? i : ( ((i<<1)%(height/2))+i/(height/4) );
		memcpy(dst, (BYTE*)src +height*stride_src +line*stride_src/2, width/2);
		dst += stride_UV;
	}

	// load U
	for(int i=0; i<height/2; i++)
	{
		int line = !deinterlace ? i : ( ((i<<1)%(height/2))+i/(height/4) );
		memcpy(dst, (BYTE*)src +height*stride_src*5/4 +line*stride_src/2, width/2);
		dst += stride_UV;
	}
}

void copy_p01x(int width, int height, void *src, int stride_src, void*Y, int stride_Y, void *UV, int stride_UV, bool deinterlace/*=false*/)
{
	BYTE *dst = (BYTE*)Y;
	for(int i=0; i<height; i++)
	{
		int line = !deinterlace ? i : ( ((i<<1)%height)+i/height );
		memcpy(dst, (BYTE*)src+line*stride_src, width*2);
		dst += stride_Y;
	}

	dst = (BYTE*)UV;
	for(int i=0; i<height/2; i++)
	{
		int line = !deinterlace ? i : ( ((i<<1)%(height/2))+i/(height/2) );
		memcpy(dst, (BYTE*)src +height*stride_src +line*stride_src, width*2);
		dst += stride_UV;
	}
}