/*
 * $Id: IPinHook.cpp 3087 2011-05-05 22:11:00Z jonasno $
 *
 * (C) 2003-2006 Gabest
 * (C) 2006-2011 see AUTHORS
 *
 * This file is part of mplayerc.
 *
 * Mplayerc is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mplayerc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <d3dx9.h>
#include <dxva.h>
#include <dxva2api.h>
#include <streams.h>
#include "IPinHook.h"

#if defined(_DEBUG) && defined(DXVA_LOGFILE_A)
#define LOG_FILE_DXVA				_T("dxva_ipinhook.log")
#define LOG_FILE_PICTURE			_T("picture.log")
#define LOG_FILE_SLICELONG			_T("slicelong.log")
#define LOG_FILE_SLICESHORT			_T("sliceshort.log")
#define LOG_FILE_BITSTREAM			_T("bitstream.log")
#endif

//#define LOG_BITSTREAM
//#define LOG_MATRIX

REFERENCE_TIME		g_tSegmentStart			= 0;
REFERENCE_TIME		g_tSampleStart			= 0;
GUID				g_guidDXVADecoder		= GUID_NULL;
int					g_nDXVAVersion			= 0;

IPinCVtbl*			g_pPinCVtbl				= NULL;
IMemInputPinCVtbl*	g_pMemInputPinCVtbl		= NULL;

// === DirectShow hooks
static HRESULT (STDMETHODCALLTYPE * NewSegmentOrg)(IPinC * This, /* [in] */ REFERENCE_TIME tStart, /* [in] */ REFERENCE_TIME tStop, /* [in] */ double dRate) = NULL;

static HRESULT STDMETHODCALLTYPE NewSegmentMine(IPinC * This, /* [in] */ REFERENCE_TIME tStart, /* [in] */ REFERENCE_TIME tStop, /* [in] */ double dRate)
{
	g_tSegmentStart = tStart;
	return NewSegmentOrg(This, tStart, tStop, dRate);
}

static HRESULT ( STDMETHODCALLTYPE *ReceiveOrg )( IMemInputPinC * This, IMediaSample *pSample) = NULL;

static HRESULT STDMETHODCALLTYPE ReceiveMine(IMemInputPinC * This, IMediaSample *pSample)
{
	REFERENCE_TIME rtStart, rtStop;
	if(pSample && SUCCEEDED(pSample->GetTime(&rtStart, &rtStop))) {
		g_tSampleStart = rtStart;
	}
	return ReceiveOrg(This, pSample);
}

void UnhookNewSegmentAndReceive()
{
	BOOL res;
	DWORD flOldProtect = 0;

	// Casimir666 : unhook previous VTables
	if (g_pPinCVtbl && g_pMemInputPinCVtbl) {
		res = VirtualProtect(g_pPinCVtbl, sizeof(IPinCVtbl), PAGE_WRITECOPY, &flOldProtect);
		if (g_pPinCVtbl->NewSegment == NewSegmentMine) {
			g_pPinCVtbl->NewSegment = NewSegmentOrg;
		}
		res = VirtualProtect(g_pPinCVtbl, sizeof(IPinCVtbl), flOldProtect, &flOldProtect);

		res = VirtualProtect(g_pMemInputPinCVtbl, sizeof(IMemInputPinCVtbl), PAGE_WRITECOPY, &flOldProtect);
		if (g_pMemInputPinCVtbl->Receive == ReceiveMine) {
			g_pMemInputPinCVtbl->Receive = ReceiveOrg;
		}
		res = VirtualProtect(g_pMemInputPinCVtbl, sizeof(IMemInputPinCVtbl), flOldProtect, &flOldProtect);

		g_pPinCVtbl			= NULL;
		g_pMemInputPinCVtbl = NULL;
		NewSegmentOrg		= NULL;
		ReceiveOrg			= NULL;
	}
}

bool HookNewSegmentAndReceive(IPinC* pPinC, IMemInputPinC* pMemInputPinC)
{
	if(!pPinC || !pMemInputPinC || (GetVersion()&0x80000000)) {
		return false;
	}

	g_tSegmentStart		= 0;
	g_tSampleStart		= 0;

	BOOL res;
	DWORD flOldProtect = 0;

	UnhookNewSegmentAndReceive();

	// Casimir666 : change sizeof(IPinC) to sizeof(IPinCVtbl) to fix crash with EVR hack on Vista!
	res = VirtualProtect(pPinC->lpVtbl, sizeof(IPinCVtbl), PAGE_WRITECOPY, &flOldProtect);
	if(NewSegmentOrg == NULL) {
		NewSegmentOrg = pPinC->lpVtbl->NewSegment;
	}
	pPinC->lpVtbl->NewSegment = NewSegmentMine; // Function sets global variable(s)
	res = VirtualProtect(pPinC->lpVtbl, sizeof(IPinCVtbl), flOldProtect, &flOldProtect);

	// Casimir666 : change sizeof(IMemInputPinC) to sizeof(IMemInputPinCVtbl) to fix crash with EVR hack on Vista!
	res = VirtualProtect(pMemInputPinC->lpVtbl, sizeof(IMemInputPinCVtbl), PAGE_WRITECOPY, &flOldProtect);
	if(ReceiveOrg == NULL) {
		ReceiveOrg = pMemInputPinC->lpVtbl->Receive;
	}
	pMemInputPinC->lpVtbl->Receive = ReceiveMine; // Function sets global variable(s)
	res = VirtualProtect(pMemInputPinC->lpVtbl, sizeof(IMemInputPinCVtbl), flOldProtect, &flOldProtect);

	g_pPinCVtbl			= pPinC->lpVtbl;
	g_pMemInputPinCVtbl = pMemInputPinC->lpVtbl;

	return true;
}


HRESULT hookIPin_Recieve(IPin* pin, proc_IMemInputPin_Receive newRecieve, proc_IMemInputPin_Receive *oldRecieve)
{
	if (!pin)
		return E_POINTER;

	IMemInputPin *p = NULL;
	pin->QueryInterface(IID_IMemInputPin, (void**)&p);
	if (!p)
		return E_NOINTERFACE;

	IMemInputPinC *p2 = (IMemInputPinC*)p;
	DWORD flOldProtect = 0;


	BOOL res = VirtualProtect(p2->lpVtbl, sizeof(IMemInputPinCVtbl), PAGE_WRITECOPY, &flOldProtect);
	if(oldRecieve != NULL) {
		*oldRecieve = p2->lpVtbl->Receive;
	}

	if (newRecieve)
		p2->lpVtbl->Receive = newRecieve; // Function sets global variable(s)

	res = VirtualProtect(p2->lpVtbl, sizeof(IMemInputPinCVtbl), flOldProtect, &flOldProtect);

	p->Release();

	return S_OK;
}