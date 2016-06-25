#pragma once
#include <Windows.h>
#include <d3d9.h>
#include <atlbase.h>

class my12doom_auto_shader
{
public:
	my12doom_auto_shader();
	HRESULT set_source(IDirect3DDevice9 *device, const DWORD *data, int datasize, bool is_ps, DWORD *aes_key);
	~my12doom_auto_shader();

	HRESULT invalid();
	HRESULT restore();
	operator IDirect3DPixelShader9*();
	operator IDirect3DVertexShader9*();

protected:
	DWORD *m_data;
	int m_datasize;
	DWORD *m_key;
	bool m_has_key;
	bool m_is_ps;
	CComPtr<IDirect3DPixelShader9> m_ps;
	CComPtr<IDirect3DVertexShader9> m_vs;
	IDirect3DDevice9 *m_device;
};