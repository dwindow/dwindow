#pragma once
#include <Windows.h>
#include <stdio.h>

int wcsexplode(const wchar_t *string_to_explode, const wchar_t *delimeter, wchar_t **out, int max_part /* = 0xfffffff */);

static const int S_RAWDATA = 2;

class ICommandReciever
{
public:
	virtual HRESULT execute_command(wchar_t *command, wchar_t *out = NULL, const wchar_t *arg1 = NULL, const wchar_t *arg2 = NULL, const wchar_t *arg3 = NULL, const wchar_t *arg4 = NULL)
	{
		if (command == NULL)
			return E_POINTER;

		int args_count = 4;
		if (arg4== NULL) args_count = 3;
		if (arg3== NULL) args_count = 2;
		if (arg2== NULL) args_count = 1;
		if (arg1== NULL) args_count = 0;
		const wchar_t *args[4] = {arg1,arg2,arg3,arg4};

		HRESULT hr = execute_command_adv(command, out, args, args_count);

		return hr;
	}
	virtual HRESULT execute_command_line(wchar_t * command_line, wchar_t*out = NULL)
	{
		wchar_t * splitted[5] = {0};
		wcsexplode(command_line, L"|", splitted, 5);

		HRESULT hr = execute_command(splitted[0], out, splitted[1], splitted[2], splitted[3], splitted[4]);

		for(int i=0; i<5; i++)
			if (splitted[i])
				free(splitted[i]);
		
		return hr;
	}
	virtual HRESULT execute_command_adv(wchar_t *command, wchar_t *out, const wchar_t **args, int args_count) PURE;
};