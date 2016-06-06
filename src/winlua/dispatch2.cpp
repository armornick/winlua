#include "winlua_dispatch.hpp"

#pragma comment(lib, "Ole32.lib")

/* ------------------------------------------------------------
WinLua IDispatch utility functions
------------------------------------------------------------ */
#define WINLUA_IDISPATCH_META "winlua.IDispatch"

void winlua_push_idispatch(lua_State *L, IDispatch *obj)
{
	winlua_push_interface(L, obj, WINLUA_IDISPATCH_META);
}

IDispatch** winlua_get_idispatch(lua_State *L, int idx)
{
	return static_cast<IDispatch**>(luaL_checkudata(L, idx, WINLUA_IDISPATCH_META));
}

static const char *invoke_error_to_string(HRESULT err)
{
	switch (err)
	{
		case DISP_E_BADPARAMCOUNT: return "invalid number of parameters";
		case DISP_E_BADVARTYPE: return "invalid argument type";
		case DISP_E_EXCEPTION: return "an exception was raised";
		case DISP_E_MEMBERNOTFOUND: return "requested member does not exist";
		case DISP_E_NONAMEDARGS: return "named arguments not supported";
		case DISP_E_OVERFLOW: return "one or more argument(s) could not be coerced to the needed type";
		case DISP_E_PARAMNOTFOUND: return "parameter not found";
		case DISP_E_TYPEMISMATCH: return "one or more of the arguments could not be coerced";
		case DISP_E_UNKNOWNINTERFACE: return "interface identifier passed in riid is not IID_NULL";
		case DISP_E_PARAMNOTOPTIONAL: return "required parameter was omitted";
	}
	return "unknown error has occurred";
}

void winlua_push_variant(lua_State *L, VARIANT& variant)
{
	switch (variant.vt)
	{
		case VT_NULL:
			lua_pushnil(L);
			break;
		case VT_I2:
			lua_pushinteger(L, V_I2(&variant));
			break;
		case VT_I4:
			lua_pushinteger(L, V_I4(&variant));
			break;
		case VT_R4:
			lua_pushnumber(L, V_R4(&variant));
			break;
		case VT_R8:
			lua_pushnumber(L, V_R8(&variant));
			break;
		// case VT_CY
		// case VT_DATE
		case VT_BSTR:
			wstring_to_utf8(L, V_BSTR(&variant));
			SysFreeString(V_BSTR(&variant));
			break;
		case VT_DISPATCH:
			winlua_push_idispatch(L, V_DISPATCH(&variant));
			break;
		// case VT_ERROR
		case VT_BOOL:
			lua_pushboolean(L, V_BOOL(&variant));
			break;
		// case VT_VARIANT
		// case VT_DECIMAL
		case VT_I1:
			lua_pushinteger(L, V_I1(&variant));
			break;
		case VT_UI1:
			lua_pushinteger(L, V_UI1(&variant));
			break;
		case VT_UI2:
			lua_pushinteger(L, V_UI2(&variant));
			break;
		case VT_UI4:
			lua_pushinteger(L, V_UI4(&variant));
			break;
		case VT_I8:
			lua_pushinteger(L, V_I8(&variant));
			break;
		case VT_UI8:
			lua_pushinteger(L, V_UI8(&variant));
			break;
		case VT_INT:
			lua_pushinteger(L, V_INT(&variant));
			break;
		case VT_UINT:
			lua_pushinteger(L, V_UINT(&variant));
			break;
		// case VT_VOID
		// case VT_PTR
		// case VT_SAFEARRAY
		// case VT_CARRAY
		// case VT_USERDEFINED
		// case VT_RECORD
		// case VT_INT_PTR
		// case VT_UINT_PTR
		// case VT_FILETIME
		// case VT_BLOB
		// case VT_ARRAY

		default:
			lua_pushnil(L);
			break;
	}
}


/* ------------------------------------------------------------
WinLua IDispatch methods
------------------------------------------------------------ */
static int idispatch__gc(lua_State *L)
{
	printf("attempting to finalize IDispatch\n");
	IDispatch **pobj = winlua_get_idispatch(L, 1);
	SafeRelease(*pobj);
	*pobj = NULL;
	return 0;
}

