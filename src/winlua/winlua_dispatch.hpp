#ifndef _LUAWIN_DISPATCH_HPP_INCLUDED_
#define _LUAWIN_DISPATCH_HPP_INCLUDED_

#include "winlua.hpp"
#include <Ole2.h>
#include <Objbase.h>

/* ------------------------------------------------------------
LuaWin COM helper templates
------------------------------------------------------------ */

template< typename T >
void luawin_push_interface(lua_State *L, T obj, const char *meta)
{
	T *pobj = static_cast<T*>(lua_newuserdata(L, sizeof(T)));
	luaL_setmetatable(L, meta);
	*pobj = obj;
}

template< typename T >
void SafeRelease(T obj)
{
	if (obj)
	{
		obj->Release();
	}
}

/* ------------------------------------------------------------
LuaWin OLE functions
------------------------------------------------------------ */
#define WINLUA_DISPATCH_ATEXIT "WinLuaOle"
int ole_finalizer(lua_State *L);

int LuaCreateObject(lua_State *L, wchar_t *progidW, IDispatch*& disp);

const char *invkind_to_string(INVOKEKIND kind);

#endif