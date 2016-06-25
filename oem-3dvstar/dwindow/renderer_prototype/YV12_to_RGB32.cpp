#include "YV12_to_RGB32.h"

BYTE clip( int i)
{
	return i > 255 ? 255 : (i<0 ? 0 : i);
}

void YV12_to_RGB32(BYTE *Y, BYTE * U, BYTE * V, BYTE* dst, int width, int height, int strideY, int strideUV, int strideRGB)	// stride is in bytes!
{
	// Colour conversion from
	// http://www.poynton.com/notes/colour_and_gamma/ColorFAQ.html#RTFToC30
	//
	// YCbCr in Rec. 601 format
	// RGB values are in the range [0..255]
	//
	// [R]     1    [ 298.082       0       408.583 ]   ([ Y  ]   [  16 ])
	// [G] =  --- * [ 298.082   -100.291   -208.120 ] * ([ Cb ] - [ 128 ])
	// [B]    256   [ 298.082    516.411       0    ]   ([ Cr ]   [ 128 ])

	long YCoeff    = long (298.082 * 256 + 0.5);
	long VtoRCoeff = long (408.583 * 256 + 0.5);
	long VtoGCoeff = long (208.120 * 256 + 0.5);
	long UtoGCoeff = long (100.291 * 256 + 0.5);
	long UtoBCoeff = long (516.411 * 256 + 0.5);

	for ( int y = 0; y<height; y+=2 )
	{
		for ( int x = 0; x<width; x+=2 )
		{
			BYTE * srcU = U + x/2 + y/2 * strideUV;
			BYTE * srcV = V + x/2 + y/2 * strideUV;
			BYTE * srcY = Y + x + y * strideY;

			long scaledChromaToR = 32768 + VtoRCoeff * (srcV[0] - 128);
			long scaledChromaToG = 32768 - VtoGCoeff * (srcV[0] - 128) - UtoGCoeff *(srcU[0] - 128);
			long scaledChromaToB = 32768 + UtoBCoeff * (srcU[0] - 128);

			BYTE * ptr = dst + 4 * x + y * strideRGB;

			//top-left pixel
			long scaledY = (srcY[0] - 16) * YCoeff;
			ptr[0] = clip(( scaledY + scaledChromaToB ) >> 16);
			ptr[1] = clip(( scaledY + scaledChromaToG ) >> 16);
			ptr[2] = clip(( scaledY + scaledChromaToR ) >> 16);
			ptr[3] = 255;             

			//top-right pixel
			scaledY = (srcY[1] - 16) * YCoeff;
			ptr[4] = clip(( scaledY + scaledChromaToB ) >> 16);
			ptr[5] = clip(( scaledY + scaledChromaToG ) >> 16);
			ptr[6] = clip(( scaledY + scaledChromaToR ) >> 16);
			ptr[7] = 255;             

			ptr = dst + 4 * x + (y+1) * strideRGB;

			//bottom-left pixel
			scaledY = (srcY[strideY] - 16) * YCoeff;      
			ptr[0] = clip(( scaledY + scaledChromaToB ) >> 16);
			ptr[1] = clip(( scaledY + scaledChromaToG ) >> 16);
			ptr[2] = clip(( scaledY + scaledChromaToR ) >> 16);
			ptr[3] = 255;             

			//bottom-right pixel
			scaledY = (srcY[strideY+1] - 16) * YCoeff;
			ptr[4] = clip(( scaledY + scaledChromaToB ) >> 16);
			ptr[5] = clip(( scaledY + scaledChromaToG ) >> 16);
			ptr[6] = clip(( scaledY + scaledChromaToR ) >> 16);
			ptr[7] = 255;             
		}
	}
}