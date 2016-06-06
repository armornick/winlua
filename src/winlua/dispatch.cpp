#include "winlua_dispatch.hpp"

#pragma comment(lib, "Ole32.lib")

/* ------------------------------------------------------------
WinLua Dispatch functions
------------------------------------------------------------ */

int ole_finalizer(lua_State *L)
{
	printf("finalizing OLE runtime\n");
	OleUninitialize();
	return 0;
}


int LuaCreateObject(lua_State *L, wchar_t *progidW, IDispatch*& disp)
{
	HRESULT ret; CLSID clsid;
	IUnknown *punk = NULL; IDispatch *pdisp = NULL;

	ret = CLSIDFromString(progidW, &clsid);
	if (FAILED(ret)) { goto error; }

	ret = CoCreateInstance(clsid, NULL, CLSCTX_ALL, IID_IUnknown, (void **)&punk);
	if (FAILED(ret)) { goto error; }

	ret = punk->QueryInterface(IID_IDispatch, (void **)&pdisp);
	if (FAILED(ret)) { goto error; }

	punk->Release();
	disp = pdisp;
	return 0;

error:
	if (punk) { punk->Release(); }
	if (pdisp) { pdisp->Release(); }
	return luaL_error(L, "error while trying to create object");
}

const char *invkind_to_string(INVOKEKIND kind)
{
	switch (kind)
	{
		case INVOKE_FUNC: return "method";
		case INVOKE_PROPERTYGET: return "getter";
		case INVOKE_PROPERTYPUT: return "setter";
		case INVOKE_PROPERTYPUTREF: return "refsetter";
	}
	return "unknown";
}

/* ------------------------------------------------------------
LuaWin TypeInfo utility functions
------------------------------------------------------------ */
#define WINLUA_TYPEINFO_META "winlua.TypeInfo"

void winlua_push_typeinfo(lua_State *L, ITypeInfo *obj)
{
	winlua_push_interface(L, obj, WINLUA_TYPEINFO_META);
}

ITypeInfo** winlua_get_typeinfo(lua_State *L, int idx)
{
	return static_cast<ITypeInfo**>(luaL_checkudata(L, idx, WINLUA_TYPEINFO_META));
}

static void typedesc_to_string(luaL_Buffer *buffer, TYPEDESC tdesc)
{
	VARTYPE vt = tdesc.vt;

	switch (vt)
	{
		case VT_EMPTY: luaL_addstring(buffer, "VT_EMPTY"); break;
		case VT_NULL: luaL_addstring(buffer, "VT_NULL"); break;
		case VT_I2: luaL_addstring(buffer, "VT_I2"); break;
		case VT_I4: luaL_addstring(buffer, "VT_I4"); break;
		case VT_R4: luaL_addstring(buffer, "VT_R4"); break;
		case VT_R8: luaL_addstring(buffer, "VT_R8"); break;
		case VT_CY: luaL_addstring(buffer, "VT_CY"); break;
		case VT_DATE: luaL_addstring(buffer, "VT_DATE"); break;
		case VT_BSTR: luaL_addstring(buffer, "VT_BSTR"); break;
		case VT_DISPATCH: luaL_addstring(buffer, "VT_DISPATCH"); break;
		case VT_ERROR: luaL_addstring(buffer, "VT_ERROR"); break;
		case VT_BOOL: luaL_addstring(buffer, "VT_BOOL"); break;
		case VT_VARIANT: luaL_addstring(buffer, "VT_VARIANT"); break;
		case VT_UNKNOWN: luaL_addstring(buffer, "VT_UNKNOWN"); break;
		case VT_DECIMAL: luaL_addstring(buffer, "VT_DECIMAL"); break;
		case VT_I1: luaL_addstring(buffer, "VT_I1"); break;
		case VT_UI1: luaL_addstring(buffer, "VT_UI1"); break;
		case VT_UI2: luaL_addstring(buffer, "VT_UI2"); break;
		case VT_UI4: luaL_addstring(buffer, "VT_UI4"); break;
		case VT_I8: luaL_addstring(buffer, "VT_I8"); break;
		case VT_UI8: luaL_addstring(buffer, "VT_UI8"); break;
		case VT_INT: luaL_addstring(buffer, "VT_INT"); break;
		case VT_UINT: luaL_addstring(buffer, "VT_UINT"); break;
		case VT_VOID: luaL_addstring(buffer, "VT_VOID"); break;
		case VT_HRESULT: luaL_addstring(buffer, "VT_HRESULT"); break;
		case VT_SAFEARRAY: luaL_addstring(buffer, "VT_SAFEARRAY"); break;
		case VT_CARRAY: luaL_addstring(buffer, "VT_CARRAY"); break;
		case VT_USERDEFINED: luaL_addstring(buffer, "VT_USERDEFINED"); break;
		case VT_LPSTR: luaL_addstring(buffer, "VT_LPSTR"); break;
		case VT_LPWSTR: luaL_addstring(buffer, "VT_LPWSTR"); break;
		case VT_RECORD: luaL_addstring(buffer, "VT_RECORD"); break;
		case VT_INT_PTR: luaL_addstring(buffer, "VT_INT_PTR"); break;
		case VT_UINT_PTR: luaL_addstring(buffer, "VT_UINT_PTR"); break;
		case VT_FILETIME: luaL_addstring(buffer, "VT_FILETIME"); break;
		case VT_BLOB: luaL_addstring(buffer, "VT_BLOB"); break;
		case VT_STREAM: luaL_addstring(buffer, "VT_STREAM"); break;
		case VT_STORAGE: luaL_addstring(buffer, "VT_STORAGE"); break;
		case VT_STREAMED_OBJECT: luaL_addstring(buffer, "VT_STREAMED_OBJECT"); break;
		case VT_STORED_OBJECT: luaL_addstring(buffer, "VT_STORED_OBJECT"); break;
		case VT_BLOB_OBJECT: luaL_addstring(buffer, "VT_BLOB_OBJECT"); break;
		case VT_CF: luaL_addstring(buffer, "VT_CF"); break;
		case VT_CLSID: luaL_addstring(buffer, "VT_CLSID"); break;
		case VT_VERSIONED_STREAM: luaL_addstring(buffer, "VT_VERSIONED_STREAM"); break;
		case VT_BSTR_BLOB: luaL_addstring(buffer, "VT_BSTR_BLOB"); break;
		case VT_VECTOR: luaL_addstring(buffer, "VT_VECTOR"); break;
		case VT_ARRAY: luaL_addstring(buffer, "VT_ARRAY"); break;
		case VT_BYREF: luaL_addstring(buffer, "VT_BYREF"); break;
		case VT_PTR:
		{
			luaL_addstring(buffer, "VT_PTR ");
			typedesc_to_string(buffer, *tdesc.lptdesc );
			break;
		}
		default: luaL_addstring(buffer, "???"); break;
	}
}

