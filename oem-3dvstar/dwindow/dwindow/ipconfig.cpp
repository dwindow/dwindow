#include "ipconfig.h"
#include <stdio.h>
#include <atlbase.h>

const char* bat_template_base = "FOR /F \"tokens=2*\" %%%%i IN ('ipconfig/all^|find /i \"“‘Ã´Õ¯  ≈‰∆˜ \"') DO set name=%%%%i %%%%j \r\n"
"FOR /F \"tokens=1* delims=:\" %%%%i in (\"%%name%%\") do set cardname=%%%%i\r\n";
const char* bat_template_ip_static = "netsh interface ip set address name=\"%%cardname%%\" source=static %s %s %s\r\n";
const char* bat_template_ip_dhcp = "netsh interface ip set address name=\"%%cardname%%\" source=dhcp\r\n";
const char* bat_template_dns_static = "netsh interface ip set dns \"%%cardname%%\" source=static addr=%s\r\n";
const char* bat_template_dns_dhcp = "netsh interface ip set dns \"%%cardname%%\" source=dhcp\r\n";



// in global_funcs.cpp
DWORD shellexecute_and_wait(const wchar_t *file, const wchar_t *parameter);

HRESULT setip(const char *ip, const char *netmask, const char *gateway, const char *dns)
{
	wchar_t tmp[MAX_PATH];
	GetTempPathW(MAX_PATH, tmp);
	wcscat(tmp, L"ipconfig.bat");
	FILE * f = _wfopen(tmp, L"wb");
	if (!f)
		return E_FAIL;

	const char * dummy = "\"\"";

	fprintf(f, bat_template_base);
	fprintf(f, bat_template_ip_static, (ip&&strlen(ip)) ? ip : dummy, (netmask&&strlen(netmask)) ? netmask : dummy, (gateway&&(strlen(gateway))) ? gateway : dummy);
	fprintf(f, bat_template_dns_static, dns ? dns : dummy);
	fclose(f);

	shellexecute_and_wait(tmp, NULL);
	
	return S_OK;
}
HRESULT setip_dhcp(const char *dns/* = NULL*/)		// dns=NULL means use dhcp dns
{
	wchar_t tmp[MAX_PATH];
	GetTempPathW(MAX_PATH, tmp);
	wcscat(tmp, L"ipconfig.bat");
	FILE * f = _wfopen(tmp, L"wb");
	if (!f)
		return E_FAIL;

	const char * dummy = "\"\"";

	fprintf(f, bat_template_base);
	fprintf(f, bat_template_ip_dhcp);
	fprintf(f, dns? bat_template_dns_static : bat_template_dns_static, dns ? dns : dummy);
	fclose(f);

	shellexecute_and_wait(tmp, NULL);

	return S_OK;
}