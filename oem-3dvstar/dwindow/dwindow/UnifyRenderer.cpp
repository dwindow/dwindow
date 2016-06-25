#include "UnifyRenderer.h"


// VMR 9 windowless

CVMR9Windowless::CVMR9Windowless(HWND hwnd)
{
	CComPtr<IDirect3D9>			m_D3D;
	CComPtr<IDirect3DDevice9>	m_D3Ddevice;   
	m_D3D.Attach(Direct3DCreate9(D3D_SDK_VERSION));

	D3DPRESENT_PARAMETERS d3dpp;    
	ZeroMemory( &d3dpp, sizeof(d3dpp) );   
	d3dpp.Windowed         = TRUE;   
	d3dpp.SwapEffect       = D3DSWAPEFFECT_DISCARD;   
	d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;   
	d3dpp.hDeviceWindow    = hwnd;   

	m_D3D->CreateDevice(    
		D3DADAPTER_DEFAULT, // always the primary display adapter   
		D3DDEVTYPE_HAL,   
		NULL,   
		D3DCREATE_HARDWARE_VERTEXPROCESSING,   
		&d3dpp,   
		&m_D3Ddevice);

	if (m_D3Ddevice == NULL)
		return;

	m_D3Ddevice->CreateOffscreenPlainSurface(
		4096, 4096,
		D3DFMT_A8R8G8B8,
		D3DPOOL_SYSTEMMEM,
		&m_surface, NULL);

	base_filter.CoCreateInstance(CLSID_VideoMixingRenderer9);
	if (!base_filter)
		return;

	CComQIPtr<IVMRFilterConfig9, &IID_IVMRFilterConfig9> mode_cfg(base_filter);
	mode_cfg->SetRenderingMode(VMR9Mode_Windowless);


	base_filter->QueryInterface(IID_IVMRWindowlessControl9, (void**)&m_config);
	base_filter->QueryInterface(IID_IVMRMixerBitmap9, (void**)&m_bmp);

	SetVideoClippingWindow(hwnd);
}

CVMR9Windowless::~CVMR9Windowless()
{
	m_surface = NULL;
}

HRESULT CVMR9Windowless::DisplayModeChanged()
{
	return m_config->DisplayModeChanged();
}

HRESULT CVMR9Windowless::RepaintVideo(HWND hwnd, HDC hdc)
{
	return m_config->RepaintVideo(hwnd, hdc);
}

HRESULT CVMR9Windowless::GetNativeVideoSize(LONG *lpWidth, LONG *lpHeight, LONG *lpARWidth, LONG *lpARHeight)
{
	return m_config->GetNativeVideoSize(lpWidth, lpHeight, lpARWidth, lpARHeight);
}

HRESULT CVMR9Windowless::SetVideoPosition(const LPRECT lpSRCRect, const LPRECT lpDSTRect)
{
	return m_config->SetVideoPosition(lpSRCRect, lpDSTRect);
}

HRESULT CVMR9Windowless::SetVideoClippingWindow(HWND hwnd)
{
	return m_config->SetVideoClippingWindow(hwnd);
}

HRESULT CVMR9Windowless::SetAspectRatioMode(DWORD mode)
{
	return m_config->SetAspectRatioMode(mode);
}

HRESULT CVMR9Windowless::SetOutputRect(DWORD stream, const UnifyVideoNormalizedRect *rect)
{
	VMR9NormalizedRect rect_vmr9 = {rect->left, rect->top, rect->right, rect->bottom};
	CComQIPtr<IVMRMixerControl9, &IID_IVMRMixerControl9> mixer(base_filter);
	return mixer->SetOutputRect(stream, &rect_vmr9);
}

