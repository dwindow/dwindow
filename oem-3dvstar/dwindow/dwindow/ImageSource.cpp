#include <assert.h>
#include <InitGuid.h>
#include "ImageSource.h"
#include "../MPOCodec/MPOParser.h"
#include "../MPOCodec/3DPParser.h"
#include "../png2raw/include/il/il.h"
#pragma comment(lib, "../png2raw/lib/DevIL.lib")

#define FPS 24
#define LENGTH 10
#define safe_delete(x) {if(x) delete[]x; x=NULL;}

// my12doomSource
my12doomImageSource::my12doomImageSource(LPUNKNOWN lpunk, HRESULT *phr)
:CSource(NAME("my12doom Image Source"), lpunk, CLSID_my12doomImageSource),
m_layout(IStereoLayout_Unknown)
{
	m_curfile[0] = NULL;
	m_decoded_data = NULL;
	m_decoded_data2 = NULL;
	ASSERT(phr);

	*phr = S_OK;
}
my12doomImageSource::~my12doomImageSource()
{
	safe_delete(m_decoded_data);
	safe_delete(m_decoded_data2);
}

STDMETHODIMP my12doomImageSource::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
	if (riid == IID_IFileSourceFilter) 
		return GetInterface((IFileSourceFilter *) this, ppv);

	if (riid == IID_IStereoLayout) 
		return GetInterface((IStereoLayout *) this, ppv);

	return __super::NonDelegatingQueryInterface(riid, ppv);
}

CUnknown * WINAPI my12doomImageSource::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
	ASSERT(phr);

	CUnknown *punk = new my12doomImageSource(lpunk, phr);
	if(punk == NULL)
	{
		if(phr)
			*phr = E_OUTOFMEMORY;
	}
	return punk;
}

HRESULT my12doomImageSource::GetLayout(DWORD *out)
{
	if (NULL == out)
		return E_POINTER;

	*out = m_layout | IStereoLayout_StillImage;
	return S_OK;
}

extern CCritSec g_ILLock;

STDMETHODIMP my12doomImageSource::Load(LPCOLESTR pszFileName, __in_opt const AM_MEDIA_TYPE *pmt)
{
	if (m_curfile[0])
		return E_FAIL;

	wcscpy(m_curfile, pszFileName);

	// parse file
	MpoParser mpo;
	_3DPParser _3dp;

	void *data1 = NULL;
	void *data2 = NULL;
	int size1 = 0;
	int size2 = 0;
	ILenum type = IL_TYPE_UNKNOWN;
	bool need_delete = false;
	
	if (2 == mpo.parseFile(pszFileName))
	{
		data1 = mpo.m_datas[0];
		data2 = mpo.m_datas[1];
		size1 = mpo.m_sizes[0];
		size2 = mpo.m_sizes[1];
		type = IL_JPG;
		m_layout = IStereoLayout_SideBySide;
	}
	else if (2 == _3dp.parseFile(pszFileName))
	{
		data1 = _3dp.m_datas[0];
		data2 = _3dp.m_datas[1];
		size1 = _3dp.m_sizes[0];
		size2 = _3dp.m_sizes[1];
		type = IL_JPG;
		m_layout = IStereoLayout_SideBySide;
	}
	else
	{
		FILE *f = _wfopen(pszFileName, L"rb");
		if (NULL != f)
		{
			fseek(f, 0, SEEK_END);
			size1 = ftell(f);
			fseek(f, 0, SEEK_SET);
			data1 = new char[size1];
			need_delete = true;
			fread(data1, 1, size1, f);
			fclose(f);
		}
	}

	// decode file
	if (size1 == 0)
		return VFW_E_INVALID_FILE_FORMAT;

	CAutoLock lck(&g_ILLock);
	ilInit();
	ilOriginFunc(IL_ORIGIN_UPPER_LEFT);
	ilEnable(IL_ORIGIN_SET);

	// first file / single file
	ILboolean result = ilLoadL(type, data1, size1 );
	if (!result)
	{
		ILenum err = ilGetError() ;
		printf( "the error %d\n", err );
		printf( "string is %s\n", ilGetString( err ) );
		if (need_delete) delete [] data1;
		return VFW_E_INVALID_FILE_FORMAT;
	}

	ilConvertImage(IL_BGRA, IL_UNSIGNED_BYTE);
	int decoded_size = ilGetInteger(IL_IMAGE_SIZE_OF_DATA);
	int width = ilGetInteger(IL_IMAGE_WIDTH);
	m_height = ilGetInteger(IL_IMAGE_HEIGHT);
	m_width = width;
	m_decoded_data = new char[decoded_size];
	assert(decoded_size >= width * m_height *4);
	char *data = (char*)ilGetData();
	char *dst = m_decoded_data;
	for(int y=0; y<m_height; y++)
	{
		memcpy(dst, data, width*4);
		dst += m_width*4;
		data += width*4;
	}

	// second file
	if (size2 > 0)
	{
		result = ilLoadL(type, data2, size2 );
		if (!result)
		{
			ILenum err = ilGetError() ;
			if (need_delete) delete [] data1;
			safe_delete(m_decoded_data);
			return VFW_E_INVALID_FILE_FORMAT;
		}
		ilConvertImage(IL_BGRA, IL_UNSIGNED_BYTE);
		decoded_size = ilGetInteger(IL_IMAGE_SIZE_OF_DATA);
		int width2 = ilGetInteger(IL_IMAGE_WIDTH);
		int height2 = ilGetInteger(IL_IMAGE_HEIGHT);
		m_decoded_data2 = new char[decoded_size];
		assert(decoded_size >= width * m_height *4);
		data = (char*)ilGetData();
		dst = m_decoded_data2;
		for(int y=0; y<min(height2,m_height); y++)
		{
			memcpy(dst, data, min(width, width2)*4);
			dst += m_width*4;
			data += width2*4;
		}
	}

	HRESULT hr = E_FAIL;
	m_paStreams[0] = new my12doomImageStream(&hr, this, L"Image Out", m_decoded_data, m_width, m_height);
	if(m_paStreams[0] == NULL)
	{
		if (need_delete) delete [] data1;
		return E_OUTOFMEMORY;
	}

	if (m_decoded_data2)
	{
	m_paStreams[1] = new my12doomImageStream(&hr, this, L"Image Out 2", m_decoded_data2, m_width, m_height);
	if(m_paStreams[1] == NULL)
	{
		if (need_delete) delete [] data1;
		return E_OUTOFMEMORY;
	}
	}

	if (need_delete) delete [] data1;
	return S_OK;
}

