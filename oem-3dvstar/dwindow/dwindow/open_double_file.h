// no thread-safe and non-modal support, sorry
#pragma once

#include <Windows.h>
#include "resource.h"

HRESULT open_double_file(HINSTANCE inst, HWND parent, wchar_t *left, wchar_t *right);