HRESULT CVMR9Windowless::SetAlphaBitmap(UnifyAlphaBitmap &bitmap)
{
	HRESULT hr = S_OK;

	if (FAILED(hr))
		return hr;

	RECT src = {0, 0, bitmap.width, bitmap.height};
	VMR9NormalizedRect dst = {bitmap.left, bitmap.top, bitmap.fwidth+bitmap.left, bitmap.fheight+bitmap.top};
	D3DLOCKED_RECT locked_rect;
	m_surface->LockRect(&locked_rect, NULL, NULL);
	BYTE *psrc = (BYTE*)bitmap.data;
	BYTE *pdst = (BYTE*) locked_rect.pBits;
	for(int i=0; i<bitmap.height; i++)
	{
		memcpy(pdst, psrc, bitmap.width*4);
		psrc += bitmap.width*4;
		pdst += locked_rect.Pitch;
	}
	m_surface->UnlockRect();

	VMR9AlphaBitmap bmp_info;
	bmp_info.dwFlags =  VMR9AlphaBitmap_FilterMode;
	bmp_info.hdc = NULL;
	bmp_info.pDDS = m_surface;
	bmp_info.rSrc = src;
	bmp_info.rDest= dst;
	bmp_info.fAlpha = 1;
	bmp_info.clrSrcKey = 0xffffffff;
	bmp_info.dwFilterMode = MixerPref9_AnisotropicFiltering;

	return m_bmp->SetAlphaBitmap(&bmp_info);
}

HRESULT CVMR9Windowless::ClearAlphaBitmap()
{
	VMR9AlphaBitmap paras;
	if (FAILED(m_bmp->GetAlphaBitmapParameters(&paras)))
		return E_FAIL;

	paras.rDest.left = paras.rDest.right = paras.rDest.top = paras.rDest.bottom = 0;
	return m_bmp->UpdateAlphaBitmapParameters(&paras);
}
// end VMR 9 windowless

// VMR 7 windowless

CVMR7Windowless::CVMR7Windowless(HWND hwnd)
{
	base_filter.CoCreateInstance(CLSID_VideoMixingRenderer);
	if (!base_filter)
		return;

	CComQIPtr<IVMRFilterConfig, &IID_IVMRFilterConfig> mode_cfg(base_filter);
	mode_cfg->SetRenderingMode(VMRMode_Windowless);
	mode_cfg->SetNumberOfStreams(1);

	base_filter->QueryInterface(IID_IVMRWindowlessControl, (void**)&m_config);
	base_filter->QueryInterface(IID_IVMRMixerBitmap, (void**)&m_bmp);

	SetVideoClippingWindow(hwnd);

	// ddraw
	CComPtr<IDirectDraw> ddraw1;
	if (FAILED(DirectDrawCreate(NULL, &ddraw1, NULL)))
		return;

	ddraw1->QueryInterface(IID_IDirectDraw7, (void**)&m_ddraw);
	m_ddraw->SetCooperativeLevel(hwnd, DDSCL_NORMAL);
}

CVMR7Windowless::~CVMR7Windowless()
{
}

HRESULT CVMR7Windowless::DisplayModeChanged()
{
	return m_config->DisplayModeChanged();
}

HRESULT CVMR7Windowless::RepaintVideo(HWND hwnd, HDC hdc)
{
	return m_config->RepaintVideo(hwnd, hdc);
}

HRESULT CVMR7Windowless::GetNativeVideoSize(LONG *lpWidth, LONG *lpHeight, LONG *lpARWidth, LONG *lpARHeight)
{
	return m_config->GetNativeVideoSize(lpWidth, lpHeight, lpARWidth, lpARHeight);
}

HRESULT CVMR7Windowless::SetVideoPosition(const LPRECT lpSRCRect, const LPRECT lpDSTRect)
{
	return m_config->SetVideoPosition(lpSRCRect, lpDSTRect);
}

HRESULT CVMR7Windowless::SetVideoClippingWindow(HWND hwnd)
{
	return m_config->SetVideoClippingWindow(hwnd);
}

HRESULT CVMR7Windowless::SetAspectRatioMode(DWORD mode)
{
	return m_config->SetAspectRatioMode(mode);
}

