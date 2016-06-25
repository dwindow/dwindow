#pragma  once
//------------------------------------------------------------------------------
// File: Dump.h
//
// Desc: DirectShow sample code - definitions for dump renderer.
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

#include <windows.h>
#include <streams.h>
#include <initguid.h>

class mySinkInputPin;
class mySink;
class mySinkFilter;

// {59079D04-60E9-4a9d-9B25-3E2746E8F1BA}
DEFINE_GUID(CLSID_mySink, 
			0x59079d04, 0x60e9, 0x4a9d, 0x9b, 0x25, 0x3e, 0x27, 0x46, 0xe8, 0xf1, 0xba);

//
class ImySinkCB
{
public:
	virtual HRESULT EndOfStreamCB(){return E_NOTIMPL;}
	virtual HRESULT BreakConnectCB(){return E_NOTIMPL;}
	virtual HRESULT SampleCB(IMediaSample *sample)=0;
	virtual HRESULT CheckMediaTypeCB(const CMediaType *inType){return E_NOTIMPL;}
	virtual HRESULT NewSegmentCB(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate){return E_NOTIMPL;}
};

// Main filter object
class mySinkFilter : public CBaseFilter
{
	friend mySink;
	mySink * const m_pDump;

public:

	// Constructor
	mySinkFilter(mySink *pDump,
		LPUNKNOWN pUnk,
		CCritSec *pLock,
		HRESULT *phr);

	// Pin enumeration
	CBasePin * GetPin(int n);
	int GetPinCount();

	// Open and close the file as necessary
	STDMETHODIMP Run(REFERENCE_TIME tStart);
	STDMETHODIMP Pause();
	STDMETHODIMP Stop();
};


//  Pin object

class mySinkInputPin : public CRenderedInputPin
{
	friend class mySink;
	mySink    * const m_pDump;           // Main renderer object
	CCritSec * const m_pReceiveLock;    // Sample critical section

public:

	mySinkInputPin(mySink *pDump,
		LPUNKNOWN pUnk,
		CBaseFilter *pFilter,
		CCritSec *pLock,
		CCritSec *pReceiveLock,
		HRESULT *phr);

	// Do something with this media sample
	STDMETHODIMP Receive(IMediaSample *pSample);
	STDMETHODIMP EndOfStream(void);
	STDMETHODIMP ReceiveCanBlock();

	// Check if the pin can support this specific proposed type and format
	HRESULT CheckMediaType(const CMediaType *inType);

	// Break connection
	HRESULT BreakConnect();

	// Track NewSegment
	STDMETHODIMP NewSegment(REFERENCE_TIME tStart,
		REFERENCE_TIME tStop,
		double dRate);
};


//  CYV12Dump object which has filter and pin members

class mySink : public CUnknown//, public IFileSinkFilter
{
	friend class mySinkFilter;
	friend class mySinkInputPin;

	mySinkFilter   *m_pFilter;       // Methods for filter interfaces
	mySinkInputPin *m_pPin;          // A simple rendered input pin

	CCritSec m_Lock;                // Main renderer critical section
	CCritSec m_ReceiveLock;         // Sublock for received samples

public:

	DECLARE_IUNKNOWN

	mySink(LPUNKNOWN pUnk, HRESULT *phr);
	~mySink();

	HRESULT SetCallback(ImySinkCB *cb);
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);

private:

	ImySinkCB *m_cb;

	// Overriden to say what interfaces we support where
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);
};

