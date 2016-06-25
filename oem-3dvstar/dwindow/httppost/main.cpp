// httppost.cpp : 定义控制台应用程序的入口点。
//

#include "httppost.h"


#include "stdafx.h"
#include <Windows.h>
#include <wininet.h>
#pragma comment(lib,"wininet.lib")

BOOL UseHttpSendReqEx(HINTERNET hConnect, TCHAR *upFile);
BOOL UseHttpSendReqEx(TCHAR *upFile, char *Ip, int port);
#define BUF_SIZE 4096

class myCB : public IHttpSendingCallback
{
	int HttpSendingCallback(int current, int total)
	{
		static int tick = GetTickCount();
// 		if (GetTickCount() != tick)
		{
			printf("\r%d/%d, %d%%", current, total, (__int64)current*100/total);
			tick = GetTickCount();
		}
		return 0;
	}
};

int _tmain(int argc, _TCHAR *argv[])
{
// 	httppost p(L"http://127.0.0.1/upload.php");
// 	p.addFile(L"file1", L"Z:\\core.dat");
//  	p.addFormItem(L"Hello2", L"Shit");
//  	p.addFormItem(L"Hello", L"Shit2");
//  	p.addFormItem(L"Hello3", L"纱布");
//  	p.addFile(L"file2", new char[560000], 560000);
// 	p.addHeader(L"Charsert", L"UTF-8");

// 	httppost p(L"http://www.shooter.cn/api/subapi.php");
// 	p.addFormItem(L"pathinfo", L"W:\\Bapq00000282_最后的风之子（英）3D_The Last Airbender 3D.TS");
// 	p.addFormItem(L"filehash", L"c38224b2da732ae8f005d30acb5a6d69;0e3a52e345a228bc96e55fcc4d793214;0bea98730216b6dbb0194faf3600a635;35b6dd3fd6f110001097cdbd58f768a4");
//  	p.addFormItem(L"vhash", L"");
// 	p.addFormItem(L"lang", L"");
// 	p.addFormItem(L"shortname", L"W:\\Bapq00000282_最后的风之子（英）3D_The Last Airbender 3D.TS");

	httppost p(L"http://bo3d.net/test/a-001.mkv");
// 	p.addHeader(L"Range", L"bytes=234111353-");
	int reponse_code = p.send_request();

	std::map<wchar_t*, wchar_t*> headers = p.get_response_headers();
	for(std::map<wchar_t*, wchar_t*>::iterator i = headers.begin(); i!= headers.end(); ++i)
		wprintf(L"%s - %s\n", i->first, i->second);

	char data[1024];
	int o = 0;
	FILE *f = fopen("Z:\\response.txt", "wb");
	while (o = p.read_content(data, sizeof(data)))
		fwrite(data, 1, o, f);

	fclose(f);

	return 0;
}
