#include "my12doomAutoShader.h"
#include "..\AESFile\rijndael.h"

my12doom_auto_shader::my12doom_auto_shader()
{
	m_has_key = false;
	m_device = NULL;
	m_data = NULL;
	m_key = (DWORD*)malloc(32);
}

HRESULT my12doom_auto_shader::set_source(IDirect3DDevice9 *device, const DWORD *data, int datasize, bool is_ps, DWORD *aes_key)
{
	if (!device || !data)
		return E_POINTER;

	if (m_data) free(m_data);
	m_data = NULL;

	m_device = device;
	m_is_ps = is_ps;
	m_has_key = false;
	m_data = (DWORD *) malloc(datasize);
	m_datasize = datasize;
	memcpy(m_data, data, datasize);
	if (aes_key)
	{
		memcpy(m_key, aes_key, 32);
		m_has_key = true;
	}

	invalid();
	return S_OK;
}

my12doom_auto_shader::~my12doom_auto_shader()
{
	if (m_data) free(m_data);
	free(m_key);
	m_data = NULL;
}

HRESULT my12doom_auto_shader::invalid()
{
	m_ps = NULL;
	m_vs = NULL;

	return S_OK;
}

HRESULT my12doom_auto_shader::restore()
{
	DWORD * data = m_data;
	if (m_has_key)
	{
		data = (DWORD*)malloc(m_datasize);
		memcpy(data, m_data, m_datasize);

		AESCryptor aes;
		aes.set_key((unsigned char*)m_key, 256);
		for(int i=0; i<(m_datasize/16*16)/4; i+=4)
			aes.decrypt((unsigned char*)(data+i), (unsigned char*)(data+i));
	}
	HRESULT hr = E_FAIL;
	if (m_is_ps)
		hr = m_device->CreatePixelShader(data, &m_ps);
	else 
		hr = m_device->CreateVertexShader(data, &m_vs);

	if (m_has_key)
		free(data);

	return hr;
}

my12doom_auto_shader::operator IDirect3DPixelShader9*()
{
	if (!m_is_ps)
		return NULL;
	if (!m_ps)
		restore();
	return m_ps;
}

my12doom_auto_shader::operator IDirect3DVertexShader9*()
{
	if (m_is_ps)
		return NULL;
	if (!m_vs)
		restore();
	return m_vs;
}