#include "charset.h"

int mytrim(wchar_t *str, wchar_t char_ = L' ');

inline wchar_t swap_big_little(wchar_t x)
{
	wchar_t low = x & 0xff;
	wchar_t hi = x & 0xff00;
	return (low << 8) | (hi >> 8);
}
int ConvertToUTF8(char *input, int input_size, char *output, int output_size)
{
	int op = 0;
	int p = 0;
	wchar_t line_w[1024];
	int size = input_size;
	unsigned char *n = (unsigned char*)input;

	if (n[0] == 0xFE && n[1] == 0xFF && n[3] != 0x00)				//UTF-16 Big Endian
	{
		input += 2;
		size -= 2;
		unsigned short *data = (unsigned short *)input;

		for (int i=0; i<size/2; i++)
		{
			wchar_t c = swap_big_little(data[i]);
			if (c != 0xA && c != 0xD && p<1024)
				line_w[p++] = c;
			else
			{
				if(c == 0xD)
				{
					line_w[p] = NULL;

					WideCharToMultiByte(CP_UTF8, NULL, line_w, 1024, output+op, output_size-op, NULL, NULL);
					op += strlen(output+op);
					output[op++] = '\n';
				}
				else
					p = 0;
			}
		}
	}
	else if (n[0] == 0xFF && n[1] == 0xFE && n[2] != 0x00)			//UTF-16 Little Endian
	{
		input += 2;
		size -= 2;
		unsigned short *data = (unsigned short *)input;

		for (int i=0; i<size/2; i++)
		{
			wchar_t c = data[i];
			if (c != 0xA && c != 0xD && p<1024)
				line_w[p++] = c;
			else
			{
				if(c == 0xD)
				{
					line_w[p] = NULL;

					WideCharToMultiByte(CP_UTF8, NULL, line_w, 1024, output+op, output_size-op, NULL, NULL);
					op += strlen(output+op);
					output[op++] = '\n';
				}
				else
					p = 0;
			}
		}
	}
	else if (n[0] == 0xEF && n[1] == 0xBB && n[2] == 0xBF)			//UTF-8
	{
		input += 3;
		size -= 3;
		char *data = input;
		char line[1024];
		wchar_t line_w[1024];
		int p = 0;
		for (int i=0; i<size; i++)
		{
			if (data[i] != 0xA && data[i] != 0xD && p<1024)
				line[p++] = data[i];
			else
			{
				if(data[i] == 0xD)
				{
					line[p] = NULL;
					MultiByteToWideChar(CP_UTF8, 0, line, 1024, line_w, 1024);

					WideCharToMultiByte(CP_UTF8, NULL, line_w, 1024, output+op, output_size-op, NULL, NULL);
					op += strlen(output+op);
					output[op++] = '\n';
				}
				else
					p = 0;
			}
		}
	}
	else  //Unknown BOM
	{
		char *data = input;
		char line[1024];
		wchar_t line_w[1024];
		int p = 0;
		for (int i=0; i<size; i++)
		{
			if (data[i] != 0xA && data[i] != 0xD && p<1024)
				line[p++] = data[i];
			else
			{
				if(data[i] == 0xD)
				{
					line[p] = NULL;
					MultiByteToWideChar(CP_ACP, 0, line, 1024, line_w, 1024);

					WideCharToMultiByte(CP_UTF8, NULL, line_w, 1024, output+op, output_size-op, NULL, NULL);
					op += strlen(output+op);
					output[op++] = '\n';
				}
				else
					p = 0;
			}
		}
	}

	output[op] = NULL;
	return op;
}




int mytrim(wchar_t *str, wchar_t char_ /*= L' '*/)
{
	int len = (int)wcslen(str);
	//LEADING:
	int lead = 0;
	for(int i=0; i<len; i++)
		if (str[i] != char_)
		{
			lead = i;
			break;
		}

	//ENDING:
	int end = 0;
	for (int i=len-1; i>=0; i--)
		if (str[i] != char_)
		{
			end = len - 1 - i;
			break;
		}
	//TRIMMING:
	memmove(str, str+lead, (len-lead-end)*sizeof(wchar_t));
	str[len-lead-end] = NULL;

	return len - lead - end;
}
