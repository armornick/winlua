#include "winlua.hpp"


/* ------------------------------------------------------------
WinLua Table Utility Functions
------------------------------------------------------------ */
int table_field_integer(lua_State *L, int idx, const char *field, lua_Integer& ret)
{
	ret = 0;
	idx = lua_absindex(L, idx);
	int valid = 0;

	if (lua_getfield(L, idx, field) == LUA_TNUMBER)
	{
		valid = 1;
		ret = lua_tointeger(L, -1);
	}
	lua_pop(L, 1);

	return valid;
}


/* ------------------------------------------------------------
WinLua String Utility Functions
------------------------------------------------------------ */

const char *wstring_to_utf8(lua_State *L, wchar_t *inputs)
{
	int needed = WideCharToMultiByte(CP_UTF8, 0, inputs, -1, NULL, 0, NULL, NULL);
	if (needed == 0)
	{
		lua_pushstring(L, "");
	}
	else
	{
		char *buff = static_cast<char*>(lua_newuserdata(L, sizeof(char) * (needed+1)));
		WideCharToMultiByte(CP_UTF8, 0, inputs, -1, buff, (needed+1), NULL, NULL);
		lua_pushstring(L, buff);
		lua_remove(L, -2);
	}
	return lua_tostring (L, -1);
}

wchar_t *utf8_to_wstring(lua_State *L, const char *inputs)
{
	int needed = MultiByteToWideChar(CP_UTF8, 0, inputs, -1, NULL, 0);
	wchar_t *buff = static_cast<wchar_t*>(lua_newuserdata(L, sizeof(wchar_t) * (needed+1)));
	if (needed == 0)
	{
		buff[0] = L'\0';
	}
	else
	{
		MultiByteToWideChar(CP_UTF8, 0, inputs, -1, buff, (needed+1));
	}
	return buff;
}

/* ------------------------------------------------------------
WinLua Time Utility Functions
------------------------------------------------------------ */

void winlua_push_date(lua_State *L, FILETIME& ft)
{
	LARGE_INTEGER jan1970FT = {0};
	jan1970FT.QuadPart = 116444736000000000I64; // january 1st 1970

	ULARGE_INTEGER utcFT;
	utcFT.LowPart=ft.dwLowDateTime;
	utcFT.HighPart=ft.dwHighDateTime;

	ULARGE_INTEGER utcDosTime;
	utcDosTime.QuadPart = (utcFT.QuadPart - jan1970FT.QuadPart) / 10000000;
	lua_pushinteger(L, utcDosTime.QuadPart);
}

void winlua_push_date(lua_State *L, SYSTEMTIME& st)
{
	lua_createtable(L, 0, 8);
	lua_pushinteger(L, st.wYear);
	lua_setfield(L, -2, "year");
	lua_pushinteger(L, st.wMonth);
	lua_setfield(L, -2, "month");
	lua_pushinteger(L, st.wDay);
	lua_setfield(L, -2, "day");
	lua_pushinteger(L, st.wHour);
	lua_setfield(L, -2, "hour");
	lua_pushinteger(L, st.wMinute);
	lua_setfield(L, -2, "min");
	lua_pushinteger(L, st.wSecond);
	lua_setfield(L, -2, "sec");
	lua_pushinteger(L, st.wMilliseconds);
	lua_setfield(L, -2, "msec");
	lua_pushinteger(L, st.wDayOfWeek);
	lua_setfield(L, -2, "wday");
}

int winlua_get_date(lua_State *L, int idx, SYSTEMTIME& st)
{
	idx = lua_absindex(L, idx);

	if (lua_istable(L, idx))
	{
		lua_Integer tmp;
		table_field_integer(L, idx, "year", tmp);
		st.wYear = static_cast<WORD>(tmp);
		table_field_integer(L, idx, "month", tmp);
		st.wMonth = static_cast<WORD>(tmp);
		table_field_integer(L, idx, "day", tmp);
		st.wDay = static_cast<WORD>(tmp);
		table_field_integer(L, idx, "hour", tmp);
		st.wHour = static_cast<WORD>(tmp);
		table_field_integer(L, idx, "min", tmp);
		st.wMinute = static_cast<WORD>(tmp);
		table_field_integer(L, idx, "sec", tmp);
		st.wSecond = static_cast<WORD>(tmp);
		table_field_integer(L, idx, "msec", tmp);
		st.wMilliseconds = static_cast<WORD>(tmp);
		return 1;
	}
	else
	{
		return 0;
	}
}

/* ------------------------------------------------------------
WinLua Module Utility Functions
------------------------------------------------------------ */

void winlua_at_exit(lua_State *L, const char *id, lua_CFunction finalizer)
{
	lua_newuserdata(L, 1);
	lua_createtable(L, 0, 1);
	lua_pushcfunction(L, finalizer);
	lua_setfield(L, -2, "__gc");
	lua_setmetatable(L, -2);
	lua_setfield(L, LUA_REGISTRYINDEX, id);
}