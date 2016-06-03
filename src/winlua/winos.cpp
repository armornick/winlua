#include "winlua.hpp"

/* ------------------------------------------------------------
WinLua Operating System functions
------------------------------------------------------------ */

static int winlua_getenv(lua_State *L)
{
	const char *varname = luaL_checkstring(L, 1);
	wchar_t *varnameW = utf8_to_wstring(L, varname);

	DWORD needed = GetEnvironmentVariableW(varnameW, NULL, 0);
	if (needed == 0)
	{
		DWORD err = GetLastError();
		if (err == ERROR_ENVVAR_NOT_FOUND)
		{
			lua_pushnil(L);
			return 1;
		}
		else
		{
			DWORD err = GetLastError();
			lua_pushnil(L);
			lua_pushfstring(L, "could not get needed buffer size for environment variable '%s' (%d)", varname, err);
			lua_pushinteger(L, err);
			return 3;
		}
	}

	wchar_t *buff = static_cast<wchar_t*>(lua_newuserdata(L, sizeof(wchar_t) * needed));
	needed = GetEnvironmentVariableW(varnameW, buff, needed);
	if (needed == 0)
	{
		DWORD err = GetLastError();
		lua_pushnil(L);
		lua_pushfstring(L, "could not get environment variable '%s' (%d)", varname, err);
		lua_pushinteger(L, err);
		return 3;
	}

	wstring_to_utf8(L, buff);
	lua_remove(L, -2); lua_remove(L, -2); // pop temporary buffer and name buffer
	return 1;
}

static int winlua_putenv(lua_State *L)
{
	const char *varname = luaL_checkstring(L, 1);
	const char *varvalue = luaL_checkstring(L, 2);
	wchar_t *varnameW = utf8_to_wstring(L, varname);
	wchar_t *varvalueW = utf8_to_wstring(L, varvalue);

	BOOL ret = SetEnvironmentVariableW(varnameW, varvalueW);
	if (ret == 0)
	{
		DWORD err = GetLastError();
		lua_pushnil(L);
		lua_pushfstring(L, "could not set environment variable '%s' to '%s' (%d)", varname, varvalue, err);
		lua_pushinteger(L, err);
		return 3;
	}

	lua_pop(L, 2); // pop name and value buffer
	lua_pushboolean(L, 1);
	return 1;
}

static int winlua_remove(lua_State *L)
{
	const char *filename = luaL_checkstring(L, 1);
	wchar_t *filenameW = utf8_to_wstring(L, filename);

	DWORD is_dir = GetFileAttributesW(filenameW);

	if (is_dir == INVALID_FILE_ATTRIBUTES)
	{
		DWORD err = GetLastError();
		lua_pushnil(L);
		lua_pushfstring(L, "could not determine attributes of '%s' (%d)", filename, err);
		lua_pushinteger(L, err);
		return 3;
	}

	if (is_dir & FILE_ATTRIBUTE_DIRECTORY)
	{
		BOOL ret = RemoveDirectoryW(filenameW);
		if (ret == 0)
		{
			DWORD err = GetLastError();
			lua_pushnil(L);
			lua_pushfstring(L, "could not delete directory '%s' (%d)", filename, err);
			lua_pushinteger(L, err);
			return 3;
		}
	}
	else
	{
		BOOL ret = DeleteFileW(filenameW);
		if (ret == 0)
		{
			DWORD err = GetLastError();
			lua_pushnil(L);
			lua_pushfstring(L, "could not delete file '%s' (%d)", filename, err);
			lua_pushinteger(L, err);
			return 3;
		}
	}

	lua_pushboolean(L, 1);
	return 1;
}

static int winlua_rename(lua_State *L)
{
	const char *oldname = luaL_checkstring(L, 1);
	wchar_t *oldnameW = utf8_to_wstring(L, oldname);
	const char *newname = luaL_checkstring(L, 2);
	wchar_t *newnameW = utf8_to_wstring(L, newname);

	BOOL ret = MoveFileW(oldnameW, newnameW);
	if (ret == 0)
	{
		DWORD err = GetLastError();
		lua_pushnil(L);
		lua_pushfstring(L, "could not rename file '%s' to '%s' (%d)", oldname, newname, err);
		lua_pushinteger(L, err);
		return 3;
	}
	return 0;
}

static int winlua_time(lua_State *L)
{
	SYSTEMTIME st;

	if (lua_isnoneornil(L, 1))
	{
		GetSystemTime(&st);
	}
	else
	{
		luaL_checktype(L, 1, LUA_TTABLE);
		winlua_get_date(L, 1, st);
	}

	FILETIME ft;
	SystemTimeToFileTime(&st, &ft);

	winlua_push_date(L, ft);
	return 1;
}


/* ------------------------------------------------------------
WinLua Operating System module
------------------------------------------------------------ */

static const luaL_Reg library_methods[] = {
	{"getenv", winlua_getenv},
	{"putenv", winlua_putenv},
	{"remove", winlua_remove},
	{"rename", winlua_rename},
	{"time", winlua_time},
	{NULL, NULL}
};

int luaopen_winos(lua_State *L)
{
	lua_pushcfunction(L, luaopen_os);
	lua_pushvalue(L, 1);
	lua_call(L, 1, 1);

	luaL_setfuncs(L, library_methods, 0);
	return 1;
}