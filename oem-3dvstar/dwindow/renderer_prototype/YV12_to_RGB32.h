#pragma once

#include <Windows.h>

void YV12_to_RGB32(BYTE *Y, BYTE * U, BYTE * V, BYTE* dst, int width, int height, int strideY, int strideUV, int strideRGB);	// stride is in bytes!
