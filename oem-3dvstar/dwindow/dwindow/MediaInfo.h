#pragma once

#include <Windows.h>

typedef struct _media_info_output
{
	struct _media_info_output *next;

	int level_depth;
	wchar_t key[200];
	wchar_t value[20480];
} media_info_entry;

HRESULT show_media_info(const wchar_t *filename, HWND parent);
HRESULT get_mediainfo(const wchar_t *filename, media_info_entry **out, bool use_localization = false);
HRESULT free_mediainfo(media_info_entry *p);