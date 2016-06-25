#pragma once
#include <windows.h>
#include <atlbase.h>
#include <dshow.h>

#include <d3d9.h>
#include <vmr9.h>

#include <evr9.h>
#include "..\renderer_prototype\my12doomRenderer.h"

typedef struct _UnifyVideoNormalizedRect
{
	float left;
	float top;
	float right;
	float bottom;
} UnifyVideoNormalizedRect;

typedef enum _UnifyAspectMode
{
	UnifyARMode_None	= 0,
	UnifyARMode_LetterBox	= UnifyARMode_None + 1
} UnifyAspectMode;

typedef struct _UnifyAlphaBitmap
{
	// data
	int width;
	int height;
	void *data;

	// position
	float left;
	float top;
	float fwidth;
	float fheight;
} UnifyAlphaBitmap;

class CUnifyRenderer
{
public:
	CComPtr<IBaseFilter> base_filter;
	
	// basic config
	virtual HRESULT DisplayModeChanged()=0;
	virtual HRESULT RepaintVideo(HWND hwnd, HDC hdc)=0;
    virtual HRESULT GetNativeVideoSize(LONG *lpWidth, LONG *lpHeight, LONG *lpARWidth, LONG *lpARHeight) = 0;
	virtual HRESULT SetVideoPosition(const LPRECT lpSRCRect, const LPRECT lpDSTRect) = 0;
	virtual HRESULT SetVideoClippingWindow(HWND hwnd)=0;
	virtual HRESULT SetVideoClippingWindow2(HWND hwnd){return E_NOTIMPL;}
	virtual HRESULT SetAspectRatioMode(DWORD mode)=0;
	virtual HRESULT Pump(){return E_NOTIMPL;}
	virtual HRESULT SetUI(void *data, int pitch){return E_NOTIMPL;}
	virtual HRESULT SetUIShow(bool show){return E_NOTIMPL;}

	// mixer
	virtual HRESULT SetOutputRect(DWORD stream, const UnifyVideoNormalizedRect *rect)=0;

	// alpha bitmap
	virtual HRESULT SetAlphaBitmap(UnifyAlphaBitmap &bitmap)=0;
	virtual HRESULT ClearAlphaBitmap()=0;
};

class CVMR7Windowless : public CUnifyRenderer
{
public:
	CVMR7Windowless(HWND hwnd);
	~CVMR7Windowless();
	// basic config
	virtual HRESULT DisplayModeChanged();
	virtual HRESULT RepaintVideo(HWND hwnd, HDC hdc);
    virtual HRESULT GetNativeVideoSize(LONG *lpWidth, LONG *lpHeight, LONG *lpARWidth, LONG *lpARHeight);
	virtual HRESULT SetVideoPosition(const LPRECT lpSRCRect, const LPRECT lpDSTRect);
	virtual HRESULT SetVideoClippingWindow(HWND hwnd);
	virtual HRESULT SetAspectRatioMode(DWORD mode);

	// mixer
	virtual HRESULT SetOutputRect(DWORD stream, const UnifyVideoNormalizedRect *rect);

	// alpha bitmap
	virtual HRESULT SetAlphaBitmap(UnifyAlphaBitmap &bitmap);
	virtual HRESULT ClearAlphaBitmap();

protected:
	CComPtr<IVMRWindowlessControl> m_config;
	CComPtr<IVMRMixerBitmap> m_bmp;

	// DirectDraw Surface
	CComPtr<IDirectDraw7> m_ddraw;
	CComPtr<IDirectDrawSurface7> m_surface;
};

class CVMR9Windowless : public CUnifyRenderer
{
public:
	CVMR9Windowless(HWND hwnd);
	~CVMR9Windowless();
	// basic config
	virtual HRESULT DisplayModeChanged();
	virtual HRESULT RepaintVideo(HWND hwnd, HDC hdc);
    virtual HRESULT GetNativeVideoSize(LONG *lpWidth, LONG *lpHeight, LONG *lpARWidth, LONG *lpARHeight);
	virtual HRESULT SetVideoPosition(const LPRECT lpSRCRect, const LPRECT lpDSTRect);
	virtual HRESULT SetVideoClippingWindow(HWND hwnd);
	virtual HRESULT SetAspectRatioMode(DWORD mode);

	// mixer
	virtual HRESULT SetOutputRect(DWORD stream, const UnifyVideoNormalizedRect *rect);

	// alpha bitmap
	virtual HRESULT SetAlphaBitmap(UnifyAlphaBitmap &bitmap);
	virtual HRESULT ClearAlphaBitmap();

protected:
	CComPtr<IVMRWindowlessControl9> m_config;
	CComPtr<IVMRMixerBitmap9> m_bmp;
	// D3D9 Surface
	CComPtr<IDirect3DSurface9>	m_surface;
};

class CEVRVista : public CUnifyRenderer
{
public:
	CEVRVista(HWND hwnd);
	~CEVRVista();
	// basic config
	virtual HRESULT DisplayModeChanged();
	virtual HRESULT RepaintVideo(HWND hwnd, HDC hdc);
    virtual HRESULT GetNativeVideoSize(LONG *lpWidth, LONG *lpHeight, LONG *lpARWidth, LONG *lpARHeight);
	virtual HRESULT SetVideoPosition(const LPRECT lpSRCRect, const LPRECT lpDSTRect);
	virtual HRESULT SetVideoClippingWindow(HWND hwnd);
	virtual HRESULT SetAspectRatioMode(DWORD mode);

	// mixer
	virtual HRESULT SetOutputRect(DWORD stream, const UnifyVideoNormalizedRect *rect);

	// alpha bitmap
	virtual HRESULT SetAlphaBitmap(UnifyAlphaBitmap &bitmap);
	virtual HRESULT ClearAlphaBitmap();

protected:
	CComPtr<IMFVideoDisplayControl> m_config;
	CComPtr<IMFVideoMixerBitmap> m_bmp;
	CComPtr<IMFVideoMixerControl> m_mixer;
	// D3D9 Surface
	CComPtr<IDirect3DSurface9>	m_surface;
};


class Cmy12doomRenderer : public CUnifyRenderer
{
public:
	Cmy12doomRenderer(HWND hwnd);
	~Cmy12doomRenderer();

	// basic config
	virtual HRESULT DisplayModeChanged();
	virtual HRESULT RepaintVideo(HWND hwnd, HDC hdc);
	virtual HRESULT GetNativeVideoSize(LONG *lpWidth, LONG *lpHeight, LONG *lpARWidth, LONG *lpARHeight);
	virtual HRESULT SetVideoPosition(const LPRECT lpSRCRect, const LPRECT lpDSTRect);
	virtual HRESULT SetVideoClippingWindow(HWND hwnd);
	virtual HRESULT Pump();
	virtual HRESULT SetAspectRatioMode(DWORD mode);

	// mixer
	virtual HRESULT SetOutputRect(DWORD stream, const UnifyVideoNormalizedRect *rect);

	// alpha bitmap
	virtual HRESULT SetAlphaBitmap(UnifyAlphaBitmap &bitmap);
	virtual HRESULT ClearAlphaBitmap();

	// UI
	virtual HRESULT SetUI(void *data, int pitch);
	virtual HRESULT SetUIShow(bool show);

protected:
	my12doomRenderer *m_renderer;
};