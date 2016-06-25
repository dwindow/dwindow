#include "thumbnail_creator.h"

// int remote_thumbnail_mine::on_disconnect()
// {
// 	m_owner->reconnect();
// 	return 0;
// }

int max_retry = 2;

async_thumbnail_creator::async_thumbnail_creator()
:exiting(false)
{
}
async_thumbnail_creator::~async_thumbnail_creator()
{
	close_file();
}

int async_thumbnail_creator::open_file(const wchar_t*file)		// 
{
	close_file();

	exiting = false;
	m_worker_thread = CreateThread(NULL, NULL, worker_thread_entry, this, NULL, NULL);
	wcscpy(m_file_name, file);
	seek_target = -1;

	// and send request ;
	return 0;
}
int async_thumbnail_creator::close_file()						//
{
	exiting = true;

	if (m_worker_process)
		TerminateProcess(m_worker_process, 0);

	if (m_worker_thread)
		WaitForSingleObject(m_worker_thread, INFINITE);

	m_worker_process = NULL;
	m_worker_thread = NULL;
	m_file_name[0] = NULL;

	return 0;
}
int async_thumbnail_creator::seek(__int64 target)				// seek to a frame, and start worker thread
{
	retry_left = max_retry;
	seek_target = target;
	got_frame = false;
	ResumeThread(m_worker_thread);
	return 0;
}
int async_thumbnail_creator::get_frame(bool block)				// return the frame if worker has got the picture, or error codes
{
	CAutoLock lck(&m_cs);

	return 0;
}

int async_thumbnail_creator::reconnect()
{
	return 0;
}

int async_thumbnail_creator::on_error(int error_code)
{
	return 0;
}

DWORD async_thumbnail_creator::worker_thread()
{
	// run worker thread
	wchar_t worker_path[MAX_PATH];
	GetModuleFileNameW(NULL, worker_path, MAX_PATH);
	wcscpy(wcsrchr(worker_path, L'\\'), L"\\Thumbnail.exe");
	SHELLEXECUTEINFO ShExecInfo = {0};
	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	ShExecInfo.hwnd = NULL;
	ShExecInfo.lpFile = worker_path;
	ShExecInfo.lpParameters = NULL;	
	ShExecInfo.lpDirectory = NULL;
	ShExecInfo.nShow = SW_SHOW;
	ShExecInfo.hInstApp = NULL;	
	ShExecInfo.lpVerb = L"open";

	ShellExecuteEx(&ShExecInfo);

	int o;
	// open file
	if ((o = r.open_file(m_file_name)) < 0)
		on_error(o);
	if ((o=r.set_out_format(pixel_format, width, height)) < 0)
		on_error(o);

	while (!exiting)
	{
		if (seek_target >= 0)
		{
			// seek
			if ((o=r.seek(seek_target))<0)
				on_error(o);

			// get frame
			if ((o = r.get_one_frame()) < 0)
				on_error(o);

			got_frame = true;
		}
// 		else if (seek_target == -2)
// 		{
// 			// get next frame
// 			if ((o = r.get_one_frame()) < 0)
// 				on_error(o);
// 		}


		SuspendThread(m_worker_thread);
	}

	return 0;
}