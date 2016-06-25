#include "mySink.h"

// Constructor
mySinkFilter::mySinkFilter(mySink *pDump,
						 LPUNKNOWN pUnk,
						 CCritSec *pLock,
						 HRESULT *phr) :
CBaseFilter(NAME("CYV12DumpFilter"), pUnk, pLock, CLSID_mySink),
m_pDump(pDump)
{
}


//
// GetPin
//
CBasePin * mySinkFilter::GetPin(int n)
{
	if (n == 0) {
		return m_pDump->m_pPin;
	} else {
		return NULL;
	}
}


//
// GetPinCount
//
int mySinkFilter::GetPinCount()
{
	return 1;
}


//
// Stop
//
// Overriden to close the dump file
//
STDMETHODIMP mySinkFilter::Stop()
{
	CAutoLock cObjectLock(m_pLock);

	return CBaseFilter::Stop();
}


//
// Pause
//
// Overriden to open the dump file
//
STDMETHODIMP mySinkFilter::Pause()
{
	CAutoLock cObjectLock(m_pLock);

	return CBaseFilter::Pause();
}


//
// Run
//
// Overriden to open the dump file
//
STDMETHODIMP mySinkFilter::Run(REFERENCE_TIME tStart)
{
	CAutoLock cObjectLock(m_pLock);

	return CBaseFilter::Run(tStart);
}


//
//  Definition of CYV12DumpInputPin
//
mySinkInputPin::mySinkInputPin(mySink *pDump,
							 LPUNKNOWN pUnk,
							 CBaseFilter *pFilter,
							 CCritSec *pLock,
							 CCritSec *pReceiveLock,
							 HRESULT *phr) :

CRenderedInputPin(NAME("CYV12DumpInputPin"),
				  pFilter,                   // Filter
				  pLock,                     // Locking
				  phr,                       // Return code
				  L"Input"),                 // Pin name
				  m_pReceiveLock(pReceiveLock),
				  m_pDump(pDump)
{
}


//
// CheckMediaType
//
// Check if the pin can support this specific proposed type and format
//
HRESULT mySinkInputPin::CheckMediaType(const CMediaType *inType)
{
	HRESULT hr = E_FAIL;
	if (m_pDump->m_cb)
		hr = m_pDump->m_cb->CheckMediaTypeCB(inType);
	if (hr != E_NOTIMPL)
		return hr;

	return E_FAIL;
}


//
// BreakConnect
//
// Break a connection
//
HRESULT mySinkInputPin::BreakConnect()
{
	if (m_pDump->m_cb)
		m_pDump->m_cb->BreakConnectCB();

	return CRenderedInputPin::BreakConnect();
}


//
// ReceiveCanBlock
//
// We don't hold up source threads on Receive
//
STDMETHODIMP mySinkInputPin::ReceiveCanBlock()
{
	return S_OK;
}


//
// Receive
//
// Do something with this media sample
//
STDMETHODIMP mySinkInputPin::Receive(IMediaSample *pSample)
{
	CheckPointer(pSample,E_POINTER);

	CAutoLock lock(m_pReceiveLock);

	if (m_pDump->m_cb)
		m_pDump->m_cb->SampleCB(pSample);
	return S_OK;
}

//
// EndOfStream
//
STDMETHODIMP mySinkInputPin::EndOfStream(void)
{
	if (m_pDump->m_cb)
		m_pDump->m_cb->EndOfStreamCB();

	CAutoLock lock(m_pReceiveLock);
	return CRenderedInputPin::EndOfStream();

} // EndOfStream


//
// NewSegment
//
// Called when we are seeked
//
STDMETHODIMP mySinkInputPin::NewSegment(REFERENCE_TIME tStart,
									   REFERENCE_TIME tStop,
									   double dRate)
{
	if (m_pDump->m_cb)
		m_pDump->m_cb->NewSegmentCB(tStart, tStop, dRate);

	return S_OK;

} // NewSegment


//
//  CYV12Dump class
//
mySink::mySink(LPUNKNOWN pUnk, HRESULT *phr) :
CUnknown(NAME("CYV12Dump"), pUnk),
m_pFilter(NULL),
m_pPin(NULL),
m_cb(NULL)
{
	ASSERT(phr);

	m_pFilter = new mySinkFilter(this, GetOwner(), &m_Lock, phr);
	if (m_pFilter == NULL) {
		if (phr)
			*phr = E_OUTOFMEMORY;
		return;
	}

	m_pPin = new mySinkInputPin(this,GetOwner(),
		m_pFilter,
		&m_Lock,
		&m_ReceiveLock,
		phr);
	if (m_pPin == NULL) {
		if (phr)
			*phr = E_OUTOFMEMORY;
		return;
	}
}

HRESULT mySink::SetCallback(ImySinkCB *cb)
{
	CAutoLock cObjectLock(m_pPin->m_pReceiveLock);

	m_cb = cb;

	return S_OK;
}


// Destructor

mySink::~mySink()
{
	delete m_pPin;
	delete m_pFilter;
}


//
// CreateInstance
//
// Provide the way for COM to create a dump filter
//
CUnknown * WINAPI mySink::CreateInstance(LPUNKNOWN punk, HRESULT *phr)
{
	ASSERT(phr);

	mySink *pNewObject = new mySink(punk, phr);
	if (pNewObject == NULL) {
		if (phr)
			*phr = E_OUTOFMEMORY;
	}

	return pNewObject;

} // CreateInstance


//
// NonDelegatingQueryInterface
//
// Override this to say what interfaces we support where
//
STDMETHODIMP mySink::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
	CheckPointer(ppv,E_POINTER);
	CAutoLock lock(&m_Lock);

	// Do we have this interface
	if (riid == IID_IBaseFilter || riid == IID_IMediaFilter || riid == IID_IPersist) {
		return m_pFilter->NonDelegatingQueryInterface(riid, ppv);
	}

	return CUnknown::NonDelegatingQueryInterface(riid, ppv);

} // NonDelegatingQueryInterface

