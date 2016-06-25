/*
 *  Copyright (C) 2003-2006 Gabest
 *  http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include "../BaseSplitter/BaseSplitter.h"
#include "MpegSplitterFile.h"
#include "TrayMenu.h"
#include "IOffsetMetadata.h"

class CMpegSplitterOutputPin;

#define max_offset_frame_count 128
#define max_offset_items 128
typedef struct _offset_items
{
	REFERENCE_TIME time_start;
	int frame_count;
	int offsets[max_offset_frame_count];
} offset_item;

// changed GUID to diff from 3dtv.at's
class __declspec(uuid("8eD7B1DE-3B84-4817-A96F-4C94728B1AAE"))
	CMpegSplitterFilter : public CBaseSplitterFilter, public IAMStreamSelect, public ITrayMenuCallback, public IOffsetMetadata
{
	friend class CMpegSplitterOutputPin;

	REFERENCE_TIME	m_rtStartOffset;
	bool			m_pPipoBimbo;
	CHdmvClipInfo	m_ClipInfo;

protected:
	// my12doom's codes
	CAutoPtr<TrayMenu> m_traymenu;
	bool m_PD10;

	// mvc right eye handle codes
	bool m_for_encoding;
	bool m_mvc_found;
	CAutoPtr<Packet> m_p;
	CAutoPtrList<Packet> m_pl;
	REFERENCE_TIME m_rtPrev, m_rtOffset, m_rtMaxShift;
	bool m_fHasAccessUnitDelimiters;
	CCritSec m_dummylock;
	CCritSec m_queuelock;
	CAutoPtrList<Packet> m_queue;
	HRESULT dummy_deliver_packet(CAutoPtr<Packet> p);
	// offset metadata
	offset_item m_offset_items[max_offset_items];
	int m_offset_index; //=0
	CCritSec m_offset_item_lock;
	static const REFERENCE_TIME NEXT_IDR_FRAME = -11;

	// end my12doom


	CAutoPtr<CMpegSplitterFile> m_pFile;
	HRESULT CreateOutputs(IAsyncReader* pAsyncReader);
	void	ReadClipInfo(LPCOLESTR pszFileName);

	bool DemuxInit();
	void DemuxSeek(REFERENCE_TIME rt);
	bool DemuxLoop();
	bool BuildPlaylist(LPCTSTR pszFileName, CAtlList<CHdmvClipInfo::PlaylistItem>& files);

	HRESULT DemuxNextPacket(REFERENCE_TIME rtStartOffset);

	REFERENCE_TIME m_rtPlaylistDuration;

public:
	CMpegSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr, const CLSID& clsid = __uuidof(CMpegSplitterFilter));
	void SetPipo(bool bPipo) {
		m_pPipoBimbo = bPipo;
	};

	DECLARE_IUNKNOWN
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);
	STDMETHODIMP GetClassID(CLSID* pClsID);
	STDMETHODIMP Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE* pmt);

	// IAMStreamSelect
	STDMETHODIMP Count(DWORD* pcStreams);
	STDMETHODIMP Enable(long lIndex, DWORD dwFlags);
	STDMETHODIMP Info(long lIndex, AM_MEDIA_TYPE** ppmt, DWORD* pdwFlags, LCID* plcid, DWORD* pdwGroup, WCHAR** ppszName, IUnknown** ppObject, IUnknown** ppUnk);

	// my12doom's helper function for IAMStreamSelect
	STDMETHODIMP EnableCore(long lIndex, DWORD dwFlags); 
	HRESULT RemoveDownstream(CComPtr<IPin> &input_pin);

	// IMVC
	STDMETHODIMP IsMVC();
	STDMETHODIMP GetPD10(BOOL *Enabled);
	STDMETHODIMP SetPD10(BOOL Enable);

	// IOffsetMetadata
	HRESULT GetOffset(REFERENCE_TIME time, REFERENCE_TIME frame_time, int * offset_out);

	// ITrayMenuCallback
	HRESULT Click(int id);
	HRESULT BeforeShow();
};

class CMpegSplitterOutputPin : public CBaseSplitterOutputPin, protected CCritSec
{
	friend class CMpegSplitterFilter;
	// mvc queue
	CAutoPtrList<Packet> m_q1011, m_q1012;

	CAutoPtr<Packet> m_p;
	CAutoPtrList<Packet> m_pl;
	REFERENCE_TIME m_rtPrev, m_rtOffset, m_rtMaxShift;
	bool m_fHasAccessUnitDelimiters;
	bool m_bFilterDTSMA;

protected:
	HRESULT DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
	HRESULT DeliverPacket(CAutoPtr<Packet> p);
	HRESULT DeliverMVCPacket(CAutoPtr<Packet> p);
	HRESULT DeliverEndFlush();

public:
	CMpegSplitterOutputPin(CAtlArray<CMediaType>& mts, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);
	virtual ~CMpegSplitterOutputPin();
	STDMETHODIMP	Connect(IPin* pReceivePin, const AM_MEDIA_TYPE* pmt);
	void			SetMaxShift(REFERENCE_TIME rtMaxShift) {
		m_rtMaxShift = rtMaxShift;
	};
};

class __declspec(uuid("8FD7B1DE-3B84-4817-A96F-4C94728B1AAE"))
CMpegSourceFilter : public CMpegSplitterFilter
{
public:
	CMpegSourceFilter(LPUNKNOWN pUnk, HRESULT* phr, const CLSID& clsid = __uuidof(CMpegSourceFilter));
};