static int idispatch_gettypeinfo(lua_State *L)
{
	IDispatch *disp = *(winlua_get_idispatch(L, 1));

	lua_getglobal(L, "require");
	lua_pushstring(L, "dispatch.typeinfo");
	lua_call(L, 1, 0);

	UINT count;
	HRESULT ret = disp->GetTypeInfoCount(&count);
	if (FAILED(ret))
	{
		return luaL_error(L, "type info not available");
	}

	if (count == 0)
	{
		lua_pushnil(L);
		return 1;
	}

	ITypeInfo *info = NULL;
	ret = disp->GetTypeInfo(0, LOCALE_SYSTEM_DEFAULT, &info);
	if (FAILED(ret)) 
	{ 
		return luaL_error(L, "could not get type info"); 
	}

	winlua_push_typeinfo(L, info);
	return 1;
}

static int idispatch_callmethod(lua_State *L)
{
	IDispatch *disp = *(winlua_get_idispatch(L, 1));

	const char *name = luaL_checkstring(L, 2);
	wchar_t *nameW = utf8_to_wstring(L, name);

	DISPID dispid;
	if (FAILED(disp->GetIDsOfNames(IID_NULL, &nameW, 1, 
		LOCALE_SYSTEM_DEFAULT, &dispid)))
	{
		return luaL_error(L, "GetIDsOfNames on IDispatch failed (name: '%s')", name);
	}

	int argc = lua_gettop(L) - 3; // -2 for the object and name, -1 for the temporary wstring

	/* easy case with no arguments */
	if (argc == 0)
	{
		VARIANT Result;
		VariantInit(&Result);
		DISPPARAMS NoParams = { NULL, NULL, 0, 0 };

		HRESULT hr = disp->Invoke(dispid, IID_NULL, 
			LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD, &NoParams, 
			&Result, NULL, NULL);

		if (FAILED(hr))
		{
			return luaL_error(L, "%s call failed: %s", name, invoke_error_to_string(hr));
		}

		winlua_push_variant(L, Result);
		return 1;
	}
	else
	{
		return luaL_error(L, "arguments currently not supported (%d)", lua_gettop(L));
	}
}

static int idispatch_getproperty(lua_State *L)
{
	IDispatch *disp = *(winlua_get_idispatch(L, 1));

	const char *name = luaL_checkstring(L, 2);
	wchar_t *nameW = utf8_to_wstring(L, name);

	DISPID dispid;
	if (FAILED(disp->GetIDsOfNames(IID_NULL, &nameW, 1, 
		LOCALE_SYSTEM_DEFAULT, &dispid)))
	{
		return luaL_error(L, "GetIDsOfNames on IDispatch failed (name: '%s')", name);
	}

	VARIANT Result;
	VariantInit(&Result);
	DISPPARAMS NoParams = { NULL, NULL, 0, 0 };

	HRESULT hr = disp->Invoke(dispid, IID_NULL, 
		LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET|DISPATCH_METHOD, &NoParams,
		&Result, NULL, NULL);

	if (FAILED(hr))
	{
		return luaL_error(L, "%s call failed: %s", name, invoke_error_to_string(hr));
	}

	winlua_push_variant(L, Result);
	return 1;
}

/* ------------------------------------------------------------
WinLua IDispatch functions
------------------------------------------------------------ */
static int dispatch_get_typeinfo(lua_State *L)
{
	const char *progid = luaL_checkstring(L, 1);
	wchar_t *progidW = utf8_to_wstring(L, progid);
	
	IDispatch *disp = NULL;
	if (LuaCreateObject(L, progidW, disp))
	{
		return 0;
	}

	winlua_push_idispatch(L, disp);
	return 1;
}

/* ------------------------------------------------------------
LuaWin TypeInfo module
------------------------------------------------------------ */
static const luaL_Reg idispatch_meta[] = {
	{"__gc", idispatch__gc},
	{"GetTypeInfo", idispatch_gettypeinfo},
	{"CallMethod", idispatch_callmethod},
	{"GetProperty", idispatch_getproperty},
	{NULL, NULL}
};

static void create_idispatch_meta(lua_State *L)
{
	luaL_newmetatable(L, WINLUA_IDISPATCH_META);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, idispatch_meta, 0);
	lua_pop(L, 1);
}

int luaopen_dispatch_thin(lua_State *L)
{
	if (lua_getfield(L, LUA_REGISTRYINDEX, WINLUA_DISPATCH_ATEXIT) == LUA_TNIL)
	{
		if (FAILED(OleInitialize(NULL)))
		{
			return luaL_error(L, "could not initialize OLE runtime");
		}
		winlua_at_exit(L, WINLUA_DISPATCH_ATEXIT, ole_finalizer);
	}
	/* create the typeinfo metatable */
	create_idispatch_meta(L);
	/* return the typeinfo constructor */
	lua_pushcfunction(L, dispatch_get_typeinfo);
	return 1;
}