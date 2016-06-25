#include <stdio.h>
#include <winsock2.h>
#include <Windows.h>
#include <conio.h>
#pragma  comment(lib, "ws2_32.lib")
#include "report.h"
#include "SHA1.h"
#include "..\my12doom_revision.h"


int tcp_connect(char *hostname,int port)
{
	struct hostent *host;
	if((host=gethostbyname(hostname))==NULL)
		return -1;

	int sockfd=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if(sockfd==-1)
		return -1;

	struct sockaddr_in server_addr = {0};
	server_addr.sin_family=AF_INET; 
	server_addr.sin_port=htons(port); 
	server_addr.sin_addr=*((struct in_addr *)host->h_addr);         

	if(connect(sockfd,(struct sockaddr *)(&server_addr),sizeof(struct sockaddr))==-1)
	{
		closesocket(sockfd);
		return -1;
	}

	return sockfd;
}

int main()
{
	int argc = 1;
	LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);

	if (argc<2)
		return -1;

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0) 
	{
		fprintf(stderr, "WSAStartup failed.\n");
		exit(1);
	}

	FILE * f = _wfopen(argv[1], L"rb");
	fseek(f, 0, SEEK_END);
	int filesize = ftell(f);
	fseek(f, 0, SEEK_SET);

	// calculate header
	report_header_v1 header = {sizeof(report_header_v1)};
	header.file_size = filesize;
	header.rev = my12doom_rev;
	char tmp[4096];
	int block_size;
	__int64 got = 0;
	SHA1_STATETYPE context;
	SHA1_Start(&context);
	while(block_size = fread(tmp, 1, 4096, f))
	{
		got += block_size;

		printf("\rcalculating SHA1...%d%%", got*100/filesize);
		SHA1_Hash((unsigned char*)tmp, block_size, &context);
	}
	SHA1_Finish((unsigned char*)header.sha1, &context);
	printf("\rcalculating SHA1...");
	for(int i=0; i<20; i++)
		printf("%02x", (unsigned int)header.sha1[i]);
	printf("\n");


	int fd = tcp_connect("bo3d.net", 82);
	//int fd = tcp_connect("127.0.0.1", 82);
	if (fd<0)
	{
		printf("error connectting to server\n");
	}

	send(fd, (char*)&header, sizeof(header), 0);
	int sent = 0;
	fseek(f, 0, SEEK_SET);
	while(block_size = fread(tmp, 1, 4096, f))
	{
		send(fd, tmp, block_size, 0);

		sent += block_size;

		printf("\r%d/%d sent, %d%%", sent, filesize, (__int64)sent*100/filesize);
	}

	memset(tmp, 0, sizeof(tmp));
	recv(fd, tmp, 4096, 0);
	printf("\n%s\n", tmp);

	closesocket(fd);

	return 0;
}

int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) 
{
	return main();
}