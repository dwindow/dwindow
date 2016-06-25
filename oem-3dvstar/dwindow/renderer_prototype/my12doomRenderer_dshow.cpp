// DirectShow part of my12doom renderer

#include "my12doomRenderer.h"
#include "..\dwindow\global_funcs.h"
#include <dvdmedia.h>

bool my12doom_queue_enable = true;

// base
STDMETHODIMP DRendererInputPin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	((DBaseVideoRenderer*)m_pFilter)->NewSegment(tStart, tStop, dRate);
	return __super::NewSegment(tStart, tStop, dRate);
}
STDMETHODIMP DRendererInputPin::BeginFlush()
{
	((DBaseVideoRenderer*)m_pFilter)->BeginFlush();
	return __super::BeginFlush();
}

CBasePin * DBaseVideoRenderer::GetPin(int n)
{
	// copied from __super, and changed only the input pin creation class
	CAutoLock cObjectCreationLock(&m_ObjectCreationLock);

	// Should only ever be called with zero
	ASSERT(n == 0);

	if (n != 0) {
		return NULL;
	}

	// Create the input pin if not already done so

	if (m_pInputPin == NULL) {

		// hr must be initialized to NOERROR because
		// CRendererInputPin's constructor only changes
		// hr's value if an error occurs.
		HRESULT hr = NOERROR;

		m_pInputPin = new DRendererInputPin(this,&hr,L"In");
		if (NULL == m_pInputPin) {
			return NULL;
		}

		if (FAILED(hr)) {
			delete m_pInputPin;
			m_pInputPin = NULL;
			return NULL;
		}
	}
	return m_pInputPin;
}


HRESULT DBaseVideoRenderer::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	m_thisstream = tStart;
	return S_OK;
}

HRESULT DBaseVideoRenderer::BeginFlush()
{
	return __super::BeginFlush();
}

// renderer dshow part
my12doomRendererDShow::my12doomRendererDShow(LPUNKNOWN pUnk,HRESULT *phr, my12doomRenderer *owner, int id)
: DBaseVideoRenderer(__uuidof(CLSID_my12doomRenderer),NAME("Texture Renderer"), pUnk, phr),
m_owner(owner),
m_id(id)
{
    if (phr)
        *phr = S_OK;
	m_format = CLSID_NULL;

	HRESULT hr;
	CMemAllocator * allocator = new CMemAllocator(_T("PumpAllocator"), NULL, &hr);
	ALLOCATOR_PROPERTIES allocator_properties = {1, 32, 32, 0}, pro_got;
	allocator->SetProperties(&allocator_properties, &pro_got);
	allocator->Commit();
	allocator->QueryInterface(IID_IMemAllocator, (void**)&m_allocator);


	m_time = m_thisstream = 0;
	m_queue_count = 0;
	m_queue_size = my12doom_queue_size;
	m_queue_exit = false;
	if (my12doom_queue_enable) m_queue_thread = CreateThread(NULL, NULL, queue_thread, this, NULL, NULL);

	//timeBeginPeriod(1);
}

my12doomRendererDShow::~my12doomRendererDShow()
{
	//timeEndPeriod(1);

	m_queue_exit = true;
	m_queue_size = my12doom_queue_size;
	WaitForSingleObject(m_queue_thread, INFINITE);

	m_allocator->Decommit();
}


HRESULT my12doomRendererDShow::CheckMediaType(const CMediaType *pmt)
{
    HRESULT   hr = E_FAIL;
    VIDEOINFO *pvi=0;

    CheckPointer(pmt,E_POINTER);

    if( *pmt->FormatType() != FORMAT_VideoInfo && *pmt->FormatType() != FORMAT_VideoInfo2)
        return E_INVALIDARG;

    pvi = (VIDEOINFO *)pmt->Format();

	GUID subtype = *pmt->Subtype();
    if(*pmt->Type() == MEDIATYPE_Video  &&
       (subtype == MEDIASUBTYPE_YV12 || subtype ==  MEDIASUBTYPE_NV12 || subtype == MEDIASUBTYPE_YUY2 || subtype == MEDIASUBTYPE_RGB32
	    || (GET_CONST("Stream16bit") && (subtype == MEDIASUBTYPE_P010 || subtype == MEDIASUBTYPE_P016))))
    {
        hr = m_owner->CheckMediaType(pmt, m_id);
    }

    return hr;
}

HRESULT my12doomRendererDShow::SetMediaType(const CMediaType *pmt)
{
	int width = m_owner->m_lVidWidth;
	int height= m_owner->m_lVidHeight;

	m_format = *pmt->Subtype();
	m_formattype = *pmt->FormatType();

    return m_owner->SetMediaType(pmt, m_id);
}

HRESULT	my12doomRendererDShow::BreakConnect()
{
	m_owner->BreakConnect(m_id);
	return __super::BreakConnect();
}

