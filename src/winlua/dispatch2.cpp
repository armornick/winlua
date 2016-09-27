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
		case DISP_E_BADPARAMCOUNT: return "invalid number of parameters (DISP_E_BADPARAMCOUNT)";
		case DISP_E_BADVARTYPE: return "invalid argument type (DISP_E_BADVARTYPE)";
		case DISP_E_EXCEPTION: return "an exception was raised (DISP_E_EXCEPTION)";
		case DISP_E_MEMBERNOTFOUND: return "requested member does not exist (DISP_E_MEMBERNOTFOUND)";
		case DISP_E_NONAMEDARGS: return "named arguments not supported (DISP_E_NONAMEDARGS)";
		case DISP_E_OVERFLOW: return "one or more argument(s) could not be coerced to the needed type (DISP_E_OVERFLOW)";
		case DISP_E_PARAMNOTFOUND: return "parameter not found (DISP_E_PARAMNOTFOUND)";
		case DISP_E_TYPEMISMATCH: return "one or more of the arguments could not be coerced (DISP_E_TYPEMISMATCH)";
		case DISP_E_UNKNOWNINTERFACE: return "interface identifier passed in riid is not IID_NULL (DISP_E_UNKNOWNINTERFACE)";
		case DISP_E_PARAMNOTOPTIONAL: return "required parameter was omitted (DISP_E_PARAMNOTOPTIONAL)";
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

