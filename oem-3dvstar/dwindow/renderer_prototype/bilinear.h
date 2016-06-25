#include <Windows.h>
#include <math.h>

template<typename pixel_type>
void resize(const pixel_type *src, pixel_type *dst, int src_width, int src_height, int dst_width, int dst_height, int src_stride=-1, int dst_stride=-1)
{
	int channelcount = sizeof(pixel_type);
	if (src_stride<0)
		src_stride = src_width * sizeof(pixel_type);
	if (dst_stride<0)
		dst_stride = dst_width * sizeof(pixel_type);

#define resize_access(x,y) ((pixel_type*)((BYTE*)src) + y*src_stride)[x]

	for(int i=0; i<channelcount; i++)
	{
		for(int y=0; y<dst_height; y++)
		for(int x=0; x<dst_width; x++)
		{
			float xx = (float)x * src_width / dst_width;
			float yy = (float)y * src_height / dst_height;

			int l = (int)xx;
			int r = l+1;
			float fx = xx - l;
			int t = (int)yy;
			int b = t+1;
			float fy = yy - t;
			l = max(0,l);
			r = min(r,src_width-1);
			t = max(0,t);
			t = min(t,src_height-1);
			b = max(0,b);
			b = min(b,src_height-1);


			BYTE*p = ((pixel_type*)((BYTE*)dst) + y*dst_stride)+x;
			float xt = resize_access(r,t)*fx + resize_access(l,t)*(1-fx);
			float xb = resize_access(r,b)*fx + resize_access(l,b)*(1-fx);
			float xc = xt*(1-fy) + xb * fy;
			xc = max(0, xc);
			xc = min(255, xc);

			*p = (BYTE)(xc+0.5f);
		}

		src = (pixel_type*)(((BYTE*)src)+1);
		dst = (pixel_type*)(((BYTE*)dst)+1);
	}
}