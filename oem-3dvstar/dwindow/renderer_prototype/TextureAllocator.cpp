#include "TextureAllocator.h"

extern int lockrect_texture;
extern __int64 lockrect_texture_cycle;
int lock_delay = 3;

// texture class
CPooledTexture::~CPooledTexture()
{
	m_allocator->DeleteTexture(this);
}

CPooledTexture::CPooledTexture(CTextureAllocator *pool)
{
	m_allocator = pool;
}

HRESULT CPooledTexture::Unlock()
{
	//locked_rect.pBits = NULL;
	//texture->AddDirtyRect(NULL);
	return texture->UnlockRect(0);
}

HRESULT CPooledTexture::get_first_level(IDirect3DSurface9 **out)
{
	if (!out)
		return E_POINTER;

	return texture->GetSurfaceLevel(0, out);
}

// surface class
CPooledSurface::~CPooledSurface()
{
	m_allocator->DeleteSurface(this);
}

CPooledSurface::CPooledSurface(CTextureAllocator *pool)
{
	m_allocator = pool;
}

HRESULT CPooledSurface::Unlock()
{
	hr = S_OK;
	if (locked_rect.pBits)
		hr = surface->UnlockRect();
	locked_rect.pBits = NULL;
	return hr;
}


// texture pool
CTextureAllocator::CTextureAllocator(IDirect3DDevice9 *device):
m_texture_count(0),
m_surface_count(0),
m_device(device)
{

}
CTextureAllocator::~CTextureAllocator()
{
	DestroyPool(D3DPOOL_DEFAULT);
	DestroyPool(D3DPOOL_MANAGED);
	DestroyPool(D3DPOOL_SYSTEMMEM);
}
HRESULT CTextureAllocator::CreateTexture(int width, int height, DWORD usage, D3DFORMAT format, D3DPOOL pool, CPooledTexture **out)
{
	if (out == NULL)
		return E_POINTER;

	CPooledTexture *& o = *out;
	o = new CPooledTexture(this);

	{
		// find in pool
		CAutoLock lck(&m_texture_pool_lock);
		for(int i=0; i<m_texture_count; i++)
		{
			PooledTexture &t = m_texture_pool[i];
			if (t.width == width && t.height == height && t.pool == pool && t.format == format && 
				(t.frame_passed >= lock_delay || pool != D3DPOOL_SYSTEMMEM || !(usage & D3DUSAGE_DYNAMIC)))
			{
				*(PooledTexture*)o = t;
				m_texture_count --;
				memcpy(m_texture_pool+i, m_texture_pool+i+1, (m_texture_count-i)*sizeof(PooledTexture));
				//for(int j=i; j<m_pool_count; j++)
				//	m_pool[j] = m_pool[j+1];

				if (t.creator != m_device)
				{
					printf("wtf...v2\n");
				}

				return S_OK;
			}
		}
	}

	// no match, just create new one and lock it
	o->width = width;
	o->height = height;
	o->pool = pool;
	o->usage = usage;
	o->format = format;
	o->creator = m_device;
	o->hr = m_device->CreateTexture(width, height, (usage & D3DUSAGE_AUTOGENMIPMAP) ? 0 : 1, usage, format, pool, &o->texture, NULL);
	if (FAILED(o->hr))
		return o->hr;
	if (pool == D3DPOOL_SYSTEMMEM || usage & D3DUSAGE_DYNAMIC)
		o->hr = o->texture->LockRect(0, &o->locked_rect, NULL, usage & D3DUSAGE_DYNAMIC ? D3DLOCK_DISCARD : NULL);

	lockrect_texture ++;

	if (FAILED(o->hr))
	{
		o->texture->Release();
		o->texture = NULL;
	}

	return o->hr;
}

