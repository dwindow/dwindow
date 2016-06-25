#include "audio_device.h"
#include <Mmdeviceapi.h>
#include "PolicyConfig.h"
#include <Propidl.h>
#include <Functiondiscoverykeys_devpkey.h>

wchar_t **audio_devices = NULL;
wchar_t **audio_devices_ids = NULL;
UINT audio_device_count = 0;
int default_audio_device = -1;
DWORD last_refresh_time = 0;


HRESULT SetDefaultAudioPlaybackDevice(LPCWSTR devID)
{	
	IPolicyConfigVista *pPolicyConfig;

	HRESULT hr = CoCreateInstance(__uuidof(CPolicyConfigVistaClient), 
		NULL, CLSCTX_ALL, __uuidof(IPolicyConfigVista), (LPVOID *)&pPolicyConfig);
	if (SUCCEEDED(hr))
	{
		for (DWORD reserved = eConsole; reserved < ERole_enum_count; reserved ++)
			hr = pPolicyConfig->SetDefaultEndpoint(devID, (ERole)reserved);
		pPolicyConfig->Release();
	}
	return hr;
}

HRESULT refresh_device(bool force_refresh = false)
{
	if (!force_refresh && last_refresh_time > GetTickCount() - 3000)
		return S_FALSE;

	if (audio_devices)
	{
		for(int i=0; i<audio_device_count; i++)
		{
			delete audio_devices[i];
			delete audio_devices_ids[i];
		}
		delete audio_devices;
		delete audio_devices_ids;
	}
	default_audio_device = -1;
	audio_device_count = 0;

	HRESULT hr = CoInitialize(NULL);
	if (SUCCEEDED(hr))
	{
		IMMDeviceEnumerator *pEnum = NULL;
		// Create a multimedia device enumerator.
		hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL,
			CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnum);
		if (SUCCEEDED(hr))
		{
			IMMDeviceCollection *pDevices;
			IMMDevice *defaultDevice = NULL;
			LPWSTR defaultDeviceID = NULL;
			// Enumerate the output devices.
			hr = pEnum->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
			if (SUCCEEDED(hr) && defaultDevice)
			{
				defaultDevice->GetId(&defaultDeviceID);
				defaultDevice->Release();
			}
			hr = pEnum->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pDevices);
			if (SUCCEEDED(hr))
			{
				pDevices->GetCount(&audio_device_count);
				if (SUCCEEDED(hr))
				{
					audio_devices_ids = new wchar_t*[audio_device_count];
					audio_devices = new wchar_t *[audio_device_count];

					for (UINT i = 0; i < audio_device_count; i++)
					{
						audio_devices[i] = new wchar_t[4096];
						audio_devices_ids[i] = new wchar_t[4096];

						IMMDevice *pDevice;
						hr = pDevices->Item(i, &pDevice);
						if (SUCCEEDED(hr))
						{
							LPWSTR wstrID = NULL;
							hr = pDevice->GetId(&wstrID);
							DWORD deviceState = 0;
							if (SUCCEEDED(hr))
							{
								IPropertyStore *pStore;
								hr = pDevice->OpenPropertyStore(STGM_READ, &pStore);
								if (SUCCEEDED(hr))
								{
									PROPVARIANT friendlyName;
									PropVariantInit(&friendlyName);
									hr = pStore->GetValue(PKEY_Device_FriendlyName, &friendlyName);
									if (SUCCEEDED(hr))
									{
										wcscpy(audio_devices[i], friendlyName.pwszVal);
										wcscpy(audio_devices_ids[i], wstrID);
										default_audio_device = (defaultDeviceID&&wstrID&&wcscmp(defaultDeviceID,wstrID)==0) ? i : default_audio_device;

										PropVariantClear(&friendlyName);
									}
									pStore->Release();
								}
							}
							pDevice->Release();
							if (wstrID) CoTaskMemFree(wstrID);
						}
					}
				}
				pDevices->Release();
			}
			pEnum->Release();
			if (defaultDeviceID) CoTaskMemFree(defaultDeviceID);
		}
	}


	last_refresh_time = GetTickCount();

	return S_OK;
}

int get_audio_device_count()
{
	refresh_device();

	return audio_device_count;
}
int get_default_audio_device()
{
	refresh_device();

	return default_audio_device;

}
HRESULT active_audio_device(int n)
{
	refresh_device();
	if (n<0 || n>= audio_device_count)
		return E_INVALIDARG;

	last_refresh_time = 0;

	return SetDefaultAudioPlaybackDevice(audio_devices_ids[n]);
}
HRESULT get_audio_device_name(int n, wchar_t *name)
{
	refresh_device();

	if (!name)
		return E_POINTER;

	if (n<0 || n>= audio_device_count)
		return E_INVALIDARG;

	wcscpy(name, audio_devices[n]);

	return S_OK;
}
