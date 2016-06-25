#include <stdio.h>
#include <windows.h>

#include "PGS.h"

char *types[] = {
"NO_SEGMENT       = 0xFFFF",
"PALETTE          = 0x14",			//
"OBJECT           = 0x15",			//
"PRESENTATION_SEG = 0x16",			//
"WINDOW_DEF       = 0x17",			//
"INTERACTIVE_SEG  = 0x18",
"DISPLAY          = 0x80",			//
"HDMV_SUB1        = 0x81",
"HDMV_SUB2        = 0x82"};

char *type2str(BYTE type)
{
	if (type == 0x14)
		return types[1];
	if (type == 0x15)
		return types[2];
	if (type == 0x16)
		return types[3];
	if (type == 0x17)
		return types[4];
	if (type == 0x18)
		return types[5];
	if (type == 0x80)
		return types[6];
	if (type == 0x81)
		return types[7];
	if (type == 0x82)
		return types[8];
	return types[0];
}


DWORD reverse(DWORD in)
{
	BYTE tmp[4];
	BYTE tmp2;
	*(DWORD*)tmp = in;
	tmp2 = tmp[0];
	tmp[0] = tmp[3];
	tmp[3] = tmp2;
	tmp2 = tmp[1];
	tmp[1] = tmp[2];
	tmp[2] = tmp2;
	return *(DWORD*)tmp;
}

WORD reverse2(WORD in)
{
	return ((in&0xff) << 8) | ((in&0xff00) >> 8);
}
DWORD YUV2RGB(BYTE A, BYTE Y, BYTE Cr, BYTE Cb)
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

	//A<<24, R<<16, G<<8, B

    return (A<<24) | ((BYTE)rp << 16) | ((BYTE)gp << 8) | ((BYTE)bp);
}

// parse object
/*
Object:
SHORT object_id
BYTE version_number
BYTE SEQUENCE_DESC
3BYTE n_data_size
SHORT width?
SHORT height?
[datas]
*/

void display_content(FILE *f, FILE *log, BYTE type, WORD datalen)
{
	static DWORD PALETTE[256];

	if (type == 0x14)
	{
		//PALETEE
		int idx = 0;
		BYTE *data = (BYTE*) malloc(datalen);
		fread(data, 1, datalen, f);

		BYTE palette_id = data[idx++];
		BYTE palette_version_number = data[idx++];

		int n_colors = (datalen-2)/5;
		for(int i=0; i<n_colors; i++)
		{
			BYTE color = data[idx++];
			BYTE y = data[idx++];
			BYTE u = data[idx++];
			BYTE v = data[idx++];
			BYTE a = data[idx++];

			PALETTE[color] = YUV2RGB(a,y,u,v);
		}

		free(data);
	}
	/*
	else if (type == 0x15)
	{
		// parse object
		struct {
			SHORT object_id;
			BYTE version_number;
			BYTE seq_desc;
			BYTE n_data_size[3];
			WORD width;
			WORD height;
		} header;

		fread(&header, 1, 11, f);

		header.width = reverse2(header.width);
		header.height = reverse2(header.height);

		BYTE *rle = (BYTE*) malloc(datalen-11);
		fread(rle, 1, datalen-11, f);

		//Tbitdata bs(rle, (header.n_data_size[0]<<16) + (header.n_data_size[1] <<8) + header.n_data_size[2]);
		//Tbitdata bs(rle, datalen-11);

		BYTE *decoded = (BYTE*) malloc(header.width * header.height);

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
					if (xpos < header.width)
						pos += header.width - (pos%header.width);
					else
						pos -= pos % header.width;
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
					n_pixel = ((b-0x80) <<8) + rle[idx++];
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
				decoded[pos++] = color;
		} while (pos < header.width * header.height && idx < datalen-11);


		free(rle);

		FILE *dec = fopen("E:\\test_folder\\decod.raw", "wb");
		BYTE *rgb = (BYTE*)malloc(header.width*header.height*3);
		for(int i=0; i<header.width*header.height; i++)
		{
			DWORD argb = PALETTE[decoded[i]];
			BYTE *b = (BYTE*)&argb;
			rgb[i*3] = (argb>>16) & (argb>>24);		//R
			rgb[i*3+1] = (argb>>8)& (argb>>24);		//G
			rgb[i*3+2] = (argb)& (argb>>24);		//B
		}
		fwrite(rgb, 1, header.width*header.height*3, dec);
		fclose(dec);
		free(rgb);
		free(decoded);
	}
	*/
	else if (type == 0x17)
	{
		// windows
		BYTE n_windows = 0;
		fread(&n_windows, 1, 1, f);
		struct {
			BYTE window_id;
			WORD left;
			WORD top;
			WORD width;
			WORD height;
		} data;

		fread(&data, 1, 9, f);

		data.left = reverse2(data.left);
		data.top = reverse2(data.top);
		data.width = reverse2(data.width);
		data.height = reverse2(data.height);

		fseek(f, datalen-10, SEEK_CUR);
	}
	else if (datalen < 100 && datalen > 0 && (type==0x18 || type == 0x81 || type == 0x82 || type == 0x17))
	{
		BYTE *data = (BYTE*)malloc(datalen);
		fread(data, 1, datalen, f);

		if (type != 0x17 || data[0] != 1 || data[1] != 0)
		{
			fprintf(log, "data: ");
			for(int i=0; i<datalen; i++)
				fprintf(log, "%02x ", data[i]);
			fprintf(log, "\n");
		}
		free(data);
	}
	else
		fseek(f, datalen, SEEK_CUR);
}