STDMETHODIMP my12doomImageSource::GetCurFile(__out LPOLESTR *ppszFileName, __out_opt AM_MEDIA_TYPE *pmt)
{
	if (m_curfile[0] == NULL)
		return E_FAIL;

	if (!ppszFileName)
		return E_POINTER;

	*ppszFileName = (LPOLESTR)CoTaskMemAlloc(MAX_PATH * sizeof(OLECHAR));
	wcscpy(*ppszFileName, m_curfile);

	return S_OK;
}

// Pins

my12doomImageStream::my12doomImageStream(HRESULT *phr, my12doomImageSource *pParent, LPCWSTR pPinName, void *data, int width, int height):
CSourceSeeking(NAME("SeekBall"), (IPin*)this, phr, &m_cSharedState),
CSourceStream(NAME("Bouncing Ball"),phr, pParent, pPinName),
m_width(width),
m_height(height),
m_data((char*)data),
m_frame_number(0),
m_segment_time(0)
{
	m_rtDuration = (LENGTH * 10000 * 1000);

}

my12doomImageStream::~my12doomImageStream()
{
	// don't free m_data, it is freed by filter

}
STDMETHODIMP my12doomImageStream::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
	if( riid == IID_IMediaSeeking ) 
	{
		return CSourceSeeking::NonDelegatingQueryInterface( riid, ppv );
	}
	return CSourceStream::NonDelegatingQueryInterface(riid, ppv);
}

// plots a ball into the supplied video frame
HRESULT my12doomImageStream::FillBuffer(IMediaSample *pms)
{
	pms->SetActualDataLength(m_width * m_height * 4);

	BYTE *buf = NULL;
	pms->GetPointer(&buf);
	memcpy(buf, m_data, m_width * m_height * 4);

	REFERENCE_TIME rtStart, rtStop;
	rtStart = (REFERENCE_TIME)(m_frame_number+1) * 10000000 / FPS;
	rtStop = (REFERENCE_TIME)(m_frame_number+2) * 10000000 / FPS;

	pms->SetTime(&rtStart, &rtStop);

	m_frame_number ++;

	return rtStart + m_segment_time > LENGTH * 1000 * 10000 ? S_FALSE : S_OK;
}

