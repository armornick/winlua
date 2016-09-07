#ifndef WINLUA_HPP_INCLUDED
#define WINLUA_HPP_INCLUDED

#include <lua.hpp>
#include <windows.h>

/* ------------------------------------------------------------
WinLua String Utility Functions
------------------------------------------------------------ */
const char *wstring_to_utf8(lua_State *L, wchar_t *inputs);
wchar_t *utf8_to_wstring(lua_State *L, const char *inputs);

/* ------------------------------------------------------------
WinLua Time Utility Functions
------------------------------------------------------------ */
void winlua_push_date(lua_State *L, FILETIME& ft);
void winlua_push_date(lua_State *L, SYSTEMTIME& st);

int winlua_get_date(lua_State *L, int idx, SYSTEMTIME& st);

int timet_to_systemtime(time_t &from, SYSTEMTIME& to);
int timet_to_filetime(time_t &from, FILETIME& to);

/* ------------------------------------------------------------
WinLua Module Utility Functions
------------------------------------------------------------ */
void winlua_at_exit(lua_State *L, const char *id, lua_CFunction finalizer);

#endif