void display_time(DWORD *times, FILE *log)
{
	for(int i=0; i<2; i++)
	{
		DWORD time = times[i];
		int ms = time % 1000;
		int sec = (time / 1000) % 60;
		int minute = (time / 60000) % 60;
		int hour = (time / 3600000);
		if (i == 0)
			fprintf(log, "%02d:%02d:%02d,%03d", hour, minute, sec, ms);
		else
			fprintf(log, " - %02d:%02d:%02d,%03d\n", hour, minute, sec, ms);

	}
}


int main2()
{
	FILE *f = fopen("E:\\test_folder\\avt_sup\\00001.track_4608.sup", "rb");
	FILE *log = fopen("E:\\test_folder\\pgs.log", "w");

	while (true)
	{
		BYTE header[2];
		if (fread(header, 1, 2, f) != 2)
		{
			fprintf(log, "EOF\n");
			break;
		}
		if (header[0] != 80 || header[1] != 0x47)
		{
			fprintf(log, "bad header\n");
			break;
		}

		DWORD times[2];
		if (fread(times, 1, 8, f) != 8)
		{
			fprintf(log, "EOF\n");
			break;
		}
		times[0] = reverse(times[0])/90;
		times[1] = reverse(times[1])/90;

		BYTE type;
		BYTE datlen_raw[2];
		if (fread(&type, 1, 1, f) != 1 || fread(datlen_raw, 1, 2, f) != 2)
		{
			fprintf(log, "EOF\n");
			break;
		}

		WORD datalen = (datlen_raw[0] << 8) + datlen_raw[1];
		display_time(times, log);
		fprintf(log, "%s, %d bytes.\n", type2str(type), datalen);
		display_content(f, log, type, datalen);

	}

	fclose(f);
	fclose(log);
	return 0;
}

int main()
{
	FILE *f = fopen("E:\\test_folder\\avt_sup\\00001.track_4608.sup", "rb");
	FILE *log = fopen("E:\\test_folder\\pgs2.log", "w");
	PGSParser pgs;

	BYTE buf[32768];
	int read = 0;
	while (read = fread(buf, 1, 32768, f))
	{
		pgs.add_data(buf, read);
	}

	subtitle test;
	pgs.find_subtitle(6456000, 6456027, &test);

	return 0;
}