void winlua_get_variant(lua_State *L, int idx, VARIANT& variant, bool& shouldFree)
{
	shouldFree = false;
	int ltype = lua_type(L, idx);

	switch (ltype)
	{
		case LUA_TNIL:
			V_VT(&variant) = VT_ERROR;
			V_ERROR(&variant) = DISP_E_PARAMNOTFOUND;
			break;

		case LUA_TNUMBER:
			if (lua_isinteger(L, idx))
			{
				V_VT(&variant) = VT_I8;
				V_I8(&variant) = lua_tointeger(L, idx);
			}
			else
			{
				V_VT(&variant) = VT_R8;
				V_R8(&variant) = lua_tonumber(L, idx);
			}
			break;

		case LUA_TBOOLEAN:
			V_VT(&variant) = VT_BOOL;
			V_BOOL(&variant) = lua_toboolean(L, idx) ? VARIANT_TRUE : VARIANT_FALSE;
			break;

		case LUA_TSTRING:
		{
			const char *value = lua_tostring(L, idx);
			wchar_t *valueW = utf8_to_wstring(L, value);
			V_VT(&variant) = VT_BSTR;
			V_BSTR(&variant) = SysAllocString(valueW);
			// WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), L"'", 1, NULL, NULL);
			// WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), valueW, lstrlenW(valueW), NULL, NULL);
			// WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), L"'\n", 2, NULL, NULL);
			lua_pop(L, 1);
			shouldFree = true;
		}
		break;

		case LUA_TUSERDATA:
		{
			IDispatch **pdisp = static_cast<IDispatch**>(luaL_testudata(L, idx, WINLUA_IDISPATCH_META));
			if (pdisp != NULL)
			{
				V_VT(&variant) = VT_DISPATCH;
				V_DISPATCH(&variant) = *pdisp;
			}
			else
			{
				V_VT(&variant) = VT_ERROR;
				V_ERROR(&variant) = DISP_E_PARAMNOTFOUND;
			}
		}
		break;

		case LUA_TLIGHTUSERDATA:
		default:
			V_VT(&variant) = VT_ERROR;
			V_ERROR(&variant) = DISP_E_PARAMNOTFOUND;
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

static void perform_invoke(lua_State *L, const char *member, IDispatch*& disp, DISPID& dispid, WORD type, DISPPARAMS& Params)
{
	EXCEPINFO excep = { 0 };
	VARIANT Result; VariantInit(&Result);

	HRESULT hr = disp->Invoke(dispid, IID_NULL,
		LOCALE_SYSTEM_DEFAULT, type, &Params, 
		&Result, &excep, NULL);

	if (FAILED(hr))
	{
		if (hr == DISP_E_EXCEPTION)
		{
			const char *exDescription = wstring_to_utf8(L, excep.bstrDescription);
			SysFreeString(excep.bstrDescription);
			SysFreeString(excep.bstrSource);
			SysFreeString(excep.bstrHelpFile);
			luaL_error(L, "an exception occurred while performing operation on %s: %s", member, exDescription);
		}
		luaL_error(L, "operation on %s failed: %s", member, invoke_error_to_string(hr));
	}

	winlua_push_variant(L, Result);
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
		DISPPARAMS NoParams = { NULL, NULL, 0, 0 };
		perform_invoke(L, name, disp, dispid, DISPATCH_METHOD, NoParams);
		return 1;
	}
	else
	{
		if (argc > WINLUA_DISPATCH_MAXARGC)
		{
			return luaL_error(L, "more than %d arguments currently not supported (received %d arguments)", WINLUA_DISPATCH_MAXARGC, argc);
		}

		// optional parameters can be omitted but the last parameter must be
		// a valid value
		for (int i = argc-1; i >= 0; i--)
		{
			if (lua_isnil(L, 3+i)) argc--;
			else break;
		}

		VARIANT params[WINLUA_DISPATCH_MAXARGC]; bool freeParams[WINLUA_DISPATCH_MAXARGC];

		for (int i = 0; i < argc; i++)
		{
			winlua_get_variant(L, 3+i, params[i], freeParams[i]);
		}
		
		DISPPARAMS Params = { params, NULL, argc, 0 };
		perform_invoke(L, name, disp, dispid, DISPATCH_METHOD, Params);

		for (int i = 0; i < argc; i++)
		{
			if (freeParams[i]) VariantClear(&params[i]);
		}

		return 1;
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

	DISPPARAMS NoParams = { NULL, NULL, 0, 0 };
	perform_invoke(L, name, disp, dispid, DISPATCH_PROPERTYGET|DISPATCH_METHOD, NoParams);
	return 1;
}

static int idispatch_setproperty(lua_State *L)
{
	IDispatch *disp = *(winlua_get_idispatch(L, 1));

	if (lua_gettop(L) != 3)
	{
		return luaL_error(L, "SetProperty should be called with exactly 3 arguments (got %d)", lua_gettop(L));
	}

	const char *name = luaL_checkstring(L, 2);
	wchar_t *nameW = utf8_to_wstring(L, name);

	EXCEPINFO excep = { 0 };
	DISPID dispid; DISPID dispidNamed  = DISPID_PROPERTYPUT;
	if (FAILED(disp->GetIDsOfNames(IID_NULL, &nameW, 1, 
		LOCALE_SYSTEM_DEFAULT, &dispid)))
	{
		return luaL_error(L, "GetIDsOfNames on IDispatch failed (name: '%s')", name);
	}

	VARIANT param; bool freeParam;
	winlua_get_variant(L, 3, param, freeParam);

	DISPPARAMS Params = { &param, &dispidNamed, 1, 1 };
	perform_invoke(L, name, disp, dispid, DISPATCH_PROPERTYPUT, Params);
	if (freeParam) VariantClear(&param);
	return 1;
}

/* ------------------------------------------------------------
WinLua IDispatch functions
------------------------------------------------------------ */
static int dispatch_create_object(lua_State *L)
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
	{"SetProperty", idispatch_setproperty},
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

	/* import ITypeInfo metatable */
	lua_getglobal(L, "require");
	lua_pushstring(L, "dispatch.typeinfo");
	lua_call(L, 1, 0);

	/* create the IDispatch metatable */
	create_idispatch_meta(L);
	/* return the IDispatch constructor */
	lua_pushcfunction(L, dispatch_create_object);
	return 1;
}