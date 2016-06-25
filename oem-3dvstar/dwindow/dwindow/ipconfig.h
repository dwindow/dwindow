// these functions all need administrator

#pragma once
#include <Windows.h>
#include <wchar.h>

HRESULT setip(const char *ip, const char *netmask, const char *gateway, const char *dns);
HRESULT setip_dhcp(const char *dns = NULL);		// dns=NULL means use dhcp dns