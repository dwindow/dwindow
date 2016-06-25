//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright(c) 2011 Intel Corporation. All Rights Reserved.
//

#ifndef __IGFX_S3DCONTROL_H
#define __IGFX_S3DCONTROL_H

#include <d3d9.h>
#include <dxva2api.h>

#define IGFX_MAX_S3D_MODES 20

typedef struct _IGFX_DISPLAY_MODE
{
    // Display mode: Set values to 0 for current mode
    // Note: All modes with 32BPP color only
    ULONG ulResWidth;
    ULONG ulResHeight;
    ULONG ulRefreshRate; // in Hz 
    ULONG ulReserved1; // reserved for now, must be zero

    DWORD dwReserved2; // reserved for now, must be zero
    DWORD dwReserved3; // reserved for now, must be zero
} IGFX_DISPLAY_MODE;

typedef struct _IGFX_S3DCAPS
{
    // number of entries in S3DSupportedModes[]
    ULONG ulNumEntries; 
    // array of display modes supported in S3D
    IGFX_DISPLAY_MODE S3DSupportedModes[IGFX_MAX_S3D_MODES]; 
} IGFX_S3DCAPS;

class IGFXS3DControl
{
public:
    virtual ~IGFXS3DControl() {};

    // Returns true if S3D is supported by the platform and exposes supported display modes 
    virtual HRESULT GetS3DCaps(IGFX_S3DCAPS *pCaps) = 0;

    // Switch the monitor to 3D mode
    // Call with NULL to use current display mode
    virtual HRESULT SwitchTo3D(IGFX_DISPLAY_MODE *pMode) = 0; 

    // Switch the monitor back to 2D mode
    // Call with NULL to use current display mode
    virtual HRESULT SwitchTo2D(IGFX_DISPLAY_MODE *pMode) = 0;
    
    // Share device create by the app. Note: the app must create the device while being in S3D mode, 
    // so call SwitchToS3DMode first, then create device, then call SetDevice.
    virtual HRESULT SetDevice(IDirect3DDeviceManager9 *pDeviceManager) = 0;  

    // Activate left view, requires device to be set
    virtual HRESULT SelectLeftView() = 0;

    // Activates right view, requires device to be set
    virtual HRESULT SelectRightView() = 0; 
};

#ifdef _WIN32
    #define IGFX_CDECL __cdecl
    #define IGFX_STDCALL __stdcall
#else
    #define IGFX_CDECL
    #define IGFX_STDCALL
#endif /* _WIN32 */

extern IGFXS3DControl * IGFX_CDECL CreateIGFXS3DControl();

#undef IGFX_CDECL
#undef IGFX_STDCALL

#endif // __IGFX_S3DCONTROL_H