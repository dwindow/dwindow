// no thread-safe and non-modal support, sorry
#pragma once
#include <Windows.h>
#include "resource.h"

HRESULT latency_modify_dialog(HINSTANCE inst, HWND parent, int *latency, double *ratio, bool for_audio);
// for_audio: true: disable ratio option, and display audio hints