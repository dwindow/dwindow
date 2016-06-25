#include <Windows.h>

HRESULT start_check_for_update(HWND callback_window, DWORD callback_message);			// threaded, S_OK: thread started. S_FALSE: thread already started, this call is ignored, no callback will be made.
HRESULT get_update_result(wchar_t *description, int *new_rev, wchar_t *url);