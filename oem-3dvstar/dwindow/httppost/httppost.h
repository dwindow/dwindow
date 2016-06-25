#pragma once
#include <list>
#include <map>

#ifndef LINUX
#else
	typedef long long __int64;
	#include <wchar.h>
	FILE * _wfopen(const wchar_t *file, const wchar_t *rights);
	int _wtoi(const wchar_t *str);
#endif


class IHttpSendingCallback
{
public:
	virtual int HttpSendingCallback(int current, int total) = 0;
};

class httppost
{
public:
	httppost(const wchar_t *url);
	~httppost();

	int setURL(const wchar_t *url);
	int addHeader(const wchar_t*key, const wchar_t*value);
	int addFile(const wchar_t *name, const wchar_t *local_filename);
	int addFile(const wchar_t *name, const void *data, int size);
	int addFormItem(const wchar_t *name, const wchar_t *value);

	int send_request(int max_relocation = 5);
	std::map<wchar_t*,wchar_t*> get_response_headers();
	__int64 get_content_length();
	int read_content(void *buf, int size);
	int close_connection();

	int set_callback(IHttpSendingCallback *cb);
protected:

	typedef struct
	{
		union
		{
			wchar_t *form_data;
			void *file_data;
			wchar_t *file_name;
		};
		int data_size;

		enum
		{
			form_item_data,
			form_item_file_data,
			form_item_file_name,
		} type;

		wchar_t *name;

	} form_item;

	std::list<form_item> m_form_items;
	std::map<wchar_t*,wchar_t*> m_headers;
	std::map<wchar_t*,wchar_t*> m_request_headers;
	wchar_t *m_object;
	wchar_t *m_server;
	wchar_t *m_URL;
	int m_port;
	int m_sock;
	__int64 m_content_length;
	__int64 m_content_read;
	int m_request_content_sent;
	int m_request_content_size;
	IHttpSendingCallback *m_cb;


protected:
	void delete_form_item(form_item &item);
	int send_item(int sock, form_item &item);
	int calculate_item_size(form_item &item);
	int callback();
};