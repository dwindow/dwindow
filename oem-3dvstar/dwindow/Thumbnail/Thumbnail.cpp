#include <stdio.h>
#include <Windows.h>
#include "..\AvsSlideShow\screenshotor.h"
#include "Thumbnail.h"
#include "remote_thumbnail.h"

screenshoter s;

int process_command(thumbnail_cmd cmd, void *data, thumbnail_cmd *o_cmd, void** o_data);
DWORD WINAPI server_thread(LPVOID parameter);
LONG WINAPI my_handler(struct _EXCEPTION_POINTERS *ExceptionInfo);

int wmain(int argc, wchar_t* argv[])
{
	SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);
	wchar_t *pipename = L"\\\\.\\pipe\\DWindowThumbnailPipe";
	SetUnhandledExceptionFilter(my_handler);
	server_thread(argc>1?argv[1]:pipename);

	return 0;
}

DWORD WINAPI server_thread(LPVOID parameter)
{
	while (1)
	{
	HANDLE hPipe = CreateNamedPipeW( 
		(wchar_t*)parameter,             // pipe name 
		PIPE_ACCESS_DUPLEX,      // read/write access 
		PIPE_TYPE_MESSAGE |		  // message type pipe 
		PIPE_READMODE_MESSAGE |   // message-read mode 
		PIPE_WAIT,                // blocking mode 
		PIPE_UNLIMITED_INSTANCES, // max. instances  
		500, // output buffer size 
		500, // input buffer size 
		0,                        // client time-out 
		NULL);                    // default security attribute 

	if (hPipe == INVALID_HANDLE_VALUE)
		break; // wtf ... create pipe fail?

	printf("pipe created, waitting for connection.....");
	fflush(stdout);
	if (!ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED))
		ExitProcess(-2);

	printf("connected\n");



	// read command and execute
	char *data = NULL;
	while (1)
	{
		// free
		if (data) {delete data; data = NULL;}

		// read command header
		thumbnail_cmd cmd;
		DWORD cbBytesRead = 0;
		BOOL fSuccess = ReadFile( hPipe, &cmd, sizeof(thumbnail_cmd), &cbBytesRead, NULL);
		if (!fSuccess || cbBytesRead == 0)
			break;

		// read command data if there is
		if (cmd.data_size>0)
		{
			data = new char[cmd.data_size];
			fSuccess = ReadFile( hPipe, data, cmd.data_size, &cbBytesRead, NULL);
			if (!fSuccess || cbBytesRead == 0)
				break;
		}

		// process command
		void * odata = NULL;
		thumbnail_cmd ocmd;
		if (process_command(cmd, data, &ocmd, &odata)<0)
			continue;

		// send result back
		fSuccess = WriteFile(hPipe, &ocmd, sizeof(thumbnail_cmd), &cbBytesRead, NULL);
		if (!fSuccess || cbBytesRead == 0)
			break;

		if (ocmd.data_size>0 && odata)
		{
			fSuccess = WriteFile( hPipe, odata, ocmd.data_size, &cbBytesRead, NULL);
			delete odata;
			if (!fSuccess || cbBytesRead == 0)
				break;
		}
	}

	printf("shutting down...");
	DisconnectNamedPipe(hPipe); 
	CloseHandle(hPipe);
	printf("OK\n");
	}
	return 0;
}

LONG WINAPI my_handler(struct _EXCEPTION_POINTERS *ExceptionInfo)
{
	TerminateProcess(GetCurrentProcess(), -1);

	return EXCEPTION_CONTINUE_SEARCH;
}


int width = 800;
int height = 480;
PixelFormat pixel_format = PIX_FMT_BGRA;
int process_command(thumbnail_cmd cmd, void *data, thumbnail_cmd *o_cmd, void** o_data)
{
	o_cmd->data_size = 0;
	o_cmd->cmd = o_thumbnail_cmd_OK;

	switch (cmd.cmd)
	{
	case thumbnail_cmd_open_file:
		o_cmd->cmd = s.open_file((wchar_t*)data) < 0 ? o_thumbnail_cmd_FAIL : o_thumbnail_cmd_OK;
		break;
	case thumbnail_cmd_close_file:
		o_cmd->cmd = s.close() < 0 ? o_thumbnail_cmd_FAIL : o_thumbnail_cmd_OK;;
		break;
	case thumbnail_cmd_seek:
		o_cmd->cmd = s.seek(*(int64_t*)data) < 0 ? o_thumbnail_cmd_FAIL : o_thumbnail_cmd_OK;;
		break;
	case thumbnail_cmd_set_out_format:
		{
			set_out_format_parameter * p = (set_out_format_parameter*)data;
			width = p->width;
			height = p->height;
			pixel_format = (PixelFormat)p->pixel_format;
			o_cmd->cmd = s.set_out_format((PixelFormat)p->pixel_format, p->width, p->height) < 0 ? o_thumbnail_cmd_FAIL : o_thumbnail_cmd_OK;;
		}
		break;
	case thumbnail_cmd_get_frame:
		o_cmd->data_size = width*height*4;
		*o_data = new char[o_cmd->data_size];
		o_cmd->cmd = s.get_one_frame(*o_data, width*4) < 0 ? o_thumbnail_cmd_EOF : o_thumbnail_cmd_OK;;
		break;
	default:
		return -1;
	}

	return 0;
}

int read_pipe(HANDLE pipe, int size, void **out = NULL)
{
	if (size <=0)
		return -1;

	void *p = new char[size];
	if (out)
		*out = p;
	ReadFile(pipe, *out, size, (DWORD*)&size, NULL);
	if (!out)
		delete p;

	return size;
}
