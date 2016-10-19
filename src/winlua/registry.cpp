#include "winlua.hpp"

/* ------------------------------------------------------------
WinLua Registry userdata and metatable
------------------------------------------------------------ */
#define WINLUA_REGKEY_META "WinLuaRegKey"

struct WinLuaRegKey
{
	HKEY handle;
	bool managed;
};

static void winlua_push_regkey(lua_State *L, HKEY handle, bool managed)
{
	WinLuaRegKey *udata = static_cast<WinLuaRegKey*>(lua_newuserdata(L, sizeof(WinLuaRegKey)));
	luaL_setmetatable(L, WINLUA_REGKEY_META);
	udata->handle = handle;
	udata->managed = managed;
}

static int regkey__gc(lua_State *L)
{
	WinLuaRegKey *udata = static_cast<WinLuaRegKey*>(luaL_checkudata(L, 1, WINLUA_REGKEY_META));

	if (udata->managed && udata->handle != NULL)
	{
		//printf("finalizing registry key %p\n", udata->handle);
		RegCloseKey(udata->handle);
		udata->handle = NULL;
		udata->managed = false;
	}

	return 0;
}

static int regkey_open(lua_State *L)
{
	WinLuaRegKey *udata = static_cast<WinLuaRegKey*>(luaL_checkudata(L, 1, WINLUA_REGKEY_META));
	HKEY newkey = NULL;

	const char *subkey = luaL_checkstring(L, 2);
	LONG ret = RegOpenKeyExA(udata->handle, subkey, 0, KEY_READ, &newkey);
	if (ret != ERROR_SUCCESS)
	{
		return luaL_error(L, "could not open subkey '%s' (%d)", subkey, GetLastError());
	}

	winlua_push_regkey(L, newkey, true);
	return 1;
}

#ifdef MAX_PATH
#define WINLUA_REGKEY_CLASSNAME_BUFSZ MAX_PATH
#else
#define WINLUA_REGKEY_CLASSNAME_BUFSZ 512
#endif 

static int regkey_getclass(lua_State *L)
{
	WinLuaRegKey *udata = static_cast<WinLuaRegKey*>(luaL_checkudata(L, 1, WINLUA_REGKEY_META));

	DWORD classNameLen = WINLUA_REGKEY_CLASSNAME_BUFSZ;
	char className[WINLUA_REGKEY_CLASSNAME_BUFSZ];

	LONG ret = RegQueryInfoKeyA(udata->handle,
		className, &classNameLen, // class
		NULL, // reserved
		NULL, NULL, NULL, // subkeys
		NULL, NULL, NULL, // values
		NULL, // security descriptor
		NULL // last write time
	);

	if (ret != ERROR_SUCCESS)
	{
		return luaL_error(L, "could not query class name (%d)", ret);
	}

	lua_pushstring(L, className);
	return 1;
}

/* ------------------------------------------------------------
RegKey subkey iterator functions
------------------------------------------------------------ */
/*
upvalues:
	1 -> registry key userdata
	2 -> subkey name buffer
	3 -> total amount of subkeys
	4 -> current index of iteration
*/
static int regkey_subkeys_next(lua_State *L)
{
	DWORD subkeynbr = static_cast<DWORD>(lua_tonumber(L, lua_upvalueindex(3)));
	unsigned i = static_cast<unsigned>(lua_tonumber(L, lua_upvalueindex(4)));

	/* was this the last iteration? */
	if (i >= subkeynbr) 
	{
		return 0; 
	}

	WinLuaRegKey *udata = static_cast<WinLuaRegKey*>(luaL_checkudata(L, lua_upvalueindex(1), WINLUA_REGKEY_META));
	DWORD bufflen = static_cast<DWORD>(lua_rawlen(L, lua_upvalueindex(2)));
	char *buffer = static_cast<char*>(lua_touserdata(L, lua_upvalueindex(2)));

	LONG ret = RegEnumKeyExA(udata->handle, i, buffer, &bufflen, NULL, NULL, NULL, NULL);
	if (ret == ERROR_SUCCESS)
	{
		lua_pushstring(L, buffer);
	}
	else
	{
		lua_pushnil(L);
	}

	/* increment iteration counter */
	lua_pushinteger(L, i + 1);
	lua_replace(L, lua_upvalueindex(4));

	return 1;
}

static int regkey_subkeys(lua_State *L)
{
	WinLuaRegKey *udata = static_cast<WinLuaRegKey*>(luaL_checkudata(L, 1, WINLUA_REGKEY_META));
	DWORD subkeyAmount, subkeyMaxLength;

	LONG ret = RegQueryInfoKeyA(udata->handle,
		NULL, NULL, // class
		NULL, // reserved
		&subkeyAmount, &subkeyMaxLength, NULL, // subkeys
		NULL, NULL, NULL, // values
		NULL, // security descriptor
		NULL // last write time
	);

	if (ret != ERROR_SUCCESS)
	{
		return luaL_error(L, "could not query sub keys (%d)", ret);
	}

	lua_pushvalue(L, 1);
	lua_newuserdata(L, sizeof(wchar_t) * subkeyMaxLength);
	lua_pushinteger(L, subkeyAmount);
	lua_pushinteger(L, 0);
	lua_pushcclosure(L, regkey_subkeys_next, 4);
	return 1;
}