HRESULT CVMR7Windowless::SetOutputRect(DWORD stream, const UnifyVideoNormalizedRect *rect)
{
	NORMALIZEDRECT rect_vmr9 = {rect->left, rect->top, rect->right, rect->bottom};
	CComQIPtr<IVMRMixerControl, &IID_IVMRMixerControl> mixer(base_filter);
	return mixer->SetOutputRect(stream, &rect_vmr9);
}

HRESULT CVMR7Windowless::SetAlphaBitmap(UnifyAlphaBitmap &bitmap)
{
	HRESULT hr = S_OK;
	m_surface = NULL;

	// create surface
	DDSURFACEDESC2 surfdesc    = { 0 };
	surfdesc.dwSize            = sizeof DDSURFACEDESC2;
	surfdesc.dwFlags           = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
	surfdesc.ddsCaps.dwCaps    = DDSCAPS_OFFSCREENPLAIN;
	surfdesc.dwWidth = bitmap.width;
	surfdesc.dwHeight = bitmap.height;

	if (FAILED(hr = m_ddraw->CreateSurface(&surfdesc, &m_surface, NULL)))
		return hr;

	// write to surface
	m_surface->Lock(NULL, &surfdesc, DDLOCK_WAIT | DDLOCK_NOSYSLOCK | DDLOCK_SURFACEMEMORYPTR, NULL);
	BYTE *psrc = (BYTE*) bitmap.data;
	BYTE *pdst = (BYTE*) surfdesc.lpSurface;
	for(int i=0; i<surfdesc.dwHeight; i++)
	{
		memcpy(pdst, psrc, surfdesc.dwWidth*4);
		pdst += surfdesc.lPitch;
		psrc += bitmap.width*4;
	}
	m_surface->Unlock(NULL);

	RECT src = {0, 0, bitmap.width, bitmap.height};
	NORMALIZEDRECT dst = {bitmap.left, bitmap.top, bitmap.fwidth+bitmap.left, bitmap.fheight+bitmap.top};



	VMRALPHABITMAP  bmp_info;
	bmp_info.dwFlags = VMRBITMAP_ENTIREDDS;
	bmp_info.hdc = NULL;
	bmp_info.pDDS = m_surface;
	bmp_info.rSrc = src;
	bmp_info.rDest= dst;
	bmp_info.fAlpha = 1;
	bmp_info.clrSrcKey = 0xffffffff;

	return m_bmp->SetAlphaBitmap(&bmp_info);

	/*
	LONG video_width = 0;
	LONG video_height = 0;

	if (FAILED(hr = GetNativeVideoSize(&video_width, &video_height, NULL, NULL)))
		return hr;

	if (video_width == 0 || video_height == 0)
		return E_FAIL;

	RECT src = {0,0,width,height};

	NORMALIZEDRECT dst = {((float)left)/video_width,
							  ((float)top)/video_height,
							  ((float)left+width)/video_width,
							  ((float)top+height)/video_height};

	*/
}

HRESULT CVMR7Windowless::ClearAlphaBitmap()
{
	VMRALPHABITMAP paras;
	if (FAILED(m_bmp->GetAlphaBitmapParameters(&paras)))
		return E_FAIL;

	paras.rDest.left = paras.rDest.right = paras.rDest.top = paras.rDest.bottom = 0;
	return m_bmp->UpdateAlphaBitmapParameters(&paras);
}
// end VMR 7 windowless


