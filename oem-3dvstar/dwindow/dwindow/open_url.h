// no thread-safe and non-modal support, sorry
#pragma once

#include <Windows.h>
#include "resource.h"

HRESULT open_URL(HINSTANCE inst, HWND parent, wchar_t *url);