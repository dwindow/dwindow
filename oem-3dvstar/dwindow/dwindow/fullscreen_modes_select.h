#pragma once
#include <d3d9.h>

HRESULT select_fullscreen_mode(HINSTANCE instance, HWND parent, D3DDISPLAYMODE *modes, int modes_count, int *selected);
// selected: 0 ~ modes_count-1 : that mode, others: auto
// return S_OK if the user clicked OK
// return S_FALSE on cancel, selected undefined in this case