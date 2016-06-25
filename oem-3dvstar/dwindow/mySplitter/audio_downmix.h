#include <streams.h>

// {61FEB42B-6680-411b-8378-A93483889056}
DEFINE_GUID(CLSID_DWindowAudioDownmix, 
			0x61feb42b, 0x6680, 0x411b, 0x83, 0x78, 0xa9, 0x34, 0x83, 0x88, 0x90, 0x56);

class CDWindowAudioDownmix : CTransformFilter
{
public:
	DECLARE_IUNKNOWN;
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);

	// Reveals IDWindowExtenderMono and ISpecifyPropertyPages
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

	// Overrriden from CSplitFilter base class
	HRESULT Transform(IMediaSample * pIn, IMediaSample *pOut);
	HRESULT CheckInputType(const CMediaType *mtIn);
	HRESULT CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut);
	HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProperties);
	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);

	// you know what
	CDWindowAudioDownmix(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr);
	~CDWindowAudioDownmix();

protected:
	GUID m_in_subtype;
	WAVEFORMATEX m_out_fmt;
	WAVEFORMATEX m_in_fmt;
	double m_db;		// current gain, in db

	inline double peak_PCM(void *in, int count);
	inline double peak_PCM8bit(void *in, int count);
	inline double peak_PCM24bit(void *in, int count);
	inline double peak_float(void *in, int count);

	inline void mix_a_sample_PCM(void* in, void *out, double rate);
	inline void copy_a_sample_PCM(void* in, void *out, double rate);				// just copy
	inline void mix_a_sample_PCM_24bit(void* in, void *out, double rate);
	inline void copy_a_sample_PCM_24bit(void* in, void *out, double rate);		// just copy up 16bit
	inline void copy_a_sample_PCM_8bit(void* in, void *out, double rate);		// just copy up 16bit
	inline void mix_a_sample_FLOAT(void* in, void *out, double rate);
	inline void copy_a_sample_FLOAT(void* in, void *out, double rate);	// just convert first 2 channel
};