// EVR
CEVRVista::CEVRVista(HWND hwnd)
{
	CComPtr<IDirect3D9>			m_D3D;
	CComPtr<IDirect3DDevice9>	m_D3Ddevice;   
	m_D3D.Attach(Direct3DCreate9(D3D_SDK_VERSION));

	D3DPRESENT_PARAMETERS d3dpp;    
	ZeroMemory( &d3dpp, sizeof(d3dpp) );   
	d3dpp.Windowed         = TRUE;   
	d3dpp.SwapEffect       = D3DSWAPEFFECT_DISCARD;   
	d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;   
	d3dpp.hDeviceWindow    = hwnd;   

	m_D3D->CreateDevice(    
		D3DADAPTER_DEFAULT, // always the primary display adapter   
		D3DDEVTYPE_HAL,   
		NULL,   
		D3DCREATE_HARDWARE_VERTEXPROCESSING,   
		&d3dpp,   
		&m_D3Ddevice);

	if (m_D3Ddevice == NULL)
		return;

	m_D3Ddevice->CreateOffscreenPlainSurface(
		4096, 4096,
		D3DFMT_A8R8G8B8,
		D3DPOOL_SYSTEMMEM,
		&m_surface, NULL);

	base_filter.CoCreateInstance(CLSID_EnhancedVideoRenderer);
	if (!base_filter)
		return;

	CComQIPtr<IMFGetService, &IID_IMFGetService> get_service(base_filter);

	get_service->GetService(MR_VIDEO_RENDER_SERVICE, IID_IMFVideoDisplayControl, (void**)&m_config);
	get_service->GetService(MR_VIDEO_MIXER_SERVICE, IID_IMFVideoMixerControl, (void**)&m_mixer);
	get_service->GetService(MR_VIDEO_MIXER_SERVICE, IID_IMFVideoMixerBitmap, (void**)&m_bmp);

	SetVideoClippingWindow(hwnd);
}

CEVRVista::~CEVRVista()
{
	m_surface = NULL;
}

HRESULT CEVRVista::DisplayModeChanged()
{
	return S_OK; // evr has no need to do this?
}

HRESULT CEVRVista::RepaintVideo(HWND hwnd, HDC hdc)
{
	return m_config->RepaintVideo(); // don't need hwnd and hdc?
}

HRESULT CEVRVista::GetNativeVideoSize(LONG *lpWidth, LONG *lpHeight, LONG *lpARWidth, LONG *lpARHeight)
{
	SIZE dis;
	SIZE ar;
	HRESULT hr = m_config->GetNativeVideoSize(&dis, &ar);

	if (FAILED(hr))
		return hr;
	else
	{
		if (lpWidth)
			*lpWidth = dis.cx;
		if (lpHeight)
			*lpHeight = dis.cy;
		if (lpARWidth)
			*lpARWidth = ar.cx;
		if (lpARHeight)
			*lpARHeight = ar.cy;

		return hr;
	}
}

HRESULT CEVRVista::SetVideoPosition(const LPRECT lpSRCRect, const LPRECT lpDSTRect)
{
	LONG width;
	LONG height;

	HRESULT hr = GetNativeVideoSize(&width, &height, NULL, NULL);
	if (FAILED(hr))
		return hr;

	RECT rec;
	memcpy(&rec, lpDSTRect, sizeof(rec));

	MFVideoNormalizedRect src = {	(float)lpSRCRect->left/width,
									(float)lpSRCRect->top/height,
									(float)lpSRCRect->right/width,
									(float)lpSRCRect->bottom/height};

	return m_config->SetVideoPosition(&src, lpDSTRect);
}

HRESULT CEVRVista::SetVideoClippingWindow(HWND hwnd)
{
	return m_config->SetVideoWindow(hwnd);
}

HRESULT CEVRVista::SetAspectRatioMode(DWORD mode)
{
	return m_config->SetAspectRatioMode(mode);
}

HRESULT CEVRVista::SetOutputRect(DWORD stream, const UnifyVideoNormalizedRect *rect)
{
	MFVideoNormalizedRect rect_evr9 = {rect->left, rect->top, rect->right, rect->bottom};
	return m_mixer->SetStreamOutputRect(stream, &rect_evr9);
}

