#include <Windows.h>
#include <stdio.h>
#include <locale.h>
#include <conio.h>
//#include <vld.h>
#include "srt_parser.h"

bool wcs_replace(wchar_t *to_replace, const wchar_t *searchfor, const wchar_t *replacer)
{
	const int tmp_size = 2048;
pass:
	wchar_t tmp[tmp_size];
	if (wcslen(to_replace) > tmp_size-1)
		return false;

	wchar_t *left_part = to_replace;
	wchar_t *right_part = wcsstr(to_replace, searchfor);
	if (!right_part)
		return true;

	right_part[0] = NULL;
	wsprintfW(tmp, L"%s%s%s", left_part, replacer, right_part + wcslen(searchfor));
	wcscpy(to_replace, tmp);
	goto pass;


	return false;	// ...
}

int wmain(int argc, wchar_t * argv[])
{
	setlocale( LC_ALL, "CHS" );

	if (argc<4 || argc>5)
	{
		printf("用法：\n");
		printf("srt_offset.exe [源字幕] [偏移序列文件] [输出字幕] [FPS(可选, 24或60, 默认24)]\n");
		printf("例子：\n");
		printf("srt_offset.exe avt.srt avt.txt avt_offset.srt\n");
		printf("srt_offset.exe game.srt game.txt game_offset.srt 60\n");

		printf("按任意键退出.\n");
		getch();
		return 0;
	}

	srt_parser srt;
	srt.init(5000, 800000);
	int fps = 24;
	if (argc == 5) fps = _wtoi(argv[4]);
	wprintf(L"载入 %s...", argv[1]);
	int o = srt.load(argv[1]);
	printf("%s\n", o==-1?"失败":"OK");
	wprintf(L"载入 %s, fps=%.3f...", argv[2], (float)fps/1.001);
	o = srt.load_offset_metadata(argv[2], fps);
	printf("%s\n", o==-1?"失败":"OK");
	wprintf(L"保存到 %s...", argv[2]);
	o = srt.save(argv[3]);
	printf("%s\n", o==-1?"失败":"OK");

	printf("完成。按任意键退出.\n");
	getch();

	return 0;
}