static void winlua_push_elemdesc(lua_State *L, ELEMDESC desc)
{
	unsigned short flags = desc.paramdesc.wParamFlags;

	lua_newtable(L);
	/* elemdesc flags */
	lua_pushboolean(L, flags & PARAMFLAG_FIN);
	lua_setfield(L, -2, "in");
	lua_pushboolean(L, flags & PARAMFLAG_FOUT);
	lua_setfield(L, -2, "out");
	lua_pushboolean(L, flags & PARAMFLAG_FRETVAL);
	lua_setfield(L, -2, "retval");
	lua_pushboolean(L, flags & PARAMFLAG_FOPT);
	lua_setfield(L, -2, "opt");
	/* type descriptor */
	luaL_Buffer b;
	luaL_buffinit(L, &b);
	typedesc_to_string(&b, desc.tdesc);
	luaL_pushresult(&b);
	lua_setfield(L, -2, "tdesc");
}

/* ------------------------------------------------------------
LuaWin TypeInfo methods
------------------------------------------------------------ */
static int typeinfo__gc(lua_State *L)
{
	printf("attempting to finalize ITypeInfo\n");
	ITypeInfo **pobj = winlua_get_typeinfo(L, 1);
	SafeRelease(*pobj);
	*pobj = NULL;
	return 0;
}

static int typeinfo_getname(lua_State *L)
{
	ITypeInfo *info = *(winlua_get_typeinfo(L, 1));
	MEMBERID memid = static_cast<MEMBERID>(lua_isnoneornil(L, 2) ? -1 : luaL_checkinteger(L, 2));

	BSTR nameW;
	if (SUCCEEDED(info->GetDocumentation(memid, &nameW, NULL, NULL, NULL)))
	{
		const char *name = wstring_to_utf8(L, nameW);
		SysFreeString(nameW);
		return 1;
	}
	return luaL_error(L, "GetDocumentation on ITypeInfo failed");
}

static int typeinfo_gettypeattr(lua_State *L)
{
	ITypeInfo *info = *(winlua_get_typeinfo(L, 1));

	TYPEATTR *attrs;
	if (FAILED(info->GetTypeAttr(&attrs)))
	{
		return luaL_error(L, "GetTypeAttr on ITypeInfo failed");
	}

	lua_newtable(L);
	lua_pushinteger(L, attrs->cFuncs);
	lua_setfield(L, -2, "cFuncs");
	lua_pushinteger(L, attrs->cVars);
	lua_setfield(L, -2, "cVars");
	lua_pushinteger(L, attrs->cImplTypes);
	lua_setfield(L, -2, "cImplTypes");
	lua_pushinteger(L, attrs->cbSizeInstance);
	lua_setfield(L, -2, "cbSizeInstance");
	lua_pushinteger(L, attrs->wMajorVerNum);
	lua_setfield(L, -2, "wMajorVerNum");
	lua_pushinteger(L, attrs->wMinorVerNum);
	lua_setfield(L, -2, "wMinorVerNum");
	lua_pushboolean(L, attrs->wTypeFlags & TYPEFLAG_FPREDECLID);
	lua_setfield(L, -2, "predefined");
	lua_pushboolean(L, attrs->wTypeFlags & TYPEFLAG_FHIDDEN);
	lua_setfield(L, -2, "hidden");
	lua_pushboolean(L, attrs->wTypeFlags & TYPEFLAG_FRESTRICTED);
	lua_setfield(L, -2, "restricted");
	return 1;
}

