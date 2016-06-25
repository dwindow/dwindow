#pragma once

#include <Windows.h>
#include <wchar.h>

int get_audio_device_count();
int get_default_audio_device();
HRESULT active_audio_device(int n);
HRESULT get_audio_device_name(int n, wchar_t *name);
