//////////////////////////////////////////////////////////////////////////
//
// PresentEngine.h: Defines the D3DPresentEngine object.
// 
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
//
//////////////////////////////////////////////////////////////////////////

#pragma once

//-----------------------------------------------------------------------------
// D3DPresentEngine class
//
// This class creates the Direct3D device, allocates Direct3D surfaces for
// rendering, and presents the surfaces. This class also owns the Direct3D
// device manager and provides the IDirect3DDeviceManager9 interface via
// GetService.
//
// The goal of this class is to isolate the EVRCustomPresenter class from
// the details of Direct3D as much as possible.
//-----------------------------------------------------------------------------

class ID3DPresentEngine: public SchedulerCallback
{
public:
    // State of the Direct3D device.
    enum DeviceState
    {
        DeviceOK,
        DeviceReset,    // The device was reset OR re-created.
        DeviceRemoved,  // The device was removed.
    };

    // GetService: Returns the IDirect3DDeviceManager9 interface.
    // (The signature is identical to IMFGetService::GetService but 
    // this object does not derive from IUnknown.)
    virtual HRESULT GetService(REFGUID guidService, REFIID riid, void** ppv) PURE;
    virtual HRESULT CheckFormat(D3DFORMAT format) PURE;

    // Video window / destination rectangle:
    // This object implements a sub-set of the functions defined by the 
    // IMFVideoDisplayControl interface. However, some of the method signatures 
    // are different. The presenter's implementation of IMFVideoDisplayControl 
    // calls these methods.
    virtual HRESULT SetVideoWindow(HWND hwnd) PURE;
    virtual HRESULT SetDestinationRect(const RECT& rcDest) PURE;
    virtual void ReleaseResources() PURE;

	virtual HRESULT CreateVideoSamples(IMFMediaType *pFormat, VideoSampleList& videoSampleQueue) PURE;


    virtual HRESULT CheckDeviceState(DeviceState *pState) PURE;
    virtual UINT RefreshRate() PURE;
	virtual RECT GetDestinationRect() PURE;
	virtual HWND GetVideoWindow() PURE;

};

class DefaultD3DPresentEngine : public ID3DPresentEngine
{
public:
    DefaultD3DPresentEngine(HRESULT& hr);
    virtual ~DefaultD3DPresentEngine();

    // GetService: Returns the IDirect3DDeviceManager9 interface.
    // (The signature is identical to IMFGetService::GetService but 
    // this object does not derive from IUnknown.)
    virtual HRESULT GetService(REFGUID guidService, REFIID riid, void** ppv);
    virtual HRESULT CheckFormat(D3DFORMAT format);

    // Video window / destination rectangle:
    // This object implements a sub-set of the functions defined by the 
    // IMFVideoDisplayControl interface. However, some of the method signatures 
    // are different. The presenter's implementation of IMFVideoDisplayControl 
    // calls these methods.
    HRESULT SetVideoWindow(HWND hwnd);
    HRESULT SetDestinationRect(const RECT& rcDest);
    void    ReleaseResources();

	HRESULT CreateVideoSamples(IMFMediaType *pFormat, VideoSampleList& videoSampleQueue);


	HRESULT CheckDeviceState(ID3DPresentEngine::DeviceState *pState);
    HRESULT PresentSample(IMFSample* pSample, LONGLONG llTarget, int id); 
	HRESULT PrerollSample(IMFSample* pSample, LONGLONG llTarget, int id); 
    UINT    RefreshRate() { return m_DisplayMode.RefreshRate; }
	RECT    GetDestinationRect() { return m_rcDestRect; };
	HWND    GetVideoWindow() { return m_hwnd; }

protected:
    HRESULT InitializeD3D();
    HRESULT GetSwapChainPresentParameters(IMFMediaType *pType, D3DPRESENT_PARAMETERS* pPP);
    HRESULT CreateD3DDevice();
	HRESULT CreateD3DSample(IDirect3DSurface9 *pSurface, IMFSample **ppVideoSample);
    HRESULT UpdateDestRect();

    // A derived class can override these handlers to allocate any additional D3D resources.
    virtual HRESULT OnCreateVideoSamples(D3DPRESENT_PARAMETERS& pp) { return S_OK; }
    virtual void    OnReleaseResources() { }
    virtual void    PaintFrameWithGDI();

protected:
    UINT                        m_DeviceResetToken;     // Reset token for the D3D device manager.
	HANDLE						m_DeviceHandle;

    HWND                        m_hwnd;                 // Application-provided destination window.
    RECT                        m_rcDestRect;           // Destination rectangle.
    D3DDISPLAYMODE              m_DisplayMode;          // Adapter's display mode.

    CritSec                     m_ObjectLock;           // Thread lock for the D3D device.

    // COM interfaces
    IDirect3D9Ex                *m_pD3D9;
    IDirect3DDevice9Ex          *m_pDevice;
    IDirect3DDeviceManager9     *m_pDeviceManager;        // Direct3D device manager.
    IDirect3DSurface9           *m_pSurfaceRepaint;       // Surface for repaint requests.
};