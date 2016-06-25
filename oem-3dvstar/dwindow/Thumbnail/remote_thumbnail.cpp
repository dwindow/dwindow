#include "remote_thumbnail.h"
#define error_exit(code){on_error(code);return code;}
#define test_error(code) if((code)<0) error_exit(code)
remote_thumbnail::remote_thumbnail()
:m_pipe(NULL)
{
	m_width = 400;
	m_height = 225;
	m_pixel_format = 30;
	recieved_data = NULL;
}
remote_thumbnail::~remote_thumbnail()
{
	disconnect();
	if (recieved_data) delete recieved_data;
}

int remote_thumbnail::connect(HANDLE  pipe)
{
	if (pipe == NULL || pipe == INVALID_HANDLE_VALUE)
		return error_not_connected;

	m_connected = true;
	m_pipe = pipe;

	return 0;
}

int remote_thumbnail::connect(const wchar_t *pipename)
{
	disconnect();
	while (1)
	{
		m_pipe = CreateFile(pipename,   // pipe name 
			GENERIC_READ | GENERIC_WRITE, 
			0,              // no sharing 
			NULL,           // default security attributes
			OPEN_EXISTING,  // opens existing pipe 
			0,              // default attributes 
			NULL);          // no template file 

		// Break if the pipe handle is valid. 
		if (m_pipe != INVALID_HANDLE_VALUE) 
			return connect(m_pipe);

		// Exit if an error other than ERROR_PIPE_BUSY occurs.
		if (GetLastError() != ERROR_PIPE_BUSY)
			return error_connect_error;

		// All pipe instances are busy, so wait for 2 seconds.
		if ( ! WaitNamedPipe(pipename, 2000)) 
			return error_connect_timeout;
	}

	return error_connect_error;
}

int remote_thumbnail::disconnect()
{
	if (!m_connected)
		return error_not_connected;

	m_connected = false;

	DisconnectNamedPipe(m_pipe); 
	CloseHandle(m_pipe);

	on_disconnect();

	return 0;
}
int remote_thumbnail::open_file(const wchar_t*file)
{
	cmd.cmd = thumbnail_cmd_open_file;
	cmd.data_size = wcslen(file)*2 + 2;
	if (!WriteFile(m_pipe, &cmd, sizeof(cmd), &byteWritten, NULL ) || byteWritten != sizeof(cmd))
		error_exit(error_fail);
	if (!WriteFile(m_pipe, file, wcslen(file)*2+2, &byteWritten, NULL ) || byteWritten != wcslen(file)*2+2)
		error_exit(error_fail);

	if (!ReadFile(m_pipe, &cmd, sizeof(cmd), &byteWritten, NULL) || byteWritten != sizeof(cmd))
		error_exit(error_fail);

	return set_out_format(m_pixel_format, m_width, m_height);
}
int remote_thumbnail::seek(__int64 position)
{
	cmd.cmd = thumbnail_cmd_seek;
	cmd.data_size = sizeof(position);
	if (!WriteFile(m_pipe, &cmd, sizeof(cmd), &byteWritten, NULL ) || byteWritten != sizeof(cmd))
		error_exit(error_fail);
	if (!WriteFile(m_pipe, &position, sizeof(position), &byteWritten, NULL ) || byteWritten != sizeof(position))
		error_exit(error_fail);

	if (!ReadFile(m_pipe, &cmd, sizeof(cmd), &byteWritten, NULL) || byteWritten != sizeof(cmd))
		error_exit(error_fail);

	test_error(cmd.cmd);

	return 0;
}
int remote_thumbnail::set_out_format(int pixel_format, int width, int height)
{
	set_out_format_parameter fmt = {width, height, pixel_format};
	cmd.cmd = thumbnail_cmd_set_out_format;
	cmd.data_size = sizeof(fmt);
	if (!WriteFile(m_pipe, &cmd, sizeof(cmd), &byteWritten, NULL ) || byteWritten != sizeof(cmd))
		error_exit(error_fail);
	if (!WriteFile(m_pipe, &fmt, sizeof(fmt), &byteWritten, NULL ) || byteWritten != sizeof(fmt))
		error_exit(error_fail);
	if (!ReadFile(m_pipe, &cmd, sizeof(cmd), &byteWritten, NULL) || byteWritten != sizeof(cmd))
		error_exit(error_fail);

	test_error(cmd.cmd);

	m_width = width;
	m_height = height;
	m_pixel_format = pixel_format;

	if (recieved_data) delete recieved_data;
	recieved_data = new char[m_width * m_height * 4];

	return 0;
}
int remote_thumbnail::get_one_frame()
{
	cmd.cmd = thumbnail_cmd_get_frame;
	cmd.data_size = 0;
	if (!WriteFile(m_pipe, &cmd, sizeof(cmd), &byteWritten, NULL ) || byteWritten != sizeof(cmd))
		error_exit(error_fail);

	if (!ReadFile(m_pipe, &cmd, sizeof(cmd), &byteWritten, NULL) || byteWritten != sizeof(cmd))
		error_exit(error_fail);
	if (!ReadFile(m_pipe, recieved_data, cmd.data_size, &byteWritten, NULL) || byteWritten != cmd.data_size)
		error_exit(error_fail);

	// only 3 possible return value: EOF, success, crashed
	// crashed server won't reply any data
	// 

	return cmd.cmd == o_thumbnail_cmd_EOF ? 1 : 0;
}
int remote_thumbnail::close()
{
	cmd.cmd = thumbnail_cmd_close_file;
	cmd.data_size = 0;
	if (!WriteFile(m_pipe, &cmd, sizeof(cmd), &byteWritten, NULL ) || byteWritten != sizeof(cmd))
		error_exit(error_fail);

	if (!ReadFile(m_pipe, &cmd, sizeof(cmd), &byteWritten, NULL) || byteWritten != sizeof(cmd))
		error_exit(error_fail);

	test_error(cmd.cmd);

	return 0;
}