HRESULT CEVRVista::SetAlphaBitmap(UnifyAlphaBitmap &bitmap)
{
	HRESULT hr = S_OK;

	if (FAILED(hr))
		return hr;

	RECT src = {0, 0, bitmap.width, bitmap.height};
	MFVideoNormalizedRect dst = {bitmap.left, bitmap.top, bitmap.fwidth+bitmap.left, bitmap.fheight+bitmap.top};
	D3DLOCKED_RECT locked_rect;
	m_surface->LockRect(&locked_rect, NULL, NULL);
	BYTE *psrc = (BYTE*)bitmap.data;
	BYTE *pdst = (BYTE*) locked_rect.pBits;
	for(int i=0; i<bitmap.height; i++)
	{
		memcpy(pdst, psrc, bitmap.width*4);
		psrc += bitmap.width*4;
		pdst += locked_rect.Pitch;
	}
	m_surface->UnlockRect();

	MFVideoAlphaBitmap  bmp_info;
	bmp_info.GetBitmapFromDC = FALSE;
	bmp_info.bitmap.pDDS = m_surface;
	bmp_info.params.clrSrcKey = 0xffffffff;
	bmp_info.params.dwFlags = MFVideoAlphaBitmap_SrcRect | MFVideoAlphaBitmap_DestRect | MFVideoAlphaBitmap_FilterMode;
	bmp_info.params.rcSrc = src;
	bmp_info.params.nrcDest= dst;
	bmp_info.params.fAlpha = 1;
	bmp_info.params.dwFilterMode = D3DTEXF_LINEAR;

	return m_bmp->SetAlphaBitmap(&bmp_info);
}

HRESULT CEVRVista::ClearAlphaBitmap()
{
	return m_bmp->ClearAlphaBitmap();
}

// my12doom's renderer
Cmy12doomRenderer::Cmy12doomRenderer(HWND hwnd)
{
	HRESULT hr;
	m_renderer = new my12doomRenderer(NULL, &hr , hwnd);
	if (FAILED(hr) || !m_renderer)
		return;

	m_renderer->QueryInterface(IID_IBaseFilter, (void**)&base_filter);
}
Cmy12doomRenderer::~Cmy12doomRenderer()
{

}
// basic config
HRESULT Cmy12doomRenderer::DisplayModeChanged()
{
	return S_OK;
}
HRESULT Cmy12doomRenderer::RepaintVideo(HWND hwnd, HDC hdc)
{
	return m_renderer->repaint_video();
}
HRESULT Cmy12doomRenderer::GetNativeVideoSize(LONG *lpWidth, LONG *lpHeight, LONG *lpARWidth, LONG *lpARHeight)
{
	return E_NOTIMPL;
}
HRESULT Cmy12doomRenderer::SetVideoPosition(const LPRECT lpSRCRect, const LPRECT lpDSTRect)
{
	return S_OK;
}
HRESULT Cmy12doomRenderer::SetVideoClippingWindow(HWND hwnd)
{
	return m_renderer->set_window(hwnd, NULL);
}

HRESULT Cmy12doomRenderer::Pump()
{
	return m_renderer->pump();
}
HRESULT Cmy12doomRenderer::SetAspectRatioMode(DWORD mode)
{
	if (mode == UnifyARMode_None)
		return E_NOTIMPL;
	else if (mode == UnifyARMode_LetterBox)
		return S_OK;
	else
		return E_INVALIDARG;
}

// mixer
HRESULT Cmy12doomRenderer::SetOutputRect(DWORD stream, const UnifyVideoNormalizedRect *rect)
{
	return E_NOTIMPL;
}

// alpha bitmap
HRESULT Cmy12doomRenderer::SetAlphaBitmap(UnifyAlphaBitmap &bitmap)
{
	return m_renderer->set_bmp((BYTE*)bitmap.data, bitmap.width, bitmap.height, bitmap.fwidth, bitmap.fheight, bitmap.left, bitmap.top);
}
HRESULT Cmy12doomRenderer::ClearAlphaBitmap()
{
	return m_renderer->set_bmp(NULL, 0, 0, 0, 0, 0, 0);
}
HRESULT Cmy12doomRenderer::SetUI(void *data, int pitch)
{
	return m_renderer->set_ui(data, pitch);
}
HRESULT Cmy12doomRenderer::SetUIShow(bool show)
{
	return m_renderer->set_ui_visible(show);
}