// Ask for buffers of the size appropriate to the agreed media type
HRESULT my12doomImageStream::DecideBufferSize(IMemAllocator *pIMemAlloc,	ALLOCATOR_PROPERTIES *pProperties)
{
	if (pIMemAlloc == NULL)
		return E_POINTER;

	if (pProperties == NULL)
		return E_POINTER;
	CAutoLock cAutoLock(m_pFilter->pStateLock());

	pProperties->cBuffers = 1;
	pProperties->cbBuffer = m_width * m_height * 4;

	ASSERT(pProperties->cbBuffer);

	// Ask the allocator to reserve us some sample memory, NOTE the function
	// can succeed (that is return NOERROR) but still not have allocated the
	// memory that we requested, so we must check we got whatever we wanted

	ALLOCATOR_PROPERTIES Actual;
	HRESULT hr = pIMemAlloc->SetProperties(pProperties,&Actual);
	if(FAILED(hr))
	{
		return hr;
	}

	// Is this allocator unsuitable

	if(Actual.cbBuffer < pProperties->cbBuffer)
	{
		return E_FAIL;
	}

	// Make sure that we have only 1 buffer (we erase the ball in the
	// old buffer to save having to zero a 200k+ buffer every time
	// we draw a frame)

	ASSERT(Actual.cBuffers == 1);
	return NOERROR;
}

HRESULT my12doomImageStream::DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	m_segment_time = tStart;

	return __super::DeliverNewSegment(tStart, tStop, dRate);
}

// Set the agreed media type, and set up the necessary ball parameters
HRESULT my12doomImageStream::SetMediaType(const CMediaType *pMediaType)
{
	// TODO
	return __super::SetMediaType(pMediaType);
}

// Because we calculate the ball there is no reason why we
// can't calculate it in any one of a set of formats...
HRESULT my12doomImageStream::CheckMediaType(const CMediaType *pMediaType)
{
	// TODO
	return S_OK;
}
HRESULT my12doomImageStream::GetMediaType(int iPosition, CMediaType *pmt)
{
	if (pmt == NULL)
		return E_POINTER;

	if (iPosition<0)
		return E_INVALIDARG;

	if (iPosition>0)
		return VFW_S_NO_MORE_ITEMS;

	pmt->SetType(&MEDIATYPE_Video);
	pmt->SetSubtype(&MEDIASUBTYPE_RGB32);
	pmt->SetFormatType(&FORMAT_VideoInfo);
	pmt->bFixedSizeSamples = FALSE;
	pmt->SetTemporalCompression(FALSE);
	pmt->lSampleSize = 1;

	VIDEOINFO *pvi = (VIDEOINFO*) pmt->AllocFormatBuffer(sizeof(VIDEOINFO));
	memset(pvi, 0, sizeof(VIDEOINFO));
	pvi->dwBitRate = 0;
	pvi->dwBitErrorRate = 0;
	pvi->AvgTimePerFrame = 10000000 / FPS;

	pvi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pvi->bmiHeader.biWidth = m_width;
	pvi->bmiHeader.biHeight = -m_height;
	pvi->bmiHeader.biPlanes = 1;
	pvi->bmiHeader.biBitCount = 24;
	pvi->bmiHeader.biCompression = 0;
	pvi->bmiHeader.biSizeImage = m_width * m_height * 4;
	pvi->bmiHeader.biXPelsPerMeter = 1;
	pvi->bmiHeader.biYPelsPerMeter = 1;

	return S_OK;
}

// Resets the stream time to zero
HRESULT my12doomImageStream::OnThreadCreate(void)
{
	// TODO
	return S_OK;
}
// Quality control notifications sent to us
STDMETHODIMP my12doomImageStream::Notify(IBaseFilter * pSender, Quality q)
{
	return NOERROR;
}

// CSourceSeeking
HRESULT my12doomImageStream::OnThreadStartPlay()
{
	return DeliverNewSegment(m_rtStart, m_rtStop, m_dRateSeeking);
}
HRESULT my12doomImageStream::ChangeStart()
{
	{
		CAutoLock lock(CSourceSeeking::m_pLock);
		m_frame_number = (int)(m_rtStart * FPS / 10000000 );
	}

	UpdateFromSeek();
	return S_OK;
}
HRESULT my12doomImageStream::ChangeStop()
{
	// We're already past the new stop time. Flush the graph.
	UpdateFromSeek();
	return S_OK;
}
HRESULT my12doomImageStream::ChangeRate()
{
	return S_OK;
}
STDMETHODIMP my12doomImageStream::SetRate(double)
{
	return E_NOTIMPL;
}

void my12doomImageStream::UpdateFromSeek()
{
	if (ThreadExists()) 
	{
		DeliverBeginFlush();
		// Shut down the thread and stop pushing data.
		Stop();
		DeliverEndFlush();
		// Restart the thread and start pushing data again.
		Pause();
	}
}