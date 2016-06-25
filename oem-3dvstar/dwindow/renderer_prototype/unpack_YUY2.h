#pragma once

// this function unpacks YUY2 into a 4:2:2 planar format like NV12 except UV height is same as Y
// YYYYYY   width = width
// YYYYYY   height = height
// ......
// UVUVUV   width = half width
// ....     height = height

void unpack_YUY2(int width, int height, void *src, int stride_src, void*Y, int stride_Y, void *UV, int stride_UV, bool deinterlace=false);
void copy_nv12(int width, int height, void *src, int stride_src, void*Y, int stride_Y, void *UV, int stride_UV, bool deinterlace=false);
void copy_yv12(int width, int height, void *src, int stride_src, void*Y, int stride_Y, void *UV, int stride_UV, bool deinterlace=false);
void copy_p01x(int width, int height, void *src, int stride_src, void*Y, int stride_Y, void *UV, int stride_UV, bool deinterlace=false);