#include <windows.h>

HRESULT myCreateInstance(CLSID clsid, IID iid, void**out);
HRESULT GetFileSource(const wchar_t *filename, CLSID *out, bool use_mediainfo = true);
bool isSlowExtention(const wchar_t *filename);