static int typeinfo_getfuncdesc(lua_State *L)
{
	ITypeInfo *info = *(winlua_get_typeinfo(L, 1));
	MEMBERID i = static_cast<MEMBERID>(luaL_checkinteger(L, 2));

	FUNCDESC *funcdesc;
	if (SUCCEEDED(info->GetFuncDesc(i, &funcdesc)))
	{
		lua_newtable(L);
		lua_pushinteger(L, funcdesc->memid);
		lua_setfield(L, -2, "memid");
		lua_pushstring(L, invkind_to_string(funcdesc->invkind));
		lua_setfield(L, -2, "invkind");

		lua_pushinteger(L, funcdesc->cParams);
		lua_setfield(L, -2, "cParams");
		lua_pushinteger(L, funcdesc->cParamsOpt);
		lua_setfield(L, -2, "cParamsOpt");

		winlua_push_elemdesc(L, funcdesc->elemdescFunc);
		lua_setfield(L, -2, "elemdescFunc");

		lua_pushboolean(L, funcdesc->wFuncFlags & FUNCFLAG_FRESTRICTED);
		lua_setfield(L, -2, "restricted");
		lua_pushboolean(L, funcdesc->wFuncFlags & FUNCFLAG_FHIDDEN);
		lua_setfield(L, -2, "hidden");
		lua_pushboolean(L, funcdesc->wFuncFlags & FUNCFLAG_FUSESGETLASTERROR);
		lua_setfield(L, -2, "usesGetLastError");

		lua_newtable(L);
		for (int j = 0; j < funcdesc->cParams; j++)
		{
			winlua_push_elemdesc(L, funcdesc->lprgelemdescParam[j]);
			lua_rawseti(L, -2, j+1);
		}
		lua_setfield(L, -2, "lprgelemdescParam");

		info->ReleaseFuncDesc(funcdesc);
		return 1;
	}
	return luaL_error(L, "GetFuncDesc on ITypeInfo failed");
}

static int typeinfo_getreftypeinfo(lua_State *L)
{
	ITypeInfo *info = *(winlua_get_typeinfo(L, 1));
	UINT i = static_cast<UINT>(luaL_checkinteger(L, 2));

	HREFTYPE href; ITypeInfo *implTypeInfo;
	if (FAILED(info->GetRefTypeOfImplType(i, &href)))
	{
		return luaL_error(L, "GetRefTypeOfImplType on ITypeInfo failed");
	}
	if (SUCCEEDED(info->GetRefTypeInfo(href, &implTypeInfo)))
	{
		// implTypeInfo->AddRef();
		winlua_push_typeinfo(L, implTypeInfo);
		return 1;
	}
	return luaL_error(L, "GetRefTypeInfo on ITypeInfo failed");
}

/* ------------------------------------------------------------
LuaWin TypeInfo functions
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

	UINT count;
	HRESULT ret = disp->GetTypeInfoCount(&count);
	if (FAILED(ret) || count == 0) 
	{
		return luaL_error(L, "type info not available");
	}

	ITypeInfo *info = NULL;
	ret = disp->GetTypeInfo(0, LOCALE_SYSTEM_DEFAULT, &info);
	if (FAILED(ret)) 
	{ 
		return luaL_error(L, "could not get type info"); 
	}

	disp->Release();
	winlua_push_typeinfo(L, info);
	return 1;
}

/* ------------------------------------------------------------
LuaWin TypeInfo module
------------------------------------------------------------ */
static const luaL_Reg typeinfo_meta[] = {
	{"__gc", typeinfo__gc},
	{"GetName", typeinfo_getname},
	{"GetTypeAttr", typeinfo_gettypeattr},
	{"GetFuncDesc", typeinfo_getfuncdesc},
	{"GetRefTypeInfo", typeinfo_getreftypeinfo},
	{NULL, NULL}
};

static void create_typeinfo_meta(lua_State *L)
{
	luaL_newmetatable(L, WINLUA_TYPEINFO_META);
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, typeinfo_meta, 0);
	lua_pop(L, 1);
}

int luaopen_dispatch_typeinfo(lua_State *L)
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
	create_typeinfo_meta(L);
	/* return the typeinfo constructor */
	lua_pushcfunction(L, dispatch_get_typeinfo);
	return 1;
}