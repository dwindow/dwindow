#pragma once

#include <streams.h>
#include <list>

extern "C"
{
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

int dwindow_lua_init();
int dwindow_lua_exit();
int lua_mypcall(lua_State *L, int n, int r, int flag);
int lua_save_settings();
int lua_load_settings();

class luaState
{
public:
	luaState();
	~luaState();
	operator lua_State*();
	lua_State * L;
};

class lua_variable;
class lua_const;

class lua_manager
{
public:
	lua_manager(const char* table_name);
	~lua_manager();
	
	int refresh();
	lua_variable & get_variable(const char*name);
	lua_const & get_const(const char*name);
	int delete_variable(const char*name);

protected:
	friend class lua_variable;
	friend class lua_const;
	std::list<lua_variable*> m_variables;
	std::list<lua_const*> m_consts;
	CCritSec m_cs;
	char *m_table_name;
};

class lua_variable
{
public:
	operator int();
	operator double();
	operator const wchar_t*();
	operator bool();
	void operator=(const int in);
	void operator=(const double in);
	void operator=(const wchar_t* in);
	void operator=(const bool in);
	void operator=(lua_CFunction func);		// write only
protected:
	friend class lua_manager;
	lua_variable(const char*name, lua_manager *manager);
	~lua_variable();
	lua_manager *m_manager;
	char *m_name;
};

class lua_const
{
public:
	operator int();
	operator DWORD();
	operator double();
	operator const wchar_t*();
	operator bool();
	operator RECT();
	int& operator=(const int in);
	DWORD& operator=(const DWORD in);
	double& operator=(const double in);
	wchar_t*& operator=(const wchar_t* in);
	bool& operator=(const bool in);
	RECT& operator=(const RECT in);

	int read_from_lua();
protected:
	friend class lua_manager;
	lua_const(const char*name, lua_manager *manager);
	~lua_const();
	lua_manager *m_manager;
	char *m_name;
	union
	{
		int i;
		double d;
		bool b;
		wchar_t *s;
		RECT r;
	} m_value;
	enum
	{
		_int,
		_double,
		_bool,
		_string,
		_rect,
	} m_type;
};

extern lua_manager *g_lua_core_manager;
extern lua_manager *g_lua_setting_manager;
#define GET_CONST(x) (dwindow_lua_init() == 0 ? g_lua_setting_manager : g_lua_setting_manager)->get_const(x)