/* ------------------------------------------------------------
RegKey value iterator functions
------------------------------------------------------------ */
/*
upvalues:
	1 -> registry key userdata
	2 -> value name buffer
	3 -> total amount of values
	4 -> current index of iteration
*/
static int regkey_values_next(lua_State *L)
{
	DWORD valueAmount = static_cast<DWORD>(lua_tonumber(L, lua_upvalueindex(3)));
	unsigned i = static_cast<unsigned>(lua_tonumber(L, lua_upvalueindex(4)));

	/* was this the last iteration? */
	if (i >= valueAmount) 
	{
		return 0; 
	}

	WinLuaRegKey *udata = static_cast<WinLuaRegKey*>(luaL_checkudata(L, lua_upvalueindex(1), WINLUA_REGKEY_META));
	DWORD bufflen = static_cast<DWORD>(lua_rawlen(L, lua_upvalueindex(2)));
	char *buffer = static_cast<char*>(lua_touserdata(L, lua_upvalueindex(2)));

	LONG ret = RegEnumValueA(udata->handle, i, buffer, &bufflen, NULL, NULL, NULL, NULL);
	if (ret == ERROR_SUCCESS)
	{
		lua_pushstring(L, buffer);
	}
	else
	{
		lua_pushnil(L);
	}

	/* increment iteration counter */
	lua_pushinteger(L, i + 1);
	lua_replace(L, lua_upvalueindex(4));

	return 1;
}

static int regkey_values(lua_State *L)
{
	WinLuaRegKey *udata = static_cast<WinLuaRegKey*>(luaL_checkudata(L, 1, WINLUA_REGKEY_META));
	DWORD valueAmount, valueMaxLength;

	LONG ret = RegQueryInfoKeyA(udata->handle,
		NULL, NULL, // class
		NULL, // reserved
		NULL, NULL, NULL, // subkeys
		&valueAmount, &valueMaxLength, NULL, // values
		NULL, // security descriptor
		NULL // last write time
	);

	if (ret != ERROR_SUCCESS)
	{
		return luaL_error(L, "could not query value names (%d)", ret);
	}

	lua_pushvalue(L, 1);
	lua_newuserdata(L, sizeof(wchar_t) * valueMaxLength);
	lua_pushinteger(L, valueAmount);
	lua_pushinteger(L, 0);
	lua_pushcclosure(L, regkey_values_next, 4);
	return 1;
}


/* ------------------------------------------------------------
WinLua Registry functions
------------------------------------------------------------ */

/* ------------------------------------------------------------
WinLua Registry module
------------------------------------------------------------ */

static const luaL_Reg regkey_methods[] = {
	{"__gc", regkey__gc},
	{"open", regkey_open},
	{"class", regkey_getclass},
	{"subkeys", regkey_subkeys},
	{"values", regkey_values},
	{NULL, NULL}
};

/*
static const luaL_Reg module_functions[] = {
	{"", },
	{NULL, NULL}
};
*/

static void create_regkey_meta(lua_State *L)
{
	luaL_newmetatable(L, WINLUA_REGKEY_META);  /* create metatable for registry key handles */
	lua_pushvalue(L, -1);  /* push metatable */
	lua_setfield(L, -2, "__index");  /* metatable.__index = metatable */
	luaL_setfuncs(L, regkey_methods, 0);  /* add regkey methods to new metatable */
	lua_pop(L, 1);  /* pop new metatable */
}

int luaopen_registry(lua_State *L)
{
	create_regkey_meta(L);

	lua_createtable(L, 0, 6);
	winlua_push_regkey(L, HKEY_CLASSES_ROOT, false);
	lua_setfield(L, -2, "HKEY_CLASSES_ROOT");
	winlua_push_regkey(L, HKEY_CURRENT_CONFIG, false);
	lua_setfield(L, -2, "HKEY_CURRENT_CONFIG");
	winlua_push_regkey(L, HKEY_CURRENT_USER, false);
	lua_setfield(L, -2, "HKEY_CURRENT_USER");
	winlua_push_regkey(L, HKEY_LOCAL_MACHINE, false);
	lua_setfield(L, -2, "HKEY_LOCAL_MACHINE");
	winlua_push_regkey(L, HKEY_PERFORMANCE_DATA, false);
	lua_setfield(L, -2, "HKEY_PERFORMANCE_DATA");
	winlua_push_regkey(L, HKEY_USERS, false);
	lua_setfield(L, -2, "HKEY_USERS");

	return 1;
}