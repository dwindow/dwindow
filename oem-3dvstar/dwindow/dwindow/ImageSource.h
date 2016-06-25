#include <streams.h>
#include "IStereoLayout.h"

// {472EF052-2D21-4742-977C-C02097E5C08E}
DEFINE_GUID(CLSID_my12doomImageSource, 
			0x472ef052, 0x2d21, 0x4742, 0x97, 0x7c, 0xc0, 0x20, 0x97, 0xe5, 0xc0, 0x8e);

// don't use this class for GIF images, only first frame is decoded
// warning: this filter accept Load() only once.
// once loaded, it reject any further Load() calls.
// to load more images, create a new instance

class my12doomImageSource : public CSource, public IFileSourceFilter, public IStereoLayout
{
public:
	// IUnkown
	DECLARE_IUNKNOWN;
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

	// IFileSourceFilter
	HRESULT STDMETHODCALLTYPE Load(LPCOLESTR pszFileName, __in_opt const AM_MEDIA_TYPE *pmt);
	HRESULT STDMETHODCALLTYPE GetCurFile(__out LPOLESTR *ppszFileName, __out_opt AM_MEDIA_TYPE *pmt);

	// IStereoLayout
	HRESULT GetLayout(DWORD *out);
protected:
	WCHAR m_curfile[MAX_PATH];
	int m_width;
	int m_height;
	DWORD m_layout;
	char *m_decoded_data;
	char *m_decoded_data2;
private:
	my12doomImageSource(LPUNKNOWN lpunk, HRESULT *phr);
	~my12doomImageSource();
};

class my12doomImageStream : public CSourceStream, public CSourceSeeking
{
public:
	my12doomImageStream(HRESULT *phr, my12doomImageSource *pParent, LPCWSTR pPinName, void *data, int width, int height);
	~my12doomImageStream();
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

	// plots a ball into the supplied video frame
	HRESULT FillBuffer(IMediaSample *pms);

	// Ask for buffers of the size appropriate to the agreed media type
	HRESULT DecideBufferSize(IMemAllocator *pIMemAlloc,	ALLOCATOR_PROPERTIES *pProperties);

	// Set the agreed media type, and set up the necessary ball parameters
	HRESULT SetMediaType(const CMediaType *pMediaType);

	// Because we calculate the ball there is no reason why we
	// can't calculate it in any one of a set of formats...
	HRESULT CheckMediaType(const CMediaType *pMediaType);
	HRESULT GetMediaType(int iPosition, CMediaType *pmt);

	// Resets the stream time to zero
	HRESULT OnThreadCreate(void);

	// Quality control notifications sent to us
	STDMETHODIMP Notify(IBaseFilter * pSender, Quality q);

	// CSourceSeeking
	HRESULT OnThreadStartPlay();
	HRESULT ChangeStart();
	HRESULT ChangeStop();
	HRESULT ChangeRate();
	STDMETHODIMP SetRate(double);
	HRESULT DeliverNewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);

protected:
	int m_width;
	int m_height;
	char *m_data;
	int m_frame_number;
	REFERENCE_TIME m_segment_time;
	CCritSec m_cSharedState;            // Lock on m_rtSampleTime

	void UpdateFromSeek();
};