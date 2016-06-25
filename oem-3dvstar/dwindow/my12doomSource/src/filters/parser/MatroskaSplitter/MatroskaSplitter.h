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

#include <atlbase.h>
#include <atlcoll.h>
#include "MatroskaFile.h"
#include "../BaseSplitter/BaseSplitter.h"
#include <ITrackInfo.h>
#include "..\MpegSplitter\IOffsetMetadata.h"

class MatroskaPacket : public Packet
{
protected:
	int GetDataSize() {
		int size = 0;
		POSITION pos = bg->Block.BlockData.GetHeadPosition();
		while(pos) {
			size += bg->Block.BlockData.GetNext(pos)->GetCount();
		}
		return size;
	}
public:
	CAutoPtr<MatroskaReader::BlockGroup> bg;
};

class CMatroskaSplitterOutputPin : public CBaseSplitterOutputPin
{
	HRESULT DeliverBlock(MatroskaPacket* p);

	int m_nMinCache;
	REFERENCE_TIME m_rtDefaultDuration;

	CCritSec m_csQueue;
	CAutoPtrList<MatroskaPacket> m_packets;
	CAtlList<MatroskaPacket*> m_rob;

	typedef struct {
		REFERENCE_TIME rtStart, rtStop;
	} timeoverride;
	CAtlList<timeoverride> m_tos;

protected:
	HRESULT DeliverPacket(CAutoPtr<Packet> p);

public:
	CMatroskaSplitterOutputPin(
		int nMinCache, REFERENCE_TIME rtDefaultDuration,
		CAtlArray<CMediaType>& mts, LPCWSTR pName, CBaseFilter* pFilter, CCritSec* pLock, HRESULT* phr);
	virtual ~CMatroskaSplitterOutputPin();

	HRESULT DeliverEndFlush();
	HRESULT DeliverEndOfStream();
};

class __declspec(uuid("149D2E01-C32E-4939-80F6-C07B81015A7A"))
	CMatroskaSplitterFilter : public CBaseSplitterFilter, public ITrackInfo, public IOffsetMetadata
{
	void SetupChapters(LPCSTR lng, MatroskaReader::ChapterAtom* parent, int level = 0);
	void InstallFonts();
	void SendVorbisHeaderSample();

	CAutoPtr<MatroskaReader::CMatroskaNode> m_pSegment, m_pCluster, m_pBlock;

protected:
	CAutoPtr<MatroskaReader::CMatroskaFile> m_pFile;
	HRESULT CreateOutputs(IAsyncReader* pAsyncReader);

	CAtlMap<DWORD, MatroskaReader::TrackEntry*> m_pTrackEntryMap;
	CAtlArray<MatroskaReader::TrackEntry* > m_pOrderedTrackArray;
	MatroskaReader::TrackEntry* GetTrackEntryAt(UINT aTrackIdx);

	bool DemuxInit();
	void DemuxSeek(REFERENCE_TIME rt);
	bool DemuxLoop();

	// IOffsetMetadata
	int *m_offsets;
	my12doom_offset_metadata_header m_offset_header;
	HRESULT handle_offset_file(BYTE *data, int size);
	HRESULT GetOffset(REFERENCE_TIME time, REFERENCE_TIME frame_time, int * offset_out);


public:
	CMatroskaSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr);
	virtual ~CMatroskaSplitterFilter();

	DECLARE_IUNKNOWN;
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

	// IKeyFrameInfo

	STDMETHODIMP GetKeyFrameCount(UINT& nKFs);
	STDMETHODIMP GetKeyFrames(const GUID* pFormat, REFERENCE_TIME* pKFs, UINT& nKFs);

	// ITrackInfo

	STDMETHODIMP_(UINT) GetTrackCount();
	STDMETHODIMP_(BOOL) GetTrackInfo(UINT aTrackIdx, struct TrackElement* pStructureToFill);
	STDMETHODIMP_(BOOL) GetTrackExtendedInfo(UINT aTrackIdx, void* pStructureToFill);
	STDMETHODIMP_(BSTR) GetTrackName(UINT aTrackIdx);
	STDMETHODIMP_(BSTR) GetTrackCodecID(UINT aTrackIdx);
	STDMETHODIMP_(BSTR) GetTrackCodecName(UINT aTrackIdx);
	STDMETHODIMP_(BSTR) GetTrackCodecInfoURL(UINT aTrackIdx);
	STDMETHODIMP_(BSTR) GetTrackCodecDownloadURL(UINT aTrackIdx);
};

// 
// changed guid
//class __declspec(uuid("0A68C3B5-9164-4a54-AFAF-995B2FF0E0D4"))
class __declspec(uuid("7DE55957-D7D6-4386-9078-0FD2BA96F459"))
	CMatroskaSourceFilter : public CMatroskaSplitterFilter
{
public:
	CMatroskaSourceFilter(LPUNKNOWN pUnk, HRESULT* phr);
};
