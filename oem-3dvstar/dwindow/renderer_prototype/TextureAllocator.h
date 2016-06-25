#include <d3d9.h>
#include <streams.h>
#include <atlbase.h>

class CTextureAllocator;

typedef struct stuct_PooledTexture
{
	IDirect3DTexture9 *texture;
	D3DLOCKED_RECT locked_rect;

	int width;
	int height;
	D3DFORMAT format;
	D3DPOOL pool;
	DWORD usage;

	int size;	// size in byte, used for GPU memory management

	HRESULT hr;
	IDirect3DDevice9 *creator;
	CTextureAllocator *m_allocator;

	int frame_passed;			// frame passed since last decommit
}PooledTexture;

typedef struct stuct_PooledSurface
{
	IDirect3DSurface9 *surface;
	D3DLOCKED_RECT locked_rect;

	int width;
	int height;
	D3DFORMAT format;
	D3DPOOL pool;

	HRESULT hr;
	IDirect3DDevice9 *creator;

	int frame_passed;			// frame passed since last decommit
}PooledSurface;

class CPooledTexture : public PooledTexture
{
public:
	CPooledTexture(CTextureAllocator *pool);
	~CPooledTexture();

	HRESULT get_first_level(IDirect3DSurface9 **out);
	HRESULT Unlock();

protected:
};

class CPooledSurface : public PooledSurface
{
public:
	CPooledSurface(CTextureAllocator *pool);
	~CPooledSurface();

	HRESULT Unlock();

protected:
	CTextureAllocator *m_allocator;
};

class CTextureAllocator
{
public:
	CTextureAllocator(IDirect3DDevice9 *device);
	~CTextureAllocator();
	HRESULT CreateTexture(int width, int height, DWORD flag, D3DFORMAT format, D3DPOOL pool, CPooledTexture **out);
	HRESULT DeleteTexture(CPooledTexture *texture, bool dont_pool = false);
	HRESULT CreateOffscreenSurface(int width, int height, D3DFORMAT format, D3DPOOL pool, CPooledSurface **out);
	HRESULT DeleteSurface(CPooledSurface * surface, bool dont_pool = false);
	HRESULT DestroyPool(D3DPOOL pool2destroy);
	HRESULT UpdateTexture(CPooledTexture *src, CPooledTexture *dst, RECT *dirty = NULL);
	HRESULT AfterFrameRender();
protected:
	IDirect3DDevice9 *m_device;
	PooledTexture m_texture_pool[1024];
	PooledSurface m_surface_pool[1024];
	int m_texture_count; // = 0
	int m_surface_count; // = 0
	CCritSec m_texture_pool_lock;
	CCritSec m_surface_pool_lock;
};