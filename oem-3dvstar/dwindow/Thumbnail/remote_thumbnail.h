#pragma once
#include <Windows.h>
#include "Thumbnail.h"

class remote_thumbnail
{
public:
	remote_thumbnail();		// 30 = PIX_FMT_BGRA
	~remote_thumbnail();

	int connect(HANDLE  pipe);
	int connect(const wchar_t *pipename);
	int disconnect();
	int open_file(const wchar_t*file);
	int seek(__int64 position);
	int get_one_frame();
	int set_out_format(int pixel_format, int width, int height);
	int close();

	void *recieved_data;

	enum error_code
	{
		error_not_connected = -1,
		error_fail = -2,
		error_connect_timeout = -3,
		error_connect_error = -4,
	};

protected:
	virtual int on_disconnect(){return 0;}							// default behavior: do nothing, override this to reconnect to pipe
	virtual int on_error(int error_code){return disconnect();}		// default behavior: disconnect

protected:

	bool m_connected;
	HANDLE m_pipe;

	// tmp variable
	thumbnail_cmd cmd;
	DWORD byteWritten;

	// working state
	int m_width;
	int m_height;
	int m_pixel_format;
};