HRESULT CTextureAllocator::CreateOffscreenSurface(int width, int height, D3DFORMAT format, D3DPOOL pool, CPooledSurface **out)
{
	if (out == NULL)
		return E_POINTER;

	CPooledSurface *& o = *out;
	o = new CPooledSurface(this);

	{
		// find in pool
		CAutoLock lck(&m_surface_pool_lock);
		for(int i=0; i<m_surface_count; i++)
		{
			PooledSurface &t = m_surface_pool[i];
			if (t.width == width && t.height == height && t.pool == pool && t.format == format)
			{
				*(PooledSurface*)o = t;
				m_surface_count --;
				memcpy(m_surface_pool+i, m_surface_pool+i+1, (m_surface_count-i)*sizeof(PooledSurface));

				return S_OK;
			}
		}
	}

	// no match, just create new one and lock it
	o->width = width;
	o->height = height;
	o->pool = pool;
	o->format = format;
	o->creator = m_device;
	o->hr = m_device->CreateOffscreenPlainSurface(width, height, format, pool, &o->surface, NULL);
	if (FAILED(o->hr))
		return o->hr;
	o->hr = o->surface->LockRect(&o->locked_rect, NULL, NULL);

	if (FAILED(o->hr))
	{
		o->surface->Release();
		o->surface = NULL;
	}

	return o->hr;
}
HRESULT CTextureAllocator::DeleteTexture(CPooledTexture *texture, bool dont_pool /*=false*/)
{
	if (texture->m_allocator != this || texture->creator != m_device)
	{
		printf("wtf\n");
		dont_pool = true;
	}

	if (FAILED(texture->hr))
		return S_OK;

	if (dont_pool)
		return texture->texture->Release();

	if (FAILED(texture->hr))
	{
		texture->texture->Release();
		return S_FALSE;
	}
	
	texture->frame_passed = 5;

	CAutoLock lck(&m_texture_pool_lock);
	m_texture_pool[m_texture_count++] = *texture;

	return S_OK;
}
HRESULT CTextureAllocator::DeleteSurface(CPooledSurface *surface, bool dont_pool /*=false*/)
{
	if (FAILED(surface->hr))
		return S_OK;
	if (dont_pool)
		return surface->surface->Release();

	surface->surface->Release();

	/*
	if (surface->locked_rect.pBits == NULL)
	{
		surface->hr = surface->surface->LockRect(&surface->locked_rect, NULL, NULL);
	}
	if (FAILED(surface->hr))
	{
		surface->surface->Release();
		return S_FALSE;
	}

	CAutoLock lck(&m_surface_pool_lock);
	m_surface_pool[m_surface_count++] = *surface;
	*/

	return S_OK;
}
HRESULT CTextureAllocator::DestroyPool(D3DPOOL pool2destroy)
{
	// destroy textures
	{
		CAutoLock lck(&m_texture_pool_lock);
		int i = 0;
		int j = 0;
		int c = m_texture_count;
		while (i<c)
		{
			if (m_texture_pool[i].pool == pool2destroy)
			{
				m_texture_pool[i++].texture->Release();
				m_texture_count--;
			}
			else
				m_texture_pool[j++] = m_texture_pool[i++];
		}
	}

	// destroy surfaces
	{
		CAutoLock lck(&m_surface_pool_lock);
		int i = 0;
		int j = 0;
		int c = m_surface_count;
		while (i<c)
		{
			if (m_surface_pool[i].pool == pool2destroy)
			{
				m_surface_pool[i++].surface->Release();
				m_surface_count--;
			}
			else
				m_surface_pool[j++] = m_surface_pool[i++];
		}
	}

	return S_OK;
}

HRESULT CTextureAllocator::UpdateTexture(CPooledTexture *src, CPooledTexture *dst, RECT *dirty)
{
	if (src->m_allocator != this)
	{
		printf("wtf\n");
	}


	if (!src || !dst || !src->texture || !dst->texture)
		return E_POINTER;

	if (src->pool != D3DPOOL_SYSTEMMEM || dst->pool != D3DPOOL_DEFAULT)
		return E_INVALIDARG;

	src->texture->AddDirtyRect(dirty);

	return m_device->UpdateTexture(src->texture, dst->texture);
}

HRESULT CTextureAllocator::AfterFrameRender()
{
	CAutoLock lck(&m_surface_pool_lock);
	for(int i=0; i<m_surface_count; i++)
		m_surface_pool[i].frame_passed++;

	CAutoLock lck2(&m_texture_pool_lock);
	for(int i=0; i<m_texture_count; i++)
	{
		m_texture_pool[i].frame_passed++;
		PooledTexture *texture = &m_texture_pool[i];


		if (texture->locked_rect.pBits == NULL && (texture->pool == D3DPOOL_SYSTEMMEM || texture->usage & D3DUSAGE_DYNAMIC))
		{
			LARGE_INTEGER li, l2;
			QueryPerformanceCounter(&li);
			texture->hr = texture->texture->LockRect(0, &texture->locked_rect, NULL, texture->usage & D3DUSAGE_DYNAMIC ? D3DLOCK_DISCARD : NULL);
			QueryPerformanceCounter(&l2);
			lockrect_texture ++;
			lockrect_texture_cycle += l2.QuadPart - li.QuadPart;
		}
	}

	return S_OK;
}