HRESULT my12doomRendererDShow::CompleteConnect(IPin *pRecievePin)
{
	m_owner->CompleteConnect(pRecievePin, m_id);
	return __super::CompleteConnect(pRecievePin);
}

HRESULT my12doomRendererDShow::drop_packets(int num /* = -1 */)
{
	CAutoLock lck(&m_queue_lock);
	
	if (num == -1)
		m_queue_count = 0;
	else
	{
		num = min(num, m_queue_count);
		if (num <= 0)
			return S_OK;

		m_queue_count -= num;
		//for(int i=0; i<m_queue_count; i++)
		//	m_queue[i] = m_queue[i+num];
		memmove(m_queue, m_queue+num, sizeof(dummy_packet)*(my12doom_queue_size-num));

	}
	return S_OK;
}
HRESULT my12doomRendererDShow::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	m_time = 0;
	{
		CAutoLock lck(&m_queue_lock);
		m_queue_count = 0;
	}

	{
		CAutoLock lck(&m_owner->m_queue_lock);
		gpu_sample *sample = NULL;
		while (sample = m_owner->m_left_queue.RemoveHead())
			delete sample;
		while (sample = m_owner->m_right_queue.RemoveHead())
			delete sample;
	}

	return __super::NewSegment(tStart, tStop, dRate);
}

bool my12doomRendererDShow::is_connected()
{
	if (!m_pInputPin)
		return false;
	return m_pInputPin->IsConnected() ? true : false;
}

HRESULT my12doomRendererDShow::Receive(IMediaSample *sample)
{
	if (!my12doom_queue_enable)
		return SuperReceive(sample);

	HRESULT hr = m_owner->DataPreroll(m_id, sample);
	if (S_FALSE == hr)
		return S_OK;

retry:
	m_queue_lock.Lock();

	if (m_queue_count >= m_queue_size)
	{
		m_queue_lock.Unlock();
		Sleep(1);
		goto retry;
	}

	REFERENCE_TIME start, end;
	sample->GetTime(&start, &end);
	m_queue[m_queue_count].start = start;
	m_queue[m_queue_count++].end = end;


	m_queue_lock.Unlock();
	return S_OK;
}

//  Helper function for clamping time differences
int inline TimeDiff(REFERENCE_TIME rt)
{
	if (rt < - (50 * UNITS)) {
		return -(50 * UNITS);
	} else
		if (rt > 50 * UNITS) {
			return 50 * UNITS;
		} else return (int)rt;
}

HRESULT my12doomRendererDShow::ShouldDrawSampleNow(IMediaSample *pMediaSample,
												__inout REFERENCE_TIME *ptrStart,
												__inout REFERENCE_TIME *ptrEnd)
{
	return S_FALSE;

	REFERENCE_TIME start = *ptrStart;
	REFERENCE_TIME end = *ptrEnd;

	HRESULT hr = __super::ShouldDrawSampleNow(pMediaSample, ptrStart, ptrEnd);

	*ptrStart = start;
	*ptrEnd = end;

	if (FAILED(hr) /*&& m_owner->m_frame_length > (1000*10000/45)*/)		// only drop frame on file with 45+fps
		hr = S_OK;

	return hr;
} // ShouldDrawSampleNow



HRESULT my12doomRendererDShow::DoRenderSample( IMediaSample * pSample )
{
	if (!my12doom_queue_enable)
		m_owner->DataPreroll(m_id, pSample);
	REFERENCE_TIME end;
	pSample->GetTime(&m_time, &end);
	m_owner->DoRender(m_id, pSample);
	return S_OK;
}

void my12doomRendererDShow::OnReceiveFirstSample(IMediaSample *pSample)
{
	int l = timeGetTime();
	while(m_queue_count < m_queue_size / 2 && timeGetTime()-l < 1000)			// wait 1000ms for data incoming
		Sleep(1);

	m_owner->DoRender(m_id, pSample);
	return __super::OnReceiveFirstSample(pSample);
}

DWORD WINAPI my12doomRendererDShow::queue_thread(LPVOID param)
{
	my12doomRendererDShow * _this = (my12doomRendererDShow*)param;

	SetThreadName(GetCurrentThreadId(), _this->m_id==0?"queue thread 0":"queue thread 1");
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	while (!_this->m_queue_exit)
	{
		CComPtr<IMediaSample> dummy_sample;
		dummy_sample = NULL;
		{
			CAutoLock lck(&_this->m_queue_lock);
			if (_this->m_queue_count)
			{
				_this->m_allocator->GetBuffer(&dummy_sample, &_this->m_queue->start, &_this->m_queue->end, NULL);
				dummy_sample->SetTime(&_this->m_queue->start, &_this->m_queue->end);
				memmove(_this->m_queue, _this->m_queue+1, sizeof(dummy_packet)*(my12doom_queue_size-1));
				_this->m_queue_count--;
			}
		}

		if(dummy_sample)
			_this->SuperReceive(dummy_sample);
		else
			Sleep(1);
	}

	return 0;
}
