#pragma once
#include "..\Thumbnail\remote_thumbnail.h"
#include "command_queue.h"

// class async_thumbnail_creator;
// 
// class remote_thumbnail_mine : public remote_thumbnail
// {
// public:
// 	remote_thumbnail_mine(async_thumbnail_creator *owner){}
// 	~remote_thumbnail_mine(){}
// protected:
// 	int on_disconnect();
// 	async_thumbnail_creator *m_owner;
// };
// 
class async_thumbnail_creator
{
public:
	async_thumbnail_creator();
	~async_thumbnail_creator();

	int open_file(const wchar_t*file);		// 
	int close_file();						//
	int seek(__int64 target);				// seek to a frame, and start worker thread
	int get_frame(bool block = false);		// return the frame if worker has got the picture, or error codes

protected:
	friend class remote_thumbnail;

	int reconnect();						// on any errors, kill worker process and recreate it, reopen file, reseek, retry get_frame until max retry count reached.
	int on_error(int error_code);

	DWORD worker_thread();
	static DWORD WINAPI worker_thread_entry(LPVOID p){return ((async_thumbnail_creator*)p)->worker_thread();}

	// working environment
	remote_thumbnail r;
	wchar_t m_pipe_name[200];
	HANDLE m_worker_process;
	HANDLE m_worker_thread;
	CCritSec m_cs;
	bool exiting;
	int width;
	int height;
	int pixel_format;

	// working state
	wchar_t m_file_name[MAX_PATH];
	__int64 seek_target;
	bool got_frame;
	int retry_left;
};
